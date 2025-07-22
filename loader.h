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

#pragma once

#include "coordinate.h"
#include "dataset.h"
#include "segmentation/segmentation.h"
#include "usermove.h"

#include <QCoreApplication>
#include <QFutureSynchronizer>
#include <QFutureWatcher>
#include <QMutex>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSemaphore>
#include <QThread>
#include <QThreadPool>
#include <QtGlobal>
#include <QTimer>
#include <QWaitCondition>

#include <boost/multi_array.hpp>
#include <boost/optional/optional.hpp>

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* Calculate movement trajectory for loading based on how many last single movements */
#define LL_CURRENT_DIRECTIONS_SIZE (20)
/* Max number of metrics allowed for sorting loading order */
#define LL_METRIC_NUM (20)

#define LM_LOCAL    0
#define LM_FTP      1

namespace Loader{
class Controller;
class Worker;
}

namespace Loader {
using DecompressionResult = std::tuple<bool, void*, QIODevice*>;
class Worker : public QObject {
    Q_OBJECT
    friend class Loader::Controller;
    friend boost::multi_array_ref<uint64_t, 3> getCube(const Coordinate & pos);
    friend void Segmentation::clear();
public:
    QThreadPool decompressionPool;//let pool be alive just after ~Worker
private:
    QFutureSynchronizer<void> sync;
    QThreadPool localPool;
    QNetworkAccessManager qnam;

    template<typename T>
    using ptr = std::unique_ptr<T>;
    using DecompressionOperationPtr = ptr<QFutureWatcher<DecompressionResult>>;
    using OpenWatcher = QFutureWatcher<boost::optional<bool>>;
    std::vector<std::unordered_map<CoordOfCube, ptr<OpenWatcher>>> slotOpen;
    std::vector<ptr<OpenWatcher>> solitaryConfinement;// disconnected worker threads that wait for IO
    std::vector<std::unordered_map<CoordOfCube, QNetworkReply*>> slotDownload;
    std::vector<std::unordered_map<CoordOfCube, DecompressionOperationPtr>> slotDecompression;
    std::vector<std::list<std::vector<std::uint8_t>>> slotChunk;// slot ownership
    std::vector<std::list<void *>> freeSlots;
    int currentMaxMetric;

    std::atomic_bool isFinished{false};
    std::size_t loaderMagnification{0};
    std::size_t loaderCacheSize{0};
    void CalcLoadOrderMetric(float halfSc, floatCoordinate currentMetricPos, const UserMoveType userMoveType, const floatCoordinate & direction, float *metrics);
    floatCoordinate find_close_xyz(floatCoordinate direction);
    std::vector<CoordOfCube> DcoiFromPos(const Coordinate &currentOrigin, const UserMoveType userMoveType, const floatCoordinate & direction);
    uint loadCubes();
    void snappyCacheBackupRaw(const std::size_t layerId, const CoordOfCube &, const void * cube);
    void snappyCacheClear();

    decltype(slotDecompression)::value_type::iterator finalizeDecompression(QFutureWatcher<DecompressionResult> & watcher, decltype(freeSlots)::value_type & freeSlots, decltype(slotDecompression)::value_type & decompressions, const CoordOfCube & cubeCoord);
    void abortDownloadsFinishDecompression();
    template<typename Func>
    void abortDownloadsFinishDecompression(std::size_t, Func);

    decltype(Dataset::datasets) datasets;
public://matsch
    using CacheQueue = std::unordered_set<CoordOfCube>;
    std::vector<std::vector<CacheQueue>> modifiedCacheQueue;
    using SnappySet = std::unordered_map<CoordOfCube, std::string>;
    std::vector<std::vector<SnappySet>> snappyCache;
    QMutex snappyCacheMutex;
    QMutex snappyFlushConditionMutex;
    QWaitCondition snappyFlushCondition;

    void moveToThread(QThread * targetThread);//reimplement to move qnam

    void unloadCurrentMagnification(const std::size_t);
    void unloadCurrentMagnification();
    void markCubeAsModified(const std::size_t layerId, const CoordOfCube &cubeCoord, const std::size_t magIndex);
    void snappyCacheSupplySnappy(const std::size_t layerId, const CoordOfCube, const quint64 cubeMagnification, const std::string cube);
    void flushIntoSnappyCache();
    void broadcastProgress(bool startup = false);
    Worker();
    virtual ~Worker() override;
signals:
    void progress(bool incremented, int count);
public slots:
    void cleanup(const Coordinate center);
    void downloadAndLoadCubes(const unsigned int loadingNr, const Coordinate center, const UserMoveType userMoveType, const floatCoordinate & direction, Dataset::list_t changedDatasets, const size_t cacheSize);
};

class Controller : public QObject {
    Q_OBJECT
    friend class Loader::Worker;
    QThread workerThread;
public:
    std::unique_ptr<Loader::Worker> worker;
    std::atomic_uint loadingNr{0};
    static Controller & singleton(){
        static Loader::Controller & loader = *new Loader::Controller;
        return loader;
    }
    void suspendLoader();
    Controller();
    virtual ~Controller() override;

    void startLoading(const Coordinate & center, const UserMoveType userMoveType, const floatCoordinate &direction);
    template<typename... Args>
    void snappyCacheSupplySnappy(Args&&... args) {
        emit snappyCacheSupplySnappySignal(std::forward<Args>(args)...);
    }
    void markCubeAsModified(const std::size_t layerId, const CoordOfCube &cubeCoord, const std::size_t magIndex);

    struct LockedSnappy {
        std::unique_lock<QMutex> locker;
        decltype(Loader::Worker::snappyCache[0]) & cubes;
        explicit LockedSnappy(QMutex & mutex, decltype(cubes) & cache) : locker(mutex), cubes(cache) {}
    };
    auto getAllModifiedCubes(const std::size_t layerId) {
        QMutexLocker lock(&worker->snappyFlushConditionMutex);
        //signal to run in loader thread
        QTimer::singleShot(0, worker.get(), &Worker::flushIntoSnappyCache);
        worker->snappyFlushCondition.wait(&worker->snappyFlushConditionMutex);
        return LockedSnappy{worker->snappyCacheMutex, worker->snappyCache[layerId]};
    }
public slots:
    bool isFinished();
    bool hasSnappyCache();
signals:
    void progress(int count);
    void refCountChange(bool isIncrement, int refCount);
    void loadSignal(const unsigned int loadingNr, const Coordinate center, const UserMoveType userMoveType, const floatCoordinate & direction, const Dataset::list_t & changedDatasets, const quint64 cacheSize);
    void markCubeAsModifiedSignal(const std::size_t layerId, const CoordOfCube &cubeCoord, const std::size_t magIndex);
    void snappyCacheSupplySnappySignal(const std::size_t layerId, const CoordOfCube, const quint64 cubeMagnification, const std::string cube);
};
}//namespace Loader
