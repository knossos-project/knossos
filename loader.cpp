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

#include <cmath>

#include <dirent.h>
#include "loader.h"
#include "knossos.h"
#include <sys/stat.h>
#include "ftp.h"

#ifdef KNOSSOS_USE_TURBOJPEG
#include <turbojpeg.h>
#endif

extern stateInfo *state;

C_Element *lll_new()
{
    C_Element *lll = NULL;

    lll = (C_Element *)malloc(sizeof(C_Element));
    if (NULL == lll)
    {
        printf("Out of memory\n");
        return HT_FAILURE;
    }

    lll->previous = lll;
    lll->next = lll;

    /* This is a dummy element used only for entry into the linked list. */

    SET_COORDINATE(lll->coordinate, 0, 0, 0);

    return lll;
}

void lll_del(C_Element *delElement) {
    delElement->previous->next = delElement->next;
    delElement->next->previous = delElement->previous;

    free(delElement->filename);
    free(delElement->local_filename);
    free(delElement->path);
    free(delElement->fullpath_filename);

    free(delElement);
}

void lll_rmlist(C_Element *Dcoi) {
    C_Element *currentCube = NULL, *prevCube = NULL;

    for (currentCube = Dcoi->previous; currentCube != Dcoi; currentCube = prevCube) {
        prevCube = currentCube->previous;
        lll_del(currentCube);
    }
}

