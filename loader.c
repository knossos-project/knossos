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

/*
 *      This file contains the code for the loading thread and associated
 *      static functions.
 *
 *      A more complete explanation is in the docs. They should be updated to
 *      reflect the latest developments in this file. A more thorough
 *      explanation should be given right here.
 */

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "knossos-global.h"
#include "loader.h"

#define SLEEP_FACTOR (5)

extern struct stateInfo *state;

C_Element *lll_new()
{
    C_Element *lll = NULL;

    lll = malloc(sizeof(C_Element));
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

uint32_t lll_calculate_filename(C_Element *elem) {
    char *filename = NULL;
    char *local_dir_delim = NULL;
    char *file_dir_delim = NULL;
    char *loader_path = NULL;
    char *magnificationStr = NULL;
    char typeExtension[8] = "";
    char compressionExtension[8] = "";
    FILE *cubeFile = NULL;
    int32_t readBytes = 0;
    int32_t compressionRatio = 6;
    int32_t boergens_param_1 = 0, boergens_param_2 = 0, boergens_param_3 = 0;
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
    strncpy(typeExtension, "jp2", 4);
    snprintf(compressionExtension, sizeof(compressionExtension), "%d.", compressionRatio);
    /*
    strncpy(typeExtension, "raw", 4);
    compressionExtension[0] =  NULL;
    */

    elem->filename = malloc(filenameSize);
    if(elem->filename == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(elem->filename, '\0', filenameSize);
    elem->path = malloc(filenameSize);
    if(elem->path == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(elem->path, '\0', filenameSize);
    elem->fullpath_filename = malloc(filenameSize);
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
    magnificationStr = malloc(filenameSize);
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

    local_cache_path_builder = malloc(filenameSize);
    if(local_cache_path_builder == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(local_cache_path_builder, '\0', filenameSize);
    local_cache_path_total = malloc(filenameSize);
    if(local_cache_path_total == NULL) {
        LOG("Out of memory.");
        return LLL_FAILURE;
    }
    memset(local_cache_path_total, '\0', filenameSize);
    snprintf(local_cache_path_builder, filenameSize, "%smag%d%s", state->loadFtpCachePath, state->magnification, local_dir_delim);
    strcat(local_cache_path_total, local_cache_path_builder); mkdir(local_cache_path_total);
    /* LOG("%s", local_cache_path_total); */
    snprintf(local_cache_path_builder, filenameSize, "%s%s%.4d", local_dir_delim, boergens_param_1_name, boergens_param_1);
    strcat(local_cache_path_total, local_cache_path_builder); mkdir(local_cache_path_total);
    /* LOG("%s", local_cache_path_total); */
    snprintf(local_cache_path_builder, filenameSize, "%s%s%.4d", local_dir_delim, boergens_param_2_name, boergens_param_2);
    strcat(local_cache_path_total, local_cache_path_builder); mkdir(local_cache_path_total);
    /* LOG("%s", local_cache_path_total); */
    snprintf(local_cache_path_builder, filenameSize, "%s%s%.4d", local_dir_delim, boergens_param_3_name, boergens_param_3);
    strcat(local_cache_path_total, local_cache_path_builder); mkdir(local_cache_path_total);
    /* LOG("%s", local_cache_path_total); */
    free(local_cache_path_builder);

    elem->local_filename = malloc(filenameSize);
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

uint32_t lll_put(C_Element *destElement, Hashtable *currentLoadedHash, Coordinate key) {
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
        loadedCubePtr = ht_get_element(currentLoadedHash, key);
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
    putElement->hasError = FALSE;
    putElement->curlHandle = NULL;
    putElement->isFinished = FALSE;
    putElement->isAborted = FALSE;
    putElement->isLoaded = FALSE;
    putElement->debugVal = 0;
    putElement->tickDownloaded = 0;
    putElement->tickDecompressed = 0;

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

int loader() {
    struct loaderState *loaderState = state->loaderState;
    int32_t magChange = FALSE;
    Hashtable *mergeCube2Pointer = NULL;
    uint32_t funcRetVal;
    uint32_t prevLoaderMagnification;

    SDL_LockMutex(state->protectLoadSignal);

    // Set up DCOI and freeDcSlots / freeOcSlots.
    initLoader();

    state->loaderMagnification = log2uint32(state->magnification);
    //LOG("loaderMagnification = %d", state->loaderMagnification);
    strncpy(state->loaderName, state->magNames[state->loaderMagnification], 1024);
    strncpy(state->loaderPath, state->magPaths[state->loaderMagnification], 1024);

    prevLoaderMagnification = state->loaderMagnification;

    // Start the big "signal wait -> calculate dcoi -> load cubes, repeat" loop.
    while(TRUE) {
        while(state->loadSignal == FALSE) {
            //LOG("loader received load signal: %d, %d, %d", state->currentPositionX.x, state->currentPositionX.y, state->currentPositionX.z);
            //printf("Waiting for the load signal at %ums.\n", SDL_GetTicks());
            SDL_CondWait(state->conditionLoadSignal, state->protectLoadSignal);
        }
        state->loadSignal = FALSE;
        magChange = FALSE;
        if(state->datasetChangeSignal) {
            state->datasetChangeSignal = NO_MAG_CHANGE;
            magChange = TRUE;
        }

        if(state->quitSignal == TRUE) {
            LOG("Loader quitting.");
            SDL_UnlockMutex(state->protectLoadSignal);
            break;
        }

        /*
        We create a hash that would later be assigned the union of Dc and Oc cubes already loaded by now.
        For each cube element in the hash, the datacube field is automatically initialized to NULL
        upon creation by ht_union.
        When we finished calculating the cubes that need to be loaded now, their datacube field would
        be different than NULL.
        */
        mergeCube2Pointer = ht_new(state->cubeSetElements * 20);
        if(mergeCube2Pointer == HT_FAILURE) {
            LOG("Unable to create the temporary cube2pointer table.");
            SDL_UnlockMutex(state->protectLoadSignal);
            break;
        }
        SDL_LockMutex(state->protectCube2Pointer);
        funcRetVal = ht_union(mergeCube2Pointer,
                state->Dc2Pointer[state->loaderMagnification],
                state->Oc2Pointer[state->loaderMagnification]);
        SDL_UnlockMutex(state->protectCube2Pointer);
        if (HT_SUCCESS != funcRetVal) {
            LOG("Error merging Dc2Pointer and Oc2Pointer for mag %d.", state->loaderMagnification);
            SDL_UnlockMutex(state->protectLoadSignal);
            break;
        }

        prevLoaderMagnification = state->loaderMagnification;
        state->loaderMagnification = log2uint32(state->magnification);
        //LOG("loaderMagnification = %d", state->loaderMagnification);
        strncpy(state->loaderName, state->magNames[state->loaderMagnification], 1024);
        strncpy(state->loaderPath, state->magPaths[state->loaderMagnification], 1024);

        // currentPositionX is updated only when the boundary is
        // crossed. Access to currentPositionX is synchronized through
        // the protectLoadSignal mutex.
        // DcoiFromPos fills the Dcoi list with all datacubes that
        // we want to be in memory, given our current position.
        if(DcoiFromPos(loaderState->Dcoi, magChange ? NULL : mergeCube2Pointer) != TRUE) {
            LOG("Error computing DCOI from position.");
            SDL_UnlockMutex(state->protectLoadSignal);
            break;
        }

        /*
        We shall now remove from current loaded cubes those cubes that are not required by new coordinate,
        or simply all cubes in case of magnification change
        */
        if(removeLoadedCubes(mergeCube2Pointer, prevLoaderMagnification) != TRUE) {
            LOG("Error removing already loaded cubes from DCOI.");
            break;
        }
        if (ht_rmtable(mergeCube2Pointer) != LL_SUCCESS) {
            LOG("Error removing temporary cube to pointer table. This is a memory leak.");
        }

        SDL_UnlockMutex(state->protectLoadSignal);

        // DCOI is now a list of all datacubes that we want in memory, given
        // our current position and that are not yet in memory. We go through
        // that list and load all those datacubes into free memory slots as
        // stored in the list freeDcSlots.
        if(loadCubes() == FALSE) {
            LOG("Loading of all DCOI did not complete.");
        }

        SDL_LockMutex(state->protectLoadSignal);
    }

    // Free the structures in loaderState and loaderState itself.
    if(cleanUpLoader(loaderState) == FALSE) {
        LOG("Error cleaning up loading thread.");
        return FALSE;
    }

    return TRUE;
}

int calc_nonzero_sign(float x) {
    if (x > 0) {
        return 1;
    }
    return -1;
}

void CalcLoadOrderMetric(float halfSc, floatCoordinate currentMetricPos, floatCoordinate direction, float *metrics) {
    float distance_from_plane, distance_from_direction, distance_from_origin, dot_product;
    int32_t i = 0;

    distance_from_plane = CALC_POINT_DISTANCE_FROM_PLANE(currentMetricPos, direction);
    distance_from_origin = CALC_VECTOR_NORM(currentMetricPos);
    dot_product = CALC_DOT_PRODUCT(currentMetricPos, direction);

    metrics[i++] = ( (distance_from_plane <= 1) || (distance_from_origin <= halfSc) ) ? -1.0 : 1.0;
    metrics[i++] = distance_from_plane > 1 ? 1.0 : -1.0;
    metrics[i++] = dot_product < 0 ?  1.0 : -1.0;
    metrics[i++] = distance_from_plane;
    metrics[i++] = distance_from_origin;

    state->loaderState->currentMaxMetric = MAX(state->loaderState->currentMaxMetric, i);
    /* LOG("%f\t%f\t%f\t%f\t%f\t%f",
        currentMetricPos.x, currentMetricPos.y, currentMetricPos.z,
        metrics[0], metrics[1], metrics[2]); */
}

int CompareLoadOrderMetric(const void * a, const void * b)
{
    LO_Element *elem_a, *elem_b;
    float m_a, m_b;
    int32_t metric_index;

    elem_a = (LO_Element *)a;
    elem_b = (LO_Element *)b;

    for (metric_index = 0; metric_index < state->loaderState->currentMaxMetric; metric_index++) {
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

floatCoordinate find_close_xyz(floatCoordinate direction) {
    floatCoordinate xyz[3];
    float dot_products[3];
    int32_t i;
    float max_dot_product;
    int32_t max_dot_product_i;

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

static uint32_t DcoiFromPos(C_Element *Dcoi, Hashtable *currentLoadedHash) {
    Coordinate currentDc, currentOrigin;
    floatCoordinate currentMetricPos, direction;
    Coordinate debugCoor;
    LO_Element *DcArray;
    int32_t cubeElemCount;
    int32_t i;
    int32_t edgeLen, halfSc;
    int32_t x, y, z;
    float dx = 0, dy = 0, dz = 0;
    float direction_norm;
    float floatHalfSc;

    edgeLen = state->M;
    floatHalfSc = (float)edgeLen / 2.;
    halfSc = (int32_t)floorf(floatHalfSc);

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
    /*
    for (i = 0; i < LL_CURRENT_DIRECTIONS_SIZE; i++) {
        // LOG("%d\t%d\t%d", state->currentDirections[i].x, state->currentDirections[i].y, state->currentDirections[i].z);
        dx += (float)state->currentDirections[i].x;
        dy += (float)state->currentDirections[i].y;
        dz += (float)state->currentDirections[i].z;
        // LOG("Progress %f,%f,%f", dx, dy, dz);
    }
    */
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
    DcArray = malloc(sizeof(DcArray[0]) * cubeElemCount);
    if (NULL == DcArray) {
        return FALSE;
    }
    memset(DcArray, 0, sizeof(DcArray[0]) * cubeElemCount);
    currentOrigin = Px2DcCoord(state->currentPositionX);
    i = 0;
    state->loaderState->currentMaxMetric = 0;
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
     void *,
     size_t,
     size_t,
     int (*)(const void*, const void*) );
     _quicksort(DcArray, cubeElemCount, sizeof(DcArray[0]), CompareLoadOrderMetric);
    //LOG("New Order (%f, %f, %f):", direction.x, direction.y, direction.z);
    for (i = 0; i < cubeElemCount; i++) {
        /*
        LOG("%d\t%d\t%d",
            DcArray[i].coordinate.x - currentOrigin.x,
            DcArray[i].coordinate.y - currentOrigin.y,
            DcArray[i].coordinate.z - currentOrigin.z);
        */
        if (LLL_SUCCESS != lll_put(Dcoi, currentLoadedHash, DcArray[i].coordinate)) {
            return FALSE;
        }
    }
    free(DcArray);

    return TRUE;
}

 int loadCube(loadcube_thread_struct *lts) {
    int32_t retVal = TRUE;
    CubeSlot *currentDcSlot;
    char *filename;
    //uint32_t tickCount = GetTickCount();

    /*
     * Specify either freeDcSlot or freeOcSlot.
     *
     * Caution: Not writing data into a cube slot and returning
     * FALSE from this function results in a busy wait in the
     * viewer that kills performance, so that situation should
     * never occur.
     *
     */

    //LOG("LOADER [%08X] %d started decompressing %d", GetTickCount() - lts->beginTickCount, lts->threadIndex, lts->currentCube->debugVal);

    SDL_LockMutex(state->protectLoaderSlots);
    currentDcSlot = slotListGetElement(state->loaderState->freeDcSlots);
    SDL_UnlockMutex(state->protectLoaderSlots);
    if (FALSE == currentDcSlot) {
        LOG("Error getting a slot for the next Dc, wanted to load (%d, %d, %d), mag %d dataset.",
            lts->currentCube->coordinate.x,
            lts->currentCube->coordinate.y,
            lts->currentCube->coordinate.z,
            state->magnification);
        retVal = FALSE;
        goto loadcube_ret;
    }

    if (!currentDcSlot->cube) {
        goto loadcube_ret;
    }
    if (LM_FTP == state->loadMode) {
        if (lts->currentCube->isAborted) {
            LOG("Aborted while retrieving %d,%d,%d (DEBUG %d)", lts->currentCube->coordinate.x, lts->currentCube->coordinate.y, lts->currentCube->coordinate.z, lts->currentCube->debugVal);
            retVal = FALSE;
            goto loadcube_ret;
        }
        if (lts->currentCube->hasError) {
            LOG("Error retrieving %d,%d,%d (DEBUG %d)", lts->currentCube->coordinate.x, lts->currentCube->coordinate.y, lts->currentCube->coordinate.z, lts->currentCube->debugVal);
            goto loadcube_fail;
        }
    }

    filename = (LM_FTP == state->loadMode) ? lts->currentCube->local_filename : lts->currentCube->fullpath_filename;
    if (EXIT_SUCCESS != jp2_decompress_main(filename, currentDcSlot->cube, state->cubeBytes)) {
        LOG("Decompression function failed!");
        goto loadcube_fail;
    }
    goto loadcube_manage;

loadcube_fail:
    memcpy(currentDcSlot->cube, state->loaderState->bogusDc, state->cubeBytes);

loadcube_manage:
     /* Add pointers for the dc and oc (if at least one of them could be loaded)
     * to the Cube2Pointer table.
     *
     */
    if (!retVal) {
        goto loadcube_ret;
    }
    SDL_LockMutex(state->protectCube2Pointer);
    if(ht_put(state->Dc2Pointer[state->loaderMagnification], lts->currentCube->coordinate, currentDcSlot->cube) != HT_SUCCESS) {
        LOG("Error inserting new Dc (%d, %d, %d) with slot %p into Dc2Pointer[%d].",
            lts->currentCube->coordinate.x,
            lts->currentCube->coordinate.y,
            lts->currentCube->coordinate.z,
            currentDcSlot->cube,
            state->loaderMagnification);
        retVal = FALSE;
    }
    SDL_UnlockMutex(state->protectCube2Pointer);
    if (!retVal) {
        goto loadcube_ret;
    }

    /*
     * Remove the slots
     *
     */
    SDL_LockMutex(state->protectLoaderSlots);
    if(slotListDelElement(state->loaderState->freeDcSlots, currentDcSlot) < 0) {
        LOG("Error deleting the current Dc slot %p from the list.",
            currentDcSlot->cube);
        retVal = FALSE;
    }
    SDL_UnlockMutex(state->protectLoaderSlots);
    if (!retVal) {
        goto loadcube_ret;
    }

loadcube_ret:
    if (retVal) {
        //lts->decompTime = GetTickCount() - tickCount;
    }
    lts->isBusy = FALSE;
    //LOG("LOADER [%08X] %d finished decompressing %d", GetTickCount() - lts->beginTickCount, lts->threadIndex, lts->currentCube->debugVal);
    SDL_SemPost(lts->loadCubeThreadSem);
    return retVal;
}

static CubeSlot *slotListGetElement(CubeSlotList *slotList) {
    if(slotList->elements > 0) {
        //LOG("Pop %08X (%d): %08X", slotList, slotList->elements, slotList->firstSlot);
        return slotList->firstSlot;
    }
    else {
        //LOG("Pop %08X EMPTY!", slotList);
        return FALSE;
    }
}

static int32_t slotListDelElement(CubeSlotList *slotList, CubeSlot *element) {
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

static int32_t slotListAddElement(CubeSlotList *slotList, Byte *datacube) {
    CubeSlot *newElement;

    newElement = malloc(sizeof(CubeSlot));
    if(newElement == NULL) {
        printf("Out of memory\n");
        return FAIL;
    }

    newElement->cube = datacube;
    newElement->next = slotList->firstSlot;
    newElement->previous = NULL;

    slotList->firstSlot = newElement;

    slotList->elements = slotList->elements + 1;

    //LOG("Push %08X (%d): %08X", slotList, slotList->elements, newElement);

    return slotList->elements;
}

static CubeSlotList *slotListNew() {
    CubeSlotList *newDcSlotList;

    newDcSlotList = malloc(sizeof(CubeSlotList));
    if(newDcSlotList == NULL) {
        printf("Out of memory.\n");
        return NULL;
    }
    newDcSlotList->firstSlot = NULL;
    newDcSlotList->elements = 0;

    return newDcSlotList;
}

static int32_t slotListDel(CubeSlotList *delList) {
    while(delList->elements > 0) {
        if(slotListDelElement(delList, delList->firstSlot) < 0) {
            printf("Error deleting element at %p from the slot list. %d elements remain in the list.\n",
                   delList->firstSlot->cube, delList->elements);
        }
    }

    free(delList);

    return TRUE;
}

static uint32_t cleanUpLoader(struct loaderState *loaderState) {
    uint32_t returnValue = TRUE;

    lll_rmlist(loaderState->Dcoi); free(loaderState->Dcoi);
    if(slotListDel(loaderState->freeDcSlots) == FALSE) returnValue = FALSE;
    if(slotListDel(loaderState->freeOcSlots) == FALSE) returnValue = FALSE;
    free(state->loaderState->bogusDc);
    free(state->loaderState->bogusOc);
    free(loaderState->DcSetChunk);
    free(loaderState->OcSetChunk);

    return returnValue;
}

static int32_t initLoader() {
    struct loaderState *loaderState = state->loaderState;
    FILE *bogusDc;
    uint32_t i = 0;

    // DCOI, ie. Datacubes of interest, is a hashtable that will contain
    // the coordinates of the datacubes and overlay cubes we need to
    // load given our current position.
    // See the comment about the ht_new() call in knossos.c
    loaderState->Dcoi = lll_new();
    if(loaderState->Dcoi == HT_FAILURE) {
        LOG("Unable to create Dcoi.");
        return FALSE;
    }

    // freeDcSlots / freeOcSlots are lists of pointers to locations that
    // can hold data or overlay cubes. Whenever we want to load a new
    // datacube, we load it into a location from this list. Whenever a
    // datacube in memory becomes invalid, we add the pointer to its
    // memory location back into this list.
    loaderState->freeDcSlots = slotListNew();
    if(loaderState->freeDcSlots == NULL) {
        LOG("Unable to create freeDcSlots.");
        return FALSE;
    }

    loaderState->freeOcSlots = slotListNew();
    if(loaderState->freeOcSlots == NULL) {
        LOG("Unable to create freeOcSlots.");
        return FALSE;
    }

    // These are potentially huge allocations.
    // To lock this memory block to RAM and prevent swapping, mlock() could
    // be used on UNIX and VirtualLock() on Windows. This does not appear to be
    // necessary in the real world.

    LOG("Allocating %d bytes for the datacubes.", state->cubeSetBytes);
    loaderState->DcSetChunk = malloc(state->cubeSetBytes);
    if(loaderState->DcSetChunk == NULL) {
        LOG("Unable to allocate memory for the DC memory slots.");
        return FALSE;
    }
    memset(loaderState->DcSetChunk, 0, state->cubeSetBytes);
    for(i = 0; i < state->cubeSetBytes; i += state->cubeBytes) {
        slotListAddElement(loaderState->freeDcSlots, loaderState->DcSetChunk + i);
    }

    if(state->overlay) {
        LOG("Allocating %d bytes for the overlay cubes.", state->cubeSetBytes * OBJID_BYTES);
        loaderState->OcSetChunk = malloc(state->cubeSetBytes * OBJID_BYTES);
        if(loaderState->OcSetChunk == NULL) {
            LOG("Unable to allocate memory for the OC memory slots.");
            return FALSE;
        }
        memset(loaderState->OcSetChunk, 0, state->cubeSetBytes * OBJID_BYTES);
        for(i = 0; i < state->cubeSetBytes * OBJID_BYTES; i += state->cubeBytes * OBJID_BYTES) {
            slotListAddElement(loaderState->freeOcSlots, loaderState->OcSetChunk + i);
        }
    }

    // Load the bogus dc (a placeholder when data is unavailable).
    state->loaderState->bogusDc = malloc(state->cubeBytes);
    if(state->loaderState->bogusDc == NULL) {
        LOG("Out of memory.");
        return FALSE;
    }
    bogusDc = fopen("bogus.raw", "r");
    if(bogusDc != NULL) {
        if(fread(state->loaderState->bogusDc, 1, state->cubeBytes, bogusDc) < state->cubeBytes) {
            LOG("Unable to read the correct amount of bytes from the bogus dc file.");
            memset(state->loaderState->bogusDc, '\0', state->cubeBytes);
        }
        fclose(bogusDc);
    }
    else {
        memset(state->loaderState->bogusDc, '\0', state->cubeBytes);
    }
    if(state->overlay) {
        // bogus oc is white
        state->loaderState->bogusOc = malloc(state->cubeBytes * OBJID_BYTES);
        if(state->loaderState->bogusOc == NULL) {
            LOG("Out of memory.");
            return FALSE;
        }
        memset(state->loaderState->bogusOc, '\0', state->cubeBytes * OBJID_BYTES);
    }

    return TRUE;
}

static uint32_t removeLoadedCubes(Hashtable *currentLoadedHash, uint32_t prevLoaderMagnification) {
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

        SDL_LockMutex(state->protectCube2Pointer);

        /*
         * Process Dc2Pointer if the current cube is in Dc2Pointer.
         *
         */
        if((delCubePtr = ht_get(state->Dc2Pointer[prevLoaderMagnification], currentCube->coordinate)) != HT_FAILURE) {
            if(ht_del(state->Dc2Pointer[prevLoaderMagnification], currentCube->coordinate) != HT_SUCCESS) {
                LOG("Error deleting cube (%d, %d, %d) from Dc2Pointer[%d].",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    prevLoaderMagnification);
                return FALSE;
            }


            if(slotListAddElement(state->loaderState->freeDcSlots, delCubePtr) < 1) {
                LOG("Error adding slot %p (formerly of Dc (%d, %d, %d)) into freeDcSlots.",
                    delCubePtr,
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z);
                return FALSE;
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
        if((delCubePtr = ht_get(state->Oc2Pointer[prevLoaderMagnification], currentCube->coordinate)) != HT_FAILURE) {
            if(ht_del(state->Oc2Pointer[prevLoaderMagnification], currentCube->coordinate) != HT_SUCCESS) {
                LOG("Error deleting cube (%d, %d, %d) from Oc2Pointer.",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z);
                return FALSE;
            }

            memset(delCubePtr, 0, state->cubeSetBytes * OBJID_BYTES);
            if(slotListAddElement(state->loaderState->freeOcSlots, delCubePtr) < 1) {
                LOG("Error adding slot %p (formerly of Oc (%d, %d, %d)) into freeOcSlots.",
                    delCubePtr,
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z);
                return FALSE;
            }
        }

        SDL_UnlockMutex(state->protectCube2Pointer);
    }

    return TRUE;
}

#define DECOMP_THREAD_NUM (1)
static uint32_t loadCubes() {
    C_Element *currentCube = NULL, *prevCube = NULL, *decompedCube = NULL;
    CubeSlot *currentDcSlot = NULL;
    uint32_t loadedDc = FALSE;
    SDL_Thread *ftpThread = NULL;
    ftp_thread_struct fts = {0};
    loadcube_thread_struct lts_array[DECOMP_THREAD_NUM] = {0};
    loadcube_thread_struct lts_empty = {0};
    SDL_Thread *threadHandle_array[DECOMP_THREAD_NUM] = {NULL};
    SDL_Thread *threadHandle_current;
    void *loadCubeThreadSem = SDL_CreateSemaphore(DECOMP_THREAD_NUM);
    int32_t hadError = FALSE;
    int32_t retVal = TRUE;
    int32_t cubeCount = 0, loadedCubeCount = 0;
    int32_t thread_index;
    int32_t isBreak;
    uint32_t waitTime = 0, decompTime = 0;
    uint32_t currTick, beginTickCount;
    uint32_t noDecompCurrent = 0, noDecompTotal = 0;

    for (currentCube = state->loaderState->Dcoi->previous; currentCube != state->loaderState->Dcoi; currentCube = currentCube->previous) {
        cubeCount++;
        currentCube->debugVal = cubeCount;
    }
    //beginTickCount = GetTickCount();
    if (LM_FTP == state->loadMode) {
        fts.cubeCount = cubeCount;
        fts.beginTickCount = beginTickCount;
        fts.ftpThreadSem = SDL_CreateSemaphore(0);
        fts.loaderThreadSem = SDL_CreateSemaphore(0);
        //fts.debugVal = GetTickCount();
        //LOG("DEBUG CreateThread FTP");
        ftpThread = SDL_CreateThread(ftpthreadfunc, &fts);
    }

    while (TRUE) {
        // Wait on an available decompression thread
        //LOG("LOADER [%08X] Waiting for decompression threads", GetTickCount() - beginTickCount);
        SDL_SemWait(loadCubeThreadSem);
        //LOG("LOADER [%08X] Signalled decompression threads: %d", GetTickCount() - beginTickCount, SDL_SemValue(loadCubeThreadSem));
        for (thread_index = 0; (thread_index < DECOMP_THREAD_NUM) && lts_array[thread_index].isBusy; thread_index++);
        if (DECOMP_THREAD_NUM == thread_index) {
            LOG("All threads occupied, c'est impossible! Au revoir loader!");
            retVal = FALSE;
            break;
        }
        if (NULL != threadHandle_array[thread_index]) {
            //LOG("LOADER [%08X] About to wait on vacant thread %d", GetTickCount() - beginTickCount, thread_index);
            SDL_WaitThread(threadHandle_array[thread_index], &loadedDc);
            //LOG("LOADER [%08X] Finihsed waiting on vacant thread %d", GetTickCount() - beginTickCount, thread_index);
            decompTime += lts_array[thread_index].decompTime;
            threadHandle_array[thread_index] = NULL;
            lts_array[thread_index] = lts_empty;
            loadedCubeCount++;
            if (!loadedDc) {
                //LOG("LOADER [%08X] Thread %d finished wrongfully!", GetTickCount() - beginTickCount, thread_index);
                retVal = FALSE;
                break;
            }
        }

        // We need to be able to cancel the loading when the
        // user crosses the reload-boundary
        // or when the dataset changed as a consequence of a mag switch

        SDL_LockMutex(state->protectLoadSignal);
        isBreak = (state->datasetChangeSignal != NO_MAG_CHANGE) || (state->loadSignal == TRUE) || (TRUE == hadError);
        SDL_UnlockMutex(state->protectLoadSignal);
        if (isBreak) {
            LOG("loadCubes Interrupted!");
            retVal = FALSE;
            break;
        }
        if (cubeCount == loadedCubeCount) {
            //LOG("LOADER [%08X] FINISHED!", GetTickCount() - beginTickCount);
            break;
        }

        if (LM_FTP == state->loadMode) {
            //currTick = -GetTickCount();
            SDL_SemWait(fts.loaderThreadSem);
            //currTick += GetTickCount();
            //waitTime += currTick;
            //LOG("LOADER [%08X] Waited %d, overall %d", GetTickCount() - beginTickCount, currTick, waitTime);
        }
        for (currentCube = state->loaderState->Dcoi->previous; currentCube != state->loaderState->Dcoi; currentCube = currentCube->previous) {
            if (currentCube->isLoaded) {
                continue;
            }
            if ((currentCube->isFinished) || (LM_FTP != state->loadMode)) {
                //LOG("LOADER [%08X] Found %d", GetTickCount() - beginTickCount, currentCube->debugVal);
                currentCube->isLoaded = TRUE;
                break;
            }
        }
        if (state->loaderState->Dcoi == currentCube) {
            LOG("All cubes loaded, c'est impossible! Au revoir loader!");
            retVal = FALSE;
            break;
        }
        /*
         * Load the datacube for the current coordinate.
         *
         */
        lts_array[thread_index].currentCube = currentCube;
        lts_array[thread_index].isBusy = TRUE;
        lts_array[thread_index].loadCubeThreadSem = loadCubeThreadSem;
        lts_array[thread_index].beginTickCount = beginTickCount;
        lts_array[thread_index].threadIndex = thread_index;
        lts_array[thread_index].decompTime = 0;
        threadHandle_array[thread_index] = SDL_CreateThread(loadCube, &lts_array[thread_index]);
    }

    if (LM_FTP == state->loadMode) {
        //LOG("LoadCubes posting FTP abort");
        SDL_SemPost(fts.ftpThreadSem);
        //LOG("LoadCubes waiting FTP thread");
        SDL_WaitThread(ftpThread, NULL);
        //LOG("LoadCubes FTP thread finished, destroying semaphore");
        SDL_DestroySemaphore(fts.ftpThreadSem);
        //LOG("LoadCubes FTP semaphore destroyed");
        SDL_DestroySemaphore(fts.loaderThreadSem);

        if (TRUE == retVal) {
            //LOG("loadCubes: Util (%08X) %d", fts.debugVal, (100*decompTime)/(1 + GetTickCount() - beginTickCount));
        }
    }

    for (thread_index = 0; thread_index <  DECOMP_THREAD_NUM; thread_index++) {
        if (NULL != threadHandle_array[thread_index]) {
            LOG("LOADER Decompression thread %d was open! Waiting...", thread_index);
            SDL_WaitThread(threadHandle_array[thread_index], NULL);
            //LOG("LOADER [%08X] Decompression thread %d finished.", GetTickCount() - beginTickCount, thread_index);
        }
    }
    SDL_DestroySemaphore(loadCubeThreadSem);

    lll_rmlist(state->loaderState->Dcoi);

    return retVal;
}
