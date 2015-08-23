/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */
#include "loader.h"

#include "network.h"
#include "segmentation/segmentation.h"
#include "session.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <quazip.h>
#include <quazipfile.h>

#include <snappy.h>

#include <QFile>
#include <QFuture>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtConcurrent/QtConcurrent>

#include <cmath>
#include <fstream>
#include <stdexcept>

extern "C" {
    int jp2_decompress_main(char *infile, char *buf, int bufsize);
}

#include <dirent.h>
#include <sys/stat.h>
#ifdef KNOSSOS_USE_TURBOJPEG
#include <turbojpeg.h>
#endif

bool isOverlay(const Loader::CubeType type);

constexpr bool inRange(const int value, const int min, const int max) {
    return value >= min && value < max;
}

bool insideCurrentSupercube(const Coordinate & coord, const Coordinate & center, const int & cubesPerDimension, const int & cubeSize) {
    const int halfSupercube = cubeSize * cubesPerDimension * 0.5;
    const int xcube = center.x - center.x % cubeSize + cubeSize /2;
    const int ycube = center.y - center.y % cubeSize + cubeSize / 2 ;
    const int zcube = center.z - center.z % cubeSize + cubeSize / 2;
    bool valid = true;
    valid &= inRange(coord.x, xcube - halfSupercube, xcube + halfSupercube);
    valid &= inRange(coord.y, ycube - halfSupercube, ycube + halfSupercube);
    valid &= inRange(coord.z, zcube - halfSupercube, zcube + halfSupercube);
    return valid;
}

bool currentlyVisible(const Coordinate & coord, const Coordinate & center, const int & cubesPerDimension, const int & cubeSize) {
    bool valid = insideCurrentSupercube(coord, center, cubesPerDimension, cubeSize);
    const int xmin = center.x - center.x % cubeSize;
    const int ymin = center.y - center.y % cubeSize;
    const int zmin = center.z - center.z % cubeSize;
    const bool xvalid = valid & inRange(coord.x, xmin, xmin + cubeSize);
    const bool yvalid = valid & inRange(coord.y, ymin, ymin + cubeSize);
    const bool zvalid = valid & inRange(coord.z, zmin, zmin + cubeSize);
    return xvalid || yvalid || zvalid;
}
//generalizing this needs polymorphic lambdas or return type deduction
auto currentlyVisibleWrap = [](const Coordinate & center){
    return [&center](const Coordinate & coord){
        return currentlyVisible(coord, center, state->M, state->cubeEdgeLength * state->magnification);
    };
};
auto insideCurrentSupercubeWrap = [](const Coordinate & center){
    return [&center](const Coordinate & coord){
        return insideCurrentSupercube(coord, center, state->M, state->cubeEdgeLength * state->magnification);
    };
};
bool currentlyVisibleWrapWrap(const Coordinate & center, const Coordinate & coord) {
    return currentlyVisibleWrap(center)(coord);
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
    state->viewer->oc_reslice_notify_all(cubeCoord.cube2Global(state->cubeEdgeLength, state->magnification));

}

decltype(Loader::Worker::snappyCache) Loader::Controller::getAllModifiedCubes() {
    if (worker != nullptr) {
        worker->snappyMutex.lock();
        //signal to run in loader thread
        QTimer::singleShot(0, worker.get(), &Loader::Worker::snappyCacheFlush);
        worker->snappyFlushCondition.wait(&worker->snappyMutex);
        worker->snappyMutex.unlock();
        return worker->snappyCache;
    } else {
        return decltype(Loader::Worker::snappyCache)();//{} is not working
    }
}

bool Loader::Controller::isFinished() {
    return worker != nullptr ? worker->isFinished.load() : true;//no loader == done?
}

