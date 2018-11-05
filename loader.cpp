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
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "loader.h"

#include "brainmaps.h"
#include "functions.h"
#include "network.h"
#include "segmentation/segmentation.h"
#include "session.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>

#include <snappy.h>

#include <QFile>
#include <QFuture>
#include <QImage>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtConcurrent>

#include <cmath>
#include <fstream>
#include <stdexcept>

//generalizing this needs polymorphic lambdas or return type deduction
auto currentlyVisibleWrap = [](const Coordinate & center, const Dataset & dataset){
    return [&center, &dataset](const Coordinate & coord){
        return currentlyVisible(coord, center, state->M, dataset.cubeEdgeLength * dataset.magnification);
    };
};
auto insideCurrentSupercubeWrap = [](const Coordinate & center, const Dataset & dataset){
    return [center, dataset](const CoordOfCube & coord){
        return insideCurrentSupercube(coord.cube2Global(dataset.cubeEdgeLength, dataset.magnification), center, state->M, dataset.cubeEdgeLength * dataset.magnification);
    };
};
bool currentlyVisibleWrapWrap(const Coordinate & center, const Coordinate & coord) {// only for use from main thread
    return currentlyVisibleWrap(center, Dataset::current())(coord);
}

void Loader::Controller::suspendLoader() {
    ++loadingNr;
    workerThread.quit();
    workerThread.wait();
    if (worker != nullptr) {
        worker->abortDownloadsFinishDecompression();
    }
}

Loader::Controller::~Controller() {
    suspendLoader();
}

void Loader::Controller::unloadCurrentMagnification() {
    ++loadingNr;
    emit unloadCurrentMagnificationSignal();
}

void Loader::Controller::markOcCubeAsModified(const CoordOfCube &cubeCoord, const int magnification) {
    emit markOcCubeAsModifiedSignal(cubeCoord, magnification);
    state->viewer->window->notifyUnsavedChanges();
    state->viewer->reslice_notify_all(worker.get()->snappyLayerId, cubeCoord.cube2Global(Dataset::current().cubeEdgeLength, magnification));
}

decltype(Loader::Worker::snappyCache) Loader::Controller::getAllModifiedCubes() {
    if (worker != nullptr) {
        QMutexLocker locker(&worker->snappyMutex);
        //signal to run in loader thread
        QTimer::singleShot(0, worker.get(), &Loader::Worker::flushIntoSnappyCache);
        worker->snappyFlushCondition.wait(&worker->snappyMutex);
        return worker->snappyCache;
    } else {
        return decltype(Loader::Worker::snappyCache)();//{} is not working
    }
}

