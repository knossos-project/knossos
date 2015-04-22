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

#include "knossos.h"
#include "network.h"
#include "segmentation/segmentation.h"
#include "session.h"
#include "viewer.h"

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

#include <snappy.h>

#include <QFile>
#include <QFuture>
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
    const int xcube = center.x - center.x % cubeSize;
    const int ycube = center.y - center.y % cubeSize;
    const int zcube = center.z - center.z % cubeSize;
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
auto currentlyVisibleWrap = [](const Coordinate & coord){
    return currentlyVisible(coord, state->currentPositionX, state->M, state->cubeEdgeLength);
};
auto insideCurrentSupercubeWrap = [](const Coordinate & coord){
    return insideCurrentSupercube(coord, state->currentPositionX, state->M, state->cubeEdgeLength);
};

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

#define SET_COORDINATE(coordinate, a, b, c) \
{ \
(coordinate).x = (a); \
(coordinate).y = (b); \
(coordinate).z = (c); \
}

    }
}

int calc_nonzero_sign(float x) {
    if (x > 0) {
        return 1;
    }
    return -1;
}

#define ABS(x) (((x) >= 0) ? (x) : -(x))
#define SQR(x) ((x)*(x))
#define INNER_MULT_VECTOR(v) ((v).x * (v).y * (v).z)
#define CALC_VECTOR_NORM(v) \
    ( \
        sqrt(SQR((v).x) + SQR((v).y) + SQR((v).z)) \
    )
#define CALC_DOT_PRODUCT(a, b) \
    ( \
        ((a).x * (b).x) + ((a).y * (b).y) + ((a).z * (b).z) \
    )
#define CALC_POINT_DISTANCE_FROM_PLANE(point, plane) \
    ( \
        ABS(CALC_DOT_PRODUCT((point), (plane))) / CALC_VECTOR_NORM((plane)) \
    )

void Loader::Worker::CalcLoadOrderMetric(float halfSc, floatCoordinate currentMetricPos, floatCoordinate direction, float *metrics) {
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

    this->currentMaxMetric = MAX(this->currentMaxMetric, i);
    /* qDebug("%f\t%f\t%f\t%f\t%f\t%f",
        currentMetricPos.x, currentMetricPos.y, currentMetricPos.z,
        metrics[0], metrics[1], metrics[2]); */
}

struct LO_Element {
    Coordinate coordinate;
    Coordinate offset;
    float loadOrderMetrics[LL_METRIC_NUM];
};

int Loader::Worker::CompareLoadOrderMetric(const void * a, const void * b) {
    LO_Element *elem_a, *elem_b;
    float m_a, m_b;
    int metric_index;

    elem_a = (LO_Element *)a;
    elem_b = (LO_Element *)b;

    for (metric_index = 0; metric_index < this->currentMaxMetric; metric_index++) {
        m_a = elem_a->loadOrderMetrics[metric_index];
        m_b = elem_b->loadOrderMetrics[metric_index];
        /* qDebug("i %d\ta\t%d,%d,%d\t=\t%f\tb\t%d,%d,%d\t=\t%f", metric_index,
            elem_a->offset.x, elem_a->offset.y, elem_a->offset.z, m_a,
            elem_b->offset.x, elem_b->offset.y, elem_b->offset.z, m_b); */
        if (m_a != m_b) {
            return calc_nonzero_sign(m_a - m_b);
        }
        /* If equal just continue to next comparison level */
    }

    return 0;
}