void Loader::Worker::CalcLoadOrderMetric(float halfSc, floatCoordinate currentMetricPos, floatCoordinate direction, float *metrics) {
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

    switch (state->loaderUserMoveType) {
    case USERMOVE_HORIZONTAL:
    case USERMOVE_DRILL:
        distance_from_plane = CALC_POINT_DISTANCE_FROM_PLANE(currentMetricPos, direction);
        dot_product = CALC_DOT_PRODUCT(currentMetricPos, direction);

        if (USERMOVE_HORIZONTAL == state->loaderUserMoveType) {
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
        // Priorities are XY->YZ->XZ
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

std::vector<CoordOfCube> Loader::Worker::DcoiFromPos(const Coordinate & center) {
    const float floatHalfSc = state->M / 2.;
    const int halfSc = std::floor(floatHalfSc);
    const floatCoordinate direction = state->loaderUserMoveViewportDirection;
    const int cubeElemCount = state->cubeSetElements;
    const auto currentOrigin = center.cube(state->cubeEdgeLength, state->magnification);

    int i = 0;
    currentMaxMetric = 0;
    std::vector<LO_Element> DcArray(cubeElemCount);
    for (int x = -halfSc; x < halfSc + 1; ++x) {
        for (int y = -halfSc; y < halfSc + 1; ++y) {
            for (int z = -halfSc; z < halfSc + 1; ++z) {
                DcArray[i].coordinate = {currentOrigin.x + x, currentOrigin.y + y, currentOrigin.z + z};
                DcArray[i].offset = {x, y, z};
                floatCoordinate currentMetricPos(x, y, z);
                CalcLoadOrderMetric(floatHalfSc, currentMetricPos, direction, &DcArray[i].loadOrderMetrics[0]);
                ++i;
            }
        }
    }

    // Metrics are done, reset user-move variables
    state->loaderUserMoveType = USERMOVE_NEUTRAL;
    state->loaderUserMoveViewportDirection = {0, 0, 0};

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

Loader::Worker::Worker(const QUrl & baseUrl, const Loader::API api, const Loader::CubeType typeDc, const Loader::CubeType typeOc, const QString & experimentName)
    : baseUrl{baseUrl}, api{api}, typeDc{typeDc}, typeOc{typeOc}, experimentName{experimentName}, OcModifiedCacheQueue(std::log2(state->highestAvailableMag)+1), snappyCache(std::log2(state->highestAvailableMag)+1)
{

    // freeDcSlots / freeOcSlots are lists of pointers to locations that
    // can hold data or overlay cubes. Whenever we want to load a new
    // datacube, we load it into a location from this list. Whenever a
    // datacube in memory becomes invalid, we add the pointer to its
    // memory location back into this list.

    qDebug() << "Allocating" << state->cubeSetBytes << "bytes for the datacubes.";
    for(size_t i = 0; i < state->cubeSetBytes; i += state->cubeBytes) {
        DcSetChunk.emplace_back(state->cubeBytes, 0);//zero init chunk of chars
        freeDcSlots.emplace_back(DcSetChunk.back().data());//append newest element
    }

    if(state->overlay) {
        qDebug() << "Allocating" << state->cubeSetBytes * OBJID_BYTES << "bytes for the overlay cubes.";
        for(size_t i = 0; i < state->cubeSetBytes * OBJID_BYTES; i += state->cubeBytes * OBJID_BYTES) {
            OcSetChunk.emplace_back(state->cubeBytes * OBJID_BYTES, 0);//zero init chunk of chars
            freeOcSlots.emplace_back(OcSetChunk.back().data());//append newest element
        }
    }
}

Loader::Worker::~Worker() {
    abortDownloadsFinishDecompression();

    if (state->quitSignal) {
        return;//state is dead already
    }

    state->protectCube2Pointer.lock();
    for (auto &elem : state->Dc2Pointer) { elem.clear(); }
    for (auto &elem : state->Oc2Pointer) { elem.clear(); }
    state->protectCube2Pointer.unlock();
}

template<typename CubeHash, typename Slots, typename Keep>
void unloadCubes(CubeHash & loadedCubes, Slots & freeSlots, Keep keep) {
    unloadCubes(loadedCubes, freeSlots, keep, [](const CoordOfCube &, char *){});
}

template<typename CubeHash, typename Slots, typename Keep, typename UnloadHook>
void unloadCubes(CubeHash & loadedCubes, Slots & freeSlots, Keep keep, UnloadHook todo) {
    for (auto it = std::begin(loadedCubes); it != std::end(loadedCubes);) {
        if (!keep(it->first.cube2Global(state->cubeEdgeLength, state->magnification))) {
            todo(CoordOfCube(it->first.x, it->first.y, it->first.z), it->second);
            freeSlots.emplace_back(it->second);
            it = loadedCubes.erase(it);
        } else {
            ++it;
        }
    }
}

void Loader::Worker::unloadCurrentMagnification() {
    abortDownloadsFinishDecompression([](const Coordinate &){return false;});

    state->protectCube2Pointer.lock();
    for (auto &elem : state->Dc2Pointer[loaderMagnification]) {
        freeDcSlots.emplace_back(elem.second);
    }
    state->Dc2Pointer[loaderMagnification].clear();
    for (auto &elem : state->Oc2Pointer[loaderMagnification]) {
        const auto cubeCoord = elem.first;
        const auto remSlotPtr = elem.second;
        freeOcSlots.emplace_back(elem.second);
        if (OcModifiedCacheQueue[loaderMagnification].find(cubeCoord) != std::end(OcModifiedCacheQueue[loaderMagnification])) {
            snappyCacheBackupRaw(cubeCoord, remSlotPtr);
            //remove from work queue
            OcModifiedCacheQueue[loaderMagnification].erase(cubeCoord);
        }
    }
    state->Oc2Pointer[loaderMagnification].clear();
    state->protectCube2Pointer.unlock();
}

void Loader::Worker::markOcCubeAsModified(const CoordOfCube &cubeCoord, const int magnification) {
    OcModifiedCacheQueue[std::log2(magnification)].emplace(cubeCoord);
}

void Loader::Worker::snappyCacheSupplySnappy(const CoordOfCube cubeCoord, const int magnification, const std::string cube) {
    const auto cubeMagnification = std::log2(magnification);
    snappyCache[cubeMagnification].emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple(cube));

    if (cubeMagnification == loaderMagnification) {//unload if currently loaded
        const auto globalCoord = cubeCoord.cube2Global(state->cubeEdgeLength, state->magnification);
        auto downloadIt = ocDownload.find(globalCoord);
        if (downloadIt != std::end(ocDownload)) {
            downloadIt->second->abort();
        }
        auto decompressionIt = ocDecompression.find(globalCoord);
        if (decompressionIt != std::end(ocDecompression)) {
            decompressionIt->second->waitForFinished();
        }
        state->protectCube2Pointer.lock();
        const auto coord = cubeCoord;
        auto cubePtr = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[loaderMagnification], coord);
        if (cubePtr != nullptr) {
            freeOcSlots.emplace_back(cubePtr);
            state->Oc2Pointer[loaderMagnification].erase(coord);
        }
        state->protectCube2Pointer.unlock();
    }
}

void Loader::Worker::snappyCacheBackupRaw(const CoordOfCube & cubeCoord, const char * cube) {
    //insert empty string into snappy cache
    auto snappyIt = snappyCache[loaderMagnification].emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple()).first;
    //compress cube into the new string
    snappy::Compress(reinterpret_cast<const char *>(cube), OBJID_BYTES * state->cubeBytes, &snappyIt->second);
}

void Loader::Worker::snappyCacheClear() {
    //unload all modified cubes
    for (std::size_t mag = 0; mag < OcModifiedCacheQueue.size(); ++mag) {
        unloadCubes(state->Oc2Pointer[mag], freeOcSlots, [this, mag](const Coordinate & globalCoord){
            const auto cubeCoord = globalCoord.cube(state->cubeEdgeLength, state->magnification);
            const bool unflushed = OcModifiedCacheQueue[mag].find(cubeCoord) != std::end(OcModifiedCacheQueue[mag]);
            const bool flushed = snappyCache[mag].find(cubeCoord) != std::end(snappyCache[mag]);
            return !unflushed && !flushed;//only keep cubes which are neither in snappy cache nor in modified queue
        });
        OcModifiedCacheQueue[mag].clear();
        snappyCache[mag].clear();
    }
    state->viewer->loader_notify();//a bit of a detour…
}

void Loader::Worker::snappyCacheFlush() {
    snappyMutex.lock();

    for (std::size_t mag = 0; mag < OcModifiedCacheQueue.size(); ++mag) {
        for (const auto & cubeCoord : OcModifiedCacheQueue[mag]) {
            state->protectCube2Pointer.lock();
            auto cube = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[mag], {cubeCoord.x, cubeCoord.y, cubeCoord.z});
            state->protectCube2Pointer.unlock();
            if (cube != nullptr) {
                snappyCacheBackupRaw(cubeCoord, cube);
            }
        }
        //clear work queue
        OcModifiedCacheQueue[mag].clear();
    }

    snappyFlushCondition.wakeAll();
    snappyMutex.unlock();
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
    abortDownloadsFinishDecompression([](const Coordinate &){return false;});
}

