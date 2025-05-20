/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "loader.h"

#include "brainmaps.h"
#include "coordinate.h"
#include "dataset.h"
#include "functions.h"
#include "network.h"
#include "scriptengine/scripting.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <boost/multi_array/storage_order.hpp>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <qdebug.h>
#include <qelapsedtimer.h>
#include <qfileinfo.h>
#include <qglobal.h>
#include <qhashfunctions.h>
#include <qnamespace.h>
#include <qnetworkrequest.h>
#include <quazip.h>
#include <quazipfile.h>

#include <snappy.h>

#include <QBuffer>
#include <QFile>
#include <QFuture>
#include <QImage>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtConcurrent>

#include <boost/range/combine.hpp>

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

//generalizing this needs polymorphic lambdas or return type deduction
auto currentlyVisibleWrap = [](const Coordinate & center, const Dataset & dataset){
    return [&center, &dataset](const CoordOfCube & coord){
        return currentlyVisible(dataset.cube2global(coord), center, {state->M, state->M, state->M}, dataset.scaleFactor.componentMul(dataset.cubeShape));
    };
};
auto insideCurrentSupercubeWrap = [](const Coordinate & center, const Dataset & dataset){
    return [center, dataset](const CoordOfCube & coord){
        return insideCurrentSupercube(dataset.cube2global(coord), center, {state->M, state->M, state->M}, dataset.scaleFactor.componentMul(dataset.cubeShape));
    };
};

void Loader::Controller::suspendLoader() {
    ++loadingNr;
    workerThread.quit();
    workerThread.wait();
    if (worker) {
        worker->abortDownloadsFinishDecompression();
    }
}

Loader::Controller::Controller() {
    worker = std::make_unique<Loader::Worker>();
    workerThread.setObjectName("Loader");
    worker->moveToThread(&workerThread);
    QObject::connect(worker.get(), &Loader::Worker::progress, this, [this](bool, int count){emit progress(count);});
    QObject::connect(worker.get(), &Loader::Worker::progress, this, &Loader::Controller::refCountChange);
    QObject::connect(this, &Loader::Controller::loadSignal, worker.get(), &Loader::Worker::downloadAndLoadCubes, Qt::QueuedConnection);// avoid deadlock via snappyCacheClear
    QObject::connect(this, &Loader::Controller::markCubeAsModifiedSignal, worker.get(), &Loader::Worker::markCubeAsModified, Qt::BlockingQueuedConnection);
    QObject::connect(this, &Loader::Controller::snappyCacheSupplySnappySignal, worker.get(), &Loader::Worker::snappyCacheSupplySnappy, Qt::BlockingQueuedConnection);
    workerThread.start();
}

Loader::Controller::~Controller() {
    suspendLoader();
}

void Loader::Controller::markCubeAsModified(const std::size_t layerId, const CoordOfCube &cubeCoord, const std::size_t magIndex) {
    emit markCubeAsModifiedSignal(layerId, cubeCoord, magIndex);
    state->viewer->reslice_notify_all(layerId, cubeCoord);
}

bool Loader::Controller::isFinished() {
    return worker->isFinished.load();
}

bool Loader::Controller::hasSnappyCache() {
    QMutexLocker lock{&worker->snappyCacheMutex};
    for (const auto & layer : worker->snappyCache) {
        for (const auto & mag : layer) {
            if (!mag.empty()) {
                return true;
            }
        }
    }
    return false;
}

void Loader::Worker::CalcLoadOrderMetric(float halfSc, floatCoordinate currentMetricPos, const UserMoveType userMoveType, const floatCoordinate & direction, float *metrics) {
    const auto INNER_MULT_VECTOR = [](const floatCoordinate v) {
        return v.x * v.y * v.z;
    };
    const auto CALC_VECTOR_NORM = [](const floatCoordinate v) {
        return std::sqrt(std::pow(v.x, 2) + std::pow(v.y, 2) + std::pow(v.z, 2));
    };
    const auto CALC_DOT_PRODUCT = [](const floatCoordinate a, const floatCoordinate b){
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    };
    const auto CALC_POINT_DISTANCE_FROM_PLANE = [CALC_VECTOR_NORM, CALC_DOT_PRODUCT](const floatCoordinate point, const floatCoordinate plane){
        return std::abs(CALC_DOT_PRODUCT(point, plane)) / CALC_VECTOR_NORM(plane);
    };

    float distance_from_plane, distance_from_origin, dot_product;
    int i = 0;

    distance_from_origin = CALC_VECTOR_NORM(currentMetricPos);

    switch (userMoveType) {
    case USERMOVE_HORIZONTAL:
    case USERMOVE_DRILL:
        distance_from_plane = CALC_POINT_DISTANCE_FROM_PLANE(currentMetricPos, direction);
        dot_product = CALC_DOT_PRODUCT(currentMetricPos, direction);

        if (USERMOVE_HORIZONTAL == userMoveType) {
            metrics[i++] = (0 == distance_from_plane ? -1.0 : 1.0);
            metrics[i++] = (0 == INNER_MULT_VECTOR(currentMetricPos) ? -1.0 : 1.0);
        }
        else {
            metrics[i++] = (( (distance_from_plane <= 1) || (distance_from_origin <= halfSc) ) ? -1.0 : 1.0);
            metrics[i++] = (distance_from_plane > 1 ? 1.0 : -1.0);
            metrics[i++] = (dot_product < 0 ?  1.0 : -1.0);
            metrics[i++] = distance_from_plane;
        }
        break;
    case USERMOVE_NEUTRAL:
        // Priorities are XY->ZY->XZ
        metrics[i++] = (0 == currentMetricPos.z ? -1.0 : 1.0);
        metrics[i++] = (0 == currentMetricPos.x ? -1.0 : 1.0);
        metrics[i++] = (0 == currentMetricPos.y ? -1.0 : 1.0);
        break;
    default:
        break;
    }
    metrics[i++] = distance_from_origin;

    currentMaxMetric = std::max(this->currentMaxMetric, i);
}

struct LO_Element {
    CoordOfCube coordinate;
    Coordinate offset;
    float loadOrderMetrics[LL_METRIC_NUM];
};

std::vector<CoordOfCube> Loader::Worker::DcoiFromPos(const Coordinate & cpos, const UserMoveType userMoveType, const floatCoordinate & direction) {
    const auto cubeElemCount = std::pow(state->M, 3);
    const auto centerCube = datasets[0].global2cube(cpos);
    const auto halfFOV = datasets[0].cube2global({1,1,1}) * (state->M - 1) / 2;
    const auto tlCubeOffset = datasets[0].global2cube(cpos - halfFOV) - centerCube;
    const auto brCubeOffset = datasets[0].global2cube(cpos + halfFOV) - centerCube;

    int i = 0;
    currentMaxMetric = 0;
    std::vector<LO_Element> DcArray(cubeElemCount);
    for (int x = tlCubeOffset.x; x <= brCubeOffset.x; ++x) {
        for (int y = tlCubeOffset.y; y <= brCubeOffset.y; ++y) {
            for (int z = tlCubeOffset.z; z <= brCubeOffset.z; ++z) {
                DcArray[i].coordinate = {centerCube.x + x, centerCube.y + y, centerCube.z + z};
                DcArray[i].offset = {x, y, z};
                floatCoordinate currentMetricPos(x, y, z);
                CalcLoadOrderMetric(0.5 * state->M, currentMetricPos, userMoveType, direction, &DcArray[i].loadOrderMetrics[0]);
                ++i;
            }
        }
    }

    std::sort(std::begin(DcArray), std::begin(DcArray) + cubeElemCount, [&](const LO_Element & elem_a, const LO_Element & elem_b){
        for (int metric_index = 0; metric_index < currentMaxMetric; ++metric_index) {
            float m_a = elem_a.loadOrderMetrics[metric_index];
            float m_b = elem_b.loadOrderMetrics[metric_index];
            if (m_a != m_b) {
                return (m_a - m_b) < 0;
            }
            //If equal just continue to next comparison level
        }
        return false;
    });

    std::vector<CoordOfCube> cubes;
    for (int i = 0; i < cubeElemCount; ++i) {
        cubes.emplace_back(DcArray[i].coordinate);
    }

    return cubes;
}