uint lll_calculate_filename(C_Element *elem) {
    char *local_dir_delim = NULL;
    char *file_dir_delim = NULL;
    char *magnificationStr = NULL;
    char typeExtension[8] = "";
    char compressionExtension[8] = "";
    int boergens_param_1 = 0, boergens_param_2 = 0, boergens_param_3 = 0;
    char *boergens_param_1_name = "", *boergens_param_2_name = "", *boergens_param_3_name = "";
    char *local_cache_path_builder = NULL, *local_cache_path_total = NULL;
    int filenameSize = 1000;
    Coordinate coordinate;

    coordinate = elem->coordinate;

    switch (state->loadLocalSystem) {
    case LS_UNIX:
        local_dir_delim = "/";
        break;
    case LS_WINDOWS:
        local_dir_delim = "\\";
        break;
    default:
        return LLL_FAILURE;
    }

    switch (state->loadMode) {
    case LM_LOCAL:
        file_dir_delim = local_dir_delim;
        break;
    case LM_FTP:
        file_dir_delim = "/";
        break;
    default:
        return LLL_FAILURE;
    }

    /*
    To reverse into non-compression, normal raw file read, replace
    typeExtension with "raw"
    compressionExtension with ""
    then everthing should be fine as before.
    */
    switch (state->compressionRatio) {
    case 0:
        strncpy(typeExtension, "raw", 4);
        snprintf(compressionExtension, 1, "");
        break;
    case 1000:
        strncpy(typeExtension, "jpg", 4);
        snprintf(compressionExtension, 1, "");
        break;
    case 1001:
        strncpy(typeExtension, "j2k", 4);
        snprintf(compressionExtension, 1, "");
        break;
    default:
        strncpy(typeExtension, "jp2", 4);
        snprintf(compressionExtension, sizeof(compressionExtension), "%d.", state->compressionRatio);
        break;
    }
    /*
    strncpy(typeExtension, "raw", 4);
    compressionExtension[0] =  NULL;
    */

    elem->filename = (char*)malloc(filenameSize);
    if(elem->filename == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(elem->filename, '\0', filenameSize);
    elem->path = (char*)malloc(filenameSize);
    if(elem->path == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(elem->path, '\0', filenameSize);
    elem->fullpath_filename = (char*)malloc(filenameSize);
    if(elem->fullpath_filename == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(elem->fullpath_filename, '\0', filenameSize);

    boergens_param_2 = coordinate.y;
    boergens_param_2_name = "y";
    if(state->boergens) {
        boergens_param_1 = coordinate.z;
        boergens_param_1_name = "z";
        boergens_param_3 = coordinate.x;
        boergens_param_3_name = "x";
    }
    else {
        boergens_param_1 = coordinate.x;
        boergens_param_1_name = "x";
        boergens_param_3 = coordinate.z;
        boergens_param_3_name = "z";
    }
    magnificationStr = (char*)malloc(filenameSize);
    magnificationStr[0] = '\0';
    if (LM_FTP == state->loadMode) {
        snprintf(magnificationStr, filenameSize, "mag%d%s", state->magnification, file_dir_delim);
    }

    snprintf(elem->path, filenameSize,
             "%s%s%s%.4d%s%s%.4d%s%s%.4d",
             (LM_FTP == state->loadMode) ? state->ftpBasePath : state->loaderPath,
             magnificationStr,
             boergens_param_1_name,
             boergens_param_1,
             file_dir_delim,
             boergens_param_2_name,
             boergens_param_2,
             file_dir_delim,
             boergens_param_3_name,
             boergens_param_3
             );
    free(magnificationStr);
    /* LOG("Path: %s", elem->path); */
    snprintf(elem->filename, filenameSize,
             "%s_x%.4d_y%.4d_z%.4d.%s%s",
             state->loaderName,
             coordinate.x,
             coordinate.y,
             coordinate.z,
             compressionExtension,
             typeExtension);
    snprintf(elem->fullpath_filename, filenameSize,
             "%s%s%s",
             elem->path,
             file_dir_delim,
             elem->filename);
    /* LOG("FullPath: %s", elem->fullpath_filename); */

    if (LM_FTP != state->loadMode) {
        return LLL_SUCCESS;
    }

    local_cache_path_builder = (char*)malloc(filenameSize);
    if(local_cache_path_builder == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(local_cache_path_builder, '\0', filenameSize);
    local_cache_path_total = (char*)malloc(filenameSize);
    if(local_cache_path_total == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(local_cache_path_total, '\0', filenameSize);
    snprintf(local_cache_path_builder, filenameSize, "%smag%d%s", state->loadFtpCachePath, state->magnification, local_dir_delim);
    strcat(local_cache_path_total, local_cache_path_builder);

#ifdef Q_OS_UNIX
    mkdir(local_cache_path_total, 0777);
#endif
#ifdef Q_OS_WIN
    mkdir(local_cache_path_total);
#endif
    /* LOG("%s", local_cache_path_total); */
    snprintf(local_cache_path_builder, filenameSize, "%s%s%.4d", local_dir_delim, boergens_param_1_name, boergens_param_1);
    strcat(local_cache_path_total, local_cache_path_builder);
#ifdef Q_OS_UNIX
    mkdir(local_cache_path_total, 0777);
#endif
#ifdef Q_OS_WIN
    mkdir(local_cache_path_total);
#endif
    /* LOG("%s", local_cache_path_total); */
    snprintf(local_cache_path_builder, filenameSize, "%s%s%.4d", local_dir_delim, boergens_param_2_name, boergens_param_2);
    strcat(local_cache_path_total, local_cache_path_builder);
#ifdef Q_OS_UNIX
    mkdir(local_cache_path_total, 0777);
#endif
#ifdef Q_OS_WIN
    mkdir(local_cache_path_total);
#endif
    /* LOG("%s", local_cache_path_total); */
    snprintf(local_cache_path_builder, filenameSize, "%s%s%.4d", local_dir_delim, boergens_param_3_name, boergens_param_3);
    strcat(local_cache_path_total, local_cache_path_builder);

#ifdef Q_OS_UNIX
    mkdir(local_cache_path_total, 0777);
#endif
#ifdef Q_OS_WIN
    mkdir(local_cache_path_total);
#endif
    /* LOG("%s", local_cache_path_total); */
    free(local_cache_path_builder);

    elem->local_filename = (char*)malloc(filenameSize);
    if(elem->local_filename == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(elem->local_filename, '\0', filenameSize);
    snprintf(elem->local_filename, filenameSize,
             "%s%s%s",
             local_cache_path_total,
             local_dir_delim,
             elem->filename);
    free(local_cache_path_total);

    return LLL_SUCCESS;
}

uint lll_put(C_Element *destElement, Hashtable *currentLoadedHash, Coordinate key) {
    if((key.x > 9999) ||
            (key.y > 9999) ||
            (key.z > 9999) ||
            (key.x < 0) ||
            (key.y < 0) ||
            (key.z < 0)) {
        /* LOG("Requested cube coordinate (%d, %d, %d) out of bounds.",
            key.x,
            key.y,
            key.z); */

        return LLL_SUCCESS;
    }

    /*
    If cube to be loaded is already loaded, no need to load it now,
    just mark this element as required, so we don't delete it later.
    */
    if (NULL != currentLoadedHash) {
        C2D_Element * const loadedCubePtr = Hashtable::ht_get_element(currentLoadedHash, key);
        if (HT_FAILURE != loadedCubePtr) {
            loadedCubePtr->datacube = (Byte*)(!NULL);
            return LLL_SUCCESS;
        }
    }

    C_Element * const putElement = (C_Element*)malloc(sizeof(C_Element));
    if(putElement == NULL) {
        printf("Out of memory\n");
        return LLL_FAILURE;
    }

    putElement->coordinate = key;

    putElement->filename = NULL;
    putElement->path = NULL;
    putElement->fullpath_filename = NULL;
    putElement->local_filename = NULL;
    putElement->ftp_fh = NULL;
    putElement->hasError = false;
    putElement->curlHandle = NULL;
    putElement->isFinished = false;
    putElement->isAborted = false;
    putElement->isLoaded = false;
    //putElement->debugVal = 0;
    //putElement->tickDownloaded = 0;
    //putElement->tickDecompressed = 0;

    if (LLL_SUCCESS != lll_calculate_filename(putElement)) {
        return LLL_FAILURE;
    }

    putElement->next = destElement->next;
    putElement->previous = destElement;
    destElement->next->previous = putElement;
    destElement->next = putElement;

    destElement->coordinate.x++;

    /* LOG("(%d, %d, %d)", key.x, key.y, key.z); */

    return LLL_SUCCESS;
}

Loader::Loader(QObject *parent) :
    QThread(parent)
{

}

int calc_nonzero_sign(float x) {
    if (x > 0) {
        return 1;
    }
    return -1;
}

void Loader::CalcLoadOrderMetric(float halfSc, floatCoordinate currentMetricPos, floatCoordinate direction, float *metrics) {
    float distance_from_plane, distance_from_origin, dot_product;
    int i = 0;

    distance_from_plane = CALC_POINT_DISTANCE_FROM_PLANE(currentMetricPos, direction);
    distance_from_origin = CALC_VECTOR_NORM(currentMetricPos);
    dot_product = CALC_DOT_PRODUCT(currentMetricPos, direction);

    metrics[i++] = ( (distance_from_plane <= 1) || (distance_from_origin <= halfSc) ) ? -1.0 : 1.0;
    metrics[i++] = distance_from_plane > 1 ? 1.0 : -1.0;
    metrics[i++] = dot_product < 0 ?  1.0 : -1.0;
    metrics[i++] = distance_from_plane;
    metrics[i++] = distance_from_origin;

    this->currentMaxMetric = MAX(this->currentMaxMetric, i);
    /* LOG("%f\t%f\t%f\t%f\t%f\t%f",
        currentMetricPos.x, currentMetricPos.y, currentMetricPos.z,
        metrics[0], metrics[1], metrics[2]); */
}

int Loader::CompareLoadOrderMetric(const void * a, const void * b)
{
    LO_Element *elem_a, *elem_b;
    float m_a, m_b;
    int metric_index;

    elem_a = (LO_Element *)a;
    elem_b = (LO_Element *)b;

    for (metric_index = 0; metric_index < this->currentMaxMetric; metric_index++) {
        m_a = elem_a->loadOrderMetrics[metric_index];
        m_b = elem_b->loadOrderMetrics[metric_index];
        /* LOG("i %d\ta\t%d,%d,%d\t=\t%f\tb\t%d,%d,%d\t=\t%f", metric_index,
            elem_a->offset.x, elem_a->offset.y, elem_a->offset.z, m_a,
            elem_b->offset.x, elem_b->offset.y, elem_b->offset.z, m_b); */
        if (m_a != m_b) {
            return calc_nonzero_sign(m_a - m_b);
        }
        /* If equal just continue to next comparison level */
    }

    return 0;
}

int CompareLoadOrderMetric_LoaderWrapper(const void *a, const void *b, const void *ctx) {
    Loader *thisPtr = (Loader*)ctx;
    return thisPtr->CompareLoadOrderMetric(a, b);
}

floatCoordinate Loader::find_close_xyz(floatCoordinate direction) {
    floatCoordinate xyz[3];
    float dot_products[3];
    int i;
    float max_dot_product;
    int max_dot_product_i;

    SET_COORDINATE(xyz[0], 1.0, 0.0, 0.0);
    SET_COORDINATE(xyz[1], 0.0, 1.0, 0.0);
    SET_COORDINATE(xyz[2], 0.0, 0.0, 1.0);

    for (i = 0; i < 3; i++) {
        dot_products[i] = CALC_DOT_PRODUCT(direction, xyz[i]);
    }
    max_dot_product = dot_products[0];
    max_dot_product_i = 0;
    for (i = 1; i < 3; i++) {
        if (max_dot_product < dot_products[i]) {
            max_dot_product_i = i;
            max_dot_product = dot_products[i];
        }
    }

    return xyz[max_dot_product_i];

}

uint Loader::DcoiFromPos(C_Element *Dcoi, Hashtable *currentLoadedHash) {
    Coordinate currentOrigin;
    floatCoordinate currentMetricPos, direction;
    LO_Element *DcArray;
    int cubeElemCount;
    int i;
    int edgeLen, halfSc;
    int x, y, z;
    float dx = 0, dy = 0, dz = 0;
    float direction_norm;
    float floatHalfSc;

    edgeLen = state->M;
    floatHalfSc = (float)edgeLen / 2.;
    halfSc = (int)floorf(floatHalfSc);

    for (i = 0; i < LL_CURRENT_DIRECTIONS_SIZE; i++) {
        dx += (float)state->currentDirections[i].x;
        dy += (float)state->currentDirections[i].y;
        dz += (float)state->currentDirections[i].z;
    }
    SET_COORDINATE(direction, dx, dy, dz);
    direction = find_close_xyz(direction);
    direction_norm = CALC_VECTOR_NORM(direction);
    if (0 == direction_norm) {
        LOG("No average movement, fabricating x direction!");
        dx = 1;
        SET_COORDINATE(direction, dx, dy, dz);
        direction_norm = CALC_VECTOR_NORM(direction);
    }
    NORM_VECTOR(direction, direction_norm);

    cubeElemCount = state->cubeSetElements;
    DcArray = (LO_Element*)malloc(sizeof(DcArray[0]) * cubeElemCount);
    if (NULL == DcArray) {
        return false;
    }
    memset(DcArray, 0, sizeof(DcArray[0]) * cubeElemCount);
    currentOrigin = Coordinate::Px2DcCoord(state->currentPositionX);
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
    extern void _quicksort(void *pbase, size_t total_elems, size_t size, int (*cmp)(const void*, const void*, const void *), const void *ctx);
    _quicksort(DcArray, cubeElemCount, sizeof(DcArray[0]), CompareLoadOrderMetric_LoaderWrapper, (const void*)this);
    for (i = 0; i < cubeElemCount; i++) {
        if (LLL_SUCCESS != lll_put(Dcoi, currentLoadedHash, DcArray[i].coordinate)) {
            return false;
        }
    }
    free(DcArray);

    return true;
}

extern "C" {
    int jp2_decompress_main(char *infile, char *buf, int bufsize);
}

#include <fstream>

void Loader::loadCube(loadcube_thread_struct *lts) {
    if (state->overlay) {
        state->protectLoaderSlots->lock();
        if (!lts->thisPtr->freeOcSlots.empty()) {
            auto currentOcSlot = lts->thisPtr->freeOcSlots.front();

            const auto inFilePath = std::string(lts->currentCube->fullpath_filename) + ".segmentation.raw";
            std::ifstream inFile(inFilePath, std::ios_base::binary);
            if (inFile) {
            //if (inFile.read(reinterpret_cast<char * const>(currentOcSlot), state->cubeBytes * sizeof(uint64_t)) {
                std::copy(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), currentOcSlot);

                state->protectCube2Pointer->lock();
                if (Hashtable::ht_put(state->Oc2Pointer[state->loaderMagnification], lts->currentCube->coordinate, currentOcSlot) == HT_SUCCESS) {
                    lts->thisPtr->freeOcSlots.remove(currentOcSlot);
                } else {
                    LOG("Error inserting new Oc (%d, %d, %d) with slot %p into Oc2Pointer[%d].", lts->currentCube->coordinate.x, lts->currentCube->coordinate.y, lts->currentCube->coordinate.z, currentOcSlot, state->loaderMagnification);
                }
                state->protectCube2Pointer->unlock();
            }
        }
        state->protectLoaderSlots->unlock();
    }

    bool retVal = true;
    bool isPut = false;
    char *filename;
    FILE *cubeFile = NULL;
    size_t readBytes = 0;
    Byte *currentDcSlot = NULL;
#ifdef KNOSSOS_USE_TURBOJPEG
    tjhandle _jpegDecompressor = NULL;
    Byte *localCompressedBuf = NULL;
    size_t localCompressedBufSize = 0;
    int jpegSubsamp, width, height;
#endif
    //DWORD tickCount = GetTickCount();

    /*
     * Specify either freeDcSlot or freeOcSlot.
     *
     * Caution: Not writing data into a cube slot and returning
     * false from this function results in a busy wait in the
     * viewer that kills performance, so that situation should
     * never occur.
     *
     */

    state->protectLoaderSlots->lock();
    if (!lts->thisPtr->freeDcSlots.empty()) {
        currentDcSlot = lts->thisPtr->freeDcSlots.front();
        lts->thisPtr->freeDcSlots.remove(currentDcSlot);
    }
    state->protectLoaderSlots->unlock();
    if (NULL == currentDcSlot) {
        LOG("Error getting a slot for the next Dc, wanted to load (%d, %d, %d), mag %d dataset.",
            lts->currentCube->coordinate.x,
            lts->currentCube->coordinate.y,
            lts->currentCube->coordinate.z,
            state->magnification);
        retVal = false;
        goto loadcube_ret;
    }

    if (LM_FTP == state->loadMode) {
        if (lts->currentCube->isAborted) {
            retVal = false;
            goto loadcube_ret;
        }
        if (lts->currentCube->hasError) {
            goto loadcube_fail;
        }
    }

    filename = (LM_FTP == state->loadMode) ? lts->currentCube->local_filename : lts->currentCube->fullpath_filename;

    switch(state->compressionRatio) {
    case 0:
        cubeFile = fopen(filename, "rb");

        if(cubeFile == NULL) {
            //LOG("fopen failed for %s!", filename);
            goto loadcube_fail;
        }

        readBytes = fread(currentDcSlot, 1, state->cubeBytes, cubeFile);
        if(readBytes != state->cubeBytes) {
            LOG("fread error!");
            if(fclose(cubeFile) != 0) {
                LOG("Additionally, an error occured closing the file");
            }
            goto loadcube_fail;
        } else {
            fclose(cubeFile);
        }
        break;
    case 1000:
#ifdef KNOSSOS_USE_TURBOJPEG
        cubeFile = fopen(filename, "rb");
        if(cubeFile == NULL) {
            LOG("fopen failed for %s!\n", filename);
            goto loadcube_fail;
        }
        if (0 != fseek(cubeFile, 0, SEEK_END)) {
            LOG("fseek END failed for %s!\n", filename);
            goto loadcube_fail;
        }
        localCompressedBufSize = ftell(cubeFile);
        if (0 != fseek(cubeFile, 0, SEEK_SET)) {
            LOG("fseek SET failed for %s!\n", filename);
            goto loadcube_fail;
        }
        localCompressedBuf = (Byte*)malloc(localCompressedBufSize);
        if (NULL == localCompressedBuf) {
            LOG("malloc failed!\n");
            goto loadcube_fail;
        }
        readBytes = fread(localCompressedBuf, 1, localCompressedBufSize, cubeFile);
        fclose(cubeFile);
        if (localCompressedBufSize != readBytes) {
            LOG("fread failed for %s! (%d instead of %d)\n", filename, readBytes, localCompressedBufSize);
            goto loadcube_fail;
        }

        _jpegDecompressor = tjInitDecompress();
        if (NULL == _jpegDecompressor) {
            LOG("tjInitDecompress() failed!");
            goto loadcube_fail;
        }
        if (0 != tjDecompressHeader2(_jpegDecompressor, localCompressedBuf, localCompressedBufSize, &width, &height, &jpegSubsamp)) {
            LOG("tjDecompressHeader2() failed!");
            goto loadcube_fail;
        }
        if (0 != tjDecompress2(_jpegDecompressor, localCompressedBuf, localCompressedBufSize, currentDcSlot, width, 0/*pitch*/, height, TJPF_GRAY, TJFLAG_ACCURATEDCT)) {
            LOG("tjDecompress2() failed!");
            goto loadcube_fail;
        }
#else
        LOG("JPG disabled, Knossos wasn’t compiled with config »turbojpeg«.");
#endif
        break;
    case 1001:
    default:
        if (EXIT_SUCCESS != jp2_decompress_main(filename, reinterpret_cast<char*>(currentDcSlot), state->cubeBytes)) {
            LOG("Decompression function failed!");
            goto loadcube_fail;
        }
        break;
    }
    goto loadcube_manage;

loadcube_fail:
    memcpy(currentDcSlot, lts->thisPtr->bogusDc, state->cubeBytes);

loadcube_manage:
    /* Add pointers for the dc and oc (if at least one of them could be loaded)
     * to the Cube2Pointer table.
     *
     */
    if (!retVal) {
        goto loadcube_ret;
    }
    state->protectCube2Pointer->lock();
    if(Hashtable::ht_put(state->Dc2Pointer[state->loaderMagnification], lts->currentCube->coordinate, currentDcSlot) != HT_SUCCESS) {
        LOG("Error inserting new Dc (%d, %d, %d) with slot %p into Dc2Pointer[%d].",
            lts->currentCube->coordinate.x,
            lts->currentCube->coordinate.y,
            lts->currentCube->coordinate.z,
            currentDcSlot,
            state->loaderMagnification);
        retVal = false;
    } else {
        isPut = true;
    }
    state->protectCube2Pointer->unlock();
    if (!retVal) {
        goto loadcube_ret;
    }

loadcube_ret:
    if ((NULL != currentDcSlot) && (false == isPut)) {
        state->protectLoaderSlots->lock();
        lts->thisPtr->freeDcSlots.push_front(currentDcSlot);
        state->protectLoaderSlots->unlock();
    }

#ifdef KNOSSOS_USE_TURBOJPEG
    if (NULL != _jpegDecompressor) {
        tjDestroy(_jpegDecompressor);
    }
    if (NULL != localCompressedBuf) {
        free(localCompressedBuf);
    }
#endif

    if (retVal) {
        //lts->decompTime = GetTickCount() - tickCount;
    }
    lts->isBusy = false;
    lts->loadCubeThreadSem->release();
    lts->retVal = retVal;
    return;
}

void LoadCubeThread::run() {
    loadcube_thread_struct *lts = (loadcube_thread_struct*)this->ctx;
    lts->thisPtr->loadCube(lts);
}

LoadCubeThread::LoadCubeThread(void *ctx) {
    this->ctx = ctx;
}

bool Loader::initLoader() {
    // DCOI, ie. Datacubes of interest, is a hashtable that will contain
    // the coordinates of the datacubes and overlay cubes we need to
    // load given our current position.
    // See the comment about the ht_new() call in knossos.c
    this->Dcoi = lll_new();
    if(this->Dcoi == HT_FAILURE) {
        LOG("Unable to create Dcoi.");
        return false;
    }

    // freeDcSlots / freeOcSlots are lists of pointers to locations that
    // can hold data or overlay cubes. Whenever we want to load a new
    // datacube, we load it into a location from this list. Whenever a
    // datacube in memory becomes invalid, we add the pointer to its
    // memory location back into this list.

    LOG("Allocating %d bytes for the datacubes.", state->cubeSetBytes);
    for(size_t i = 0; i < state->cubeSetBytes; i += state->cubeBytes) {
        DcSetChunk.emplace_back(state->cubeBytes, 0);//zero init chunk of chars
        freeDcSlots.emplace_back(DcSetChunk.back().data());//append newest element
    }

    if(state->overlay) {
        LOG("Allocating %u bytes for the overlay cubes.", state->cubeSetBytes * OBJID_BYTES);
        for(size_t i = 0; i < state->cubeSetBytes * OBJID_BYTES; i += state->cubeBytes * OBJID_BYTES) {
            OcSetChunk.emplace_back(state->cubeBytes * OBJID_BYTES, 0);//zero init chunk of chars
            freeOcSlots.emplace_back(OcSetChunk.back().data());//append newest element
        }
    }

    // Load the bogus dc (a placeholder when data is unavailable).
    this->bogusDc = (Byte*)malloc(state->cubeBytes);
    if(this->bogusDc == NULL) {
        LOG("Out of memory.")
        return false;
    }
    FILE * bogusDc = fopen("bogus.raw", "r");
    if(bogusDc != NULL) {
        if(fread(this->bogusDc, 1, state->cubeBytes, bogusDc) < state->cubeBytes) {
            LOG("Unable to read the correct amount of bytes from the bogus dc file.")
            memset(this->bogusDc, '\0', state->cubeBytes);
        }
        fclose(bogusDc);
    }
    else {
        memset(this->bogusDc, '\0', state->cubeBytes);
    }
    if(state->overlay) {
        // bogus oc is white
        this->bogusOc = (Byte*)malloc(state->cubeBytes * OBJID_BYTES);
        if(this->bogusOc == NULL) {
            LOG("Out of memory.")
                    return false;
        }
        memset(this->bogusOc, '\0', state->cubeBytes * OBJID_BYTES);
    }

    state->loaderMagnification = std::log2(state->magnification);
    strncpy(state->loaderName, state->magNames[state->loaderMagnification], 1024);
    strncpy(state->loaderPath, state->magPaths[state->loaderMagnification], 1024);

    prevLoaderMagnification = state->loaderMagnification;

    magChange = false;
    mergeCube2Pointer = NULL;

    return true;
}

uint Loader::removeLoadedCubes(Hashtable *currentLoadedHash, uint prevLoaderMagnification) {
    for (C2D_Element *currentCube = currentLoadedHash->listEntry->next;
         currentCube != currentLoadedHash->listEntry;
         currentCube = currentCube->next) {
        if (NULL != currentCube->datacube) {
            continue;
        }
        /*
         * This element is not required, which means we can reuse its
         * slots for new DCs / OCs.
         * As we are using the merged list as a proxy for the actual lists,
         * we need to get the pointers out of the actual lists before we can
         * add them back to the free slots lists.
         *
         */

        state->protectCube2Pointer->lock();
        Byte *delCubePtr = NULL;

        /*
         * Process Dc2Pointer if the current cube is in Dc2Pointer.
         *
         */
        if((delCubePtr = Hashtable::ht_get(state->Dc2Pointer[prevLoaderMagnification], currentCube->coordinate)) != HT_FAILURE) {
            if(Hashtable::ht_del(state->Dc2Pointer[prevLoaderMagnification], currentCube->coordinate) != HT_SUCCESS) {
                LOG("Error deleting cube (%d, %d, %d) from Dc2Pointer[%d].",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    prevLoaderMagnification);
                return false;
            }

            freeDcSlots.emplace_back(delCubePtr);
            /*
            LOG("Added %d, %d, %d => %d available",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    state->loaderState->freeDcSlots->elements);
            */
        }
        /*
         * Process Oc2Pointer if the current cube is in Oc2Pointer.
         *
         */

        if((delCubePtr = Hashtable::ht_get(state->Oc2Pointer[prevLoaderMagnification], currentCube->coordinate)) != HT_FAILURE) {
            if(Hashtable::ht_del(state->Oc2Pointer[prevLoaderMagnification], currentCube->coordinate) != HT_SUCCESS) {
                LOG("Error deleting cube (%d, %d, %d) from Oc2Pointer.",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z);
                return false;
            }

            memset(delCubePtr, 0, state->cubeSetBytes * OBJID_BYTES);
            freeOcSlots.emplace_back(delCubePtr);
        }

        state->protectCube2Pointer->unlock();
    }

    return true;
}

#define MAX_DECOMP_THREAD_NUM (64)
uint Loader::loadCubes() {
    int decompThreads = state->loaderDecompThreadsNumber;

    C_Element *currentCube = NULL;//, *prevCube = NULL, *decompedCube = NULL;
    uint loadedDc;
    FtpThread *ftpThread;
    ftp_thread_struct fts = {0};
    loadcube_thread_struct lts_array[MAX_DECOMP_THREAD_NUM] = {0};
    loadcube_thread_struct *lts_current;
    loadcube_thread_struct lts_empty = {0};
    LoadCubeThread *threadHandle_array[MAX_DECOMP_THREAD_NUM] = {NULL};
    QSemaphore *loadCubeThreadSem = new QSemaphore(decompThreads);
    int hadError = false;
    int retVal = true;
    int cubeCount = 0, cubeCountFinished = 0, cubeCountDispatched = 0;
    int thread_index;
    int isBreak;

    for (currentCube = this->Dcoi->previous; currentCube != this->Dcoi; currentCube = currentCube->previous) {
        cubeCount++;
    }

    if (LM_FTP == state->loadMode) {
        fts.cubeCount = cubeCount;
        fts.ftpThreadSem = new QSemaphore(0);
        fts.loaderThreadSem = new QSemaphore(0);
        fts.thisPtr = this;
        ftpThread = new FtpThread((void*)&fts);
        ftpThread->start();
    }

    while (true) {
        // Wait on an available decompression thread
        loadCubeThreadSem->acquire();
        for (thread_index = 0; (thread_index < decompThreads) && lts_array[thread_index].isBusy; thread_index++);
        if (decompThreads == thread_index) {
            LOG("All threads occupied, c'est impossible! Au revoir loader!");
            retVal = false;
            break;
        }
        // The following would only be called if all threads, except ONE that has just finished,
        // were occupied by now. Otherwise, it could be that other non-busy threads were also
        // available one of which would had been found by the "for" above
        if (NULL != threadHandle_array[thread_index]) {
            threadHandle_array[thread_index]->wait();
            //decompTime += lts_array[thread_index].decompTime;
            loadedDc = lts_array[thread_index].retVal;
            delete threadHandle_array[thread_index];
            threadHandle_array[thread_index] = NULL;
            lts_array[thread_index] = lts_empty;
            cubeCountFinished++;
            if (!loadedDc) {
                retVal = false;
                break;
            }
        }
        if (cubeCount == cubeCountFinished) {
            break;
        }

        // We need to be able to cancel the loading when the
        // user crosses the reload-boundary
        // or when the dataset changed as a consequence of a mag switch

        state->protectLoadSignal->lock();
        isBreak = (state->datasetChangeSignal != NO_MAG_CHANGE) || (state->loadSignal == true) || (true == hadError);
        state->protectLoadSignal->unlock();
        if (isBreak) {
            //LOG("loadCubes Interrupted!");
            retVal = false;
            break;
        }

        if (LM_FTP == state->loadMode) {
            fts.loaderThreadSem->acquire();
        }
        for (currentCube = this->Dcoi->previous; currentCube != this->Dcoi; currentCube = currentCube->previous) {
            if (currentCube->isLoaded) {
                continue;
            }
            if ((currentCube->isFinished) || (LM_FTP != state->loadMode)) {
                currentCube->isLoaded = true;
                break;
            }
        }
        if (this->Dcoi == currentCube) {
            LOG("No more cubes to load, c'est impossible! Au revoird loader!");
            break;
        }
        /*
         * Load the datacube for the current coordinate.
         *
         */
        lts_current = &lts_array[thread_index];
        *lts_current = lts_empty;
        lts_current->currentCube = currentCube;
        lts_current->isBusy = true;
        lts_current->loadCubeThreadSem = loadCubeThreadSem;
        //lts_current->beginTickCount = beginTickCount;
        lts_current->threadIndex = thread_index;
        //lts_current->decompTime = 0;
        lts_current->retVal = false;
        lts_current->thisPtr = this;
        threadHandle_array[thread_index] = new LoadCubeThread(lts_current);
        threadHandle_array[thread_index]->start(QThread::LowestPriority);

        cubeCountDispatched++;
        if (cubeCount == cubeCountDispatched) {
            break;
        }
    }

    if (LM_FTP == state->loadMode) {
        fts.ftpThreadSem->release();
        ftpThread->wait();
        delete ftpThread;
        ftpThread = NULL;
        delete fts.ftpThreadSem; fts.ftpThreadSem = NULL;
        delete fts.loaderThreadSem; fts.loaderThreadSem = NULL;
    }

    for (thread_index = 0; thread_index <  decompThreads; thread_index++) {
        if (NULL != threadHandle_array[thread_index]) {
            //LOG("LOADER Decompression thread %d was open! Waiting...", thread_index);
            threadHandle_array[thread_index]->wait();
            delete threadHandle_array[thread_index];
            threadHandle_array[thread_index] = NULL;
        }
    }
    delete loadCubeThreadSem; loadCubeThreadSem = NULL;

    return retVal;
}


/**
  * @brief This method is used for the QThread loader declared in main knossos.cpp
  * This is the old loader function from KNOSSOS 3.2
  * It works as follows:
  * - before entering the while(true) loop the loader gets initialized(initLoader)
  * - the inner loop waits for the loadSignal attribute to become true
  *   This happens when a loadSignal is sent via Viewer::sendLoadSignal(), which also emits a loadSignal
  * - The loadSignal is connected to slot Loader:load(), which handles datacube loading
  * - in short: this thread does nothing but waiting for the loadSignal and processing it
  */

void Loader::run() {
    state->protectLoadSignal->lock();

    // Set up DCOI and freeDcSlots / freeOcSlots.
    if(initLoader() == false) {
        throw std::runtime_error("initLoader failed");
    }
    initialized = true;

    // Start "signal wait" loop.
    while(true)  {
        // as long the loadSignal is false, the loops waits
        while(state->loadSignal == false) {
            state->loaderBusy = false;
            state->conditionLoadFinished->wakeOne();
            state->conditionLoadSignal->wait(state->protectLoadSignal);
            state->loaderBusy = true;
        }

        if (true == load()) {
            break;
        }
    }
}

/**
  * @brief: slot connected to loadSignal(), calculates dcoi, then loads dcs
  * This slot handles the loading of datacubes.
  * It calculates dcois, then discards those already loaded.
  * Finally, remaining dcois are loaded via loadCubes().
  * Will be interrupted when a quit signal is sent.
  *
  */
bool Loader::load() {
    uint funcRetVal;

    if(!initialized) {
        qDebug() << "Warning, Loader was not initialized";
        qDebug() << "Load: begin ended";
        return false;
    }

    state->loadSignal = false;
    magChange = false;
    if(state->datasetChangeSignal) {
        state->datasetChangeSignal = NO_MAG_CHANGE;
        magChange = true;
    }

    if(state->quitSignal == true) {
        LOG("Loader quitting.");
        state->protectLoadSignal->unlock();
        return true;
    }

    /*
    We create a hash that would later be assigned the union of Dc and Oc cubes already loaded by now.
    For each cube element in the hash, the datacube field is automatically initialized to NULL
    upon creation by ht_union.
    When we finished calculating the cubes that need to be loaded now, their datacube field would
    be different than NULL.
    */

    mergeCube2Pointer = Hashtable::ht_new(state->cubeSetElements * 20);
    if(mergeCube2Pointer == HT_FAILURE) {
        LOG("Unable to create the temporary cube2pointer table.");
        state->protectLoadSignal->unlock();
        return true;
    }
    state->protectCube2Pointer->lock();
    funcRetVal = Hashtable::ht_union(mergeCube2Pointer,
                                     state->Dc2Pointer[state->loaderMagnification],
            state->Oc2Pointer[state->loaderMagnification]);
    state->protectCube2Pointer->unlock();
    if (HT_SUCCESS != funcRetVal) {
        LOG("Error merging Dc2Pointer and Oc2Pointer for mag %d.", state->loaderMagnification);
        state->protectLoadSignal->unlock();
        return true;
    }

    prevLoaderMagnification = state->loaderMagnification;
    state->loaderMagnification = std::log(state->magnification)/std::log(2);
    strncpy(state->loaderName, state->magNames[state->loaderMagnification], 1024);
    strncpy(state->loaderPath, state->magPaths[state->loaderMagnification], 1024);

    // currentPositionX is updated only when the boundary is
    // crossed. Access to currentPositionX is synchronized through
    // the protectLoadSignal mutex.
    // DcoiFromPos fills the Dcoi list with all datacubes that
    // we want to be in memory, given our current position.


    if(DcoiFromPos(this->Dcoi, magChange ? NULL : mergeCube2Pointer) != true) {
        LOG("Error computing DCOI from position.");
        state->protectLoadSignal->unlock();
        return true;
    }



    /*
    We shall now remove from current loaded cubes those cubes that are not required by new coordinate,
    or simply all cubes in case of magnification change
    */
    if(removeLoadedCubes(mergeCube2Pointer, prevLoaderMagnification) != true) {
        LOG("Error removing already loaded cubes from DCOI.");
        state->protectLoadSignal->unlock();
        return true;
    }
    if (Hashtable::ht_rmtable(mergeCube2Pointer) != LL_SUCCESS) {
        LOG("Error removing temporary cube to pointer table. This is a memory leak.");
    }

    state->protectLoadSignal->unlock();

    if (!state->loaderDummy) {
        if(loadCubes() == false) {
            //qDebug("Loading of all DCOI did not complete.");
        }
    }
    lll_rmlist(this->Dcoi);

    state->protectLoadSignal->lock();

    return false;
}