bool Loader::Controller::isFinished() {
    return worker != nullptr ? worker->isFinished.load() : true;//no loader == done?
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

std::vector<CoordOfCube> Loader::Worker::DcoiFromPos(const CoordOfCube & currentOrigin, const UserMoveType userMoveType, const floatCoordinate & direction) {
    const float floatHalfSc = state->M / 2.;
    const int halfSc = std::floor(floatHalfSc);
    const int cubeElemCount = state->cubeSetElements;

    int i = 0;
    currentMaxMetric = 0;
    std::vector<LO_Element> DcArray(cubeElemCount);
    for (int x = -halfSc; x < halfSc + 1; ++x) {
        for (int y = -halfSc; y < halfSc + 1; ++y) {
            for (int z = -halfSc; z < halfSc + 1; ++z) {
                DcArray[i].coordinate = {currentOrigin.x + x, currentOrigin.y + y, currentOrigin.z + z};
                DcArray[i].offset = {x, y, z};
                floatCoordinate currentMetricPos(x, y, z);
                CalcLoadOrderMetric(floatHalfSc, currentMetricPos, userMoveType, direction, &DcArray[i].loadOrderMetrics[0]);
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

Loader::Worker::Worker(const decltype(datasets) & layers)
    : slotDownload(static_cast<std::size_t>(layers.size())), slotDecompression(static_cast<std::size_t>(layers.size()))
    , slotChunk(static_cast<std::size_t>(layers.size())), freeSlots(static_cast<std::size_t>(layers.size()))
    , datasets{layers}, snappyLayerId{Segmentation::singleton().layerId}
    , OcModifiedCacheQueue(static_cast<std::size_t>(std::log2(layers.front().highestAvailableMag)+1))
    , snappyCache(static_cast<std::size_t>(std::log2(layers.front().highestAvailableMag)+1))
{
    qnam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);// default is manual redirect
    state->cube2Pointer.resize(datasets.size());
    for (auto && layer : datasets) {
        layer.allocationEnabled = false;// signify that nothing is allocated yet
    }
}

Loader::Worker::~Worker() {
    abortDownloadsFinishDecompression();

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
    abortDownloadsFinishDecompression(layerId, [](const Coordinate &){return false;});
    QMutexLocker locker(&state->protectCube2Pointer);
    if (loaderMagnification >= state->cube2Pointer[layerId].size()) {
        return;
    }
    for (auto & elem : state->cube2Pointer[layerId][loaderMagnification]) {
        const auto cubeCoord = elem.first;
        const auto remSlotPtr = elem.second;
        if (layerId == snappyLayerId) {
            if (OcModifiedCacheQueue[loaderMagnification].find(cubeCoord) != std::end(OcModifiedCacheQueue[loaderMagnification])) {
                snappyCacheBackupRaw(cubeCoord, remSlotPtr);
                //remove from work queue
                OcModifiedCacheQueue[loaderMagnification].erase(cubeCoord);
            }
        }
        freeSlots[layerId].emplace_back(remSlotPtr);
        state->viewer->reslice_notify_all(layerId, cubeCoord.cube2Global(datasets[layerId].cubeEdgeLength, std::pow(2, loaderMagnification)));
    }
    state->cube2Pointer[layerId][loaderMagnification].clear();
}

void Loader::Worker::unloadCurrentMagnification() {
    for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
        unloadCurrentMagnification(layerId);
    }
}

void Loader::Worker::markOcCubeAsModified(const CoordOfCube &cubeCoord, const int magnification) {
    OcModifiedCacheQueue[static_cast<std::size_t>(std::log2(magnification))].emplace(cubeCoord);
}

void Loader::Worker::snappyCacheSupplySnappy(const CoordOfCube cubeCoord, const int magnification, const std::string cube) {
    const auto cubeMagnification = static_cast<std::size_t>(std::log2(magnification));
    if (cubeMagnification >= snappyCache.size()) {
        qWarning() << QObject::tr("ignored snappy cube (%1, %2, %3) for higher than available mag %4 ((log2(%4) = %5) ≥ %6)")
                      .arg(cubeCoord.x).arg(cubeCoord.y).arg(cubeCoord.z).arg(magnification).arg(cubeMagnification).arg(snappyCache.size());
        return;
    }
    snappyCache[cubeMagnification].emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple(cube));

    if (cubeMagnification == loaderMagnification) {//unload if currently loaded
        const auto globalCoord = cubeCoord.cube2Global(datasets.front().cubeEdgeLength, magnification);
        auto downloadIt = slotDownload[snappyLayerId].find(globalCoord);
        if (downloadIt != std::end(slotDownload[snappyLayerId])) {
            downloadIt->second->abort();
        }
        auto decompressionIt = slotDecompression[snappyLayerId].find(globalCoord);
        if (decompressionIt != std::end(slotDecompression[snappyLayerId])) {
            decompressionIt->second->waitForFinished();
        }
        QMutexLocker locker(&state->protectCube2Pointer);
        const auto coord = cubeCoord;
        auto cubePtr = cubeQuery(state->cube2Pointer, snappyLayerId, loaderMagnification, coord);
        if (cubePtr != nullptr) {
            freeSlots[snappyLayerId].emplace_back(cubePtr);
            state->cube2Pointer[snappyLayerId][loaderMagnification].erase(coord);
        }
    }
}

void Loader::Worker::snappyCacheBackupRaw(const CoordOfCube & cubeCoord, const void * cube) {
    //insert empty string into snappy cache
    auto snappyIt = snappyCache[loaderMagnification].emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple()).first;
    //compress cube into the new string
    snappy::Compress(reinterpret_cast<const char *>(cube), OBJID_BYTES * state->cubeBytes, &snappyIt->second);
}

void Loader::Worker::snappyCacheClear() {
    if (snappyLayerId >= state->cube2Pointer.size()) {
        return;
    }
    //unload all modified cubes
    for (std::size_t mag = 0; mag < state->cube2Pointer[snappyLayerId].size(); ++mag) {
        unloadCubes(state->cube2Pointer[snappyLayerId][mag], freeSlots[snappyLayerId], [this, mag](const CoordOfCube & cubeCoord){
            const bool unflushed = OcModifiedCacheQueue[mag].find(cubeCoord) != std::end(OcModifiedCacheQueue[mag]);
            const bool flushed = snappyCache[mag].find(cubeCoord) != std::end(snappyCache[mag]);
            return !unflushed && !flushed;//only keep cubes which are neither in snappy cache nor in modified queue
        });
        OcModifiedCacheQueue[mag].clear();
        snappyCache[mag].clear();
    }
    state->viewer->loader_notify();//a bit of a detour…
}

void Loader::Worker::flushIntoSnappyCache() {
    QMutexLocker locker(&snappyMutex);
    for (std::size_t mag = 0; mag < OcModifiedCacheQueue.size(); ++mag) {
        for (const auto & cubeCoord : OcModifiedCacheQueue[mag]) {
            state->protectCube2Pointer.lock();
            auto cube = cubeQuery(state->cube2Pointer, snappyLayerId, mag, {cubeCoord.x, cubeCoord.y, cubeCoord.z});
            state->protectCube2Pointer.unlock();
            if (cube != nullptr) {
                snappyCacheBackupRaw(cubeCoord, cube);
            }
        }
        //clear work queue
        OcModifiedCacheQueue[mag].clear();
    }

    snappyFlushCondition.wakeAll();
}

void Loader::Worker::moveToThread(QThread *targetThread) {
    qnam.moveToThread(targetThread);
    QObject::moveToThread(targetThread);
}

template<typename Downloads, typename Func>
void abortDownloads(Downloads & downloads, Func keep) {
    std::vector<Coordinate> abortQueue;
    for (auto && elem : downloads) {
        if (!keep(elem.first)) {
            abortQueue.emplace_back(elem.first);
        }
    }
    for (auto && elem : abortQueue) {
        downloads[elem]->abort();//abort running downloads
    }
}

template<typename Decomp, typename Func>
void finishDecompression(Decomp & decompressions, Func keep) {
    for (auto && elem : decompressions) {
        if (!keep(elem.first)) {
            //elem.second->cancel();
            elem.second->waitForFinished();
        }
    }
}

void Loader::Worker::abortDownloadsFinishDecompression() {
    for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
        abortDownloadsFinishDecompression(layerId, [](const Coordinate &){return false;});
    }
}

