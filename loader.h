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

#include <QObject>
#include <QSemaphore>
#include <QThread>
#include "knossos-global.h"

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

struct _C_Element {
    Coordinate coordinate;

    char *filename;
    char *path;
    char *fullpath_filename;
    char *local_filename;
    CURL *curlHandle;
    FILE *ftp_fh;
    int    hasError;
    int isFinished;
    int isAborted;
    int isLoaded;

    //uint debugVal;
    //DWORD tickDownloaded;
    //DWORD tickDecompressed;

    struct _C_Element *previous;
    struct _C_Element *next;
};

typedef struct _C_Element C_Element;

class Loader;

struct _ftp_thread_struct {
/*
    DWORD debugVal;
    DWORD beginTickCount;
*/
    QSemaphore *ftpThreadSem;
    QSemaphore *loaderThreadSem;
    int cubeCount;
    Loader* thisPtr;
};

typedef struct _ftp_thread_struct ftp_thread_struct;

struct _loadcube_thread_struct {
    //DWORD beginTickCount;
    //DWORD decompTime;
    int threadIndex;
    QSemaphore *loadCubeThreadSem;
    int isBusy;
    C_Element *currentCube;
    Loader* thisPtr;
    uint retVal;
};

typedef struct _loadcube_thread_struct loadcube_thread_struct;

class LoadCubeThread : public QThread
{
public:
    LoadCubeThread(void *ctx);
    void run();
protected:
    void *ctx;
};

struct _LO_Element {
        Coordinate coordinate;
        Coordinate offset;
        float loadOrderMetrics[LL_METRIC_NUM];
};

typedef struct _LO_Element LO_Element;

int calc_nonzero_sign(float x);

class Loader : public QThread
{
    Q_OBJECT
    friend class LoadCubeThread;
public:
    explicit Loader(QObject *parent = 0);
    void run();
    int CompareLoadOrderMetric(const void * a, const void * b);

    C_Element *Dcoi;
    int currentMaxMetric;
    CubeSlotList *freeDcSlots;
    CubeSlotList *freeOcSlots;
    Byte *DcSetChunk;
    Byte *OcSetChunk;
    Byte *bogusDc;
    Byte *bogusOc;
    bool magChange;
    Hashtable *mergeCube2Pointer;
signals:
    void finished();
public slots:
    bool load();
protected:    
    bool initialized;
    uint prevLoaderMagnification;
    void CalcLoadOrderMetric(float halfSc, floatCoordinate currentMetricPos, floatCoordinate direction, float *metrics);
    floatCoordinate find_close_xyz(floatCoordinate direction);
    int addCubicDcSet(int xBase, int yBase, int zBase, int edgeLen, C_Element *target, Hashtable *currentLoadedHash);
    uint DcoiFromPos(C_Element *Dcoi, Hashtable *currentLoadedHash);
    CubeSlot *slotListGetElement(CubeSlotList *slotList);
    void loadCube(loadcube_thread_struct *lts);
    int slotListDelElement(CubeSlotList *slotList, CubeSlot *element);
    bool slotListDel(CubeSlotList *delList);
    int slotListAddElement(CubeSlotList *slotList, Byte *datacube);
    CubeSlotList *slotListNew();
    bool initLoader();
    uint removeLoadedCubes(Hashtable *currentLoadedHash, uint prevLoaderMagnification);
    uint loadCubes();
};

#endif // LOADER_H
