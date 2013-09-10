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

#include <math.h>
#include <dirent.h>
#include "loader.h"
#include "knossos.h"
#include "sleeper.h"
#include <sys/stat.h>
#include "ftp.h"

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
    char *filename = NULL;
    char *local_dir_delim = NULL;
    char *file_dir_delim = NULL;
    char *loader_path = NULL;
    char *magnificationStr = NULL;
    char typeExtension[8] = "";
    char compressionExtension[8] = "";
    FILE *cubeFile = NULL;
    int readBytes = 0;
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
    if (state->compressionRatio > 0) {
        strncpy(typeExtension, "jp2", 4);
        snprintf(compressionExtension, sizeof(compressionExtension), "%d.", state->compressionRatio);
    }
    else {
        strncpy(typeExtension, "raw", 4);
        snprintf(compressionExtension, 1, "");
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
    strcat(local_cache_path_total, local_cache_path_builder); mkdir(local_cache_path_total, 0777);
    /* LOG("%s", local_cache_path_total); */
    snprintf(local_cache_path_builder, filenameSize, "%s%s%.4d", local_dir_delim, boergens_param_1_name, boergens_param_1);
    strcat(local_cache_path_total, local_cache_path_builder); mkdir(local_cache_path_total, 0777);
    /* LOG("%s", local_cache_path_total); */
    snprintf(local_cache_path_builder, filenameSize, "%s%s%.4d", local_dir_delim, boergens_param_2_name, boergens_param_2);
    strcat(local_cache_path_total, local_cache_path_builder); mkdir(local_cache_path_total, 0777);
    /* LOG("%s", local_cache_path_total); */
    snprintf(local_cache_path_builder, filenameSize, "%s%s%.4d", local_dir_delim, boergens_param_3_name, boergens_param_3);
    strcat(local_cache_path_total, local_cache_path_builder); mkdir(local_cache_path_total, 0777);
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
    C_Element *putElement = NULL;
    C2D_Element *loadedCubePtr = NULL;

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
        loadedCubePtr = Hashtable::ht_get_element(currentLoadedHash, key);
        if (HT_FAILURE != loadedCubePtr) {
            loadedCubePtr->datacube = (Byte*)(!NULL);
            return LLL_SUCCESS;
        }
    }

    putElement = (C_Element*)malloc(sizeof(C_Element));
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
    float distance_from_plane, distance_from_direction, distance_from_origin, dot_product;
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
    Coordinate currentDc, currentOrigin;
    floatCoordinate currentMetricPos, direction;
    Coordinate debugCoor;
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

    /*
    switch (state->viewerState->activeVP) {
        case VIEWPORT_XY:
            dz = state->directionSign;
            break;
        case VIEWPORT_XZ:
            dy = state->directionSign;
            break;
        case VIEWPORT_YZ:
            dx = state->directionSign;
            break;
    }
    */
    for (i = 0; i < LL_CURRENT_DIRECTIONS_SIZE; i++) {
        // LOG("%d\t%d\t%d", state->currentDirections[i].x, state->currentDirections[i].y, state->currentDirections[i].z);
        dx += (float)state->currentDirections[i].x;
        dy += (float)state->currentDirections[i].y;
        dz += (float)state->currentDirections[i].z;
        // LOG("Progress %f,%f,%f", dx, dy, dz);
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
    /* LOG("Direction %f,%f,%f", direction.x, direction.y, direction.z); */

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
extern void
        _quicksort (
             void *pbase,
             size_t total_elems,
             size_t size,
             int (*cmp)(const void*, const void*, const void *),
             const void *ctx);
     _quicksort(DcArray, cubeElemCount, sizeof(DcArray[0]), CompareLoadOrderMetric_LoaderWrapper, (const void*)this);
    //LOG("New Order (%f, %f, %f):", direction.x, direction.y, direction.z);
    for (i = 0; i < cubeElemCount; i++) {
        /*
        LOG("%d\t%d\t%d",
            DcArray[i].coordinate.x - currentOrigin.x,
            DcArray[i].coordinate.y - currentOrigin.y,
            DcArray[i].coordinate.z - currentOrigin.z);
        */
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
 void Loader::loadCube(loadcube_thread_struct *lts) {
    int retVal = true;
    CubeSlot *currentDcSlot;
    char *filename;
    FILE *cubeFile = NULL;
    size_t readBytes = 0;
    char *read_target = NULL;
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

    //LOG("LOADER [%08X] %d started decompressing %d", GetTickCount() - lts->beginTickCount, lts->threadIndex, lts->currentCube->debugVal);

    state->protectLoaderSlots->lock();
    currentDcSlot = slotListGetElement(lts->thisPtr->freeDcSlots);
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

    if (!currentDcSlot->cube) {
        goto loadcube_ret;
    }
    if (LM_FTP == state->loadMode) {
        if (lts->currentCube->isAborted) {
            //LOG("Aborted while retrieving %d,%d,%d (DEBUG %d)", lts->currentCube->coordinate.x, lts->currentCube->coordinate.y, lts->currentCube->coordinate.z, lts->currentCube->debugVal);
            retVal = false;
            goto loadcube_ret;
        }
        if (lts->currentCube->hasError) {
            //LOG("Error retrieving %d,%d,%d (DEBUG %d)", lts->currentCube->coordinate.x, lts->currentCube->coordinate.y, lts->currentCube->coordinate.z, lts->currentCube->debugVal);
            goto loadcube_fail;
        }
    }

    filename = (LM_FTP == state->loadMode) ? lts->currentCube->local_filename : lts->currentCube->fullpath_filename;
    read_target = (char*)currentDcSlot->cube;

    if (state->compressionRatio > 0) {
        if (EXIT_SUCCESS != jp2_decompress_main(filename, read_target, state->cubeBytes)) {
            LOG("Decompression function failed!");
            goto loadcube_fail;
        }
    }
    else {
        cubeFile = fopen(filename, "rb");

        if(cubeFile == NULL) {
            LOG("fopen failed for %s!", filename);
            goto loadcube_fail;
        }

        readBytes = fread(read_target, 1, state->cubeBytes, cubeFile);
        if(readBytes != state->cubeBytes) {
            LOG("fread error!");
            if(fclose(cubeFile) != 0) {
                LOG("Additionally, an error occured closing the file");
            }
            goto loadcube_fail;
        }
    }
    goto loadcube_manage;

loadcube_fail:
    memcpy(currentDcSlot->cube, lts->thisPtr->bogusDc, state->cubeBytes);

loadcube_manage:
     /* Add pointers for the dc and oc (if at least one of them could be loaded)
     * to the Cube2Pointer table.
     *
     */
    if (!retVal) {
        goto loadcube_ret;
    }
    state->protectCube2Pointer->lock();
    if(Hashtable::ht_put(state->Dc2Pointer[state->loaderMagnification], lts->currentCube->coordinate, currentDcSlot->cube) != HT_SUCCESS) {
        LOG("Error inserting new Dc (%d, %d, %d) with slot %p into Dc2Pointer[%d].",
            lts->currentCube->coordinate.x,
            lts->currentCube->coordinate.y,
            lts->currentCube->coordinate.z,
            currentDcSlot->cube,
            state->loaderMagnification);
        retVal = false;
    }
    state->protectCube2Pointer->unlock();
    if (!retVal) {
        goto loadcube_ret;
    }

    /*
     * Remove the slots
     *
     */
    state->protectLoaderSlots->lock();
    if(slotListDelElement(lts->thisPtr->freeDcSlots, currentDcSlot) < 0) {
        LOG("Error deleting the current Dc slot %p from the list.",
            currentDcSlot->cube);
        retVal = false;
    }
    state->protectLoaderSlots->unlock();
    if (!retVal) {
        goto loadcube_ret;
    }

loadcube_ret:
    if (retVal) {
        //lts->decompTime = GetTickCount() - tickCount;
    }
    lts->isBusy = false;
    //LOG("LOADER [%08X] %d finished decompressing %d", GetTickCount() - lts->beginTickCount, lts->threadIndex, lts->currentCube->debugVal);
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

 /**
 * @brief slotListGetElement
 * @param slotList
 * @return false changed to NULL
 */
CubeSlot *Loader::slotListGetElement(CubeSlotList *slotList) {
    if(slotList->elements > 0) {
        return slotList->firstSlot;
    }
    else {
        return NULL;
    }
}

int Loader::slotListDelElement(CubeSlotList *slotList, CubeSlot *element) {
    if(element->next != NULL) {
        element->next->previous = element->previous;
    }
    if(element->previous != NULL) {
        element->previous->next = element->next;
    }
    else {
        // element is the first elements in the list.
        slotList->firstSlot = element->next;
    }

    free(element);

    slotList->elements = slotList->elements - 1;

    return slotList->elements;
}

bool Loader::slotListDel(CubeSlotList *delList){
    while(delList->elements > 0) {
        if(slotListDelElement(delList, delList->firstSlot) < 0) {
            printf("Error deleting element at %p from the slot list. %d elements remain in the list.\n",
                   delList->firstSlot->cube, delList->elements);
        }
    }

    free(delList);

    return true;
}

int Loader::slotListAddElement(CubeSlotList *slotList, Byte *datacube) {
    CubeSlot *newElement;

    newElement = (CubeSlot*)malloc(sizeof(CubeSlot));
    if(newElement == NULL) {
        printf("Out of memory\n");
        return FAIL;
    }

    newElement->cube = datacube;
    newElement->next = slotList->firstSlot;
    newElement->previous = NULL;

    slotList->firstSlot = newElement;

    slotList->elements = slotList->elements + 1;

    return slotList->elements;
}

CubeSlotList *Loader::slotListNew() {
    CubeSlotList *newDcSlotList;

    newDcSlotList = (CubeSlotList*)malloc(sizeof(CubeSlotList));
    if(newDcSlotList == NULL) {
        printf("Out of memory.\n");
        return NULL;
    }
    newDcSlotList->firstSlot = NULL;
    newDcSlotList->elements = 0;

    return newDcSlotList;
}

bool Loader::initLoader() {
    FILE *bogusDc;
    uint i = 0;

    // DCOI, ie. Datacubes of interest, is a hashtable that will contain
    // the coordinates of the datacubes and overlay cubes we need to
    // load given our current position.
    // See the comment about the ht_new() call in knossos.c
    this->Dcoi = lll_new();
    if(this->Dcoi == HT_FAILURE) {
        LOG("Unable to create Dcoi.")
        return false;
    }

    // freeDcSlots / freeOcSlots are lists of pointers to locations that
    // can hold data or overlay cubes. Whenever we want to load a new
    // datacube, we load it into a location from this list. Whenever a
    // datacube in memory becomes invalid, we add the pointer to its
    // memory location back into this list.
    this->freeDcSlots = slotListNew();
    if(this->freeDcSlots == NULL) {
        LOG("Unable to create freeDcSlots.")
        return false;
    }

    this->freeOcSlots = slotListNew();
    if(this->freeOcSlots == NULL) {
        LOG("Unable to create freeOcSlots.")
        return false;
    }

    // These are potentially huge allocations.
    // To lock this memory block to RAM and prevent swapping, mlock() could
    // be used on UNIX and VirtualLock() on Windows. This does not appear to be
    // necessary in the real world.

    LOG("Allocating %d bytes for the datacubes.", state->cubeSetBytes)
    this->DcSetChunk = (Byte*)malloc(state->cubeSetBytes);
    if(this->DcSetChunk == NULL) {
        LOG("Unable to allocate memory for the DC memory slots.")
        return false;
    }
    memset(this->DcSetChunk, 0, state->cubeSetBytes);
    for(i = 0; i < state->cubeSetBytes; i += state->cubeBytes) {
        slotListAddElement(this->freeDcSlots, this->DcSetChunk + i);
    }

    if(state->overlay) {
        LOG("Allocating %d bytes for the overlay cubes.", state->cubeSetBytes * OBJID_BYTES)
        this->OcSetChunk = (Byte*)malloc(state->cubeSetBytes * OBJID_BYTES);
        if(this->OcSetChunk == NULL) {
            LOG("Unable to allocate memory for the OC memory slots.")
            return false;
        }
        memset(this->OcSetChunk, 0, state->cubeSetBytes * OBJID_BYTES);
        for(i = 0; i < state->cubeSetBytes * OBJID_BYTES; i += state->cubeBytes * OBJID_BYTES)
            slotListAddElement(this->freeOcSlots, this->OcSetChunk + i);

    }

    // Load the bogus dc (a placeholder when data is unavailable).
    this->bogusDc = (Byte*)malloc(state->cubeBytes);
    if(this->bogusDc == NULL) {
        LOG("Out of memory.")
        return false;
    }
    bogusDc = fopen("bogus.raw", "r");
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

    state->loaderMagnification = Knossos::log2uint32(state->magnification);
    //LOG("loaderMagnification = %d", state->loaderMagnification);
    strncpy(state->loaderName, state->magNames[state->loaderMagnification], 1024);
    strncpy(state->loaderPath, state->magPaths[state->loaderMagnification], 1024);

    prevLoaderMagnification = state->loaderMagnification;

    magChange = false;
    mergeCube2Pointer = NULL;

    return true;
}

uint Loader::removeLoadedCubes(Hashtable *currentLoadedHash, uint prevLoaderMagnification) {
    C2D_Element *currentCube = NULL, *nextCube = NULL;
    C_Element *currentCCube = NULL;
    Byte *delCubePtr = NULL;

    for (currentCube = currentLoadedHash->listEntry->next;
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


            if(slotListAddElement(this->freeDcSlots, delCubePtr) < 1) {
                LOG("Error adding slot %p (formerly of Dc (%d, %d, %d)) into freeDcSlots.",
                    delCubePtr,
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z);
                return false;
            }
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
            if(slotListAddElement(this->freeOcSlots, delCubePtr) < 1) {
                LOG("Error adding slot %p (formerly of Oc (%d, %d, %d)) into freeOcSlots.",
                    delCubePtr,
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z);
                return false;
            }
        }

        state->protectCube2Pointer->unlock();
    }

    return true;
}

#define DECOMP_THREAD_NUM (1)
uint Loader::loadCubes() {
    C_Element *currentCube = NULL, *prevCube = NULL, *decompedCube = NULL;
    CubeSlot *currentDcSlot = NULL;
    uint loadedDc;
    FtpThread *ftpThread;
    ftp_thread_struct fts = {0};
    loadcube_thread_struct lts_array[DECOMP_THREAD_NUM] = {0};
    loadcube_thread_struct *lts_current;
    loadcube_thread_struct lts_empty = {0};
    LoadCubeThread *threadHandle_array[DECOMP_THREAD_NUM] = {NULL};
    QSemaphore *loadCubeThreadSem = new QSemaphore(DECOMP_THREAD_NUM);
    int hadError = false;
    int retVal = true;
    int cubeCount = 0, loadedCubeCount = 0;
    int thread_index;
    int isBreak;
    //DWORD waitTime = 0, decompTime = 0;
    //DWORD currTick, beginTickCount;
    //DWORD noDecompCurrent = 0, noDecompTotal = 0;

    for (currentCube = this->Dcoi->previous; currentCube != this->Dcoi; currentCube = currentCube->previous) {
        cubeCount++;
        //currentCube->debugVal = cubeCount;
    }
    //beginTickCount = GetTickCount();
    if (LM_FTP == state->loadMode) {
        fts.cubeCount = cubeCount;
        //fts.beginTickCount = beginTickCount;
        fts.ftpThreadSem = new QSemaphore(0);
        fts.loaderThreadSem = new QSemaphore(0);
        //fts.debugVal = GetTickCount();
        //LOG("DEBUG CreateThread FTP");
        fts.thisPtr = this;
        ftpThread = new FtpThread((void*)&fts);
        ftpThread->start();
    }

    while (true) {
        // Wait on an available decompression thread
        //LOG("LOADER [%08X] Waiting for decompression threads", GetTickCount() - beginTickCount);
        loadCubeThreadSem->acquire();
        //LOG("LOADER [%08X] Signalled decompression threads: %d", GetTickCount() - beginTickCount, SDL_SemValue(loadCubeThreadSem));
        for (thread_index = 0; (thread_index < DECOMP_THREAD_NUM) && lts_array[thread_index].isBusy; thread_index++);
        if (DECOMP_THREAD_NUM == thread_index) {
            LOG("All threads occupied, c'est impossible! Au revoir loader!");
            retVal = false;
            break;
        }
        if (NULL != threadHandle_array[thread_index]) {
            //LOG("LOADER [%08X] About to wait on vacant thread %d", GetTickCount() - beginTickCount, thread_index);
            threadHandle_array[thread_index]->wait();
            //LOG("LOADER [%08X] Finihsed waiting on vacant thread %d", GetTickCount() - beginTickCount, thread_index);
            //decompTime += lts_array[thread_index].decompTime;
            loadedDc = lts_array[thread_index].retVal;
            delete threadHandle_array[thread_index];
            threadHandle_array[thread_index] = NULL;
            lts_array[thread_index] = lts_empty;
            loadedCubeCount++;
            if (!loadedDc) {
                //LOG("LOADER [%08X] Thread %d finished wrongfully!", GetTickCount() - beginTickCount, thread_index);
                retVal = false;
                break;
            }
        }

        // We need to be able to cancel the loading when the
        // user crosses the reload-boundary
        // or when the dataset changed as a consequence of a mag switch

        state->protectLoadSignal->lock();
        isBreak = (state->datasetChangeSignal != NO_MAG_CHANGE) || (state->loadSignal == true) || (true == hadError);
        state->protectLoadSignal->unlock();
        if (isBreak) {
            LOG("loadCubes Interrupted!");
            retVal = false;
            break;
        }
        if (cubeCount == loadedCubeCount) {
            //LOG("LOADER [%08X] FINISHED!", GetTickCount() - beginTickCount);
            break;
        }

        if (LM_FTP == state->loadMode) {
            //currTick = -GetTickCount();
            fts.loaderThreadSem->acquire();
            //currTick += GetTickCount();
            //waitTime += currTick;
            //LOG("LOADER [%08X] Waited %d, overall %d", GetTickCount() - beginTickCount, currTick, waitTime);
        }
        for (currentCube = this->Dcoi->previous; currentCube != this->Dcoi; currentCube = currentCube->previous) {
            if (currentCube->isLoaded) {
                continue;
            }
            if ((currentCube->isFinished) || (LM_FTP != state->loadMode)) {
                //LOG("LOADER [%08X] Found %d", GetTickCount() - beginTickCount, currentCube->debugVal);
                currentCube->isLoaded = true;
                break;
            }
        }
        if (this->Dcoi == currentCube) {
            LOG("All cubes loaded, c'est impossible! Au revoir loader!");
            retVal = false;
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
        threadHandle_array[thread_index]->start();
    }

    if (LM_FTP == state->loadMode) {
        //LOG("LoadCubes posting FTP abort");
        fts.ftpThreadSem->release();
        //LOG("LoadCubes waiting FTP thread");
        ftpThread->wait();
        delete ftpThread;
        ftpThread = NULL;
        //LOG("LoadCubes FTP thread finished, destroying semaphore");
        delete fts.ftpThreadSem; fts.ftpThreadSem = NULL;
        //LOG("LoadCubes FTP semaphore destroyed");
        delete fts.loaderThreadSem; fts.loaderThreadSem = NULL;

        if (true == retVal) {
            //LOG("loadCubes: Util (%08X) %d", fts.debugVal, (100*decompTime)/(1 + GetTickCount() - beginTickCount));
        }
    }

    for (thread_index = 0; thread_index <  DECOMP_THREAD_NUM; thread_index++) {
        if (NULL != threadHandle_array[thread_index]) {
            LOG("LOADER Decompression thread %d was open! Waiting...", thread_index);
            threadHandle_array[thread_index]->wait();
            //LOG("LOADER [%08X] Decompression thread %d finished.", GetTickCount() - beginTickCount, thread_index);
            delete threadHandle_array[thread_index];
            threadHandle_array[thread_index] = NULL;
        }
    }
    delete loadCubeThreadSem; loadCubeThreadSem = NULL;

    lll_rmlist(this->Dcoi);

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
    qDebug() << "Loader: start begin";
    state->protectLoadSignal->lock();   

    // Set up DCOI and freeDcSlots / freeOcSlots.
    initLoader();
    initialized = true;

    // Start "signal wait" loop.
    while(true)  {
       // as long the loadSignal is false, the loops waits
       while(state->loadSignal == false) {
            state->conditionLoadSignal->wait(state->protectLoadSignal);
       }

       qDebug("loader received load signal: %d, %d, %d", state->currentPositionX.x, state->currentPositionX.y, state->currentPositionX.z);
       qDebug("Waiting for the load signal at %ums.\n", state->time.elapsed());
       if (true == load()) {
           break;
       }
    }

    qDebug() << "Loader: start ended";
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

    qDebug() << "Load: load begin";
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
    state->loaderMagnification = Knossos::log2uint32(state->magnification);
    //LOG("loaderMagnification = %d", state->loaderMagnification);
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

    if(loadCubes() == false) {
        qDebug("Loading of all DCOI did not complete.");
    }
    state->protectLoadSignal->lock();

    //state = true;
    qDebug() << "Loader: load ended";

    return false;
}