template<typename Func>
void Loader::Worker::abortDownloadsFinishDecompression(std::size_t layerId, Func keep) {
    abortDownloads(slotDownload[layerId], keep);
    finishDecompression(slotDecompression[layerId], keep);
}

std::pair<bool, void*> decompressCube(void * currentSlot, QIODevice & reply, const std::size_t layerId, const Dataset dataset, decltype(state->cube2Pointer)::value_type::value_type & cubeHash, const Coordinate globalCoord) {
    if (!reply.isOpen()) {// sanity check, finished replies with no error should be ready for reading (https://bugreports.qt.io/browse/QTBUG-45944)
        return {false, currentSlot};
    }
    QThread::currentThread()->setPriority(QThread::IdlePriority);
    bool success = false;

    auto data = reply.read(reply.bytesAvailable());//readAll can be very slow – https://bugreports.qt.io/browse/QTBUG-45926
    const std::size_t availableSize = data.size();
    if (dataset.type == Dataset::CubeType::RAW_UNCOMPRESSED) {
        const std::size_t expectedSize = state->cubeBytes;
        if (availableSize == expectedSize) {
            std::copy(std::begin(data), std::end(data), reinterpret_cast<std::uint8_t *>(currentSlot));
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::RAW_JPG || dataset.type == Dataset::CubeType::RAW_J2K || dataset.type == Dataset::CubeType::RAW_JP2_6 || dataset.type == Dataset::CubeType::RAW_PNG) {
        const auto image = QImage::fromData(data).convertToFormat(QImage::Format_Indexed8);
        const qint64 expectedSize = state->cubeBytes;
        if (image.byteCount() == expectedSize) {
            std::copy(image.bits(), image.bits() + image.byteCount(), reinterpret_cast<std::uint8_t *>(currentSlot));
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_16) {
        const std::size_t expectedSize = state->cubeBytes * OBJID_BYTES / 4;
        if (availableSize == expectedSize) {
            boost::multi_array_ref<uint16_t, 1> dataRef(reinterpret_cast<uint16_t *>(data.data()), boost::extents[std::pow(dataset.cubeEdgeLength, 3)]);
            boost::multi_array_ref<uint64_t, 1> slotRef(reinterpret_cast<uint64_t *>(currentSlot), boost::extents[std::pow(dataset.cubeEdgeLength, 3)]);
            std::copy(std::begin(dataRef), std::end(dataRef), std::begin(slotRef));
            success = true;
        }
    } else if (dataset.type == Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_64) {
        const std::size_t expectedSize = state->cubeBytes * OBJID_BYTES;
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
                const std::size_t expectedSize = state->cubeBytes * OBJID_BYTES;
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
        cubeHash[globalCoord.cube(dataset.cubeEdgeLength, dataset.magnification)] = currentSlot;
        state->protectCube2Pointer.unlock();
        state->viewer->reslice_notify_all(layerId, globalCoord);
    }

    return {success, currentSlot};
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
                if (OcModifiedCacheQueue[loaderMagnification].find(cubeCoord) != std::end(OcModifiedCacheQueue[loaderMagnification])) {
                    snappyCacheBackupRaw(cubeCoord, remSlotPtr);
                    //remove from work queue
                    OcModifiedCacheQueue[loaderMagnification].erase(cubeCoord);
                }
            }
        });
    }
}

