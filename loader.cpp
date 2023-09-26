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
#include "functions.h"
#include "network.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

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

//generalizing this needs polymorphic lambdas or return type deduction
auto currentlyVisibleWrap = [](const Coordinate & center, const Dataset & dataset){
    return [&center, &dataset](const CoordOfCube & coord){
        return currentlyVisible(dataset.cube2global(coord), center, state->M, dataset.scaleFactor * dataset.cubeEdgeLength);
    };
};
auto insideCurrentSupercubeWrap = [](const Coordinate & center, const Dataset & dataset){
    return [center, dataset](const CoordOfCube & coord){
        return insideCurrentSupercube(dataset.cube2global(coord), center, state->M, dataset.scaleFactor * dataset.cubeEdgeLength);
    };
};
bool currentlyVisibleWrapWrap(const Coordinate & center, const Coordinate & coord) {// only for use from main thread
    return currentlyVisible(coord, center, state->M, Dataset::current().scaleFactor * Dataset::current().cubeEdgeLength);
}

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
    QObject::connect(this, &Loader::Controller::unloadCurrentMagnificationSignal, worker.get(), static_cast<void(Loader::Worker::*)()>(&Loader::Worker::unloadCurrentMagnification), Qt::BlockingQueuedConnection);
    QObject::connect(this, &Loader::Controller::markCubeAsModifiedSignal, worker.get(), &Loader::Worker::markCubeAsModified, Qt::BlockingQueuedConnection);
    QObject::connect(this, &Loader::Controller::snappyCacheSupplySnappySignal, worker.get(), &Loader::Worker::snappyCacheSupplySnappy, Qt::BlockingQueuedConnection);
    workerThread.start();
}

Loader::Controller::~Controller() {
    suspendLoader();
}

void Loader::Controller::unloadCurrentMagnification() {
    ++loadingNr;
    // blocking queued connections stall when there is no receiver
    // and loader is suspended when updateDatasetMag tries to load a new dataset
    if (workerThread.isRunning()) {
        emit unloadCurrentMagnificationSignal();
    } else if (worker) {
        worker->unloadCurrentMagnification();
    }
}

void Loader::Controller::markCubeAsModified(const std::size_t layerId, const CoordOfCube &cubeCoord, const int magnification) {
    emit markCubeAsModifiedSignal(layerId, cubeCoord, magnification);
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
    abortDownloadsFinishDecompression(layerId, [](const CoordOfCube &){return false;});
    QMutexLocker locker(&state->protectCube2Pointer);
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

void Loader::Worker::markCubeAsModified(const std::size_t layerId, const CoordOfCube &cubeCoord, const int magnification) {
    modifiedCacheQueue[layerId][static_cast<std::size_t>(std::log2(magnification))].emplace(cubeCoord);
}

void Loader::Worker::snappyCacheSupplySnappy(const std::size_t layerId, const CoordOfCube cubeCoord, const quint64 cubeMagnification, const std::string cube) {
    QMutexLocker lock{&snappyCacheMutex};
    if (cubeMagnification >= snappyCache[layerId].size()) {
        qWarning() << QObject::tr("ignored snappy cube (%1, %2, %3) for higher than available log2(mag) = %4 ≥ %5)")
                      .arg(cubeCoord.x).arg(cubeCoord.y).arg(cubeCoord.z).arg(cubeMagnification).arg(snappyCache[layerId].size());
        return;
    }
    snappyCache[layerId][cubeMagnification].emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple(cube));

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
    auto snappyIt = snappyCache[layerId][loaderMagnification].emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple()).first;
    //compress cube into the new string
    snappy::Compress(reinterpret_cast<const char *>(cube), OBJID_BYTES * std::pow(datasets[layerId].cubeEdgeLength, 3), &snappyIt->second);
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