template<typename Func>
void Loader::Worker::abortDownloadsFinishDecompression(Func keep) {
    abortDownloads(dcDownload, keep);
    abortDownloads(ocDownload, keep);
    finishDecompression(dcDecompression, keep);
    finishDecompression(ocDecompression, keep);
}

std::pair<bool, char*> decompressCube(char * currentSlot, QIODevice & reply, const Loader::CubeType type, coord2bytep_map_t & cubeHash, const Coordinate globalCoord, const int magnification) {
    QThread::currentThread()->setPriority(QThread::IdlePriority);
    bool success = false;

    auto data = reply.read(reply.bytesAvailable());//readAll can be very slow – https://bugreports.qt.io/browse/QTBUG-45926
    const std::size_t availableSize = data.size();
    if (type == Loader::CubeType::RAW_UNCOMPRESSED) {
        const std::size_t expectedSize = state->cubeBytes;
        if (availableSize == expectedSize) {
            std::copy(std::begin(data), std::end(data), currentSlot);
            success = true;
        }
    } else if (type == Loader::CubeType::RAW_JPG) {
        const auto image = QImage::fromData(data).convertToFormat(QImage::Format_Indexed8);
        const qint64 expectedSize = state->cubeBytes;
        if (image.byteCount() == expectedSize) {
            std::copy(image.bits(), image.bits() + image.byteCount(), currentSlot);
            success = true;
        }
    } else if (type == Loader::CubeType::RAW_J2K || type == Loader::CubeType::RAW_JP2_6) {
        QTemporaryFile file(QDir::tempPath() + QString("/XXXXXX.%1").arg(type == Loader::CubeType::RAW_J2K ? "j2k" : "jp2"));
        success = file.open();
        success &= file.write(data) == data.size();
        file.close();
        success &= EXIT_SUCCESS == jp2_decompress_main(file.fileName().toUtf8().data(), reinterpret_cast<char*>(currentSlot), state->cubeBytes);
    } else if (type == Loader::CubeType::SEGMENTATION_UNCOMPRESSED) {
        const std::size_t expectedSize = state->cubeBytes * OBJID_BYTES;
        if (availableSize == expectedSize) {
            std::copy(std::begin(data), std::end(data), currentSlot);
            success = true;
        }
    } else if (type == Loader::CubeType::SEGMENTATION_SZ_ZIP) {
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
    }  else {
        qDebug() << "unsupported format";
    }

    if (success) {
        state->protectCube2Pointer.lock();
        cubeHash[globalCoord.cube(state->cubeEdgeLength, magnification)] = currentSlot;
        state->protectCube2Pointer.unlock();
        if (isOverlay(type)) {
            state->viewer->oc_reslice_notify_all(globalCoord);
        } else {
            state->viewer->dc_reslice_notify_all(globalCoord);
        }
    }

    return {success, currentSlot};
}