void Loader::Controller::startLoading(const Coordinate & center, const UserMoveType userMoveType, const floatCoordinate & direction) {
    if (worker != nullptr) {
        worker->isFinished = false;
        emit loadSignal(++loadingNr, center, userMoveType, direction, Dataset::datasets, Segmentation::singleton().layerId);
    }
}

void Loader::Worker::broadcastProgress(bool startup) {
    std::size_t count{0};
    for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
        count += slotDownload[layerId].size() + slotDecompression[layerId].size();
    }
    isFinished = count == 0;
    emit progress(startup, count);
}

void Loader::Worker::downloadAndLoadCubes(const unsigned int loadingNr, const Coordinate center, const UserMoveType userMoveType, const floatCoordinate & direction, const Dataset::list_t & changedDatasets, const size_t segmentationLayer) {
    cleanup(center);
    // freeSlots[] are lists of pointers to locations that
    // can hold data or overlay cubes. Whenever we want to load a new
    // datacube, we load it into a location from this list. Whenever a
    // datacube in memory becomes invalid, we add the pointer to its
    // memory location back into this list.
    for (std::size_t layerId{0}; layerId < changedDatasets.size(); ++layerId) {
        state->cube2Pointer[layerId].resize(std::log2(changedDatasets[layerId].highestAvailableMag)+1);
        if (datasets[layerId].allocationEnabled && !changedDatasets[layerId].allocationEnabled) {
            unloadCurrentMagnification(layerId);
            slotChunk[layerId].clear();
            freeSlots[layerId].clear();
        }
        if (!changedDatasets[layerId].allocationEnabled || datasets[layerId].allocationEnabled) {
            continue;
        }
        const auto overlayFactor = changedDatasets[layerId].isOverlay() ? OBJID_BYTES : 1;
        qDebug() << "Allocating" << state->cubeSetBytes * overlayFactor / 1024. / 1024. << "MiB for cubes.";
        for (std::size_t i = 0; i < state->cubeSetBytes * overlayFactor; i += state->cubeBytes * overlayFactor) {
            slotChunk[layerId].emplace_back(state->cubeBytes * overlayFactor, 0);// zero init chunk of chars
            freeSlots[layerId].emplace_back(slotChunk[layerId].back().data());// append newest element
        }
    }
    datasets = changedDatasets;
    snappyLayerId = segmentationLayer;

    const auto magnification = datasets.front().magnification;
    loaderMagnification = datasets.front().magIndex;
    const auto cubeEdgeLen = datasets.front().cubeEdgeLength;
    const auto Dcoi = DcoiFromPos(center.cube(cubeEdgeLen, magnification), userMoveType, direction);//datacubes of interest prioritized around the current position
    //split dcoi into slice planes and rest
    std::vector<std::pair<std::size_t, Coordinate>> allCubes;
    for (auto && todo : Dcoi) {
        const Coordinate globalCoord = todo.cube2Global(cubeEdgeLen, magnification);
        QMutexLocker locker(&state->protectCube2Pointer);
        for (std::size_t layerId{0}; layerId < datasets.size(); ++layerId) {
            // only queue downloads which are necessary
            if (cubeQuery(state->cube2Pointer, layerId, loaderMagnification, globalCoord.cube(cubeEdgeLen, magnification)) == nullptr) {
                allCubes.emplace_back(layerId, globalCoord);
            }
        }
    }

    auto startDownload = [this, center](const std::size_t layerId, const Dataset dataset, const Coordinate globalCoord, decltype(slotDownload)::value_type & downloads
            , decltype(slotDecompression)::value_type & decompressions, decltype(freeSlots)::value_type & freeSlots, decltype(state->cube2Pointer)::value_type::value_type & cubeHash){
        if (dataset.isOverlay()) {
            auto snappyIt = snappyCache[loaderMagnification].find(globalCoord.cube(dataset.cubeEdgeLength, dataset.magnification));
            if (snappyIt != std::end(snappyCache[loaderMagnification])) {
                if (!freeSlots.empty()) {
                    auto downloadIt = downloads.find(globalCoord);
                    if (downloadIt != std::end(downloads)) {
                        downloadIt->second->abort();
                    }
                    auto decompressionIt = decompressions.find(globalCoord);
                    if (decompressionIt != std::end(decompressions)) {
                        decompressionIt->second->waitForFinished();
                    }
                    const auto cubeCoord = globalCoord.cube(dataset.cubeEdgeLength, dataset.magnification);
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
                        cubeHash[globalCoord.cube(dataset.cubeEdgeLength, dataset.magnification)] = currentSlot;
                        state->protectCube2Pointer.unlock();

                        state->viewer->reslice_notify_all(layerId, globalCoord);
                    } else {
                        freeSlots.emplace_back(currentSlot);
                        qCritical() << layerId << globalCoord << "snappy extract failed" << snappyIt->second.size();
                    }
                } else {
                    qCritical() << layerId << globalCoord << "no slots for snappy extract" << cubeHash.size() << freeSlots.size();
                }
                return;
            }
        }
        state->protectCube2Pointer.lock();
        const bool cubeNotAlreadyLoaded = cubeHash.count(globalCoord.cube(dataset.cubeEdgeLength, dataset.magnification)) == 0;
        state->protectCube2Pointer.unlock();
        const bool cubeNotDownloading = downloads.find(globalCoord) == std::end(downloads);
        const bool cubeNotDecompressing = decompressions.find(globalCoord) == std::end(decompressions);

        if (cubeNotAlreadyLoaded && cubeNotDownloading && cubeNotDecompressing) {
            if (dataset.type == Dataset::CubeType::SNAPPY) {
                if (!freeSlots.empty()) {
                    auto * currentSlot = freeSlots.front();
                    freeSlots.pop_front();
                    std::fill(reinterpret_cast<std::uint8_t *>(currentSlot), reinterpret_cast<std::uint8_t *>(currentSlot) + state->cubeBytes * (dataset.isOverlay() ? OBJID_BYTES : 1), 0);
                    state->protectCube2Pointer.lock();
                    cubeHash[globalCoord.cube(dataset.cubeEdgeLength, dataset.magnification)] = currentSlot;
                    state->protectCube2Pointer.unlock();
                    state->viewer->reslice_notify_all(layerId, globalCoord);
                } else {
                    qCritical() << layerId << globalCoord << "no slots for snappy extract" << cubeHash.size() << freeSlots.size();
                }
                return;
            }

            auto request = dataset.apiSwitch(globalCoord);
//            request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
//            request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);

            QByteArray payload;
            if (globalCoord == center.cube(dataset.cubeEdgeLength, dataset.magnification).cube2Global(dataset.cubeEdgeLength, dataset.magnification)) {
                //the first download usually finishes last (which is a bug) so we put it alone in the high priority bucket
                request.setPriority(QNetworkRequest::HighPriority);
            }
            auto * reply = [&]{
                if (dataset.api == Dataset::API::GoogleBrainmaps) {
                    request.setRawHeader("Content-Type", "application/octet-stream");
                    const QString json(R"json({"geometry":{"corner":"%1,%2,%3", "size":"%4,%4,%4", "scale":%5}, "subvolume_format":"SINGLE_IMAGE", "image_format_options":{"image_format":"JPEG", "jpeg_quality":70}})json");
                    const auto magCoord = globalCoord / dataset.magnification;
                    payload = json.arg(magCoord.x).arg(magCoord.y).arg(magCoord.z).arg(dataset.cubeEdgeLength).arg(loaderMagnification).toUtf8();
                } else if (dataset.api == Dataset::API::WebKnossos) {
                    request.setRawHeader("Content-Type", "application/json");
                    payload = QString{R"json([{"position":[%1,%2,%3],"zoomStep":%4,"cubeSize":%5,"fourBit":false}])json"}.arg(globalCoord.x).arg(globalCoord.y).arg(globalCoord.z).arg(static_cast<std::size_t>(std::log2(dataset.magnification))).arg(dataset.cubeEdgeLength).toUtf8();
                }
                if (dataset.api == Dataset::API::WebKnossos || dataset.api == Dataset::API::GoogleBrainmaps) {
                    return qnam.post(request, payload);
                } else {
                    return qnam.get(request);
                }
            }();

            reply->setParent(nullptr);//reparent, so it don’t gets destroyed with qnam
            downloads[globalCoord] = reply;
            broadcastProgress(true);
            QObject::connect(reply, &QNetworkReply::finished, this, [this, layerId, dataset, reply, globalCoord, &downloads, &decompressions, &freeSlots, &cubeHash](){
                if (freeSlots.empty()) {
                    qCritical() << layerId << globalCoord << static_cast<int>(dataset.type) << "no slots for decompression" << cubeHash.size() << freeSlots.size();
                    reply->deleteLater();
                    downloads.erase(globalCoord);
                    broadcastProgress();
                    return;
                }
                if (reply->error() == QNetworkReply::NoError) {
                    auto * currentSlot = freeSlots.front();
                    freeSlots.pop_front();
                    auto * watcher = new QFutureWatcher<DecompressionResult>;
                    QObject::connect(watcher, &QFutureWatcher<DecompressionResult>::finished, this, [this, reply, dataset, layerId, &freeSlots, &decompressions, globalCoord, watcher, currentSlot](){
                        if (!watcher->isCanceled()) {
                            auto result = watcher->result();

                            if (!result.first) {//decompression unsuccessful
                                qCritical() << layerId << globalCoord << static_cast<int>(dataset.type) << "decompression failed → no fill";
                                freeSlots.emplace_back(result.second);
                            }
                        } else {
                            qCritical() << layerId << globalCoord << static_cast<int>(dataset.type) << "future canceled";
                            freeSlots.emplace_back(currentSlot);
                        }
                        reply->deleteLater();
                        decompressions.erase(globalCoord);
                        broadcastProgress();
                    });
                    decompressions[globalCoord].reset(watcher);
                    downloads.erase(globalCoord);
                    watcher->setFuture(QtConcurrent::run(&decompressionPool, std::bind(&decompressCube, currentSlot, std::ref(*reply), layerId, dataset, std::ref(cubeHash), globalCoord)));
                } else {
                    if (reply->error() == QNetworkReply::ContentNotFoundError) {//404 → fill
                        auto * currentSlot = freeSlots.front();
                        freeSlots.pop_front();
                        std::fill(reinterpret_cast<std::uint8_t *>(currentSlot), reinterpret_cast<std::uint8_t *>(currentSlot) + state->cubeBytes * (dataset.isOverlay() ? OBJID_BYTES : 1), 0);
                        state->protectCube2Pointer.lock();
                        cubeHash[globalCoord.cube(dataset.cubeEdgeLength, dataset.magnification)] = currentSlot;
                        state->protectCube2Pointer.unlock();
                        state->viewer->reslice_notify_all(layerId, globalCoord);
                    } else {
                        if (reply->error() != QNetworkReply::OperationCanceledError) {
                            qCritical() << layerId << globalCoord << static_cast<int>(dataset.type) << reply->request().url() << reply->errorString() << reply->readAll();
                            if (dataset.api == Dataset::API::GoogleBrainmaps) {
                                qDebug() << "GoogleBrainmaps error" << reply->error();
                                if (reply->error() == QNetworkReply::ContentAccessDenied || reply->error() == QNetworkReply::AuthenticationRequiredError) {
                                    auto pair = getBrainmapsToken();
                                    if (pair.first) {
                                        Dataset::datasets[layerId].token = datasets[layerId].token = pair.second;
                                    }
                                }
                            }
                        }
                    }
                    reply->deleteLater();
                    downloads.erase(globalCoord);
                    broadcastProgress();
                }
            });
        }
    };

    const auto workaroundProcessLocalImmediately = datasets[0].url.scheme() == "file" ? [](){QCoreApplication::processEvents();} : [](){};
    for (auto [layerId, globalCoord] : allCubes) {
        if (loadingNr == Loader::Controller::singleton().loadingNr) {
            if (datasets[layerId].loadingEnabled) {
                try {
                    startDownload(layerId, datasets[layerId], globalCoord, slotDownload[layerId], slotDecompression[layerId], freeSlots[layerId], state->cube2Pointer.at(layerId).at(loaderMagnification));
                } catch (const std::out_of_range &) {}
                workaroundProcessLocalImmediately();//https://bugreports.qt.io/browse/QTBUG-45925
            }
        }
    }
}