std::vector<Coordinate> Loader::Worker::DcoiFromPos() {
    Coordinate currentOrigin;
    floatCoordinate currentMetricPos, direction;
    LO_Element *DcArray;
    int cubeElemCount;
    int i;
    int edgeLen, halfSc;
    int x, y, z;
    float floatHalfSc;

    edgeLen = state->M;
    floatHalfSc = (float)edgeLen / 2.;
    halfSc = (int)floorf(floatHalfSc);

    direction = state->loaderUserMoveViewportDirection;

    cubeElemCount = state->cubeSetElements;
    DcArray = (LO_Element*)malloc(sizeof(DcArray[0]) * cubeElemCount);
    if (NULL == DcArray) {
        return {};
    }
    memset(DcArray, 0, sizeof(DcArray[0]) * cubeElemCount);
    currentOrigin = Coordinate::Px2DcCoord(state->currentPositionX, state->cubeEdgeLength);
    i = 0;
    this->currentMaxMetric = 0;
    for (x = -halfSc; x < halfSc + 1; x++) {
        for (y = -halfSc; y < halfSc + 1; y++) {
            for (z = -halfSc; z < halfSc + 1; z++) {
                SET_COORDINATE(DcArray[i].coordinate, currentOrigin.x + x, currentOrigin.y + y, currentOrigin.z + z);
                SET_COORDINATE(DcArray[i].offset, x, y, z);
                SET_COORDINATE(currentMetricPos, (float)x, (float)y, (float)z);
                CalcLoadOrderMetric(floatHalfSc, currentMetricPos, direction, &DcArray[i].loadOrderMetrics[0]);
                i++;
            }
        }
    }

    // Metrics are done, reset user-move variables
    state->loaderUserMoveType = USERMOVE_NEUTRAL;
    SET_COORDINATE(state->loaderUserMoveViewportDirection, 0, 0, 0);

    //TODO i just wanted to get rid of qsort.cpp, feel free to merge the comparitor into this lambda (and add const to the arguments)
    std::sort(DcArray, DcArray+cubeElemCount, [&](const LO_Element & lhs, const LO_Element & rhs){
        return this->CompareLoadOrderMetric(reinterpret_cast<const void*>(&lhs), reinterpret_cast<const void*>(&rhs)) < 0;
    });

    std::vector<Coordinate> cubes;
    for (i = 0; i < cubeElemCount; i++) {
        cubes.emplace_back(DcArray[i].coordinate);
    }
    free(DcArray);

    return cubes;
}

Loader::Worker::Worker(const QUrl & baseUrl, const Loader::API api, const Loader::CubeType typeDc, const Loader::CubeType typeOc, const QString & experimentName)
    : baseUrl{baseUrl}, api{api}, typeDc{typeDc}, typeOc{typeOc}, experimentName{experimentName}
{
    bogusDc = decltype(bogusDc)(state->cubeBytes, 127);
    bogusOc = decltype(bogusOc)(state->cubeBytes * OBJID_BYTES, 0);

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

    state->loaderMagnification = std::log2(state->magnification);
    prevLoaderMagnification = state->loaderMagnification;

    QObject::connect(this, &Loader::Worker::dc_reslice_notify, state->viewer, &Viewer::dc_reslice_notify);
    QObject::connect(this, &Loader::Worker::oc_reslice_notify, state->viewer, &Viewer::oc_reslice_notify);
}

Loader::Worker::~Worker() {
    abortDownloadsFinishDecompression(*this, [](const Coordinate &){return false;});

    state->protectCube2Pointer->lock();
    for (auto &elem : state->Dc2Pointer) { elem.clear(); }
    for (auto &elem : state->Oc2Pointer) { elem.clear(); }
    state->protectCube2Pointer->unlock();
    DcSetChunk.clear();
    OcSetChunk.clear();
    freeDcSlots.clear();
    freeOcSlots.clear();
}

template<typename CubeHash, typename Slots, typename Keep>
void unloadCubes(CubeHash & loadedCubes, Slots & freeSlots, Keep keep) {
    unloadCubes(loadedCubes, freeSlots, keep, [](const CoordOfCube &, char *){});
}

template<typename CubeHash, typename Slots, typename Keep, typename UnloadHook>
void unloadCubes(CubeHash & loadedCubes, Slots & freeSlots, Keep keep, UnloadHook todo) {
    int totalCount = 0, unloadCount = 0, keepCount = 0;

    auto copy = loadedCubes;
    for (auto && elem : copy) {
//        qDebug() << "cube" << elem.first.x << elem.first.y << elem.first.z;
        ++totalCount;
        if (!keep(elem.first.legacy2Global(state->cubeEdgeLength))) {
            todo(CoordOfCube(elem.first.x, elem.first.y, elem.first.z), elem.second);
//            qDebug() << "unload" << elem.first.x << elem.first.y << elem.first.z;
            ++unloadCount;
            auto * slot = elem.second;
            state->protectCube2Pointer->lock();
            loadedCubes.erase(elem.first);
            state->protectCube2Pointer->unlock();

            freeSlots.emplace_back(slot);
        } else {
//            qDebug() << "keep" << state->currentPositionX.x/128. << state->currentPositionX.y/128. << state->currentPositionX.z/128. << elem.first.x << elem.first.y << elem.first.z;
            ++keepCount;
        }
    }
    qDebug() << totalCount << "total:" << keepCount << "kept" << unloadCount << "unloaded" << freeSlots.size() << "free";
}

