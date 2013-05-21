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
#include "loader.h"
#include "knossos.h"
#include "sleeper.h"

extern stateInfo *state;

Loader::Loader(QObject *parent) :
    QThread(parent)
{

}

bool Loader::addCubicDcSet(int xBase, int yBase, int zBase, int edgeLen, Hashtable *target) {
    Coordinate currentDc;
    int x, y, z;

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

bool Loader::DcoiFromPos(Hashtable *Dcoi) {
    Coordinate currentDc, origin;
    int x = 0, y = 0, z = 0;
    int halfSc;

    origin = state->currentPositionX;
    origin = Coordinate::Px2DcCoord(origin);

    halfSc = (int)floorf((float)state->M / 2.);

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
CubeSlot *Loader::slotListGetElement(CubeSlotList *slotList) {
    if(slotList->elements > 0) {
        return slotList->firstSlot;
    }
    else {
        return NULL;
    }
}


bool Loader::loadCube(Coordinate coordinate, Byte *freeDcSlot, Byte *freeOcSlot) {
    char *filename = NULL;
    char typeExtension[8] = "";
    FILE *cubeFile = NULL;
    uint readBytes = 0;

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

#ifdef Q_OS_UNIX
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
        readBytes = (uint)fread(freeDcSlot, 1, state->cubeBytes, cubeFile);
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
        readBytes = (uint)fread(freeOcSlot, 1, state->cubeBytes * OBJID_BYTES, cubeFile);
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
        memcpy(freeDcSlot, this->bogusDc, state->cubeBytes);
    }
    else {
        memcpy(freeOcSlot, this->bogusOc, state->cubeBytes * OBJID_BYTES);
    }

    free(filename);
    return true;
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
    this->Dcoi = Hashtable::ht_new(state->cubeSetElements * 10);
    if(this->Dcoi == HT_FAILURE) {
        LOG("Unable to create Dcoi.");
        return false;
    }

    // freeDcSlots / freeOcSlots are lists of pointers to locations that
    // can hold data or overlay cubes. Whenever we want to load a new
    // datacube, we load it into a location from this list. Whenever a
    // datacube in memory becomes invalid, we add the pointer to its
    // memory location back into this list.
    this->freeDcSlots = slotListNew();
    if(this->freeDcSlots == NULL) {
        LOG("Unable to create freeDcSlots.");
        return false;
    }

    this->freeOcSlots = slotListNew();
    if(this->freeOcSlots == NULL) {
        LOG("Unable to create freeOcSlots.");
        return false;
    }

    // These are potentially huge allocations.
    // To lock this memory block to RAM and prevent swapping, mlock() could
    // be used on UNIX and VirtualLock() on Windows. This does not appear to be
    // necessary in the real world.

    LOG("Allocating %d bytes for the datacubes.", state->cubeSetBytes);
    this->DcSetChunk = (Byte*)malloc(state->cubeSetBytes);
    if(this->DcSetChunk == NULL) {
        LOG("Unable to allocate memory for the DC memory slots.");
        return false;
    }

    for(i = 0; i < state->cubeSetBytes; i += state->cubeBytes)
        slotListAddElement(this->freeDcSlots, this->DcSetChunk + i);


    if(state->overlay) {
        LOG("Allocating %d bytes for the overlay cubes.", state->cubeSetBytes * OBJID_BYTES);
        this->OcSetChunk = (Byte*)malloc(state->cubeSetBytes * OBJID_BYTES);
        if(this->OcSetChunk == NULL) {
            LOG("Unable to allocate memory for the OC memory slots.");
            return false;
        }

        for(i = 0; i < state->cubeSetBytes * OBJID_BYTES; i += state->cubeBytes * OBJID_BYTES)
            slotListAddElement(this->freeOcSlots, this->OcSetChunk + i);

    }

    // Load the bogus dc (a placeholder when data is unavailable).
    this->bogusDc = (Byte*)malloc(state->cubeBytes);
    if(this->bogusDc == NULL) {
        LOG("Out of memory.");
        return false;
    }
    bogusDc = fopen("bogus.raw", "r");
    if(bogusDc != NULL) {
        if(fread(this->bogusDc, 1, state->cubeBytes, bogusDc) < state->cubeBytes) {
            LOG("Unable to read the correct amount of bytes from the bogus dc file.");
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
            LOG("Out of memory.");
            return false;
        }
        memset(this->bogusOc, '\0', state->cubeBytes * OBJID_BYTES);
    }

    return true;
}

bool Loader::removeLoadedCubes(int magChange) {
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
        qDebug("Unable to create the temporary cube2pointer table.");
        return false;
    }
    state->protectCube2Pointer->lock();
    if(Hashtable::ht_union(mergeCube2Pointer,
                state->Dc2Pointer[state->loaderMagnification],
                state->Oc2Pointer[state->loaderMagnification]) != HT_SUCCESS) {
        qDebug("Error merging Dc2Pointer and Oc2Pointer for mag %d.", state->loaderMagnification);
        return false;
    }
    state->protectCube2Pointer->unlock();

    currentCube = mergeCube2Pointer->listEntry->next;
    while(currentCube != mergeCube2Pointer->listEntry) {
        nextCube = currentCube->next;

        if((Hashtable::ht_get(this->Dcoi, currentCube->coordinate) == HT_FAILURE)
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

                if(slotListAddElement(this->freeDcSlots, delCubePtr) < 1) {
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
        else {
            /*
             * This element is both in Dcoi and at least one of the 2Pointer tables.
             * This means we already loaded it and we can remove it from
             * Dcoi.
             *
             */

            if(Hashtable::ht_del(this->Dcoi, currentCube->coordinate) != HT_SUCCESS) {
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

bool Loader::loadCubes() {
    C2D_Element *currentCube = NULL, *nextCube = NULL;
    CubeSlot *currentDcSlot = NULL, *currentOcSlot = NULL;
    uint loadedDc = false, loadedOc = false;

    nextCube = currentCube = this->Dcoi->listEntry->next;
    while(nextCube != this->Dcoi->listEntry) {
        nextCube = currentCube->next;

        // Load the datacube for the current coordinate.

        if((currentDcSlot = slotListGetElement(this->freeDcSlots)) == false) {
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
            if((currentOcSlot = slotListGetElement(this->freeOcSlots)) == false) {
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

                qDebug("Error inserting new Dc (%d, %d, %d) with slot %p into Dc2Pointer[%d].",
                    currentCube->coordinate.x,
                    currentCube->coordinate.y,
                    currentCube->coordinate.z,
                    currentDcSlot->cube,
                    state->loaderMagnification);

                return false;
            }

            qDebug("inserting new Dc (%d, %d, %d) with slot %p into Dc2Pointer[%d].",
                currentCube->coordinate.x,
                currentCube->coordinate.y,
                currentCube->coordinate.z,
                currentDcSlot->cube,
                state->loaderMagnification);
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
            if(slotListDelElement(this->freeDcSlots, currentDcSlot) < 0) {
                LOG("Error deleting the current Dc slot %p from the list.",
                    currentDcSlot->cube);
                return false;
            }
        }
        if(loadedOc) {
            if(slotListDelElement(this->freeOcSlots, currentOcSlot) < 0) {
                LOG("Error deleting the current Oc slot %p from the list.",
                    currentOcSlot->cube);
                return false;
            }
        }

        //Remove the current cube from Dcoi

        if(Hashtable::ht_del(this->Dcoi, currentCube->coordinate) != HT_SUCCESS) {
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

            if(Hashtable::ht_rmtable(this->Dcoi) != LL_SUCCESS) {
                LOG("Error removing Dcoi. This is a memory leak.");
            }

            // See the comment about the ht_new() call in knossos.c
            this->Dcoi = Hashtable::ht_new(state->cubeSetElements * 10);
            if(this->Dcoi == HT_FAILURE) {
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
       load();
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
void Loader::load() {
    qDebug() << "Load: load begin";
    if(!initialized) {
        qDebug() << "Warning, Loader was not initialized";
        qDebug() << "Load: begin ended";
        return;
    }


    int magChange = false;

    state->loadSignal = false;
    magChange = false;
    if(state->datasetChangeSignal) {
        state->datasetChangeSignal = NO_MAG_CHANGE;
        magChange = true;
    }

    if(state->quitSignal == true) {
        qDebug("Loader quitting.");
        return;
    }

    // currentPositionX is updated only when the boundary is
    // crossed. Access to currentPositionX is synchronized through
    // the protectLoadSignal mutex.
    // DcoiFromPos fills the Dcoi hashtable with all datacubes that
    // we want to be in memory, given our current position.
    if(DcoiFromPos(this->Dcoi) != true) {
        qDebug("Error computing DCOI from position.");
        return;
    }


    state->protectLoadSignal->unlock();
    // DCOI now contains the coordinates of all cubes we want, based
    // on our current position. However, some of those might already be
    // in memory. We remove them.
    if(removeLoadedCubes(magChange) != true) {
        qDebug("Error removing already loaded cubes from DCOI.");
        return;
    }

    state->loaderMagnification = Knossos::log2uint32(state->magnification);

    strncpy(state->loaderName, state->magNames[state->loaderMagnification], 1024);
    strncpy(state->loaderPath, state->magPaths[state->loaderMagnification], 1024);
    //2012.12.11 HARDCODED for testing loader
    //strncpy(state->loaderName, "e1088_mag1_large", 1024);
    // DCOI is now a list of all datacubes that we want in memory, given
    // our current position and that are not yet in memory. We go through
    // that list and load all those datacubes into free memory slots as
    // stored in the list freeDcSlots.

    if(loadCubes() == false) {
        qDebug("Loading of all DCOI did not complete.");
    }
    state->protectLoadSignal->lock();

    //state = true;
    qDebug() << "Loader: load ended";
}
