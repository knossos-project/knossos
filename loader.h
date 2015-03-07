#ifndef LOADER_H
#define LOADER_H

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
#include "hashtable.h"
#include "segmentation/segmentation.h"

#include <QCoreApplication>
#include <QFutureWatcher>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSemaphore>
#include <QThread>
#include <QThreadPool>
#include <QTimer>

#include <boost/multi_array.hpp>

#include <atomic>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* Calculate movement trajectory for loading based on how many last single movements */
#define LL_CURRENT_DIRECTIONS_SIZE (20)
/* Max number of metrics allowed for sorting loading order */
#define LL_METRIC_NUM (20)
#define MAX(a,b) ((a) > (b) ? (a) : (b))
/*
* For the loader ad-hoc linked list
*/
#define LLL_SUCCESS  1
#define LLL_FAILURE  0

#define LM_LOCAL    0
#define LM_FTP      1

#define FTP_RETRY_NUM 3

using CURL = void;

struct C_Element {
    Coordinate coordinate; // coordinate * cubeEdgeLength = minimal coordinate in the cube; NOT center coordinate

    char *data_filename;
    char *overlay_filename;
    char *path;
    char *fullpath_data_filename;
    char *fullpath_overlay_filename;
    char *local_data_filename;
    char *local_overlay_filename;
    CURL *curlDataHandle;
    CURL *curlOverlayHandle;
    FILE *ftp_data_fh;
    FILE *ftp_overlay_fh;
    bool hasError;
    bool hasDataError;
    bool hasOverlayError;
    int retries;
    bool isFinished;
    bool isDataFinished;
    bool isOverlayFinished;
    bool isAborted;
    int isLoaded;

    //uint debugVal;
    //DWORD tickDownloaded;
    //DWORD tickDecompressed;

    C_Element *previous;
    C_Element *next;
};

namespace Loader{
class Controller;
class Worker;
}

int calc_nonzero_sign(float x);

namespace Loader {
class Worker;
enum class API {
    Heidelbrain, WebKnossos, GoogleBrainmaps
};
enum class CubeType {
    RAW_UNCOMPRESSED, RAW_JPG, RAW_J2K, RAW_JP2_6, SEGMENTATION_UNCOMPRESSED, SEGMENTATION_SZ_ZIP
};

class Worker : public QObject {
    Q_OBJECT
    friend boost::multi_array_ref<uint64_t, 3> getCube(const Coordinate & pos);
    friend void Segmentation::clear();
private:
    QThreadPool decompressionPool;//let pool be alive just after ~Worker
    QNetworkAccessManager qnam;

    template <typename T>
    using ptr = std::unique_ptr<T>;
    using ReplyPtr = ptr<QNetworkReply>;
    using DecompressionResult = std::pair<bool, char*>;
    using DecompressionOperationPtr = ptr<QFutureWatcher<DecompressionResult>>;
    std::unordered_map<Coordinate, ReplyPtr> dcDownload;
    std::unordered_map<Coordinate, ReplyPtr> ocDownload;
    std::unordered_map<Coordinate, DecompressionOperationPtr> dcDecompression;
    std::unordered_map<Coordinate, DecompressionOperationPtr> ocDecompression;
    std::list<std::vector<char>> DcSetChunk;
    std::list<std::vector<char>> OcSetChunk;
    std::list<char*> freeDcSlots;
    std::list<char*> freeOcSlots;
    std::vector<char> bogusDc;
    int currentMaxMetric;

    uint prevLoaderMagnification;
    void CalcLoadOrderMetric(float halfSc, floatCoordinate currentMetricPos, floatCoordinate direction, float *metrics);
    floatCoordinate find_close_xyz(floatCoordinate direction);
    int addCubicDcSet(int xBase, int yBase, int zBase, int edgeLen, C_Element *target, coord2bytep_map_t *currentLoadedHash);
    std::vector<Coordinate> DcoiFromPos();
    void removeLoadedCubes(const coord2bytep_map_t &currentLoadedHash, uint prevLoaderMagnification);
    uint loadCubes();
    void snappyCacheAdd(const CoordOfCube &, const char *cube);
    void snappyCacheClear();

    template<typename Func>
    friend void abortDownloadsFinishDecompression(Loader::Worker&, Func);

    QUrl baseUrl;
    API api;
    CubeType typeDc;
    CubeType typeOc;
    QString experimentName;
public://matsch
    std::atomic_bool skipDownloads{false};
    std::unordered_set<CoordOfCube> OcModifiedCacheQueue;
    std::unordered_map<CoordOfCube, std::string> snappyCache;
    std::vector<char> bogusOc;

    void moveToThread(QThread * targetThread);//reimplement to move qnam

    void snappyCacheFlush();
    Worker(const QUrl & baseUrl, const API api, const CubeType typeDc, const CubeType typeOc, const QString & experimentName);
    ~Worker();
    int CompareLoadOrderMetric(const void * a, const void * b);

signals:
    void dc_reslice_notify();
    void oc_reslice_notify();

public slots:
    void cleanup();
    void downloadAndLoadCubes(const unsigned int loadingNr);
};

class Controller : public QObject {
    Q_OBJECT
    QThread workerThread;
    friend class Loader::Worker;
public:
    std::unique_ptr<Loader::Worker> worker;
    std::atomic_uint loadingNr{0};
    static Controller & singleton(){
        static Loader::Controller loader;
        return loader;
    }
    void waitForWorkerThread() {
        workerThread.quit();
        workerThread.wait();
    }
    ~Controller() {
        waitForWorkerThread();
    }
    template<typename... Args>
    void restart(Args&&... args) {
        waitForWorkerThread();
        worker.reset(new Loader::Worker(std::forward<Args>(args)...));
        worker->moveToThread(&workerThread);
        QObject::connect(this, &Loader::Controller::load, worker.get(), &Loader::Worker::downloadAndLoadCubes);
        workerThread.start();
    }
    void startLoading();
signals:
    void load(const unsigned int loadingNr);
};

}

#endif // LOADER_H