void Loader::Worker::snappyCacheAddSnappy(const CoordOfCube cubeCoord, const std::string cube) {
    snappyCache.emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple(cube));

    state->protectCube2Pointer->lock();
    const auto coord = cubeCoord.cube2Legacy();
    auto cubePtr = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[state->loaderMagnification], coord);
    if (cubePtr != nullptr) {
        freeOcSlots.emplace_back(cubePtr);
        state->Oc2Pointer[state->loaderMagnification].erase(coord);
    }
    state->protectCube2Pointer->unlock();
}

void Loader::Worker::snappyCacheAddRaw(const CoordOfCube & cubeCoord, const char * cube) {
    //insert empty string into snappy cache
    auto snappyIt = snappyCache.emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple()).first;
    //compress cube into the new string
    snappy::Compress(reinterpret_cast<const char *>(cube), OBJID_BYTES * state->cubeBytes, &snappyIt->second);
}

void Loader::Worker::snappyCacheClear() {
    //unload all modified cubes
    unloadCubes(state->Oc2Pointer[state->loaderMagnification], freeOcSlots, [this](const Coordinate & cubeCoord){
        const bool unflushed = OcModifiedCacheQueue.find(cubeCoord.cube(state->cubeEdgeLength)) != std::end(OcModifiedCacheQueue);
        const bool flushed = snappyCache.find(cubeCoord.cube(state->cubeEdgeLength)) != std::end(snappyCache);
        return !unflushed && !flushed;//only keep cubes which are neither in snappy cache nor in modified queue
    });
    OcModifiedCacheQueue.clear();
    snappyCache.clear();
    Loader::Controller::singleton().startLoading();//a bit of a detour…
}

void Loader::Worker::snappyCacheFlush() {
    snappyMutex.lock();

    for (const auto & cubeCoord : OcModifiedCacheQueue) {
        state->protectCube2Pointer->lock();
        auto cube = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[state->loaderMagnification], {cubeCoord.x, cubeCoord.y, cubeCoord.z});
        state->protectCube2Pointer->unlock();
        if (cube != nullptr) {
            snappyCacheAddRaw(cubeCoord, cube);
        }
    }
    //clear work queue
    OcModifiedCacheQueue.clear();

    snappyFlushCondition.wakeAll();
    snappyMutex.unlock();
}

void Loader::Worker::removeLoadedCubes(const coord2bytep_map_t &currentLoadedHash, uint prevLoaderMagnification) {
    for (auto kv : currentLoadedHash) {
        const auto coord = kv.first;
        if (NULL != kv.second) {
            continue;
        }

        state->protectCube2Pointer->lock();
        auto * delDcCubePtr = Coordinate2BytePtr_hash_get_or_fail(state->Dc2Pointer[prevLoaderMagnification], coord);
        auto * delOcCubePtr = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[prevLoaderMagnification], coord);
        state->protectCube2Pointer->unlock();

        //reclaim cube slots from
        //data
        if(delDcCubePtr != nullptr) {
            state->protectCube2Pointer->lock();
            state->Dc2Pointer[prevLoaderMagnification].erase(coord);
            state->protectCube2Pointer->unlock();
            freeDcSlots.emplace_back(delDcCubePtr);
        }
        //overlay
        if(delOcCubePtr != nullptr) {
            const auto cubeCoord = CoordOfCube(coord.x, coord.y, coord.z);
            if (OcModifiedCacheQueue.find(cubeCoord) != std::end(OcModifiedCacheQueue)) {
                snappyCacheAddRaw(cubeCoord, delOcCubePtr);
                //remove from work queue
                OcModifiedCacheQueue.erase(cubeCoord);
            }

            state->protectCube2Pointer->lock();
            state->Oc2Pointer[prevLoaderMagnification].erase(coord);
            state->protectCube2Pointer->unlock();
            freeOcSlots.emplace_back(delOcCubePtr);
        }
    }
}

