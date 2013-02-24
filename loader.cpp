//2012.12.11 contains hardcoded loaderName for testing loader (in start())
#include <math.h>

#include "loader.h"
#include "knossos-global.h"
#include "knossos.h"
#include "sleeper.h"


extern stateInfo *state;

Loader::Loader(QObject *parent) :
    QObject(parent)
{

}

static bool addCubicDcSet(int32_t xBase, int32_t yBase, int32_t zBase, int32_t edgeLen, Hashtable *target) {
    Coordinate currentDc;
    int32_t x, y, z;

    for(x = xBase; x < xBase + edgeLen; x++) {
        for(y = yBase; y < yBase + edgeLen; y++) {
            for(z = zBase; z < zBase + edgeLen; z++) {
                SET_COORDINATE(currentDc, x, y, z);
                Hashtable::ht_put(target, currentDc, (Byte *)HT_SUCCESS);
            }
        }
    }

    return true;
}

static bool DcoiFromPos(Hashtable *Dcoi) {
    Coordinate currentDc, origin;
    int32_t x = 0, y = 0, z = 0;
    int32_t halfSc;

    origin = state->currentPositionX;
    origin = Coordinate::Px2DcCoord(origin);

    halfSc = (int32_t)floorf((float)state->M / 2.);

    /* We start off by adding the datacubes that contain the data
       for the orthogonal slice viewports */

    /* YZ Viewport, keep x constant */
    x = origin.x;
    for(y = origin.y - halfSc; y <= origin.y + halfSc; y++) {
        for(z = origin.z - halfSc; z <= origin.z + halfSc; z++) {
            SET_COORDINATE(currentDc, x, y, z);
            Hashtable::ht_put(Dcoi, currentDc, (Byte *)HT_SUCCESS);
        }
    }

    /* XZ Viewport, keep y constant */
    y = origin.y;
    for(x = origin.x - halfSc; x <= origin.x + halfSc; x++) {
        for(z = origin.z - halfSc; z <= origin.z + halfSc; z++) {
            SET_COORDINATE(currentDc, x, y, z);
            Hashtable::ht_put(Dcoi, currentDc, (Byte *)HT_SUCCESS);
        }
    }

    /* XY Viewport, keep z constant */
    z = origin.z;
    for(x = origin.x - halfSc; x <= origin.x + halfSc; x++) {
        for(y = origin.y - halfSc; y <= origin.y + halfSc; y++) {
            SET_COORDINATE(currentDc, x, y, z);
            Hashtable::ht_put(Dcoi, currentDc, (Byte *)HT_SUCCESS);
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

    return true;
}

/**
 * @brief slotListGetElement
 * @param slotList
 * @return false changed to NULL
 */
static CubeSlot *slotListGetElement(CubeSlotList *slotList) {
    if(slotList->elements > 0) {
        return slotList->firstSlot;
    }
    else {
        return NULL;
    }
}


static bool loadCube(Coordinate coordinate, Byte *freeDcSlot, Byte *freeOcSlot) {
    char *filename = NULL;
    char typeExtension[8] = "";
    FILE *cubeFile = NULL;
    uint32_t readBytes = 0;

    /*
     * Specify either freeDcSlot or freeOcSlot.
     *
     * Caution: Not writing data into a cube slot and returning
     * false from this function results in a busy wait in the
     * viewer that kills performance, so that situation should
     * never occur.
     *
     */

    if((freeDcSlot && freeOcSlot) ||
       (!freeDcSlot && !freeOcSlot)) {
        return false;
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

    if(freeDcSlot) {
        strncpy(typeExtension, "raw", 4);
    }
    else {
        strncpy(typeExtension, "overlay", 8);
    }
    filename = (char*)malloc(strlen(state->loaderPath) + strlen(state->loaderName) + strlen(typeExtension) + 38);
    if(filename == NULL) {
        LOG("Out of memory.");
        goto loadcube_fail;
    }
    memset(filename, '\0', strlen(state->loaderPath) + strlen(state->loaderName) + strlen(typeExtension) + 38);

#ifdef LINUX
    if(state->boergens) {
        snprintf(filename, strlen(state->loaderPath) + strlen(state->loaderName) + strlen(typeExtension) + 38,
                 "%sz%.4d/y%.4d/x%.4d/%s_x%.4d_y%.4d_z%.4d.%s",
                 state->loaderPath,
                 coordinate.z,
                 coordinate.y,
                 coordinate.x,
                 state->loaderName,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 typeExtension);
    }
    else {
        snprintf(filename, strlen(state->loaderPath) + strlen(state->loaderName) + strlen(typeExtension) + 38,
                 "%sx%.4d/y%.4d/z%.4d/%s_x%.4d_y%.4d_z%.4d.%s",
                 state->loaderPath,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 state->loaderName,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 typeExtension);
    }
#else
    //state->boergens= 1;
    if(state->boergens) {
        snprintf(filename, strlen(state->loaderPath) + strlen(state->loaderName) + strlen(typeExtension) + 38,
                 "%sz%.4d\\y%.4d\\x%.4d\\%s_x%.4d_y%.4d_z%.4d.%s",
                 state->loaderPath,
                 coordinate.z,
                 coordinate.y,
                 coordinate.x,
                 state->loaderName,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 typeExtension);
                 qDebug("filename: %s", filename);
    }
    else {
        snprintf(filename, strlen(state->loaderPath) + strlen(state->loaderName) + strlen(typeExtension) + 38,
                 "%sx%.4d\\y%.4d\\z%.4d\\%s_x%.4d_y%.4d_z%.4d.%s",
                 state->loaderPath,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 state->loaderName,
                 coordinate.x,
                 coordinate.y,
                 coordinate.z,
                 typeExtension);
                 qDebug("filename: %s", filename);
    }
#endif

    // The b is for compatibility with non-UNIX systems and denotes a
    // binary file.
    cubeFile = fopen(filename, "rb");

    if(cubeFile == NULL) {
        perror(filename);
        goto loadcube_fail;
    }

    if(freeDcSlot) {
        readBytes = (uint32_t)fread(freeDcSlot, 1, state->cubeBytes, cubeFile);
        if(readBytes != state->cubeBytes) {
            LOG("Could read only %d / %d bytes from DC file %s.",
                readBytes,
                state->cubeBytes,
                filename);
            if(fclose(cubeFile) != 0) {
                LOG("Additionally, an error occured closing the file");
            }
            goto loadcube_fail;
        }
        qDebug("read cubeFile successfully");
    }
    else {
        readBytes = (uint32_t)fread(freeOcSlot, 1, state->cubeBytes * OBJID_BYTES, cubeFile);
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
    return true;

loadcube_fail:
    if(freeDcSlot) {
        memcpy(freeDcSlot, state->loaderState->bogusDc, state->cubeBytes);
    }
    else {
        memcpy(freeOcSlot, state->loaderState->bogusOc, state->cubeBytes * OBJID_BYTES);
    }

    free(filename);
    return true;
}

// ALWAYS give the slotList*Element functions an element that
//      1) really is in slotList.
//      2) really exists.
// The functions don't check for that and things will break if it's not the case.
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

static bool slotListDel(CubeSlotList *delList){
    while(delList->elements > 0) {
        if(slotListDelElement(delList, delList->firstSlot) < 0) {
            printf("Error deleting element at %p from the slot list. %d elements remain in the list.\n",
                   delList->firstSlot->cube, delList->elements);
        }
    }

    free(delList);

    return true;
}
static bool cleanUpLoader(struct loaderState *loaderState) {
    bool returnValue = true;

    if(Hashtable::ht_rmtable(loaderState->Dcoi) == LL_FAILURE) { returnValue = false; }
    else if(slotListDel(loaderState->freeDcSlots) == false) { returnValue = false; }
    else if(slotListDel(loaderState->freeOcSlots) == false) { returnValue = false; }
    free(state->loaderState->bogusDc);
    free(state->loaderState->bogusOc);
    free(loaderState->DcSetChunk);
    free(loaderState->OcSetChunk);

    return returnValue;
}


static int32_t slotListAddElement(CubeSlotList *slotList, Byte *datacube) {
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

static CubeSlotList *slotListNew() {
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

static bool initLoader() {
    loaderState *loaderState = state->loaderState;
    FILE *bogusDc;
    uint32_t i = 0;

    // DCOI, ie. Datacubes of interest, is a hashtable that will contain
    // the coordinates of the datacubes and overlay cubes we need to
    // load given our current position.
    // See the comment about the ht_new() call in knossos.c
    loaderState->Dcoi = Hashtable::ht_new(state->cubeSetElements * 10);
    if(loaderState->Dcoi == HT_FAILURE) {
        LOG("Unable to create Dcoi.");
        return false;
    }

    // freeDcSlots / freeOcSlots are lists of pointers to locations that
    // can hold data or overlay cubes. Whenever we want to load a new
    // datacube, we load it into a location from this list. Whenever a
    // datacube in memory becomes invalid, we add the pointer to its
    // memory location back into this list.
    loaderState->freeDcSlots = slotListNew();
    if(loaderState->freeDcSlots == NULL) {
        LOG("Unable to create freeDcSlots.");
        return false;
    }

    loaderState->freeOcSlots = slotListNew();
    if(loaderState->freeOcSlots == NULL) {
        LOG("Unable to create freeOcSlots.");
        return false;
    }

    // These are potentially huge allocations.
    // To lock this memory block to RAM and prevent swapping, mlock() could
    // be used on UNIX and VirtualLock() on Windows. This does not appear to be
    // necessary in the real world.

    LOG("Allocating %d bytes for the datacubes.", state->cubeSetBytes);
    loaderState->DcSetChunk = (Byte*)malloc(state->cubeSetBytes);
    if(loaderState->DcSetChunk == NULL) {
        LOG("Unable to allocate memory for the DC memory slots.");
        return false;
    }

    for(i = 0; i < state->cubeSetBytes; i += state->cubeBytes)
        slotListAddElement(loaderState->freeDcSlots, loaderState->DcSetChunk + i);


    if(state->overlay) {
        LOG("Allocating %d bytes for the overlay cubes.", state->cubeSetBytes * OBJID_BYTES);
        loaderState->OcSetChunk = (Byte*)malloc(state->cubeSetBytes * OBJID_BYTES);
        if(loaderState->OcSetChunk == NULL) {
            LOG("Unable to allocate memory for the OC memory slots.");
            return false;
        }

        for(i = 0; i < state->cubeSetBytes * OBJID_BYTES; i += state->cubeBytes * OBJID_BYTES)
            slotListAddElement(loaderState->freeOcSlots, loaderState->OcSetChunk + i);

    }

    // Load the bogus dc (a placeholder when data is unavailable).
    state->loaderState->bogusDc = (Byte*)malloc(state->cubeBytes);
    if(state->loaderState->bogusDc == NULL) {
        LOG("Out of memory.");
        return false;
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
        state->loaderState->bogusOc = (Byte*)malloc(state->cubeBytes * OBJID_BYTES);
        if(state->loaderState->bogusOc == NULL) {
            LOG("Out of memory.");
            return false;
        }
        memset(state->loaderState->bogusOc, '\0', state->cubeBytes * OBJID_BYTES);
    }

    return true;
}
static bool removeLoadedCubes(int32_t magChange) {
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

    mergeCube2Pointer = Hashtable::ht_new(state->cubeSetElements * 20);
    if(mergeCube2Pointer == HT_FAILURE) {
        LOG("Unable to create the temporary cube2pointer table.");
        return false;
    }
    state->protectCube2Pointer->lock();
    if(Hashtable::ht_union(mergeCube2Pointer,
                state->Dc2Pointer[state->loaderMagnification],
                state->Oc2Pointer[state->loaderMagnification]) != HT_SUCCESS) {
        LOG("Error merging Dc2Pointer and Oc2Pointer for mag %d.", state->loaderMagnification);
        return false;
    }
    state->protectCube2Pointer->unlock();

    currentCube = mergeCube2Pointer->listEntry->next;
    while(currentCube != mergeCube2Pointer->listEntry) {
        nextCube = currentCube->next;

        if((Hashtable::ht_get(state->loaderState->Dcoi, currentCube->coordinate) == HT_FAILURE)
           || magChange) {
            /*
             * This element is not in Dcoi, which means we can reuse its
             * slots for new DCs / OCs.
             * A mag change means that all slot elements can be reused (i.e. their
             * data content became useless).
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
            if((delCubePtr = Hashtable::ht_get(state->Dc2Pointer[state->loaderMagnification],
                                               currentCube->coordinate))
                            != HT_FAILURE) {
                if(Hashtable::ht_del(state->Dc2Pointer[state->loaderMagnification],
                                     currentCube->coordinate)
                              != HT_SUCCESS) {
                    LOG("Error deleting cube (%d, %d, %d) from Dc2Pointer[%d].",
                        currentCube->coordinate.x,
                        currentCube->coordinate.y,
                        currentCube->coordinate.z,
                        state->loaderMagnification);
                    return false;
                }

                if(slotListAddElement(state->loaderState->freeDcSlots, delCubePtr) < 1) {
                    LOG("Error adding slot %p (formerly of Dc (%d, %d, %d)) into freeDcSlots.",
                        delCubePtr,
                        currentCube->coordinate.x,
                        currentCube->coordinate.y,
                        currentCube->coordinate.z);
                    return false;
                }
            }

            /*
             * Process Oc2Pointer if the current cube is in Oc2Pointer.
             *
             */
            if((delCubePtr = Hashtable::ht_get(state->Oc2Pointer[state->loaderMagnification],
                                               currentCube->coordinate))
                            != HT_FAILURE) {
                if(Hashtable::ht_del(state->Oc2Pointer[state->loaderMagnification],
                                     currentCube->coordinate)
                              != HT_SUCCESS) {
                    LOG("Error deleting cube (%d, %d, %d) from Oc2Pointer.",
                        currentCube->coordinate.x,
                        currentCube->coordinate.y,
                        currentCube->coordinate.z);
                    return false;
                }

                if(slotListAddElement(state->loaderState->freeOcSlots, delCubePtr) < 1) {
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
        else {
            /*
             * This element is both in Dcoi and at least one of the 2Pointer tables.
             * This means we already loaded it and we can remove it from
             * Dcoi.
             *
             */

            if(Hashtable::ht_del(state->loaderState->Dcoi, currentCube->coordinate) != HT_SUCCESS) {
                LOG("Error deleting  Dc (%d, %d, %d) from DCOI.\n",
                       currentCube->coordinate.x,
                       currentCube->coordinate.y,
                       currentCube->coordinate.z);
                return false;
            }
        }

        currentCube = nextCube;
    }

    if(Hashtable::ht_rmtable(mergeCube2Pointer) != LL_SUCCESS) {
        LOG("Error removing temporary cube to pointer table. This is a memory leak.");
    }
    return true;
}
static bool loadCubes() {
    C2D_Element *currentCube = NULL, *nextCube = NULL;
    CubeSlot *currentDcSlot = NULL, *currentOcSlot = NULL;
    uint32_t loadedDc = false, loadedOc = false;

    nextCube = currentCube = state->loaderState->Dcoi->listEntry->next;
    while(nextCube != state->loaderState->Dcoi->listEntry) {
        nextCube = currentCube->next;

        // Load the datacube for the current coordinate.

        if((currentDcSlot = slotListGetElement(state->loaderState->freeDcSlots)) == false) {
            LOG("Error getting a slot for the next Dc, wanted to load (%d, %d, %d), mag %d dataset.",
                currentCube->coordinate.x,
                currentCube->coordinate.y,
                currentCube->coordinate.z,
                state->magnification);
            return false;
        }

        loadedDc = loadCube(currentCube->coordinate, currentDcSlot->cube, NULL);
        if(!loadedDc) {
            LOG("Error loading Dc (%d, %d, %d) into slot %p, mag %d dataset.",
                currentCube->coordinate.x,
                currentCube->coordinate.y,
                currentCube->coordinate.z,
                currentDcSlot->cube,
                state->magnification);
        }

        // Load the overlay cube if overlay is activated.

        if(state->overlay) {
            if((currentOcSlot = slotListGetElement(state->loaderState->freeOcSlots)) == false) {
                LOG("Error getting a slot for the next Oc, wanted to load (%d, %d, %d), mag%d dataset.",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    state->magnification);
                return false;
            }

            loadedOc = loadCube(currentCube->coordinate, NULL, currentOcSlot->cube);
            if(!loadedOc) {
                LOG("Error loading Oc (%d, %d, %d) into slot %p, mag%d dataset..",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    currentOcSlot->cube,
                    state->magnification);
            }
        }

        // Add pointers for the dc and oc (if at least one of them could be loaded)
        // to the Cube2Pointer table.

        state->protectCube2Pointer->lock();
        if(loadedDc) {
            if(Hashtable::ht_put(state->Dc2Pointer[state->loaderMagnification],
                                 currentCube->coordinate, currentDcSlot->cube)
                          != HT_SUCCESS) {
                LOG("Error inserting new Dc (%d, %d, %d) with slot %p into Dc2Pointer[%d].",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    currentDcSlot->cube,
                    state->loaderMagnification);
                return false;
            }
            //qDebug("inserting new Dc (%d, %d, %d) with slot %p into Dc2Pointer[%d].",
            //    currentCube->coordinate.x,
            //    currentCube->coordinate.y,
            //    currentCube->coordinate.z,
            //    currentDcSlot->cube,
            //    state->loaderMagnification);
        }

        if(loadedOc) {
            if(Hashtable::ht_put(state->Oc2Pointer[state->loaderMagnification],
                                 currentCube->coordinate, currentOcSlot->cube)
                          != HT_SUCCESS) {
                LOG("Error inserting new Dc (%d, %d, %d) with slot %p into Oc2Pointer[%d].",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    currentOcSlot->cube,
                    state->loaderMagnification);
                return false;
            }
        }
        state->protectCube2Pointer->unlock();

        //Remove the slots

        if(loadedDc) {
            if(slotListDelElement(state->loaderState->freeDcSlots, currentDcSlot) < 0) {
                LOG("Error deleting the current Dc slot %p from the list.",
                    currentDcSlot->cube);
                return false;
            }
        }
        if(loadedOc) {
            if(slotListDelElement(state->loaderState->freeOcSlots, currentOcSlot) < 0) {
                LOG("Error deleting the current Oc slot %p from the list.",
                    currentOcSlot->cube);
                return false;
            }
        }

        //Remove the current cube from Dcoi

        if(Hashtable::ht_del(state->loaderState->Dcoi, currentCube->coordinate) != HT_SUCCESS) {
            LOG("Error deleting cube coordinates (%d, %d, %d) from DCOI.",
                currentCube->coordinate.x,
                currentCube->coordinate.y,
                currentCube->coordinate.z);
            return false;
        }

        // We need to be able to cancel the loading when the
        // user crosses the reload-boundary
        // or when the dataset changed as a consequence of a mag switch

        state->protectLoadSignal->lock();
        if((state->datasetChangeSignal != NO_MAG_CHANGE) || (state->loadSignal == true)) {
            state->protectLoadSignal->unlock();

            if(Hashtable::ht_rmtable(state->loaderState->Dcoi) != LL_SUCCESS) {
                LOG("Error removing Dcoi. This is a memory leak.");
            }

            // See the comment about the ht_new() call in knossos.c
            state->loaderState->Dcoi = Hashtable::ht_new(state->cubeSetElements * 10);
            if(state->loaderState->Dcoi == HT_FAILURE) {
                LOG("Error creating new empty Dcoi. Fatal.");
                _Exit(false);
            }
            return false;
        }
        else {
            state->protectLoadSignal->unlock();
        }
        currentCube = nextCube;
    }
    return true;
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

void Loader::start() {
    qDebug() << "Loader: start begin";
    state->protectLoadSignal->lock();

    // Set up DCOI and freeDcSlots / freeOcSlots.
    initLoader();

    // Start "signal wait" loop.
    while(true)  {
       // as long the loadSignal is false, the loops waits
       while(state->loadSignal == false) {
            //LOG("loader received load signal: %d, %d, %d", state->currentPositionX.x, state->currentPositionX.y, state->currentPositionX.z);
            //printf("Waiting for the load signal at %ums.\n", SDL_GetTicks());
            state->conditionLoadSignal->wait(state->protectLoadSignal);
       }
    }

    // Free the structures in loaderState and loaderState itself.
    if(cleanUpLoader(state->loaderState) == false) {
        LOG("Error cleaning up loading thread.");
        return;
    }

    QThread::currentThread()->quit();
    emit finished();
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
void Loader::load() {
    qDebug() << "Load: load begin";
    loaderState *loaderState = state->loaderState;
    int32_t magChange = false;

    state->loadSignal = false;
    magChange = false;
    if(state->datasetChangeSignal) {
        state->datasetChangeSignal = NO_MAG_CHANGE;
        magChange = true;
    }

    if(state->quitSignal == true) {
        LOG("Loader quitting.");
        return;
    }

    // currentPositionX is updated only when the boundary is
    // crossed. Access to currentPositionX is synchronized through
    // the protectLoadSignal mutex.
    // DcoiFromPos fills the Dcoi hashtable with all datacubes that
    // we want to be in memory, given our current position.
    if(DcoiFromPos(loaderState->Dcoi) != true) {
        LOG("Error computing DCOI from position.");
        return;
    }

    state->protectLoadSignal->unlock();

    // DCOI now contains the coordinates of all cubes we want, based
    // on our current position. However, some of those might already be
    // in memory. We remove them.
    if(removeLoadedCubes(magChange) != true) {
        LOG("Error removing already loaded cubes from DCOI.");
        return;
    }

    state->loaderMagnification = Knossos::log2uint32(state->magnification);
    strncpy(state->loaderName, state->magNames[state->loaderMagnification], 1024);
    strncpy(state->loaderPath, state->magPaths[state->loaderMagnification], 1024);
        //2012.12.11 HARDCODED for testing loader
    strncpy(state->loaderName, "e1088_mag1_large", 1024);
    // DCOI is now a list of all datacubes that we want in memory, given
    // our current position and that are not yet in memory. We go through
    // that list and load all those datacubes into free memory slots as
    // stored in the list freeDcSlots.
    if(loadCubes() == false) {
        LOG("Loading of all DCOI did not complete.");
    }
    state->protectLoadSignal->lock();
    qDebug() << "Loader: load ended";
}