Loader::DecompressionResult decompressCube(void * currentSlot, QIODevice & reply, const std::size_t layerId, const Dataset dataset, decltype(state->cube2Pointer)::value_type::value_type & cubeHash, const CoordOfCube cubeCoord) {
    if (!reply.isOpen()) {// sanity check, finished replies with no error should be ready for reading (https://bugreports.qt.io/browse/QTBUG-45944)
        qCritical() << layerId << cubeCoord << static_cast<int>(dataset.type) << "decompression failed → no fill";
        return {false, currentSlot, &reply};
    }
    QThread::currentThread()->setPriority(QThread::IdlePriority);
    bool success = false;

    auto data = reply.read(reply.bytesAvailable());//readAll can be very slow – https://bugreports.qt.io/browse/QTBUG-45926
    const auto cubeVxCount = std::pow(dataset.cubeEdgeLength, 3);
    const std::size_t availableSize = data.size();
    if (dataset.type == Dataset::CubeType::RAW_UNCOMPRESSED) {
        const std::size_t expectedSize = cubeVxCount;
        if (availableSize == expectedSize) {
            std::copy(std::begin(data), std::end(data), reinterpret_cast<std::uint8_t *>(currentSlot));
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::RAW_JPG || dataset.type == Dataset::CubeType::RAW_J2K || dataset.type == Dataset::CubeType::RAW_JP2_6 || dataset.type == Dataset::CubeType::RAW_PNG) {
        const auto image = QImage::fromData(data).convertToFormat(QImage::Format_Grayscale8);
        const qint64 expectedSize = cubeVxCount;
        if (image.sizeInBytes() == expectedSize) {
            std::copy(image.bits(), image.bits() + image.sizeInBytes(), reinterpret_cast<std::uint8_t *>(currentSlot));
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_16) {
        const std::size_t expectedSize = cubeVxCount * OBJID_BYTES / 4;
        if (availableSize == expectedSize) {
            boost::multi_array_ref<uint16_t, 1> dataRef(reinterpret_cast<uint16_t *>(data.data()), boost::extents[std::pow(dataset.cubeEdgeLength, 3)]);
            boost::multi_array_ref<uint64_t, 1> slotRef(reinterpret_cast<uint64_t *>(currentSlot), boost::extents[std::pow(dataset.cubeEdgeLength, 3)]);
            std::copy(std::begin(dataRef), std::end(dataRef), std::begin(slotRef));
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
        if (loaderMagnification >= state->cube2Pointer[layerId].size()) {
            continue;
        }
        QMutexLocker locker(&state->protectCube2Pointer);
        unloadCubes(state->cube2Pointer[layerId][loaderMagnification], freeSlots[layerId], insideCurrentSupercubeWrap(center, datasets[layerId])
                    , [this, layerId](const CoordOfCube & cubeCoord, void * remSlotPtr){
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
        const auto magCount = static_cast<std::size_t>(std::log2(changedDatasets[layerId].highestAvailableMag) + 1);
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
            if (datasets[layerId].allocationEnabled && changedDatasets[layerId].allocationEnabled
                    && loaderCacheSize == cacheSize
                    && datasets[layerId].type == changedDatasets[layerId].type
                    && datasets[layerId].cubeEdgeLength == changedDatasets[layerId].cubeEdgeLength) {
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

        const auto cubeBytes = std::pow(changedDatasets[layerId].cubeEdgeLength, 3) * overlayFactor;
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
    datasets = changedDatasets;
    loaderCacheSize = cacheSize;

    if (loaderMagnification != datasets[0].magIndex) {
        unloadCurrentMagnification();
        loaderMagnification = datasets[0].magIndex;
    }
    const auto Dcoi = DcoiFromPos(center, userMoveType, direction);//datacubes of interest prioritized around the current position
    //split dcoi into slice planes and rest
    std::vector<std::pair<std::size_t, CoordOfCube>> allCubes;
    for (auto && todo : Dcoi) {
        QMutexLocker locker(&state->protectCube2Pointer);
        for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
            // only queue downloads which are necessary
            if (cubeQuery(state->cube2Pointer, layerId, loaderMagnification, todo) == nullptr) {
                allCubes.emplace_back(layerId, todo);
            }
        }
    }

    auto startDownload = [this, center, loadingNr](const std::size_t layerId, const Dataset dataset, const CoordOfCube cubeCoord, decltype(slotDownload)::value_type & downloads
            , decltype(slotDecompression)::value_type & decompressions, decltype(freeSlots)::value_type & freeSlots, decltype(state->cube2Pointer)::value_type::value_type & cubeHash){
        auto & opens = slotOpen[layerId];
        const auto c = dataset.cube2global(cubeCoord);
        const auto b = dataset.boundary;
        if (c.x < 0 || c.y < 0 || c.z < 0 || c.x >= b.x || c.y >= b.y || c.z >= b.z) {
            return;
        }
        if (dataset.isOverlay()) {
            QMutexLocker lock{&snappyCacheMutex};
            auto snappyIt = snappyCache[layerId][loaderMagnification].find(cubeCoord);
            if (snappyIt != std::end(snappyCache[layerId][loaderMagnification])) {
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
                    const std::size_t cubeBytes = std::pow(dataset.cubeEdgeLength, 3) * (dataset.isOverlay() ? OBJID_BYTES : 1);
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
                    const auto inmagCoord = cubeCoord * dataset.cubeEdgeLength;
                    request.setRawHeader("Content-Type", "application/octet-stream");
                    const QString json(R"json({"geometry":{"corner":"%1,%2,%3", "size":"%4,%4,%4", "scale":%5}, "subvolume_format":"SINGLE_IMAGE", "image_format_options":{"image_format":"JPEG", "jpeg_quality":70}})json");
                    payload = json.arg(inmagCoord.x).arg(inmagCoord.y).arg(inmagCoord.z).arg(dataset.cubeEdgeLength).arg(loaderMagnification).toUtf8();
                } else if (dataset.api == Dataset::API::WebKnossos) {
                    const auto globalCoord = dataset.cube2global(cubeCoord);
                    request.setRawHeader("Content-Type", "application/json");
                    payload = QString{R"json([{"position":[%1,%2,%3],"zoomStep":%4,"cubeSize":%5,"fourBit":false}])json"}.arg(globalCoord.x).arg(globalCoord.y).arg(globalCoord.z).arg(static_cast<std::size_t>(std::log2(dataset.magnification))).arg(dataset.cubeEdgeLength).toUtf8();
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
                        const std::size_t cubeBytes = std::pow(dataset.cubeEdgeLength, 3) * (dataset.isOverlay() ? OBJID_BYTES : 1);
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
                watcher.setFuture(QtConcurrent::run(&localPool, [loadingNr, &io, path]() -> boost::optional<bool> {
                    // immediately exit unstarted thread from the previous loadSignal
                    if (loadingNr != Loader::Controller::singleton().loadingNr) {
                        return boost::none;
                    }
                    QFile file(path);
                    file.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
                    io.open(QIODevice::WriteOnly | QIODevice::Unbuffered);
                    const int size = file.size();
                    auto * fmap = file.map(0, size);
                    if (fmap == nullptr && QFile{path}.exists()) {
                        qWarning() << "mmap not used, but file exists, for" << path << size;
                    }
                    const int chunksize = 32*1024;
                    for (int offset{}; offset < size; offset += chunksize) {
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
                    startDownload(layerId, datasets[layerId], cubeCoord, slotDownload[layerId], slotDecompression[layerId], freeSlots[layerId], state->cube2Pointer.at(layerId).at(loaderMagnification));
                } catch (const std::out_of_range &) {}
            }
        }
    }
}