QUrl knossosCubeUrl(QUrl base, QString && experimentName, const Coordinate & coord, const int cubeEdgeLength, const int magnification, const Loader::CubeType type) {
    const auto cubeCoord = coord.cube(cubeEdgeLength, magnification);
    auto pos = QString("/mag%1/x%2/y%3/z%4/")
            .arg(magnification)
            .arg(cubeCoord.x, 4, 10, QChar('0'))
            .arg(cubeCoord.y, 4, 10, QChar('0'))
            .arg(cubeCoord.z, 4, 10, QChar('0'));
    auto filename = QString(("%1_mag%2_x%3_y%4_z%5%6"))//2012-03-07_AreaX14_mag1_x0000_y0000_z0000.j2k
            .arg(experimentName.section(QString("_mag"), 0, 0))
            .arg(magnification)
            .arg(cubeCoord.x, 4, 10, QChar('0'))
            .arg(cubeCoord.y, 4, 10, QChar('0'))
            .arg(cubeCoord.z, 4, 10, QChar('0'));

    if (type == Loader::CubeType::RAW_UNCOMPRESSED) {
        filename = filename.arg(".raw");
    } else if (type == Loader::CubeType::RAW_JPG) {
        filename = filename.arg(".jpg");
    } else if (type == Loader::CubeType::RAW_J2K) {
        filename = filename.arg(".j2k");
    } else if (type == Loader::CubeType::RAW_JP2_6) {
        filename = filename.arg(".6.jp2");
    } else if (type == Loader::CubeType::SEGMENTATION_SZ_ZIP) {
        filename = filename.arg(".seg.sz.zip");
    } else if (type == Loader::CubeType::SEGMENTATION_UNCOMPRESSED) {
        filename = filename.arg(".seg");
    }

    base.setPath(base.path() + pos + filename);

    return base;
}