void Loader::Worker::moveToThread(QThread *targetThread) {
    qnam.moveToThread(targetThread);
    QObject::moveToThread(targetThread);
}

template<typename Work, typename Func>//you really don’t wanna spell out the type of dcWork
void abortDownloads(Work & work, Func keep) {
    std::vector<Coordinate> abortQueue;
    for (auto && elem : work) {
        if (!keep(elem.first)) {
            abortQueue.emplace_back(elem.first);
        }
    }
    for (auto && elem : abortQueue) {
        qDebug() << "abort" << elem.x << elem.y << elem.z;
        work[elem]->abort();//abort running downloads
        work.erase(elem);
    }
}

template<typename Work, typename Func>//you really don’t wanna spell out the type of dcWork
void finishDecompression(Work & work, Func keep) {
    for (auto && elem : work) {
        if (!keep(elem.first) && elem.second != nullptr) {
            //elem.second->cancel();
            elem.second->waitForFinished();
        }
    }
}

template<typename Func>
void Loader::abortDownloadsFinishDecompression(Loader::Worker & worker, Func keep) {
    qDebug() << worker.dcDownload.size() << worker.dcDecompression.size() << worker.ocDownload.size() << worker.ocDecompression.size() << "before abortion";
    abortDownloads(worker.dcDownload, keep);
    abortDownloads(worker.ocDownload, keep);
    finishDecompression(worker.dcDecompression, keep);
    finishDecompression(worker.ocDecompression, keep);
    qDebug() << worker.dcDownload.size() << worker.dcDecompression.size() << worker.ocDownload.size() << worker.ocDecompression.size() << "after abortion";
}

std::pair<bool, char*> decompressCube(char * currentSlot, QByteArray & data, const Loader::CubeType type, coord2bytep_map_t & cubeHash, const Coordinate & globalCoord) {
    QThread::currentThread()->setPriority(QThread::IdlePriority);
    bool success = false;

    if (type == Loader::CubeType::RAW_UNCOMPRESSED) {
        const qint64 expectedSize = state->cubeBytes;
        std::copy(std::begin(data), std::end(data), currentSlot);
        success = data.size() == expectedSize;
        qDebug() << "raw cube" << success;
    } else if (type == Loader::CubeType::RAW_JPG) {
        int width, height, jpegSubsamp;

        auto jpegDecompressor = tjInitDecompress();
        const bool doFirst = jpegDecompressor != nullptr;
        const bool doSecond = doFirst && (tjDecompressHeader2(jpegDecompressor, reinterpret_cast<unsigned char *>(data.data()), data.size(), &width, &height, &jpegSubsamp) == 0);
        const bool doThird = doSecond && (tjDecompress2(jpegDecompressor, reinterpret_cast<unsigned char *>(data.data()), data.size(), reinterpret_cast<unsigned char *>(currentSlot), width, 0/*pitch*/, height, TJPF_GRAY, TJFLAG_ACCURATEDCT) == 0);

        if (doThird) {
            success = true;
            qDebug() << "decompressed cube";
        } else {
            qDebug() << tjGetErrorStr();
        }

        if (doFirst) {
            tjDestroy(jpegDecompressor);
        }
    } else if (type == Loader::CubeType::RAW_J2K || type == Loader::CubeType::RAW_JP2_6) {
        QTemporaryFile file(QDir::tempPath() + "/XXXXXX.jp2");
        success = file.open();
        success &= file.write(data) == data.size();
        file.close();
        success &= EXIT_SUCCESS == jp2_decompress_main(file.fileName().toUtf8().data(), reinterpret_cast<char*>(currentSlot), state->cubeBytes);
        qDebug() << "jpeg2000" << success;
    } else if (type == Loader::CubeType::SEGMENTATION_UNCOMPRESSED) {
        const qint64 expectedSize = state->cubeBytes * OBJID_BYTES;
        std::copy(std::begin(data), std::end(data), currentSlot);
        success = data.size() == expectedSize;
        qDebug() << "seg cube" << success;
    } else if (type == Loader::CubeType::SEGMENTATION_SZ_ZIP) {
        QBuffer buffer(&data);
        QuaZip archive(&buffer);//QuaZip needs a random access QIODevice
        if (archive.open(QuaZip::mdUnzip)) {
            archive.goToFirstFile();
            QuaZipFile file(&archive);
            if (file.open(QIODevice::ReadOnly)) {
                auto data = file.readAll();
                success = snappy::RawUncompress(data.data(), data.size(), reinterpret_cast<char*>(currentSlot));
                qDebug() << "seg.sz.zip cube" << success;
            }
            archive.close();
        }
    }  else {
        qDebug() << "unsupported format";
    }

    if (success) {
        state->protectCube2Pointer->lock();
        cubeHash[globalCoord.global2Legacy(state->cubeEdgeLength)] = currentSlot;
        state->protectCube2Pointer->unlock();
        if (currentlyVisibleWrap(globalCoord)) {
            if (isOverlay(type)) {
                QTimer::singleShot(0, state->viewer, &Viewer::oc_reslice_notify);
            } else {
                QTimer::singleShot(0, state->viewer, &Viewer::dc_reslice_notify);
            }
        }
    }

    return {success, currentSlot};
}