Loader::Worker::Worker() {
    qnam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);// default is manual redirect
}

Loader::Worker::~Worker() {
    abortDownloadsFinishDecompression();

    qDebug() << "solitaryConfinement waiting for" << solitaryConfinement.size();
    QElapsedTimer timer;
    timer.start();
    for (auto & elem : solitaryConfinement) {
        elem->waitForFinished();
    }
    qDebug() << "solitaryConfinement" << timer.nsecsElapsed()/1e6 << "ms";

    if (state->quitSignal) {
        return;//state is dead already
    }

    QMutexLocker locker(&state->protectCube2Pointer);
    for (auto & layer : state->cube2Pointer) {
        for (auto & elem : layer) {
            elem.clear();
        }
    }
}

template<typename CubeHash, typename Slots, typename Keep>
void unloadCubes(CubeHash & loadedCubes, Slots & freeSlots, Keep keep) {
    unloadCubes(loadedCubes, freeSlots, keep, [](const CoordOfCube &, void *){});
}

template<typename CubeHash, typename Slots, typename Keep, typename UnloadHook>
void unloadCubes(CubeHash & loadedCubes, Slots & freeSlots, Keep keep, UnloadHook todo) {
    for (auto it = std::begin(loadedCubes); it != std::end(loadedCubes);) {
        if (!keep(it->first)) {
            todo(CoordOfCube(it->first.x, it->first.y, it->first.z), it->second);
            freeSlots.emplace_back(it->second);
            it = loadedCubes.erase(it);
        } else {
            ++it;
        }
    }
}

void Loader::Worker::unloadCurrentMagnification(const std::size_t layerId) {
    if (layerId >= datasets.size()) {
        return;
    }
    abortDownloadsFinishDecompression(layerId, [](const CoordOfCube &){return false;});
    QMutexLocker locker(&state->protectCube2Pointer);
    const auto loaderMagnification = datasets[layerId].magIndex;
    if (loaderMagnification >= state->cube2Pointer[layerId].size()) {
        return;
    }
    for (auto & elem : state->cube2Pointer[layerId][loaderMagnification]) {
        const auto cubeCoord = elem.first;
        const auto remSlotPtr = elem.second;
        if (modifiedCacheQueue[layerId][loaderMagnification].find(cubeCoord) != std::end(modifiedCacheQueue[layerId][loaderMagnification])) {
            snappyCacheBackupRaw(layerId, cubeCoord, remSlotPtr);
            //remove from work queue
            modifiedCacheQueue[layerId][loaderMagnification].erase(cubeCoord);
        }
        freeSlots[layerId].emplace_back(remSlotPtr);
        state->viewer->reslice_notify_all(layerId, cubeCoord);
    }
    state->cube2Pointer[layerId][loaderMagnification].clear();
}

void Loader::Worker::unloadCurrentMagnification() {
    for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
        unloadCurrentMagnification(layerId);
    }
}

void Loader::Worker::markCubeAsModified(const std::size_t layerId, const CoordOfCube &cubeCoord, const std::size_t magIndex) {
    modifiedCacheQueue[layerId][magIndex].emplace(cubeCoord);
}

void Loader::Worker::snappyCacheSupplySnappy(const std::size_t layerId, const CoordOfCube cubeCoord, const quint64 cubeMagnification, const std::string cube) {
    QMutexLocker lock{&snappyCacheMutex};
    if (cubeMagnification >= snappyCache[layerId].size()) {
        qWarning() << QObject::tr("ignored snappy cube (%1, %2, %3) for higher than available log2(mag) = %4 ≥ %5)")
                      .arg(cubeCoord.x).arg(cubeCoord.y).arg(cubeCoord.z).arg(cubeMagnification).arg(snappyCache[layerId].size());
        return;
    }
    snappyCache[layerId][cubeMagnification].emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple(cube));

    const auto loaderMagnification = datasets[layerId].magIndex;
    if (cubeMagnification == loaderMagnification) {//unload if currently loaded
        auto openIt = slotOpen[layerId].find(cubeCoord);
        if (openIt != std::end(slotOpen[layerId])) {
            openIt->second->cancel();
        }
        auto downloadIt = slotDownload[layerId].find(cubeCoord);
        if (downloadIt != std::end(slotDownload[layerId])) {
            downloadIt->second->abort();
        }
        auto decompressionIt = slotDecompression[layerId].find(cubeCoord);
        if (decompressionIt != std::end(slotDecompression[layerId])) {
            decompressionIt->second->waitForFinished();
        }
        QMutexLocker locker(&state->protectCube2Pointer);
        auto cubePtr = cubeQuery(state->cube2Pointer, layerId, loaderMagnification, cubeCoord);
        if (cubePtr != nullptr) {
            freeSlots[layerId].emplace_back(cubePtr);
            state->cube2Pointer[layerId][loaderMagnification].erase(cubeCoord);
        }
    }
}

void Loader::Worker::snappyCacheBackupRaw(const std::size_t layerId, const CoordOfCube & cubeCoord, const void * cube) {
    QMutexLocker lock{&snappyCacheMutex};
    //insert empty string into snappy cache
    auto snappyIt = snappyCache[layerId][datasets[layerId].magIndex].emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple()).first;
    //compress cube into the new string
    snappy::Compress(reinterpret_cast<const char *>(cube), OBJID_BYTES * datasets[layerId].cubeShape.prod(), &snappyIt->second);
}

void Loader::Worker::snappyCacheClear() {
    QMutexLocker lock{&snappyCacheMutex};
    //unload all modified cubes
    for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
        for (std::size_t mag = 0; mag < state->cube2Pointer[layerId].size(); ++mag) {
            unloadCubes(state->cube2Pointer[layerId][mag], freeSlots[layerId], [this, layerId, mag](const CoordOfCube & cubeCoord){
                const bool unflushed = modifiedCacheQueue[layerId][mag].find(cubeCoord) != std::end(modifiedCacheQueue[layerId][mag]);
                const bool flushed = snappyCache[layerId][mag].find(cubeCoord) != std::end(snappyCache[layerId][mag]);
                return !unflushed && !flushed;//only keep cubes which are neither in snappy cache nor in modified queue
            });
            modifiedCacheQueue[layerId][mag].clear();
            snappyCache[layerId][mag].clear();
        }
    }
    state->viewer->loader_notify();//a bit of a detour…
}

void Loader::Worker::flushIntoSnappyCache() {
    QMutexLocker locker(&snappyFlushConditionMutex);
    for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
        for (std::size_t mag = 0; mag < modifiedCacheQueue[layerId].size(); ++mag) {
            for (const auto & cubeCoord : modifiedCacheQueue[layerId][mag]) {
                state->protectCube2Pointer.lock();
                auto cube = cubeQuery(state->cube2Pointer, layerId, mag, {cubeCoord.x, cubeCoord.y, cubeCoord.z});
                state->protectCube2Pointer.unlock();
                if (cube != nullptr) {
                    snappyCacheBackupRaw(layerId, cubeCoord, cube);
                }
            }
            //clear work queue
            modifiedCacheQueue[layerId][mag].clear();
        }
    }

    snappyFlushCondition.wakeAll();
}