QUrl googleCubeUrl(QUrl base, Coordinate coord, const int scale, const int cubeEdgeLength, const Loader::CubeType type) {
    auto query = QUrlQuery(base);
    auto path = base.path() + "/binary/subvolume";

    if (type == Loader::CubeType::RAW_UNCOMPRESSED) {
        path += "/format=raw";
    } else if (type == Loader::CubeType::RAW_JPG) {
        path += "/format=singleimage";
    }

    path += "/scale=" + QString::number(scale);// >= 0
    path += "/size=" + QString("%1,%1,%1").arg(cubeEdgeLength);// <= 128³
    path += "/corner=" + QString("%1,%2,%3").arg(coord.x).arg(coord.y).arg(coord.z);

    query.addQueryItem("alt", "media");

    base.setPath(path);
    base.setQuery(query);
    //<volume_id>/binary/subvolume/corner=5376,5504,2944/size=64,64,64/scale=0/format=singleimage?access_token=<oauth2_token>
    return base;
}

QUrl webKnossosCubeUrl(QUrl base, Coordinate coord, const int unknownScale, const int cubeEdgeLength, const Loader::CubeType type) {
    auto query = QUrlQuery(base);
    query.addQueryItem("cubeSize", QString::number(cubeEdgeLength));

    QString layer;
    if (type == Loader::CubeType::RAW_UNCOMPRESSED) {
        layer = "color";
    } else if (type == Loader::CubeType::SEGMENTATION_UNCOMPRESSED) {
        layer = "volume";
    }

    auto path = base.path() + "/layers/" + layer + "/mag%1/x%2/y%3/z%4/bucket.raw";//mag >= 1
    path = path.arg(unknownScale).arg(coord.x / state->cubeEdgeLength).arg(coord.y / state->cubeEdgeLength).arg(coord.z / state->cubeEdgeLength);
    base.setPath(path);
    base.setQuery(query);

    return base;
}