QUrl knossosCubeUrl(QUrl base, QString && experimentName, const CoordOfCube & coord, const int magnification, const Loader::CubeType type) {
    auto pos = QString("/mag%1/x%2/y%3/z%4/")
            .arg(magnification)
            .arg(coord.x, 4, 10, QChar('0'))
            .arg(coord.y, 4, 10, QChar('0'))
            .arg(coord.z, 4, 10, QChar('0'));
    auto filename = QString(("%1_mag%2_x%3_y%4_z%5%6"))//2012-03-07_AreaX14_mag1_x0000_y0000_z0000.j2k
            .arg(experimentName.section(QString("_mag"), 0, 0))
            .arg(magnification)
            .arg(coord.x, 4, 10, QChar('0'))
            .arg(coord.y, 4, 10, QChar('0'))
            .arg(coord.z, 4, 10, QChar('0'));

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
    auto query = QUrlQuery(base.query());

    if (type == Loader::CubeType::RAW_UNCOMPRESSED) {
        query.addQueryItem("format", "raw");
    } else if (type == Loader::CubeType::RAW_JPG) {
        query.addQueryItem("format", "singleimage");
    }

    query.addQueryItem("scale", QString::number(scale));// >= 0
    query.addQueryItem("size", QString("%1,%1,%1").arg(cubeEdgeLength));// <= 128³
    query.addQueryItem("corner", QString("%1,%2,%3").arg(coord.x).arg(coord.y).arg(coord.z));
    auto path = base.path() + ":subvolume";
    base.setPath(path);
    base.setQuery(query);
    //<volume_id>:subvolume?corner=10,10,10&size=50,50,50&scale=1&format=singleimage&key=<auth_key>
    return base;
}