void Loader::Worker::moveToThread(QThread *targetThread) {
    qnam.moveToThread(targetThread);
    QObject::moveToThread(targetThread);
}

template<typename Downloads, typename Func>
void abortDownloads(Downloads & downloads, Func keep) {
    std::vector<CoordOfCube> abortQueue;
    for (auto && elem : downloads) {
        if (!keep(elem.first)) {
            abortQueue.emplace_back(elem.first);
        }
    }
    for (auto && elem : abortQueue) {
        downloads[elem]->abort();//abort running downloads
    }
}

void Loader::Worker::abortDownloadsFinishDecompression() {
    for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
        abortDownloadsFinishDecompression(layerId, [](const CoordOfCube &){return false;});
    }
}

decltype(Loader::Worker::slotDecompression)::value_type::iterator Loader::Worker::finalizeDecompression(QFutureWatcher<DecompressionResult> & watcher, decltype(freeSlots)::value_type & freeSlots, decltype(slotDecompression)::value_type & decompressions, const CoordOfCube & cubeCoord) {
    auto [success, currentSlot, io] = watcher.result();
    if (!success) {//decompression unsuccessful
        freeSlots.emplace_back(currentSlot);
    }
    io->deleteLater();
    auto it = decompressions.erase(decompressions.find(cubeCoord));
    broadcastProgress();
    return it;
}

template<typename Func>
void Loader::Worker::abortDownloadsFinishDecompression(std::size_t layerId, Func keep) {
    for (auto & elem : slotOpen[layerId]) {
        elem.second->blockSignals(true);
        elem.second->cancel();
    }
    for (auto & elem : slotOpen[layerId]) {
        solitaryConfinement.emplace_back(std::move(elem.second));
    }
    solitaryConfinement.erase(std::remove_if(std::begin(solitaryConfinement), std::end(solitaryConfinement),
        [](const auto & val){ return val->isFinished(); }), std::end(solitaryConfinement));
    slotOpen[layerId].clear();
    broadcastProgress();
    abortDownloads(slotDownload[layerId], keep);
    for (auto it = std::begin(slotDecompression[layerId]); it != std::end(slotDecompression[layerId]);) {
        if (!keep(it->first)) {
            it->second->waitForFinished();
            it = finalizeDecompression(*it->second, freeSlots[layerId], slotDecompression[layerId], it->first);
        } else {
            ++it;
        }
    }
}

#include <boost/endian/conversion.hpp>
#include <boost/endian/buffers.hpp>

template<std::uint8_t bits>
struct arb {
    struct it {
        unsigned char const * bytebase;
        std::size_t bitoffset;
        bool operator!=(const it & other) {
            return bitoffset != other.bitoffset;
        }
        std::uint32_t operator*() const {
            return (boost::endian::load_little_u32(bytebase + bitoffset / 8) >> (bitoffset % 8)) & ((1uL << bits) - 1);
        }
        auto & operator++() {
            bitoffset += bits;
            return *this;
        }
        auto & operator--() {
            bitoffset -= bits;
            return *this;
        }
    } sentinel;
    arb(unsigned char const * ptr, std::size_t size) : sentinel{ptr, size*bits} {}
    auto begin() {
        return it{sentinel.bytebase, 0};
    }
    auto end() {
        return sentinel;
    }
};

auto minishards_from_shard(unsigned char const * data, const std::size_t num_minishards) {
    std::vector<std::array<std::uint64_t,2>> minishards(num_minishards);
    for (std::size_t mi{0}; mi < num_minishards; ++mi) {
        minishards[mi] = {boost::endian::load_little_u64(data+mi*16), boost::endian::load_little_u64(data+mi*16+8)};
    }
    return minishards;
}

auto chunks_from_minishards(unsigned char const * data, const std::size_t & size, const std::size_t shard_data_offset) {
    std::unordered_map<std::uint64_t, std::array<std::uint64_t,2>>  chunks;
    auto num_chunks = size/8/3;
    std::size_t id_delta{}, offset_delta{};
    for (std::size_t ci{0}; ci < num_chunks; ++ci) {
        auto * p = data + ci*8;
        const auto chunk_id = boost::endian::load_little_u64(p);
        const auto offset = boost::endian::load_little_u64(p + num_chunks*8);
        const auto size = boost::endian::load_little_u64(p + 2*num_chunks*8);
        // std::cout << chunk_id+id_delta << ' ';
        chunks[chunk_id+id_delta] = {shard_data_offset+offset+offset_delta, size};
        // qDebug() << (chunk_id+id_delta) << (shard_data_offset+offset+offset_delta) << size;
        id_delta += chunk_id;
        offset_delta += offset+size;
    }
    return chunks;
}

#include <iostream>

void degzip(QByteArray & data) {
    auto * ref = reinterpret_cast<unsigned char const *>(data.data());
    if (data.size() < 2 || ref[0] != 0x1F || ref[1] != 0x8B) {
        return;
    }
    std::vector<unsigned char> decompressed;
    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    strm.next_in = const_cast<Bytef*>(ref);
    strm.avail_in = data.size();
    if (inflateInit2(&strm, MAX_WBITS + 16) != Z_OK) {// Use windowBits = MAX_WBITS + 16 to enable gzip decoding
        std::cerr << "inflateInit2 failed!" << std::endl;
        return;
    }
    const size_t chunkSize = 262144; // 256KB chunks
    std::vector<unsigned char> buffer(chunkSize);

    int ret;
    do {
        strm.next_out = buffer.data();
        strm.avail_out = buffer.size();

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            std::cerr << "inflate failed with error: " << ret << std::endl;
            inflateEnd(&strm);
            return;
        }

        // Calculate number of bytes decompressed in this iteration
        size_t bytesDecompressed = buffer.size() - strm.avail_out;
        decompressed.insert(decompressed.end(), buffer.begin(), buffer.begin() + bytesDecompressed);
    } while (ret != Z_STREAM_END);

    if (ret != Z_STREAM_END) {
        std::cerr << "inflate failed with error 2: " << ret << std::endl;
        inflateEnd(&strm);
        return;
    }
    inflateEnd(&strm);
    // auto data2 = qUncompress(data);
    data = QByteArray(reinterpret_cast<char *>(decompressed.data()), decompressed.size());
}