void Loader::Worker::cleanup(const Coordinate center) {
    abortDownloadsFinishDecompression(currentlyVisibleWrap(center));
    state->protectCube2Pointer.lock();
    unloadCubes(state->Dc2Pointer[loaderMagnification], freeDcSlots, insideCurrentSupercubeWrap(center));
    unloadCubes(state->Oc2Pointer[loaderMagnification], freeOcSlots, insideCurrentSupercubeWrap(center), [this](const CoordOfCube & cubeCoord, char * remSlotPtr){
        if (OcModifiedCacheQueue[loaderMagnification].find(cubeCoord) != std::end(OcModifiedCacheQueue[loaderMagnification])) {
            snappyCacheBackupRaw(cubeCoord, remSlotPtr);
            //remove from work queue
            OcModifiedCacheQueue[loaderMagnification].erase(cubeCoord);
        }
    });
    state->protectCube2Pointer.unlock();
}

void Loader::Controller::startLoading(const Coordinate & center) {
    if (worker != nullptr) {
        worker->isFinished = false;
        emit loadSignal(++loadingNr, center);
    }
}

bool isOverlay(const Loader::CubeType type) {
    switch (type) {
    case Loader::CubeType::RAW_UNCOMPRESSED:
    case Loader::CubeType::RAW_JPG:
    case Loader::CubeType::RAW_J2K:
    case Loader::CubeType::RAW_JP2_6:
        return false;
    case Loader::CubeType::SEGMENTATION_UNCOMPRESSED:
    case Loader::CubeType::SEGMENTATION_SZ_ZIP:
        return true;
    };
    throw std::runtime_error("unknown value for Loader::CubeType");
}

void Loader::Worker::broadcastProgress(bool startup) {
    auto count = dcDownload.size() + dcDecompression.size() + ocDownload.size() + ocDecompression.size();
    isFinished = count == 0;
    emit progress(startup, count);
}

