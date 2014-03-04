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
#include "knossos-global.h"

#include <list>
#include <vector>

#include <QObject>
#include <QSemaphore>
#include <QThread>

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

/*
For Loader file loader
*/
#define LS_UNIX     0
#define LS_WINDOWS  1

#define LM_LOCAL    0
#define LM_FTP      1

struct C_Element {
    Coordinate coordinate;

    char *filename;
    char *path;
    char *fullpath_filename;
    char *local_filename;
    CURL *curlHandle;
    FILE *ftp_fh;
    int hasError;
    int isFinished;
    int isAborted;
    int isLoaded;

    //uint debugVal;
    //DWORD tickDownloaded;
    //DWORD tickDecompressed;

    C_Element *previous;
    C_Element *next;
};

class Loader;

struct ftp_thread_struct {
/*
    DWORD debugVal;
    DWORD beginTickCount;
*/
    QSemaphore *ftpThreadSem;
    QSemaphore *loaderThreadSem;
    int cubeCount;
    Loader* thisPtr;
};

struct loadcube_thread_struct {
    //DWORD beginTickCount;
    //DWORD decompTime;
    int threadIndex;
    QSemaphore *loadCubeThreadSem;
    int isBusy;
    C_Element *currentCube;
    Loader* thisPtr;
    bool retVal;
    int threadCount;
};

class LoadCubeThread : public QThread
{
public:
    LoadCubeThread(void *ctx);
    void run();
protected:
    void *ctx;
};

struct LO_Element {
        Coordinate coordinate;
        Coordinate offset;
        float loadOrderMetrics[LL_METRIC_NUM];
};

int calc_nonzero_sign(float x);

class Loader : public QThread {
    Q_OBJECT
    friend class LoadCubeThread;
private:
    std::list<std::vector<Byte>> DcSetChunk;
    std::list<std::vector<Byte>> OcSetChunk;
    std::list<Byte*> freeDcSlots;
    std::list<Byte*> freeOcSlots;
    Byte *bogusDc;
    Byte *bogusOc;
    bool magChange;
    int currentMaxMetric;
    Hashtable *mergeCube2Pointer;
    void run();
    bool initLoader();
    bool initialized;
    uint prevLoaderMagnification;
    void CalcLoadOrderMetric(float halfSc, floatCoordinate currentMetricPos, floatCoordinate direction, float *metrics);
    floatCoordinate find_close_xyz(floatCoordinate direction);
    int addCubicDcSet(int xBase, int yBase, int zBase, int edgeLen, C_Element *target, Hashtable *currentLoadedHash);
    uint DcoiFromPos(C_Element *Dcoi, Hashtable *currentLoadedHash);
    void loadCube(loadcube_thread_struct *lts);
    uint removeLoadedCubes(Hashtable *currentLoadedHash, uint prevLoaderMagnification);
    uint loadCubes();
public:
    explicit Loader(QObject *parent = 0);
    int CompareLoadOrderMetric(const void * a, const void * b);

    C_Element *Dcoi;
signals:
    void finished();
public slots:
    bool load();
};

#endif // LOADER_H