QUrl webKnossosCubeUrl(QUrl base, Coordinate coord, const int unknownScale, const int cubeEdgeLength, const Loader::CubeType type) {
    auto query = QUrlQuery(base.query());
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

void Loader::Worker::cleanup() {
    qDebug() << "cleanup";

    prevLoaderMagnification = state->loaderMagnification;
    state->loaderMagnification = std::log(state->magnification)/std::log(2);

    // currentPositionX is updated only when the boundary is
    // crossed. Access to currentPositionX is synchronized through
    // the protectLoadSignal mutex.
    // DcoiFromPos fills the Dcoi list with all datacubes that
    // we want to be in memory, given our current position.
    abortDownloadsFinishDecompression(*this, currentlyVisibleWrap);
    //[](const CoordOfCube &){return false;}
    //&currentlyVisible
    unloadCubes(state->Dc2Pointer[prevLoaderMagnification], freeDcSlots, insideCurrentSupercubeWrap);
    unloadCubes(state->Oc2Pointer[prevLoaderMagnification], freeOcSlots, insideCurrentSupercubeWrap, [this](const CoordOfCube & cubeCoord, char * remSlotPtr){
        if (OcModifiedCacheQueue.find(cubeCoord) != std::end(OcModifiedCacheQueue)) {
            snappyCacheAddRaw(cubeCoord, remSlotPtr);
            //remove from work queue
            OcModifiedCacheQueue.erase(cubeCoord);
        }
    });
}

void Loader::Controller::startLoading() {
    if (worker != nullptr) {
        emit loadSignal(++loadingNr);
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

void Loader::Worker::downloadAndLoadCubes(const unsigned int loadingNr) {
    QTime time;
    time.start();

    cleanup();

    const auto Dcoi = DcoiFromPos();
    //split dcoi into slice planes and rest
    std::vector<Coordinate> visibleCubes;
    std::vector<Coordinate> cacheCubes;
    for (auto && todo : Dcoi) {//first elem of Dcoi is contentless
        Coordinate globalCoord(todo.x * state->cubeEdgeLength, todo.y * state->cubeEdgeLength, todo.z * state->cubeEdgeLength);
        state->protectCube2Pointer->lock();
        const bool dcNotAlreadyLoaded = Coordinate2BytePtr_hash_get_or_fail(state->Dc2Pointer[state->loaderMagnification], globalCoord.global2Legacy(state->cubeEdgeLength)) == nullptr;
        const bool ocNotAlreadyLoaded = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[state->loaderMagnification], globalCoord.global2Legacy(state->cubeEdgeLength)) == nullptr;
        state->protectCube2Pointer->unlock();
        if (dcNotAlreadyLoaded || ocNotAlreadyLoaded) {//only queue downloads which are necessary
            if (currentlyVisibleWrap(globalCoord)) {
                visibleCubes.emplace_back(globalCoord);
            } else {
                cacheCubes.emplace_back(globalCoord);
            }
        }
    }

    auto startDownload = [this](const Coordinate globalCoord, const CubeType type, decltype(dcDownload) & downloads, decltype(dcDecompression) & decompressions, decltype(freeDcSlots) & freeSlots, decltype(state->Dc2Pointer[0]) & cubeHash){
        if (isOverlay(type)) {
            auto snappyIt = snappyCache.find(globalCoord.cube(state->cubeEdgeLength));
            if (snappyIt != std::end(snappyCache)) {
                if (!freeSlots.empty()) {
                    auto * currentSlot = freeSlots.front();
                    freeSlots.pop_front();
                    //directly uncompress snappy cube into the OC slot
                    qDebug() << globalCoord.x << globalCoord.y << globalCoord.z << "snappy extract";
                    snappy::RawUncompress(snappyIt->second.c_str(), snappyIt->second.size(), reinterpret_cast<char*>(currentSlot));
                    state->protectCube2Pointer->lock();
                    cubeHash[globalCoord.global2Legacy(state->cubeEdgeLength)] = currentSlot;
                    state->protectCube2Pointer->unlock();
                    if (currentlyVisibleWrap(globalCoord)) {
                        emit oc_reslice_notify();
                    };
                } else {
                    qDebug() << globalCoord.x << globalCoord.y << globalCoord.z << "no slots";
                }
                return;
            }
        }

        auto apiSwitch = [this](const Coordinate globalCoord, const CubeType type){
            switch (api) {
            case Loader::API::GoogleBrainmaps:
                return googleCubeUrl(baseUrl, globalCoord, state->loaderMagnification, state->cubeEdgeLength, type);
            case Loader::API::Heidelbrain:
                return knossosCubeUrl(baseUrl, QString(state->name), globalCoord.cube(state->cubeEdgeLength), state->magnification, type);
            case Loader::API::WebKnossos:
                return webKnossosCubeUrl(baseUrl, globalCoord, state->loaderMagnification + 1, state->cubeEdgeLength, type);
            }
            throw std::runtime_error("unknown value for Loader::API");
        };
        QUrl dcUrl = apiSwitch(globalCoord, type);
        qDebug() << "url: " << dcUrl.toString();

        state->protectCube2Pointer->lock();
        const bool cubeNotAlreadyLoaded = Coordinate2BytePtr_hash_get_or_fail(cubeHash, globalCoord.global2Legacy(state->cubeEdgeLength)) == nullptr;
        state->protectCube2Pointer->unlock();

        if (downloads.find(globalCoord) == std::end(downloads) && cubeNotAlreadyLoaded) {
            QTime ping;
            ping.start();
            auto request = QNetworkRequest(dcUrl);
            //request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
            //request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
            auto * reply = qnam.get(request);
            reply->setParent(nullptr);//reparent, so it don’t gets destroyed with qnam
            downloads[globalCoord].reset(reply);
            QObject::connect(reply, &QNetworkReply::finished, [this, ping, reply, type, globalCoord, &downloads, &decompressions, &freeSlots, &cubeHash](){
                if (freeSlots.empty()) {
                    qDebug() << "no slots";
                    downloads.erase(globalCoord);
                    return;
                }
                auto * currentSlot = freeSlots.front();
                freeSlots.pop_front();
                if (reply->error() == QNetworkReply::NoError) {
                    const auto fini = ping.elapsed();
                    qDebug() << globalCoord.x << globalCoord.y << globalCoord.z << "ping" << fini;

                    auto future = QtConcurrent::run(&decompressionPool, std::bind(&decompressCube, currentSlot, reply->readAll(), type, std::ref(cubeHash), globalCoord));

                    auto * watcher = new QFutureWatcher<DecompressionResult>;
                    QObject::connect(watcher, &QFutureWatcher<DecompressionResult>::finished, [this, &cubeHash, &freeSlots, &decompressions, &downloads, globalCoord, watcher, type, currentSlot](){
                        downloads.erase(globalCoord);
                        if (!watcher->isCanceled()) {
                            auto result = watcher->result();

                            qDebug() << globalCoord.x << globalCoord.y << globalCoord.z << result.first << static_cast<void*>(result.second);
                            if (!result.first) {//decompression unsuccessful
                                qDebug() << globalCoord.x << globalCoord.y << globalCoord.z << "failed → no fill";
                                freeSlots.emplace_back(result.second);
                            }
                        } else {
                            qDebug() << globalCoord.x << globalCoord.y << globalCoord.z << "future canceled";
                            freeSlots.emplace_back(currentSlot);
                        }

                        qDebug() << static_cast<int>(type) << cubeHash.size() << "hash" << freeSlots.size() << "free";
                        decompressions.erase(globalCoord);
                    });
                    decompressions[globalCoord].reset(watcher);
                    watcher->setFuture(future);
                } else {//download error → fill
                    qDebug() << globalCoord.x << globalCoord.y << globalCoord.z << reply->errorString();
                    std::fill(currentSlot, currentSlot + state->cubeBytes * (isOverlay(type) ? OBJID_BYTES : 1), 0);
                    state->protectCube2Pointer->lock();
                    cubeHash[globalCoord.global2Legacy(state->cubeEdgeLength)] = currentSlot;
                    state->protectCube2Pointer->unlock();
                    //downloads.erase(globalCoord);//evil³
                }
            });
        } else {
            qDebug() << globalCoord.x << globalCoord.y << globalCoord.z << "already working";
        }
    };

    auto typeDcOverride = state->compressionRatio == 0 ? CubeType::RAW_UNCOMPRESSED : typeDc;
    for (auto globalCoord : visibleCubes) {
        if (loadingNr == Loader::Controller::singleton().loadingNr) {
            startDownload(globalCoord, typeDcOverride, dcDownload, dcDecompression, freeDcSlots, state->Dc2Pointer[state->loaderMagnification]);
            if (state->overlay) {
                startDownload(globalCoord, typeOc, ocDownload, ocDecompression, freeOcSlots, state->Oc2Pointer[state->loaderMagnification]);
            }
        }
    }

    //start the rest of the downloads after finishing the slice planes
    qDebug() << "adding additional cache cubes";
    for (auto globalCoord : cacheCubes) {
        //don’t continue downloads if they were interrupted
        //causes a stack underflow in LoaderWorker::abortDownloadsFinishDecompression
        if (loadingNr == Loader::Controller::singleton().loadingNr) {
            startDownload(globalCoord, typeDcOverride, dcDownload, dcDecompression, freeDcSlots, state->Dc2Pointer[state->loaderMagnification]);
            if (state->overlay) {
                startDownload(globalCoord, typeOc, ocDownload, ocDecompression, freeOcSlots, state->Oc2Pointer[state->loaderMagnification]);
            }
        }
    }

    qDebug() << "starting done" << visibleCubes.size() << "+" << cacheCubes.size() << time.elapsed() << "ms";
    qDebug() << "current hash" << state->Dc2Pointer[state->loaderMagnification].size() << state->Oc2Pointer[state->loaderMagnification].size();
}