void Loader::Worker::downloadAndLoadCubes(const unsigned int loadingNr, const Coordinate center) {
    QTime time;
    time.start();

    cleanup(center);
    loaderMagnification = std::log2(state->magnification);

    const auto Dcoi = DcoiFromPos(center);//datacubes of interest prioritized around the current position
    //split dcoi into slice planes and rest
    std::vector<Coordinate> allCubes;
    std::vector<Coordinate> visibleCubes;
    std::vector<Coordinate> cacheCubes;
    for (auto && todo : Dcoi) {
        const Coordinate globalCoord = todo.cube2Global(state->cubeEdgeLength, state->magnification);
        state->protectCube2Pointer.lock();
        const bool dcNotAlreadyLoaded = Coordinate2BytePtr_hash_get_or_fail(state->Dc2Pointer[loaderMagnification], globalCoord.cube(state->cubeEdgeLength, state->magnification)) == nullptr;
        const bool ocNotAlreadyLoaded = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[loaderMagnification], globalCoord.cube(state->cubeEdgeLength, state->magnification)) == nullptr;
        state->protectCube2Pointer.unlock();
        if (dcNotAlreadyLoaded || ocNotAlreadyLoaded) {//only queue downloads which are necessary
            allCubes.emplace_back(globalCoord);
            if (currentlyVisibleWrap(center)(globalCoord)) {
                visibleCubes.emplace_back(globalCoord);
            } else {
                cacheCubes.emplace_back(globalCoord);
            }
        }
    }

    auto startDownload = [this, center](const Coordinate globalCoord, const CubeType type, decltype(dcDownload) & downloads, decltype(dcDecompression) & decompressions, decltype(freeDcSlots) & freeSlots, decltype(state->Dc2Pointer[0]) & cubeHash){
        if (isOverlay(type)) {
            auto snappyIt = snappyCache[loaderMagnification].find(globalCoord.cube(state->cubeEdgeLength, state->magnification));
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
                    const auto cubeCoord = globalCoord.cube(state->cubeEdgeLength, state->magnification);
                    state->protectCube2Pointer.lock();
                    auto * currentSlot = Coordinate2BytePtr_hash_get_or_fail(cubeHash, cubeCoord);
                    cubeHash.erase(cubeCoord);
                    state->protectCube2Pointer.unlock();
                    if (currentSlot == nullptr) {
                        currentSlot = freeSlots.front();
                        freeSlots.pop_front();
                    }
                    //directly uncompress snappy cube into the OC slot
                    const auto success = snappy::RawUncompress(snappyIt->second.c_str(), snappyIt->second.size(), reinterpret_cast<char*>(currentSlot));
                    if (success) {
                        state->protectCube2Pointer.lock();
                        cubeHash[globalCoord.cube(state->cubeEdgeLength, state->magnification)] = currentSlot;
                        state->protectCube2Pointer.unlock();

                        state->viewer->oc_reslice_notify_all(globalCoord);
                    } else {
                        freeSlots.emplace_back(currentSlot);
                        qCritical() << globalCoord.x << globalCoord.y << globalCoord.z << "snappy extract" << snappyIt->second.size() << "failed";
                    }
                } else {
                    qCritical() << globalCoord.x << globalCoord.y << globalCoord.z << "no slots";
                }
                return;
            }
        }

        auto apiSwitch = [this](const Coordinate globalCoord, const CubeType type){
            switch (api) {
            case Loader::API::GoogleBrainmaps:
                return googleCubeUrl(baseUrl, globalCoord, loaderMagnification, state->cubeEdgeLength, type);
            case Loader::API::Heidelbrain:
                return knossosCubeUrl(baseUrl, QString(state->name), globalCoord, state->cubeEdgeLength, state->magnification, type);
            case Loader::API::WebKnossos:
                return webKnossosCubeUrl(baseUrl, globalCoord, loaderMagnification + 1, state->cubeEdgeLength, type);
            }
            throw std::runtime_error("unknown value for Loader::API");
        };
        QUrl dcUrl = apiSwitch(globalCoord, type);

        state->protectCube2Pointer.lock();
        const bool cubeNotAlreadyLoaded = Coordinate2BytePtr_hash_get_or_fail(cubeHash, globalCoord.cube(state->cubeEdgeLength, state->magnification)) == nullptr;
        state->protectCube2Pointer.unlock();
        const bool cubeNotDownloading = downloads.find(globalCoord) == std::end(downloads);
        const bool cubeNotDecompressing = decompressions.find(globalCoord) == std::end(decompressions);

        if (cubeNotAlreadyLoaded && cubeNotDownloading && cubeNotDecompressing) {
            //transform googles oauth2 token from query item to request header
            QUrlQuery originalQuery(dcUrl);
            auto reducedQuery = originalQuery;
            reducedQuery.removeQueryItem("access_token");
            dcUrl.setQuery(reducedQuery);

            auto request = QNetworkRequest(dcUrl);

            if (originalQuery.hasQueryItem("access_token")) {
                const auto authorization =  QString("Bearer ") + originalQuery.queryItemValue("access_token");
                request.setRawHeader("Authorization", authorization.toUtf8());
            }
            //request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
            //request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
            if (globalCoord == center.cube(state->cubeEdgeLength, state->magnification).cube2Global(state->cubeEdgeLength, state->magnification)) {
                //the first download usually finishes last (which is a bug) so we put it alone in the high priority bucket
                request.setPriority(QNetworkRequest::HighPriority);
            }
            auto * reply = qnam.get(request);
            reply->setParent(nullptr);//reparent, so it don’t gets destroyed with qnam
            downloads[globalCoord] = reply;
            broadcastProgress(true);
            QObject::connect(reply, &QNetworkReply::finished, [this, reply, type, globalCoord, center, &downloads, &decompressions, &freeSlots, &cubeHash](){
                if (freeSlots.empty()) {
                    qCritical() << "no slots" << static_cast<int>(type) << cubeHash.size() << freeSlots.size();
                    downloads[globalCoord]->deleteLater();
                    downloads.erase(globalCoord);
                    broadcastProgress();
                    return;
                }
                auto * currentSlot = freeSlots.front();
                freeSlots.pop_front();
                if (reply->error() == QNetworkReply::NoError) {

                    auto future = QtConcurrent::run(&decompressionPool, std::bind(&decompressCube, currentSlot, std::ref(*reply), type, std::ref(cubeHash), globalCoord, state->magnification));

                    auto * watcher = new QFutureWatcher<DecompressionResult>;
                    QObject::connect(watcher, &QFutureWatcher<DecompressionResult>::finished, [this, &cubeHash, &freeSlots, &downloads, &decompressions, globalCoord, watcher, type, currentSlot](){
                        if (!watcher->isCanceled()) {
                            auto result = watcher->result();

                            if (!result.first) {//decompression unsuccessful
                                qCritical() << globalCoord.x << globalCoord.y << globalCoord.z << "decompression" << static_cast<int>(type) << "failed → no fill";
                                freeSlots.emplace_back(result.second);
                            }
                        } else {
                            qCritical() << globalCoord.x << globalCoord.y << globalCoord.z << "future canceled";
                            freeSlots.emplace_back(currentSlot);
                        }

                        downloads[globalCoord]->deleteLater();
                        downloads.erase(globalCoord);
                        decompressions.erase(globalCoord);
                        broadcastProgress();
                    });
                    decompressions[globalCoord].reset(watcher);
                    watcher->setFuture(future);
                } else {
                    if (reply->error() == QNetworkReply::ContentNotFoundError) {//404 → fill
                        std::fill(currentSlot, currentSlot + state->cubeBytes * (isOverlay(type) ? OBJID_BYTES : 1), 0);
                        state->protectCube2Pointer.lock();
                        cubeHash[globalCoord.cube(state->cubeEdgeLength, state->magnification)] = currentSlot;
                        state->protectCube2Pointer.unlock();
                        state->viewer->oc_reslice_notify_all(globalCoord);
                    } else {
                        if (reply->error() != QNetworkReply::OperationCanceledError) {
                            qCritical() << globalCoord.x << globalCoord.y << globalCoord.z << reply->errorString();
                        }
                        state->protectCube2Pointer.lock();
                        freeSlots.emplace_back(currentSlot);
                        state->protectCube2Pointer.unlock();
                    }
                    downloads[globalCoord]->deleteLater();
                    downloads.erase(globalCoord);
                    broadcastProgress();
                }
            });
        }
    };

    const auto workaroundProcessLocalImmediately = baseUrl.scheme() == "file" ? [](){QCoreApplication::processEvents();} : [](){};
    auto typeDcOverride = state->compressionRatio == 0 ? CubeType::RAW_UNCOMPRESSED : typeDc;
    for (auto globalCoord : allCubes) {
        if (loadingNr == Loader::Controller::singleton().loadingNr) {
            startDownload(globalCoord, typeDcOverride, dcDownload, dcDecompression, freeDcSlots, state->Dc2Pointer[loaderMagnification]);
            if (state->overlay) {
                startDownload(globalCoord, typeOc, ocDownload, ocDecompression, freeOcSlots, state->Oc2Pointer[loaderMagnification]);
            }
            workaroundProcessLocalImmediately();//https://bugreports.qt.io/browse/QTBUG-45925
        }
    }
}
