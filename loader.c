/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2012
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

extern struct stateInfo *state;

int loader() {
    struct loaderState *loaderState = state->loaderState;

    SDL_LockMutex(state->protectLoadSignal);

    // Set up DCOI and freeDcSlots / freeOcSlots.
    initLoader(state);

    // Start the big "signal wait -> calculate dcoi -> load cubes, repeat" loop.
    while(TRUE) {
        while(state->loadSignal == FALSE) {
            //printf("Waiting for the load signal at %ums.\n", SDL_GetTicks());
            SDL_CondWait(state->conditionLoadSignal, state->protectLoadSignal);
        }
        state->loadSignal = FALSE;

        if(state->quitSignal == TRUE) {
            LOG("Loader quitting.");
            break;
        }

        // currentPositionX is updated only when the boundary is
        // crossed. Access to currentPositionX is synchronized through
        // the protectLoadSignal mutex.
        // DcoiFromPos fills the Dcoi hashtable with all datacubes that
        // we want to be in memory, given our current position.
        if(DcoiFromPos(loaderState->Dcoi, state) != TRUE) {
            LOG("Error computing DCOI from position.");
            continue;
        }

        SDL_UnlockMutex(state->protectLoadSignal);

        // DCOI now contains the coordinates of all cubes we want, based
        // on our current position. However, some of those might already be
        // in memory. We remove them.
        if(removeLoadedCubes(state) != TRUE) {
            LOG("Error removing already loaded cubes from DCOI.");
            continue;
        }

        // DCOI is now a list of all datacubes that we want in memory, given
        // our current position and that are not yet in memory. We go through
        // that list and load all those datacubes into free memory slots as
        // stored in the list freeDcSlots.
        if(loadCubes(state) == FALSE) {
            // LOG("Loading of all DCOI did not complete.");
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

static uint32_t DcoiFromPos(Hashtable *Dcoi, struct stateInfo *state) {
    Coordinate currentDc, origin;
    int32_t x = 0, y = 0, z = 0;
    int32_t halfSc;

    origin = state->currentPositionX;
    origin = Px2DcCoord(origin, state);

    halfSc = (int32_t)floorf((float)state->M / 2.);

    /* We start off by adding the datacubes that contain the data
       for the orthogonal slice viewports */

    /* YZ Viewport, keep x constant */
    x = origin.x;
    for(y = origin.y - halfSc; y <= origin.y + halfSc; y++) {
        for(z = origin.z - halfSc; z <= origin.z + halfSc; z++) {
            SET_COORDINATE(currentDc, x, y, z);
            ht_put(Dcoi, currentDc, (Byte *)HT_SUCCESS);
        }
    }

    /* XZ Viewport, keep y constant */
    y = origin.y;
    for(x = origin.x - halfSc; x <= origin.x + halfSc; x++) {
        for(z = origin.z - halfSc; z <= origin.z + halfSc; z++) {
            SET_COORDINATE(currentDc, x, y, z);
            ht_put(Dcoi, currentDc, (Byte *)HT_SUCCESS);
        }
    }

    /* XY Viewport, keep z constant */
    z = origin.z;
    for(x = origin.x - halfSc; x <= origin.x + halfSc; x++) {
        for(y = origin.y - halfSc; y <= origin.y + halfSc; y++) {
            SET_COORDINATE(currentDc, x, y, z);
            ht_put(Dcoi, currentDc, (Byte *)HT_SUCCESS);
        }
    }

    /* Now, we add the datacubes that were not added yet. The missing datacubes
       form 8 cubic "holes" at the corners of the supercube */

    origin.x = origin.x - halfSc;
    origin.y = origin.y - halfSc;
    origin.z = origin.z - halfSc;

    addCubicDcSet(origin.x, origin.y, origin.z, halfSc, Dcoi);
    addCubicDcSet(origin.x + halfSc + 1, origin.y, origin.z, halfSc, Dcoi);
    addCubicDcSet(origin.x, origin.y, origin.z + halfSc + 1, halfSc, Dcoi);
    addCubicDcSet(origin.x + halfSc + 1, origin.y, origin.z + halfSc + 1, halfSc, Dcoi);
    addCubicDcSet(origin.x, origin.y + halfSc + 1, origin.z, halfSc, Dcoi);
    addCubicDcSet(origin.x + halfSc + 1, origin.y + halfSc + 1, origin.z, halfSc, Dcoi);
    addCubicDcSet(origin.x, origin.y + halfSc + 1, origin.z + halfSc + 1, halfSc, Dcoi);
    addCubicDcSet(origin.x + halfSc + 1, origin.y + halfSc + 1, origin.z + halfSc + 1, halfSc, Dcoi);

    return TRUE;
}

static int32_t addCubicDcSet(int32_t xBase, int32_t yBase, int32_t zBase, int32_t edgeLen, Hashtable *target) {
    Coordinate currentDc;
    int32_t x, y, z;

    for(x = xBase; x < xBase + edgeLen; x++) {
        for(y = yBase; y < yBase + edgeLen; y++) {
            for(z = zBase; z < zBase + edgeLen; z++) {
                SET_COORDINATE(currentDc, x, y, z);
                ht_put(target, currentDc, (Byte *)HT_SUCCESS);
            }
        }
    }

    return TRUE;
}

static uint32_t loadCube(Coordinate coordinate,
                         Byte *freeDcSlot,
                         Byte *freeOcSlot,
                         struct stateInfo *state) {

    char *filename = NULL;
    char typeExtension[8] = "";
    FILE *cubeFile = NULL;
    int32_t readBytes = 0;

    /*
     * Specify either freeDcSlot or freeOcSlot.
     *
     * Caution: Not writing data into a cube slot and returning
     * FALSE from this function results in a busy wait in the
     * viewer that kills performance, so that situation should
     * never occur.
     *
     */
    if((freeDcSlot && freeOcSlot) ||
       (!freeDcSlot && !freeOcSlot)) {
        return FALSE;
    }

    if((coordinate.x > 9999) ||
       (coordinate.y > 9999) ||
       (coordinate.z > 9999) ||
       (coordinate.x < 0) ||
       (coordinate.y < 0) ||
       (coordinate.z < 0)) {
       // LOG("Requested cube coordinate (%d, %d, %d) out of bounds.",
       //     coordinate.x,
       //     coordinate.y,
       //     coordinate.z);

        goto loadcube_fail;
    }

    if(freeDcSlot)
        strncpy(typeExtension, "raw", 4);
    else
        strncpy(typeExtension, "overlay", 8);

    filename = malloc(strlen(state->path) + strlen(state->name) + strlen(typeExtension) + 38);
    if(filename == NULL) {
        LOG("Out of memory.");
        goto loadcube_fail;
    }
    memset(filename, '\0', strlen(state->path) + strlen(state->name) + strlen(typeExtension) + 38);

#ifdef LINUX
    if(state->boergens) {
        snprintf(filename, strlen(state->path) + strlen(state->name) + strlen(typeExtension) + 38,
                 "%sz%.4d/y%.4d/x%.4d/%s_x%.4d_y%.4d_z%.4d.%s",
                 state->path,
                 coordinate.z,
                 coordinate.y,
                 coordinate.x,
                 state->name,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 typeExtension);
    }
    else {
        snprintf(filename, strlen(state->path) + strlen(state->name) + strlen(typeExtension) + 38,
                 "%sx%.4d/y%.4d/z%.4d/%s_x%.4d_y%.4d_z%.4d.%s",
                 state->path,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 state->name,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 typeExtension);
    }
#else
    if(state->boergens) {
        snprintf(filename, strlen(state->path) + strlen(state->name) + strlen(typeExtension) + 38,
                 "%sz%.4d\\y%.4d\\x%.4d\\%s_x%.4d_y%.4d_z%.4d.%s",
                 state->path,
                 coordinate.z,
                 coordinate.y,
                 coordinate.x,
                 state->name,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 typeExtension);
    }
    else {
        snprintf(filename, strlen(state->path) + strlen(state->name) + strlen(typeExtension) + 38,
                 "%sx%.4d\\y%.4d\\z%.4d\\%s_x%.4d_y%.4d_z%.4d.%s",
                 state->path,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 state->name,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 typeExtension);
    }
#endif

    // The b is for compatibility with non-UNIX systems and denotes a
    // binary file.

    cubeFile = fopen(filename, "rb");
    if(cubeFile == NULL) {
        goto loadcube_fail;
    }

    if(freeDcSlot) {
        readBytes = (int32_t)fread(freeDcSlot, 1, state->cubeBytes, cubeFile);
        if(readBytes != state->cubeBytes) {
            LOG("Could read only %d / %d bytes from DC file %s.",
                readBytes,
                state->cubeBytes,
                filename);
            if(fclose(cubeFile) != 0)
                LOG("Additionally, an error occured closing the file");

            goto loadcube_fail;
        }
    }
    else {
        readBytes = (int32_t)fread(freeOcSlot, 1, state->cubeBytes * OBJID_BYTES, cubeFile);
        if(readBytes != state->cubeBytes * OBJID_BYTES) {
            LOG("Could read only %d / %d bytes from OC file %s.",
                readBytes,
                state->cubeBytes * OBJID_BYTES,
                filename);
            if(fclose(cubeFile) != 0)
                LOG("Additionally, an error occured closing the file");

            goto loadcube_fail;
        }
    }

    if(fclose(cubeFile) != 0) {
        LOG("Error closing cube file %s.", filename);
    }

    free(filename);
    return TRUE;

loadcube_fail:

    if(freeDcSlot) {
        memcpy(freeDcSlot, state->loaderState->bogusDc, state->cubeBytes);
    }
    else {
        memcpy(freeOcSlot, state->loaderState->bogusOc, state->cubeBytes * OBJID_BYTES);
    }

    free(filename);
    return TRUE;
}

static CubeSlot *slotListGetElement(CubeSlotList *slotList) {
    if(slotList->elements > 0) {
        return slotList->firstSlot;
    }
    else {
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

    if(ht_rmtable(loaderState->Dcoi) == LL_FAILURE) returnValue = FALSE;
    if(slotListDel(loaderState->freeDcSlots) == FALSE) returnValue = FALSE;
    if(slotListDel(loaderState->freeOcSlots) == FALSE) returnValue = FALSE;
    free(state->loaderState->bogusDc);
    free(state->loaderState->bogusOc);
    free(loaderState->DcSetChunk);
    free(loaderState->OcSetChunk);

    return returnValue;
}

static int32_t initLoader(struct stateInfo *state) {
    struct loaderState *loaderState = state->loaderState;
    FILE *bogusDc;
    uint32_t i = 0;

    // DCOI, ie. Datacubes of interest, is a hashtable that will contain
    // the coordinates of the datacubes and overlay cubes we need to
    // load given our current position.
    // See the comment about the ht_new() call in knossos.c
    loaderState->Dcoi = ht_new(state->cubeSetElements * 10);
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

    for(i = 0; i < state->cubeSetBytes; i += state->cubeBytes)
        slotListAddElement(loaderState->freeDcSlots, loaderState->DcSetChunk + i);


    if(state->overlay) {
        LOG("Allocating %d bytes for the overlay cubes.", state->cubeSetBytes * OBJID_BYTES);
        loaderState->OcSetChunk = malloc(state->cubeSetBytes * OBJID_BYTES);
        if(loaderState->OcSetChunk == NULL) {
            LOG("Unable to allocate memory for the OC memory slots.");
            return FALSE;
        }

        for(i = 0; i < state->cubeSetBytes * OBJID_BYTES; i += state->cubeBytes * OBJID_BYTES)
            slotListAddElement(loaderState->freeOcSlots, loaderState->OcSetChunk + i);

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
    else
        memset(state->loaderState->bogusDc, '\0', state->cubeBytes);

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

static uint32_t removeLoadedCubes(struct stateInfo *state) {
    C2D_Element *currentCube = NULL, *nextCube = NULL;
    Byte *delCubePtr = NULL;
    Hashtable *mergeCube2Pointer = NULL;

    /*
     * We merge the Dc2Pointer and Oc2Pointer tables to a new
     * temporary table mergeCube2Pointer (using the standard
     * set union).
     * This table is used to remove the unneeded entries
     * from Dcoi, Dc2Pointer and Oc2Pointer:
     *
     *  - If an element is in Dcoi and merge2Pointer, we remove it from
     *    Dcoi.
     *  - If an element is in merge2Pointer but not Dcoi, we attempt to
     *    remove it from Oc2Pointer and Dc2Pointer.
     *
     */

    mergeCube2Pointer = ht_new(state->cubeSetElements * 20);
    if(mergeCube2Pointer == HT_FAILURE) {
        LOG("Unable to create the temporary cube2pointer table.");
        return FALSE;
    }

    if(ht_union(mergeCube2Pointer,
                state->Dc2Pointer,
                state->Oc2Pointer) != HT_SUCCESS) {
        LOG("Error merging Dc2Pointer and Oc2Pointer.");
        return FALSE;
    }

    currentCube = mergeCube2Pointer->listEntry->next;
    while(currentCube != mergeCube2Pointer->listEntry) {
        nextCube = currentCube->next;

        if(ht_get(state->loaderState->Dcoi, currentCube->coordinate) == HT_FAILURE) {
            /*
             * This element is not in Dcoi, which means we can reuse its
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
            if((delCubePtr = ht_get(state->Dc2Pointer, currentCube->coordinate)) != HT_FAILURE) {
                if(ht_del(state->Dc2Pointer, currentCube->coordinate) != HT_SUCCESS) {
                    LOG("Error deleting cube (%d, %d, %d) from Dc2Pointer.",
                        currentCube->coordinate.x,
                        currentCube->coordinate.y,
                        currentCube->coordinate.z);
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
            }

            /*
             * Process Oc2Pointer if the current cube is in Oc2Pointer.
             *
             */
            if((delCubePtr = ht_get(state->Oc2Pointer, currentCube->coordinate)) != HT_FAILURE) {
                if(ht_del(state->Oc2Pointer, currentCube->coordinate) != HT_SUCCESS) {
                    LOG("Error deleting cube (%d, %d, %d) from Oc2Pointer.",
                        currentCube->coordinate.x,
                        currentCube->coordinate.y,
                        currentCube->coordinate.z);
                    return FALSE;
                }

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
        else {
            /*
             * This element is both in Dcoi and at least one of the 2Pointer tables.
             * This means we already loaded it and we can remove it from
             * Dcoi.
             *
             */

            if(ht_del(state->loaderState->Dcoi, currentCube->coordinate) != HT_SUCCESS) {
                printf("Error deleting  Dc (%d, %d, %d) from DCOI.\n",
                       currentCube->coordinate.x,
                       currentCube->coordinate.y,
                       currentCube->coordinate.z);
                return FALSE;
            }
        }

        currentCube = nextCube;
    }

    if(ht_rmtable(mergeCube2Pointer) != LL_SUCCESS)
        LOG("Error removing temporary cube to pointer table. This is a memory leak.");

    return TRUE;
}

static uint32_t loadCubes(struct stateInfo *state) {
    C2D_Element *currentCube = NULL, *nextCube = NULL;
    CubeSlot *currentDcSlot = NULL, *currentOcSlot = NULL;
    uint32_t loadedDc = FALSE, loadedOc = FALSE;

    nextCube = currentCube = state->loaderState->Dcoi->listEntry->next;
    while(nextCube != state->loaderState->Dcoi->listEntry) {
        nextCube = currentCube->next;

        /*
         * Load the datacube for the current coordinate.
         *
         */
        if((currentDcSlot = slotListGetElement(state->loaderState->freeDcSlots)) == FALSE) {
            LOG("Error getting a slot for the next Dc, wanted to load (%d, %d, %d).",
                currentCube->coordinate.x,
                currentCube->coordinate.y,
                currentCube->coordinate.z);
            return FALSE;
        }


        loadedDc = loadCube(currentCube->coordinate, currentDcSlot->cube, NULL, state);
        if(!loadedDc) {
            LOG("Error loading Dc (%d, %d, %d) into slot %p.",
                currentCube->coordinate.x,
                currentCube->coordinate.y,
                currentCube->coordinate.z,
                currentDcSlot->cube);
        }

        /*
         * Load the overlay cube if overlay is activated.
         *
         */

        if(state->overlay) {
            if((currentOcSlot = slotListGetElement(state->loaderState->freeOcSlots)) == FALSE) {
                LOG("Error getting a slot for the next Oc, wanted to load (%d, %d, %d).",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z);
                return FALSE;
            }

            loadedOc = loadCube(currentCube->coordinate, NULL, currentOcSlot->cube, state);
            if(!loadedOc) {
                LOG("Error loading Oc (%d, %d, %d) into slot %p.",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    currentOcSlot->cube);
            }
        }

        /*
         * Add pointers for the dc and oc (if at least one of them could be loaded)
         * to the Cube2Pointer table.
         *
         */
        SDL_LockMutex(state->protectCube2Pointer);
        if(loadedDc) {
            if(ht_put(state->Dc2Pointer, currentCube->coordinate, currentDcSlot->cube) != HT_SUCCESS) {
                LOG("Error inserting new Dc (%d, %d, %d) with slot %p into Dc2Pointer.",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    currentDcSlot->cube);
                return FALSE;
            }
        }

        if(loadedOc) {
            if(ht_put(state->Oc2Pointer, currentCube->coordinate, currentOcSlot->cube) != HT_SUCCESS) {
                LOG("Error inserting new Dc (%d, %d, %d) with slot %p into Dc2Pointer.",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    currentOcSlot->cube);
                return FALSE;
            }
        }
        SDL_UnlockMutex(state->protectCube2Pointer);

        /*
         * Remove the slots
         *
         */
        if(loadedDc) {
            if(slotListDelElement(state->loaderState->freeDcSlots, currentDcSlot) < 0) {
                LOG("Error deleting the current Dc slot %p from the list.",
                    currentDcSlot->cube);
                return FALSE;
            }
        }
        if(loadedOc) {
            if(slotListDelElement(state->loaderState->freeOcSlots, currentOcSlot) < 0) {
                LOG("Error deleting the current Oc slot %p from the list.",
                    currentOcSlot->cube);
                return FALSE;
            }
        }

        /*
         * Remove the current cube from Dcoi
         *
         */

        if(ht_del(state->loaderState->Dcoi, currentCube->coordinate) != HT_SUCCESS) {
            LOG("Error deleting cube coordinates (%d, %d, %d) from DCOI.",
                currentCube->coordinate.x,
                currentCube->coordinate.y,
                currentCube->coordinate.z);
            return FALSE;
        }

        // We need to be able to cancel the loading when the
        // user crosses the boundary while we are loading.

        if(state->loadSignal == TRUE) {
            if(ht_rmtable(state->loaderState->Dcoi) != LL_SUCCESS) {
                LOG("Error removing Dcoi. This is a memory leak.");
            }

            // See the comment about the ht_new() call in knossos.c
            state->loaderState->Dcoi = ht_new(state->cubeSetElements * 10);
            if(state->loaderState->Dcoi == HT_FAILURE) {
                LOG("Error creating new empty Dcoi. Fatal.");
                _Exit(FALSE);
            }

            return FALSE;
        }

        currentCube = nextCube;
    }

    return TRUE;
}