Loader::DecompressionResult decompressCube(void * currentSlot, QIODevice & reply, const std::size_t layerId, const Dataset dataset, decltype(state->cube2Pointer)::value_type::value_type & cubeHash, const CoordOfCube cubeCoord) {
    if (!reply.isOpen()) {// sanity check, finished replies with no error should be ready for reading (https://bugreports.qt.io/browse/QTBUG-45944)
        qCritical() << layerId << cubeCoord << static_cast<int>(dataset.type) << "decompression failed → no fill";
        return {false, currentSlot, &reply};
    }
    QThread::currentThread()->setPriority(QThread::IdlePriority);
    bool success = false;

    auto data = reply.read(reply.bytesAvailable());//readAll can be very slow – https://bugreports.qt.io/browse/QTBUG-45926
    const auto cubeVxCount = dataset.cubeShape.prod();
    const auto partialCubeShape = (dataset.cube2global(cubeCoord + 1) / dataset.scaleFactor).capped({}, dataset.boundary / dataset.scaleFactor + 1) - dataset.cube2global(cubeCoord) / dataset.scaleFactor;
    const auto partialCubeVxCount = partialCubeShape.prod();
    const std::size_t availableSize = data.size();
    if (dataset.isOverlay() && (dataset.api == Dataset::API::Precomputed || dataset.api == Dataset::API::Sharded)) {
        const std::size_t expectedSize = cubeVxCount * OBJID_BYTES;
        if (availableSize <= expectedSize) {
            degzip(data);
            // std::copy(std::begin(data), std::end(data), reinterpret_cast<std::uint8_t *>(currentSlot));
            // std::fill(reinterpret_cast<std::uint64_t *>(currentSlot) + availableSize/OBJID_BYTES, reinterpret_cast<std::uint64_t *>(currentSlot) + cubeVxCount, 1);

            boost::multi_array_ref<std::uint64_t, 3> slotRef(reinterpret_cast<std::uint64_t *>(currentSlot), boost::extents[dataset.cubeShape.z][dataset.cubeShape.y][dataset.cubeShape.x]);

            const auto num_channels = boost::endian::load_little_u32(reinterpret_cast<unsigned char const *>(data.data()));

            auto cpd = floatCoordinate{partialCubeShape} / dataset.gpuCubeShape;
            auto cpd2 = Coordinate(std::ceil(cpd.x), std::ceil(cpd.y), std::ceil(cpd.z));

            QElapsedTimer t;
            QFutureSynchronizer<void> sync;
            t.start();
            // qDebug() << "foo" << num_channels << cubeCoord << availableSize << data.size() << expectedSize << dataset.gpuCubeShape << cpd << cpd2;
            for (int y = 0; y < cpd2.y; ++y)
            sync.addFuture(QtConcurrent::run(&Loader::Controller::singleton().worker->decompressionPool, [&,y](){
                    std::vector<std::uint64_t> output(dataset.gpuCubeShape.prod());
                    for (int z = 0; z < cpd2.z; ++z)
                    for (int x = 0; x < cpd2.x; ++x) {
                            auto headerByteOffset = 8 * (x + cpd2.x * (y + cpd2.y * z));

                            const auto * data2 = reinterpret_cast<unsigned char const *>(data.data()+4*num_channels);

                            const auto lookupTableByteOffset = 4*boost::endian::load_little_u24(data2 + headerByteOffset);
                            const std::uint8_t encodedBits = data2[headerByteOffset + 3];
                            const auto encodedValuesByteOffset = 4*boost::endian::load_little_u32(data2 + headerByteOffset + 4);

                            if (encodedBits > 0) {
                                std::uint64_t const * const lut = reinterpret_cast<const std::uint64_t *>(data2 + lookupTableByteOffset);// or 32 bit
                                // bits.insert(lut.size());

                                auto resolve = [&]<std::size_t c>(){
                                    arb<c> keys(data2 + encodedValuesByteOffset, dataset.gpuCubeShape.prod());
                                    std::transform(std::begin(keys), std::end(keys), std::begin(output), [&lut](auto key){ return lut[key]; });
                                };
                                switch (encodedBits) {
                                case 1: resolve.template operator()<1>(); break;
                                case 2: resolve.template operator()<2>(); break;
                                case 4: resolve.template operator()<4>(); break;
                                case 8: resolve.template operator()<8>(); break;
                                case 16: resolve.template operator()<16>(); break;
                                case 32: resolve.template operator()<32>(); break;
                                };
                            } else {
                                // output = decltype(output)(output.size(), boost::endian::load_little_u32(data2 + lookupTableByteOffset));
                                std::fill(std::begin(output), std::end(output), boost::endian::load_little_u32(data2 + lookupTableByteOffset));
                            }
                            if (dataset.global2cube(Coordinate{12891, 13090, 0}) == cubeCoord && (Coordinate{12891, 13090, 0} - dataset.cube2global(cubeCoord)) / dataset.gpuCubeShape == Coordinate{x,y,z}) {
                            }

                            boost::const_multi_array_ref<std::uint64_t, 3> dataCube(output.data(), boost::extents[dataset.gpuCubeShape.z][dataset.gpuCubeShape.y][dataset.gpuCubeShape.x]);
                            const auto & s = dataset.gpuCubeShape;
                            using range = boost::multi_array_types::index_range;
                            auto e = (Coordinate{x,y,z}).componentMul(s);
                            auto slice  = boost::indices[range(z*s.z, std::min((z+1)*s.z, dataset.cubeShape.z))][range(y*s.y, std::min((y+1)*s.y, dataset.cubeShape.y))][range(x*s.x, std::min((x+1)*s.x, dataset.cubeShape.x))];
                            auto slice2 = boost::indices[range(0, slotRef[slice].shape()[0])][range(0, slotRef[slice].shape()[1])][range(0, slotRef[slice].shape()[2])];
                            slotRef[slice] = dataCube[slice2];
                            //     qDebug() << "foo" << cubeCoord << availableSize << expectedSize << dataset.gpuCubeShape << cpd << cpd2 << x << y << z << headerByteOffset << lookupTableByteOffset << encodedBits << encodedValuesByteOffset;
                            if (encodedBits > 0 || lookupTableByteOffset == encodedValuesByteOffset) {
                            } else {
                                // qDebug() << encodedBits << lookupTableByteOffset << encodedValuesByteOffset << boost::endian::load_little_u32(data2 + lookupTableByteOffset) << boost::endian::load_little_u32(data2 + encodedValuesByteOffset);
                                // qDebug() << x << y << z << cubeCoord << (z*s.z, std::min((z+1)*s.z, dataset.cubeShape.z)) << (y*s.y, std::min((y+1)*s.y, dataset.cubeShape.y)) << (x*s.x, std::min((x+1)*s.x, dataset.cubeShape.x));
                            }
                    }
                }));

            sync.waitForFinished();
            // qDebug() << t.nsecsElapsed()/1e6;
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::RAW_UNCOMPRESSED) {
        const std::size_t expectedSize = cubeVxCount;
        if (availableSize == expectedSize) {
            std::copy(std::begin(data), std::end(data), reinterpret_cast<std::uint8_t *>(currentSlot));
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::RAW_JPG || dataset.type == Dataset::CubeType::RAW_J2K || dataset.type == Dataset::CubeType::RAW_JP2_6 || dataset.type == Dataset::CubeType::RAW_PNG) {
        const auto image = QImage::fromData(data).convertToFormat(QImage::Format_Grayscale8);
        // qDebug() << cubeCoord << availableSize << image.size() << image.sizeInBytes() << image.bytesPerLine() << partialCubeVxCount << partialCubeShape << (dataset.cube2global(cubeCoord + 1) / dataset.scaleFactor).capped({}, dataset.boundary / dataset.scaleFactor + 1) << dataset.cube2global(cubeCoord) / dataset.scaleFactor;
        const qint64 expectedSize = cubeVxCount;
        if (image.sizeInBytes() == expectedSize) {
            std::copy(image.bits(), image.bits() + image.sizeInBytes(), reinterpret_cast<std::uint8_t *>(currentSlot));
            success = true;
        } else if (image.sizeInBytes() >= partialCubeVxCount) {
            bool needsAttention = (partialCubeShape.x > 1 && (partialCubeShape.x % 2 != 0 || partialCubeShape.y % 2 != 0)) || (image.height() != image.width() && partialCubeShape.x == image.height() && partialCubeShape.y == image.width());
            using range = boost::multi_array_types::index_range;
            auto extents = partialCubeShape.z == 1 || needsAttention ? boost::extents[1][image.height()][image.bytesPerLine()] : boost::extents[partialCubeShape.z][partialCubeShape.y][partialCubeShape.x];
            boost::const_multi_array_ref<uint8_t, 3> dataRef(image.bits(), extents);
            boost::multi_array_ref<uint8_t, 3> slotRef(reinterpret_cast<uint8_t *>(currentSlot), boost::extents[dataset.cubeShape.z][dataset.cubeShape.y][dataset.cubeShape.x]);
            std::fill(reinterpret_cast<std::uint8_t *>(currentSlot), reinterpret_cast<std::uint8_t *>(currentSlot) + cubeVxCount, 0);
            auto v = slotRef[boost::indices[range(0, partialCubeShape.z)][range(0, partialCubeShape.y)][range(0, partialCubeShape.x)]];
            if (needsAttention) {
                boost::multi_array<uint8_t, 3> b = dataRef[boost::indices[range(0, 1)][range(0, image.height())][range(0, image.width())]];
                b.reshape(boost::array<decltype(b)::index, 3>{partialCubeShape.z, partialCubeShape.y, partialCubeShape.x});
                v = b;
            } else {
                v = dataRef[boost::indices[range(0, partialCubeShape.z)][range(0, partialCubeShape.y)][range(0, partialCubeShape.x)]];
            }

            // for (std::size_t y = 0; y < partialCubeShape.y; ++y) {
            //     // std::copy(image.scanLine(y), image.scanLine(y) + partialCubeShape.x, reinterpret_cast<std::uint8_t *>(currentSlot)+y*dataset.cubeShape.x);
            //     std::copy(image.constBits() + y *  - partialCubeShape.x, image.constBits() + y * image.bytesPerLine() + , reinterpret_cast<std::uint8_t *>(currentSlot) + y * dataset.cubeShape.x);
            // }

            // auto right_view  = slotRef[boost::indices[range::all().start(partialCubeShape.z)][range()][range()]];
            // auto bottom_view = slotRef[boost::indices[range()][range::all().start(partialCubeShape.y)][range()]];
            // auto behind_view = slotRef[boost::indices[range()][range()][range::all().start(partialCubeShape.x)]];
            // for (auto & view : {right_view, bottom_view, behind_view}) {
            //     std::fill(view.begin(), view.end(), 0);
            // }
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_16) {
        const std::size_t expectedSize = cubeVxCount * OBJID_BYTES / 4;
        if (availableSize == expectedSize) {
            boost::const_multi_array_ref<uint16_t, 1> dataRef(reinterpret_cast<const uint16_t *>(data.data()), boost::extents[cubeVxCount]);
            boost::multi_array_ref<uint64_t, 1> slotRef(reinterpret_cast<uint64_t *>(currentSlot), boost::extents[cubeVxCount]);
            slotRef = dataRef;
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_64) {
        const std::size_t expectedSize = cubeVxCount * OBJID_BYTES;
        if (availableSize == expectedSize) {
            std::copy(std::begin(data), std::end(data), reinterpret_cast<std::uint64_t *>(currentSlot));
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::SEGMENTATION_SZ_ZIP) {
        QBuffer buffer(&data);
        QuaZip archive(&buffer);//QuaZip needs a random access QIODevice
        if (archive.open(QuaZip::mdUnzip)) {
            archive.goToFirstFile();
            QuaZipFile file(&archive);
            if (file.open(QIODevice::ReadOnly)) {
                auto data = file.readAll();
                std::size_t uncompressedSize;
                snappy::GetUncompressedLength(data.data(), data.size(), &uncompressedSize);
                const std::size_t expectedSize = cubeVxCount * OBJID_BYTES;
                if (uncompressedSize == expectedSize) {
                    success = snappy::RawUncompress(data.data(), data.size(), reinterpret_cast<char*>(currentSlot));
                }
            }
            archive.close();
        }
    } else {
        qDebug() << "unsupported format";
    }

    if (success) {
        state->protectCube2Pointer.lock();
        cubeHash[cubeCoord] = currentSlot;
        state->protectCube2Pointer.unlock();
        state->viewer->reslice_notify_all(layerId, cubeCoord);
    }

    return {success, currentSlot, &reply};
}

void Loader::Worker::cleanup(const Coordinate center) {
    for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
        abortDownloadsFinishDecompression(layerId, currentlyVisibleWrap(center, datasets[layerId]));
        const auto loaderMagnification = datasets[layerId].magIndex;
        if (loaderMagnification >= state->cube2Pointer[layerId].size()) {
            continue;
        }
        QMutexLocker locker(&state->protectCube2Pointer);
        unloadCubes(state->cube2Pointer[layerId][loaderMagnification], freeSlots[layerId], insideCurrentSupercubeWrap(center, datasets[layerId])
                    , [this, layerId, loaderMagnification](const CoordOfCube & cubeCoord, void * remSlotPtr){
            if (datasets[layerId].isOverlay()) {// TODO is it the snappy layer?
                if (modifiedCacheQueue[layerId][loaderMagnification].find(cubeCoord) != std::end(modifiedCacheQueue[layerId][loaderMagnification])) {
                    snappyCacheBackupRaw(layerId, cubeCoord, remSlotPtr);
                    //remove from work queue
                    modifiedCacheQueue[layerId][loaderMagnification].erase(cubeCoord);
                }
            }
            state->viewer->reslice_notify_all(layerId, cubeCoord);
        });
    }
}

void Loader::Controller::startLoading(const Coordinate & center, const UserMoveType userMoveType, const floatCoordinate & direction) {
    if (worker && !Dataset::datasets.empty()) {
        worker->isFinished = false;
        workerThread.start();
        emit loadSignal(++loadingNr, center, userMoveType, direction, Dataset::datasets, state->M);
    }
}

void Loader::Worker::broadcastProgress(bool startup) {
    std::size_t count{0};
    for (const auto & tup : boost::combine(slotOpen, slotDownload, slotDecompression)) {
        count += tup.get<0>().size() + tup.get<1>().size() + tup.get<2>().size();
    }
    isFinished = count == 0;
    emit progress(startup, count);
}

void Loader::Worker::downloadAndLoadCubes(const unsigned int loadingNr, const Coordinate center, const UserMoveType userMoveType, const floatCoordinate & direction, Dataset::list_t changedDatasets, const size_t cacheSize) {
    cleanup(center);
    // freeSlots[] are lists of pointers to locations that
    // can hold data or overlay cubes. Whenever we want to load a new
    // datacube, we load it into a location from this list. Whenever a
    // datacube in memory becomes invalid, we add the pointer to its
    // memory location back into this list.
    if (changedDatasets.size() != datasets.size()) {
        if (changedDatasets.size() < datasets.size()) {
            unloadCurrentMagnification();
        }
        slotOpen.resize(changedDatasets.size());
        slotDownload.resize(changedDatasets.size());
        slotDecompression.resize(changedDatasets.size());
        slotChunk.resize(changedDatasets.size());
        freeSlots.resize(changedDatasets.size());
        {
            QMutexLocker locker(&state->protectCube2Pointer);
            state->cube2Pointer.resize(changedDatasets.size());
        }
        {
            QMutexLocker lock{&snappyCacheMutex};
            modifiedCacheQueue.resize(changedDatasets.size());
            snappyCache.resize(changedDatasets.size());
        }
    }
    for (std::size_t layerId{0}; layerId < changedDatasets.size(); ++layerId) {
        const auto magCount = static_cast<std::size_t>(changedDatasets[layerId].highestAvailableMagIndex + 1);
        {
            QMutexLocker locker(&state->protectCube2Pointer);
            state->cube2Pointer[layerId].resize(magCount);
        }
        {
            QMutexLocker lock{&snappyCacheMutex};
            modifiedCacheQueue[layerId].resize(magCount);
            snappyCache[layerId].resize(magCount);
        }
        if (layerId < datasets.size()) {
            if (changedDatasets[layerId].magIndex != datasets[layerId].magIndex || changedDatasets[layerId].url != datasets[layerId].url || changedDatasets[layerId].experimentname != datasets[layerId].experimentname) {
                unloadCurrentMagnification(layerId);
            }
            if (datasets[layerId].allocationEnabled && changedDatasets[layerId].allocationEnabled
                    && loaderCacheSize == cacheSize
                    && datasets[layerId].type == changedDatasets[layerId].type
                    && datasets[layerId].cubeShape == changedDatasets[layerId].cubeShape) {
                continue;// loader-relevant layer properties didn’t change
            }
            unloadCurrentMagnification(layerId);
            slotChunk[layerId].clear();
            freeSlots[layerId].clear();
        }
        if (!changedDatasets[layerId].allocationEnabled) {
            continue;
        }
        const auto overlayFactor = changedDatasets[layerId].isOverlay() ? OBJID_BYTES : 1;

        const auto cubeBytes = changedDatasets[layerId].cubeShape.prod() * overlayFactor;
        const auto cubeSetElements = std::pow(state->M, 3);
        const auto cubeSetBytes = cubeSetElements * cubeBytes;
        qDebug() << layerId << "Allocating" << cubeSetBytes / 1024. / 1024. << "MiB for cubes.";
        QElapsedTimer time;
        time.start();
        for (std::size_t i = 0; i < cubeSetBytes; i += cubeBytes) {
            slotChunk[layerId].emplace_back(cubeBytes, 0);// zero init chunk of chars
            freeSlots[layerId].emplace_back(slotChunk[layerId].back().data());// append newest element
        }
        qDebug() << "in" << qSetRealNumberPrecision(2) << time.nsecsElapsed()/1e9 << "s";
    }
    static std::vector<std::unordered_map<std::uint64_t, std::array<std::uint64_t,2>>> chunkid2chunk(datasets.size());
    static std::vector<std::unordered_map<CoordOfCube, std::array<std::uint64_t,2>>> cube2chunk(datasets.size());
    static std::vector<QSet<QUrl>> fourohfour(datasets.size());

    if (changedDatasets.size() != datasets.size()) {
        chunkid2chunk.clear();
        chunkid2chunk.resize(changedDatasets.size());
        cube2chunk.clear();
        cube2chunk.resize(changedDatasets.size());
        fourohfour.clear();
        fourohfour.resize(changedDatasets.size());
    }
    for (std::size_t layerId{0}; layerId < std::min(datasets.size(), changedDatasets.size()); ++layerId) {
        if (datasets[layerId].url != changedDatasets[layerId].url || datasets[layerId].magIndex != changedDatasets[layerId].magIndex) {
            chunkid2chunk[layerId].clear();
            cube2chunk[layerId].clear();
            fourohfour[layerId].clear();
        }
    }

    datasets = changedDatasets;
    loaderCacheSize = cacheSize;

    //split dcoi into slice planes and rest
    std::vector<std::pair<std::size_t, CoordOfCube>> allCubes;
    {
        const auto Dcoi = DcoiFromPos(center, userMoveType, direction);//datacubes of interest prioritized around the current position
        QMutexLocker locker(&state->protectCube2Pointer);
        for (auto && todo : Dcoi) {
            for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
                // only queue downloads which are necessary
                if (cubeQuery(state->cube2Pointer, layerId, datasets[layerId].magIndex, todo) == nullptr) {
                    allCubes.emplace_back(layerId, todo);
                }
            }
        }
    }

    QSet<QPair<quint64,QUrl>> shards;
    QMap<QPair<quint64,QUrl>,QSet<quint64>> shard2minishards;
    for (std::size_t i{0}; i < allCubes.size(); ++i) {
        const auto layerId = allCubes[i].first;
        const auto & dataset = datasets[layerId];
        const auto cubeCoord = allCubes[i].second;
        if (dataset.api == Dataset::API::Sharded) {
            if (auto it = chunkid2chunk[layerId].find(dataset.chunkid(cubeCoord)); it != std::end(chunkid2chunk[layerId])) {
                cube2chunk[layerId].emplace(cubeCoord, it->second);
            } else if(auto shard = dataset.precomputedCubeUrl(cubeCoord, true); !fourohfour[layerId].contains(shard)) {
                // allCubes.erase(std::next(std::begin(allCubes), i));
                shards.insert({layerId,shard});
                shard2minishards[{layerId,shard}].insert((dataset.minishard(cubeCoord)));
            }
        }
    }

    for (const auto & [layerId,shard] : shards) {
        qDebug() << "l" << shard;

        auto num_minishards = (1u << datasets[layerId].bits[datasets[layerId].magIndex].minishard_bits);
        auto shard_data_offset = num_minishards * 16;

        if (shard.scheme() == "file") {
            QFile f(shard.toLocalFile());
            f.open(QIODevice::ReadOnly);
            auto data = f.read(shard_data_offset);
            if (data.size() > 0) {
                qDebug() << "foo" << data.size();
                const auto minishards = minishards_from_shard(reinterpret_cast<unsigned char const *>(data.data()), num_minishards);
                // for (std::size_t mi{0}; mi < minishards.size(); ++mi) {
                for (const auto mi : qAsConst(shard2minishards[{layerId,shard}])) {
                    if (minishards[mi][0] < minishards[mi][1]) {
                        QFile f(shard.toLocalFile());
                        f.open(QIODevice::ReadOnly);
                        /*qDebug() << */f.seek(shard_data_offset+minishards[mi][0]);
                        data = f.read(minishards[mi][1]-minishards[mi][0]);
                        degzip(data);
                        if (data.size() > 0) {
                            // qDebug() << "bar" << data.size();
                            for (auto chunkid : chunks_from_minishards((reinterpret_cast<unsigned char const *>(data.data())), data.size(), shard_data_offset)) {
                                chunkid2chunk[layerId][chunkid.first] = chunkid.second;
                            }
                        }
                    }
                }
            }
        } else {
            QNetworkRequest request(shard);
            request.setRawHeader("Range", QString("bytes=%1-%2").arg(0).arg(shard_data_offset - 1).toUtf8());
            auto * reply = qnam.get(request);
            QObject::connect(reply, &QNetworkReply::finished, [this, reply, num_minishards, shard, shard_data_offset, layerId, shard2minishards](){
                if (reply->error() != QNetworkReply::NoError) {
                    qDebug() << "shard" << reply->request().url() << reply->errorString();
                    if (reply->error() == QNetworkReply::ContentNotFoundError) {
                        fourohfour[layerId].insert(shard);
                    }
                    return;
                }
                auto data = reply->readAll();
                qDebug() << "foo" << data.size() << reply->rawHeaderPairs();
                const auto minishards = minishards_from_shard(reinterpret_cast<unsigned char const *>(data.data()), num_minishards);
                // for (std::size_t mi{0}; mi < minishards.size(); ++mi) {
                for (auto mi : shard2minishards[{layerId,shard}]) {
                    if (minishards[mi][0] < minishards[mi][1]) {
                        QNetworkRequest request(shard);
                        request.setRawHeader("Range", QString("bytes=%1-%2").arg(shard_data_offset + minishards[mi][0]).arg(shard_data_offset + minishards[mi][1] - 1).toUtf8());
                        auto * reply = qnam.get(request);
                        QObject::connect(reply, &QNetworkReply::finished, [reply, minishards, layerId, shard_data_offset](){
                            if (reply->error() != QNetworkReply::NoError) {
                                qDebug() << "minishard" << reply->request().url() << reply->errorString();
                                return;
                            }
                            auto data = reply->readAll();
                            degzip(data);
                            // qDebug() << "bar" << data.size() << reply->rawHeaderPairs();
                            for (auto chunkid : chunks_from_minishards((reinterpret_cast<unsigned char const *>(data.data())), data.size(), shard_data_offset)) {
                                chunkid2chunk[layerId][chunkid.first] = chunkid.second;
                            }
                            // state->viewer->loader_notify();
                            reply->deleteLater();
                        });
                    }
                }
                reply->deleteLater();
            });
        }
    }

    qDebug() << "foobar" << std::accumulate(std::begin(chunkid2chunk), std::end(chunkid2chunk), 0, [](std::size_t sum, const auto & m){ return sum + m.size(); })
             << std::accumulate(std::begin(shard2minishards), std::end(shard2minishards), 0, [](std::size_t sum, const auto & m){ return sum + m.size(); }) << shards.size();

    // for (std::size_t i{0}; i < allCubes.size(); ++i) {
    //     const auto layerId = allCubes[i].first;
    //     const auto & dataset = datasets[layerId];
    //     const auto cubeCoord = allCubes[i].second;
    //     if (dataset.api == Dataset::API::Sharded) {
    //         if (auto it = chunkid2chunk[layerId].find(dataset.chunkid(cubeCoord)); it == std::end(chunkid2chunk[layerId])) {
    //             allCubes.erase(std::next(std::begin(allCubes), i));
    //         }
    //     }
    // }

    for (std::size_t i{0}; i < allCubes.size(); ++i) {
        const auto layerId = allCubes[i].first;
        const auto & dataset = datasets[layerId];
        const auto cubeCoord = allCubes[i].second;
        if (dataset.api == Dataset::API::Sharded) {
            if (auto it = chunkid2chunk[layerId].find(dataset.chunkid(cubeCoord)); it != std::end(chunkid2chunk[layerId])) {
                cube2chunk[layerId].emplace(cubeCoord, it->second);
            } else {
                allCubes.erase(std::next(std::begin(allCubes), i));
                shards.insert({layerId,dataset.precomputedCubeUrl(cubeCoord, true)});
            }
        }
    }

    auto startDownload = [this, center, loadingNr](const std::size_t layerId, Dataset dataset, const CoordOfCube cubeCoord, decltype(slotDownload)::value_type & downloads
            , decltype(slotDecompression)::value_type & decompressions, decltype(freeSlots)::value_type & freeSlots, decltype(state->cube2Pointer)::value_type::value_type & cubeHash){
        auto & opens = slotOpen[layerId];
        const auto c = dataset.cube2global(cubeCoord);
        const auto b = floatCoordinate(dataset.boundary) * dataset.scales[0].x / datasets[0].scales[0].x;
        if (c.x < 0 || c.y < 0 || c.z < 0 || c.x >= b.x || c.y >= b.y || c.z >= b.z) {
            return;
        }
        if (dataset.isOverlay()) {
            QMutexLocker lock{&snappyCacheMutex};
            auto snappyIt = snappyCache[layerId][dataset.magIndex].find(cubeCoord);
            if (snappyIt != std::end(snappyCache[layerId][dataset.magIndex])) {
                if (!freeSlots.empty()) {
                    auto downloadIt = downloads.find(cubeCoord);
                    if (downloadIt != std::end(downloads)) {
                        downloadIt->second->abort();
                    }
                    auto decompressionIt = decompressions.find(cubeCoord);
                    if (decompressionIt != std::end(decompressions)) {
                        decompressionIt->second->waitForFinished();
                    }
                    state->protectCube2Pointer.lock();
                    const auto currentSlotIt = cubeHash.find(cubeCoord);
                    auto * currentSlot = currentSlotIt != std::end(cubeHash) ? currentSlotIt->second : freeSlots.front();
                    cubeHash.erase(cubeCoord);
                    state->protectCube2Pointer.unlock();
                    if (currentSlot == freeSlots.front()) {
                        freeSlots.pop_front();
                    }
                    //directly uncompress snappy cube into the OC slot
                    const auto success = snappy::RawUncompress(snappyIt->second.c_str(), snappyIt->second.size(), reinterpret_cast<char*>(currentSlot));
                    if (success) {
                        state->protectCube2Pointer.lock();
                        cubeHash[cubeCoord] = currentSlot;
                        state->protectCube2Pointer.unlock();

                        state->viewer->reslice_notify_all(layerId, cubeCoord);
                    } else {
                        freeSlots.emplace_back(currentSlot);
                        qCritical() << layerId << cubeCoord << "snappy extract failed" << snappyIt->second.size();
                    }
                } else {
                    qCritical() << layerId << cubeCoord << "no slots for snappy extract" << cubeHash.size() << freeSlots.size();
                }
                return;
            }
        }
        state->protectCube2Pointer.lock();
        const bool cubeNotAlreadyLoaded = cubeHash.count(cubeCoord) == 0;
        state->protectCube2Pointer.unlock();
        const bool cubeNotDownloading = downloads.count(cubeCoord) == 0 && opens.count(cubeCoord) == 0;
        const bool cubeNotDecompressing = decompressions.count(cubeCoord) == 0;

        if (cubeNotAlreadyLoaded && cubeNotDownloading && cubeNotDecompressing) {
            if (dataset.type == Dataset::CubeType::SNAPPY) {
                if (!freeSlots.empty()) {
                    auto * currentSlot = freeSlots.front();
                    freeSlots.pop_front();
                    const std::size_t cubeBytes = dataset.cubeShape.prod() * (dataset.isOverlay() ? OBJID_BYTES : 1);
                    std::fill(reinterpret_cast<std::uint8_t *>(currentSlot), reinterpret_cast<std::uint8_t *>(currentSlot) + cubeBytes, 0);
                    state->protectCube2Pointer.lock();
                    cubeHash[cubeCoord] = currentSlot;
                    state->protectCube2Pointer.unlock();
                    state->viewer->reslice_notify_all(layerId, cubeCoord);
                } else {
                    qCritical() << layerId << cubeCoord << "no slots for snappy extract" << cubeHash.size() << freeSlots.size();
                }
                return;
            }

            auto request = dataset.apiSwitch(cubeCoord);
            if (dataset.api == Dataset::API::Sharded) {
                if (!cube2chunk[layerId].contains(cubeCoord)) {
                    return;
                }
                request.setRawHeader("Range", QString("bytes=%1-%2").arg(cube2chunk[layerId][cubeCoord][0]).arg(cube2chunk[layerId][cubeCoord][0]+cube2chunk[layerId][cubeCoord][1] - 1).toUtf8());
            }
//            request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
//            request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
//            request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, true);

            QByteArray payload;
            if (cubeCoord == dataset.global2cube(center)) {
                //the first download usually finishes last (which is a bug) so we put it alone in the high priority bucket
                request.setPriority(QNetworkRequest::HighPriority);
            }
            auto & io = [&]() -> QIODevice & {
                if (dataset.api == Dataset::API::GoogleBrainmaps) {
                    const auto inmagCoord = cubeCoord.componentMul(dataset.cubeShape);
                    request.setRawHeader("Content-Type", "application/octet-stream");
                    const QString json(R"json({"geometry":{"corner":"%1,%2,%3", "size":"%4,%5,%6", "scale":%7}, "subvolume_format":"SINGLE_IMAGE", "image_format_options":{"image_format":"JPEG", "jpeg_quality":70}})json");
                    payload = json.arg(inmagCoord.x).arg(inmagCoord.y).arg(inmagCoord.z).arg(dataset.cubeShape.x).arg(dataset.cubeShape.y).arg(dataset.cubeShape.z).arg(dataset.magIndex).toUtf8();
                } else if (dataset.api == Dataset::API::WebKnossos) {
                    const auto globalCoord = dataset.cube2global(cubeCoord);
                    request.setRawHeader("Content-Type", "application/json");
                    payload = QString{R"json([{"position":[%1,%2,%3],"zoomStep":%4,"cubeSize":%5,"fourBit":false}])json"}.arg(globalCoord.x).arg(globalCoord.y).arg(globalCoord.z).arg(dataset.magIndex).arg(dataset.cubeShape.x).toUtf8();
                }
                if (dataset.url.scheme() == "file") {
                    return *new QBuffer{};
                }
                if (dataset.api == Dataset::API::WebKnossos || dataset.api == Dataset::API::GoogleBrainmaps) {
                    return *qnam.post(request, payload);
                } else {
                    return *qnam.get(request);
                }
            }();
            auto processDownload = [this, layerId, dataset, &io, cubeCoord, &downloads, &decompressions, &freeSlots, &cubeHash](bool exists = false){
                if (freeSlots.empty()) {
                    qCritical() << layerId << cubeCoord << static_cast<int>(dataset.type) << "no slots for decompression" << cubeHash.size() << freeSlots.size();
                    io.deleteLater();
                    downloads.erase(cubeCoord);
                    broadcastProgress();
                    return;
                }
                auto * maybeReply = dynamic_cast<QNetworkReply*>(&io);
                if ((maybeReply != nullptr && maybeReply->error() == QNetworkReply::NoError) || (maybeReply == nullptr && exists)) {
                    auto * currentSlot = freeSlots.front();
                    freeSlots.pop_front();
                    auto * watcher = new QFutureWatcher<DecompressionResult>;
                    QObject::connect(watcher, &QFutureWatcher<DecompressionResult>::finished, this, [this, watcher, &freeSlots, &decompressions, cubeCoord](){
                        finalizeDecompression(*watcher, freeSlots, decompressions, cubeCoord);
                    });
                    io.setParent(nullptr);// reparent, so it doesn’t get destroyed with qnam
                    decompressions[cubeCoord].reset(watcher);
                    downloads.erase(cubeCoord);
                    watcher->setFuture(QtConcurrent::run(&decompressionPool, std::bind(&decompressCube, currentSlot, std::ref(io), layerId, dataset, std::ref(cubeHash), cubeCoord)));
                } else {
                    if ((maybeReply != nullptr && maybeReply->error() == QNetworkReply::ContentNotFoundError) || (maybeReply == nullptr && !exists)) {//404 → fill
                        auto * currentSlot = freeSlots.front();
                        freeSlots.pop_front();
                        const std::size_t cubeBytes = dataset.cubeShape.prod() * (dataset.isOverlay() ? OBJID_BYTES : 1);
                        std::fill(reinterpret_cast<std::uint8_t *>(currentSlot), reinterpret_cast<std::uint8_t *>(currentSlot) + cubeBytes, 0);
                        state->protectCube2Pointer.lock();
                        cubeHash[cubeCoord] = currentSlot;
                        state->protectCube2Pointer.unlock();
                        state->viewer->reslice_notify_all(layerId, cubeCoord);
                    } else {
                        if(maybeReply != nullptr && maybeReply->error() != QNetworkReply::OperationCanceledError) {
                            qCritical() << layerId << cubeCoord << static_cast<int>(dataset.type) << maybeReply->request().url() << maybeReply->errorString() << maybeReply->readAll();
                            if (dataset.api == Dataset::API::GoogleBrainmaps) {
                                qDebug() << "GoogleBrainmaps error" << maybeReply->error();
                                if (maybeReply->error() == QNetworkReply::ContentAccessDenied || maybeReply->error() == QNetworkReply::AuthenticationRequiredError) {
                                    auto pair = getBrainmapsToken();
                                    if (pair.first) {
                                        Dataset::datasets[layerId].token = datasets[layerId].token = pair.second;
                                    }
                                }
                            }
                        }
                    }
                    io.deleteLater();
                    downloads.erase(cubeCoord);
                    broadcastProgress();
                }
            };
            if (dataset.url.scheme() != "file") {
                downloads[cubeCoord] = &dynamic_cast<QNetworkReply &>(io);
                QObject::connect(downloads[cubeCoord], &QNetworkReply::finished, this, processDownload);
            } else if (Annotation::singleton().embeddedDataset) {
                const auto path = QFileInfo{*Annotation::singleton().embeddedDataset}.dir().path() + request.url().toLocalFile();
                const bool exists = Annotation::singleton().extraFiles.contains(path);
                if (exists) {
                    dynamic_cast<QBuffer &>(io).setBuffer(&Annotation::singleton().extraFiles[path]);
                    io.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
                }
                processDownload(exists);
            } else {
                opens[cubeCoord] = std::make_unique<OpenWatcher>();
                auto & watcher = *opens[cubeCoord];
                io.setParent(&watcher);// reparent, so it gets destroyed upon cleanup
                const auto path = request.url().toLocalFile();
                QObject::connect(&watcher, &std::remove_reference_t<decltype(watcher)>::finished, this, [this, &watcher, processDownload, &opens, cubeCoord, path](){
                    if (!watcher.isCanceled()) {
                        if (auto res = watcher.result()) {// skip loadingNr-aborted open
                            processDownload(res.get() || QFile{path}.exists());
                        }
                    }
                    opens.erase(cubeCoord);
                    broadcastProgress();
                });
                localPool.setMaxThreadCount(1024);
                watcher.setFuture(QtConcurrent::run(&localPool, [loadingNr, &io, path, dataset, layerId, cubeCoord]() -> boost::optional<bool> {
                    // immediately exit unstarted thread from the previous loadSignal
                    if (loadingNr != Loader::Controller::singleton().loadingNr || !QFile{path}.exists()) {
                        return boost::none;
                    }
                    QFile file(path);
                    file.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
                    io.open(QIODevice::WriteOnly | QIODevice::Unbuffered);
                    std::size_t offset2 = dataset.api == Dataset::API::Sharded ? cube2chunk[layerId][cubeCoord][0] : 0;
                    const std::size_t size = dataset.api == Dataset::API::Sharded ? cube2chunk[layerId][cubeCoord][1] : file.size();
                    auto * fmap = file.map(offset2, size);
                    if (fmap == nullptr) {
                        if (QFile{path}.exists()) {
                            qWarning() << "mmap not used, but file exists, for" << path << offset2 << size << QFile{path}.size();
                        }
                        file.seek(offset2);
                    }
                    const std::size_t chunksize = 32*1024;
                    for (std::size_t offset{}; offset < size; offset += chunksize) {
                        if (loadingNr != Loader::Controller::singleton().loadingNr) {
                            return boost::none;
                        }
                        if (fmap != nullptr) {
                            io.write(reinterpret_cast<const char *>(fmap) + offset, std::min(chunksize, size - offset));
                        } else {
                            io.write(file.read(chunksize));
                        }
                        if (loadingNr != Loader::Controller::singleton().loadingNr) {
                            return boost::none;
                        }
                    }
                    io.close();
                    io.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
                    return size != 0;
                }));
            }
            broadcastProgress(true);
        }
    };

    for (auto [layerId, cubeCoord] : allCubes) {
        if (loadingNr == Loader::Controller::singleton().loadingNr) {
            if (datasets[layerId].loadingEnabled) {
                try {
                    startDownload(layerId, datasets[layerId], cubeCoord, slotDownload[layerId], slotDecompression[layerId], freeSlots[layerId], state->cube2Pointer.at(layerId).at(datasets[layerId].magIndex));
                } catch (const std::out_of_range &) {}
            }
        }
    }
}
