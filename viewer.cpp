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

#include "viewer.h"
#include <QDebug>
#include "knossos.h"
#include "client.h"
#include "skeletonizer.h"
#include "renderer.h"
#include "eventmodel.h"
#include "remote.h"
#include "sleeper.h"
#include "mainwindow.h"
#include "viewport.h"
#include <QGLContext>

extern  stateInfo *tempConfig;
extern  stateInfo *state;
extern Viewer *viewerEventObj;

int correct_cubes = 0;
int cubes_in_backlog = 0;

Viewer::Viewer(QObject *parent) :
    QThread(parent)
{
    window = new MainWindow();
    window->showMaximized();

    vp = new Viewport(window, VIEWPORT_XY);
    vp2 = new Viewport(window, VIEWPORT_XZ);
    vp3 = new Viewport(window, VIEWPORT_YZ);
    vp4 = new Viewport(window, VIEWPORT_SKELETON);

    vp->setGeometry(5, 40, 500, 500);
    vp2->setGeometry(510, 40, 500, 500);
    vp3->setGeometry(5, 545, 500, 500);
    vp4->setGeometry(510, 545, 500, 500);

    vp->show();
    vp2->show();
    vp3->show();
    vp4->show();

    connect(this, SIGNAL(now()), vp, SLOT(updateGL()));
    connect(this, SIGNAL(now2()), vp2, SLOT(updateGL()));
    connect(this, SIGNAL(now3()), vp3, SLOT(updateGL()));
    connect(this, SIGNAL(now4()), vp4, SLOT(updateGL()));

    // init the viewer thread and all subsystems handled by it
    if(!Viewer::initViewer()) {
       LOG("Error initializing the viewer.");
       return;
    }

    if(Renderer::initRenderer() == false) {
        qDebug("Error initializing the rendering system.");

    }

    SET_COORDINATE(state->viewerState->currentPosition, 830, 1000, 830)


    sendLoadSignal(state->viewerState->currentPosition.x,
                   state->viewerState->currentPosition.y,
                   state->viewerState->currentPosition.z,
                   NO_MAG_CHANGE);
}

static vpList* vpListNew() {
    vpList *newVpList = NULL;

    newVpList = (vpList *) malloc(sizeof(vpList));
    if(newVpList == NULL) {
        qDebug("Out of memory.\n");
        return NULL;
    }
    newVpList->entry = NULL;
    newVpList->elements = 0;

    return newVpList;
}

static int32_t vpListAddElement(vpList *vpList, vpConfig *vpConfig, vpBacklog *backlog) {
    vpListElement *newElement;

    newElement = (vpListElement *) malloc(sizeof(vpListElement));
    if(newElement == NULL) {
        qDebug("Out of memory\n");
        // Do not return false here. That's a bug. FAIL is hackish... Is there a
        // better way? */
        return FAIL;
    }

    newElement->vpConfig = vpConfig;
    newElement->backlog = backlog;

    if(vpList->entry != NULL) {
        vpList->entry->next->previous = newElement;
        newElement->next = vpList->entry->next;
        vpList->entry->next = newElement;
        newElement->previous = vpList->entry;
    }
    else {
        vpList->entry = newElement;
        vpList->entry->next = newElement;
        vpList->entry->previous = newElement;
    }

    vpList->elements = vpList->elements + 1;

    return vpList->elements;
}

static vpBacklog *backlogNew() {
    vpBacklog *newBacklog;

    newBacklog = (vpBacklog *) malloc(sizeof( vpBacklog));
    if(newBacklog == NULL) {
        qDebug("Out of memory.\n");
        return NULL;
    }
    newBacklog->entry = NULL;
    newBacklog->elements = 0;

    return newBacklog;
}

static vpList *vpListGenerate(viewerState *viewerState) {
    vpList *newVpList = NULL;
    vpBacklog *currentBacklog = NULL;
    uint32_t i = 0;

    newVpList = vpListNew();
    if(newVpList == NULL) {
        qDebug("Error generating new vpList.");
        _Exit(false);
    }

    for(i = 0; i < viewerState->numberViewports; i++) {
        if(viewerState->vpConfigs[i].type == VIEWPORT_SKELETON) {
            continue;
        }
        currentBacklog = backlogNew();
        if(currentBacklog == NULL) {
            qDebug("Error creating backlog.");
            _Exit(false);
        }
        vpListAddElement(newVpList, &(viewerState->vpConfigs[i]), currentBacklog);
    }
    return newVpList;
}

static int32_t backlogDelElement(vpBacklog *backlog, vpBacklogElement *element) {
    if(element->next == element) {
        // This is the only element in the list
        backlog->entry = NULL;
    }
    else {
        element->next->previous = element->previous;
        element->previous->next = element->next;
        backlog->entry = element->next;
    }
    free(element);

    backlog->elements = backlog->elements - 1;

    return backlog->elements;
}

static bool backlogDel(vpBacklog *backlog) {
    while(backlog->elements > 0) {
        if(backlogDelElement(backlog, backlog->entry) < 0) {
            qDebug("Error deleting element at %p from the backlog. %d elements remain in the list.",
                backlog->entry, backlog->elements);
            return false;
        }
    }

    free(backlog);

    return true;
}


static int32_t vpListDelElement( vpList *list,  vpListElement *element) {
    if(element->next == element) {
        // This is the only element in the list
        list->entry = NULL;
    } else {
        element->next->previous = element->previous;
        element->previous->next = element->next;
        list->entry = element->next;
    }

    if(backlogDel(element->backlog) == false) {
        qDebug("Error deleting backlog at %p of vpList element at %p.",
               element->backlog, element);
        return FAIL;
    }
    free(element);

    list->elements = list->elements - 1;

    return list->elements;
}



static bool vpListDel(vpList *list) {
    while(list->elements > 0) {
        if(vpListDelElement(list, list->entry) < 0) {
            qDebug("Error deleting element at %p from the slot list %d elements remain in the list.",
                   list->entry, list->elements);
            return false;
        }
    }

    free(list);
    return true;
}

static int32_t backlogAddElement(vpBacklog *backlog, Coordinate datacube, uint32_t dcOffset,
                                 Byte *slice, uint32_t x_px, uint32_t y_px, uint32_t cubeType) {
    vpBacklogElement *newElement;

    newElement = (vpBacklogElement *) malloc(sizeof( vpBacklogElement));
    if(newElement == NULL) {
        qDebug("Out of memory.");
        /* Do not return false here. That's a bug. FAIL is hackish... Is there a better way? */
        return FAIL;
    }

    newElement->slice = slice;
    SET_COORDINATE(newElement->cube, datacube.x, datacube.y, datacube.z);
    newElement->x_px = x_px;
    newElement->y_px = y_px;
    newElement->dcOffset = dcOffset;
    newElement->cubeType = cubeType;

    if(backlog->entry != NULL) {
        backlog->entry->next->previous = newElement;
        newElement->next = backlog->entry->next;
        backlog->entry->next = newElement;
        newElement->previous = backlog->entry;
    }
    else {
        backlog->entry = newElement;
        backlog->entry->next = newElement;
        backlog->entry->previous = newElement;
    }
    backlog->elements = backlog->elements + 1;
    return backlog->elements;
}

static bool sliceExtract_standard(Byte *datacube,
                                       Byte *slice,
                                       vpConfig *vpConfig) {
    int32_t i, j;

    switch(vpConfig->type) {
    case SLICE_XY:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = slice[1]
                     = slice[2]
                     = *datacube;

            datacube++;
            slice += 3;
        }
        break;
    case SLICE_XZ:
        for(j = 0; j < state->cubeEdgeLength; j++) {
            for(i = 0; i < state->cubeEdgeLength; i++) {
                slice[0] = slice[1]
                         = slice[2]
                         = *datacube;

                datacube++;
                slice += 3;
            }
            datacube = datacube
                       + state->cubeSliceArea
                       - state->cubeEdgeLength;
        }
        break;
    case SLICE_YZ:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = slice[1]
                     = slice[2]
                     = *datacube;
            datacube += state->cubeEdgeLength;
            slice += 3;
        }
        break;
    }
    return true;
}


static bool sliceExtract_adjust(Byte *datacube,
                                     Byte *slice,
                                     vpConfig *vpConfig) {
    int32_t i, j;

    switch(vpConfig->type) {
    case SLICE_XY:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = state->viewerState->datasetAdjustmentTable[0][*datacube];
            slice[1] = state->viewerState->datasetAdjustmentTable[1][*datacube];
            slice[2] = state->viewerState->datasetAdjustmentTable[2][*datacube];

            datacube++;
            slice += 3;
        }
        break;
    case SLICE_XZ:
        for(j = 0; j < state->cubeEdgeLength; j++) {
            for(i = 0; i < state->cubeEdgeLength; i++) {
                slice[0] = state->viewerState->datasetAdjustmentTable[0][*datacube];
                slice[1] = state->viewerState->datasetAdjustmentTable[1][*datacube];
                slice[2] = state->viewerState->datasetAdjustmentTable[2][*datacube];

                datacube++;
                slice += 3;
            }

            datacube  = datacube
                        + state->cubeSliceArea
                        - state->cubeEdgeLength;
        }
        break;
    case SLICE_YZ:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = state->viewerState->datasetAdjustmentTable[0][*datacube];
            slice[1] = state->viewerState->datasetAdjustmentTable[1][*datacube];
            slice[2] = state->viewerState->datasetAdjustmentTable[2][*datacube];

            datacube += state->cubeEdgeLength;
            slice += 3;
        }
        break;
    }
    return true;
}


static bool dcSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    datacube += dcOffset;

    if(state->viewerState->datasetAdjustmentOn) {
        /* Texture type GL_RGB and we need to adjust coloring */
        sliceExtract_adjust(datacube, slice, vpConfig);
    }
    else {
        /* Texture type GL_RGB and we don't need to adjust anything*/
        sliceExtract_standard(datacube, slice, vpConfig);
    }
    return true;
}

static bool ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    int32_t i, j;
    int32_t objId, *objIdP;

    objIdP = &objId;
    datacube += dcOffset;

    switch(vpConfig->type) {
    case SLICE_XY:
        for(i = 0; i < state->cubeSliceArea; i++) {
            memcpy(objIdP, datacube, OBJID_BYTES);
            slice[0] = state->viewerState->overlayColorMap[0][objId % 256];
            slice[1] = state->viewerState->overlayColorMap[1][objId % 256];
            slice[2] = state->viewerState->overlayColorMap[2][objId % 256];
            slice[3] = state->viewerState->overlayColorMap[3][objId % 256];

            //printf("(%d, %d, %d, %d)", slice[0], slice[1], slice[2], slice[3]);

            datacube += OBJID_BYTES;
            slice += 4;
        }
        break;
    case SLICE_XZ:
        for(j = 0; j < state->cubeEdgeLength; j++) {
            for(i = 0; i < state->cubeEdgeLength; i++) {
                memcpy(objIdP, datacube, OBJID_BYTES);
                slice[0] = state->viewerState->overlayColorMap[0][objId % 256];
                slice[1] = state->viewerState->overlayColorMap[1][objId % 256];
                slice[2] = state->viewerState->overlayColorMap[2][objId % 256];
                slice[3] = state->viewerState->overlayColorMap[3][objId % 256];

                datacube += OBJID_BYTES;
                slice += 4;
            }

            datacube = datacube
                       + state->cubeSliceArea * OBJID_BYTES
                       - state->cubeEdgeLength * OBJID_BYTES;
        }
        break;
    case SLICE_YZ:
        for(i = 0; i < state->cubeSliceArea; i++) {
            memcpy(objIdP, datacube, OBJID_BYTES);
            slice[0] = state->viewerState->overlayColorMap[0][objId % 256];
            slice[1] = state->viewerState->overlayColorMap[1][objId % 256];
            slice[2] = state->viewerState->overlayColorMap[2][objId % 256];
            slice[3] = state->viewerState->overlayColorMap[3][objId % 256];

            datacube += state->cubeEdgeLength * OBJID_BYTES;
            slice += 4;
        }

        break;
    }
    return true;
}

static bool vpHandleBacklog(vpListElement *currentVp, viewerState *viewerState) {
    vpBacklogElement *currentElement = NULL,
                     *nextElement = NULL;
    Byte *cube = NULL;
    uint32_t elements = 0,
              i = 0;

    if(currentVp->backlog->entry == NULL) {
        qDebug("Called vpHandleBacklog, but there is no backlog.");
        return false;
    }

    elements = currentVp->backlog->elements;
    currentElement = currentVp->backlog->entry;
    qDebug() << "starting to handle backlog";
    for(i = 0; i < elements; i++)  {
        nextElement = currentElement->next;

        if(currentElement->cubeType == CUBE_DATA) {
            state->protectCube2Pointer->lock();
            qDebug() << currentElement->cube.x << " " << currentElement->cube.y << " " << currentElement->cube.z;
            cube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentElement->cube);
            state->protectCube2Pointer->unlock();

            if(cube == HT_FAILURE) {
                //qDebug() << "failed to get cube in backlog";
                // if(currentElement->cube.x >= 3) {
                       //LOG("handleBL: currentDc %d, %d, %d", currentElement->cube.x, currentElement->cube.y, currentElement->cube.z);
                 //}
                 //LOG("failed to get cube in viewer");
            } else {
                cubes_in_backlog += 1;
                qDebug() << "vpHandleBacklog" << cubes_in_backlog;
                dcSliceExtract(cube,
                               currentElement->slice,
                               currentElement->dcOffset,
                               currentVp->vpConfig);

                glBindTexture(GL_TEXTURE_2D, currentVp->vpConfig->texture.texHandle);
                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                currentElement->x_px,
                                currentElement->y_px,
                                state->cubeEdgeLength,
                                state->cubeEdgeLength,
                                GL_RGB,
                                GL_UNSIGNED_BYTE,
                                currentElement->slice);
                glBindTexture(GL_TEXTURE_2D, 0);
                backlogDelElement(currentVp->backlog, currentElement);
            }
        }
        else if(currentElement->cubeType == CUBE_OVERLAY) {

            state->protectCube2Pointer->lock();
            cube = Hashtable::ht_get((Hashtable*) state->Oc2Pointer, currentElement->cube);
            state->protectCube2Pointer->unlock();

            if(cube == HT_FAILURE) {

            }
            else {
                ocSliceExtract(cube,
                               currentElement->slice,
                               currentElement->dcOffset,
                               currentVp->vpConfig);

                glBindTexture(GL_TEXTURE_2D, currentVp->vpConfig->texture.overlayHandle);
                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                currentElement->x_px,
                                currentElement->y_px,
                                state->cubeEdgeLength,
                                state->cubeEdgeLength,
                                GL_RGBA,
                                GL_UNSIGNED_BYTE,
                                currentElement->slice);
                glBindTexture(GL_TEXTURE_2D, 0);
                backlogDelElement(currentVp->backlog, currentElement);
            }
        }

        currentElement = nextElement;
    }

    if(currentVp->backlog->elements != 0) {
        return false;
    }
    else {
        return true;
    }
}

static int32_t texIndex(uint32_t x,
                        uint32_t y,
                        uint32_t colorMultiplicationFactor,
                        viewportTexture *texture) {
    uint32_t index = 0;

    index = x * state->cubeSliceArea + y
            * (texture->edgeLengthDc * state->cubeSliceArea)
            * colorMultiplicationFactor;

    return index;
}

static bool vpGenerateTexture(vpListElement *currentVp, viewerState *viewerState) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.

    uint32_t x_px = 0, x_dc = 0, y_px = 0, y_dc = 0;
    Coordinate upperLeftDc, currentDc, currentPosition_dc;
    Coordinate currPosTrans, leftUpperPxInAbsPxTrans;

    Byte *datacube = NULL, *overlayCube = NULL;
    int32_t dcOffset = 0, index = 0;

    CPY_COORDINATE(currPosTrans, viewerState->currentPosition);
    DIV_COORDINATE(currPosTrans, state->magnification);

    CPY_COORDINATE(leftUpperPxInAbsPxTrans, currentVp->vpConfig->texture.leftUpperPxInAbsPx);
    DIV_COORDINATE(leftUpperPxInAbsPxTrans, state->magnification);

    currentPosition_dc = Coordinate::Px2DcCoord(currPosTrans);

    upperLeftDc = Coordinate::Px2DcCoord(leftUpperPxInAbsPxTrans);

    // We calculate the coordinate of the DC that holds the slice that makes up the upper left
    // corner of our texture.
    // dcOffset is the offset by which we can index into a datacube to extract the first byte of
    // slice relevant to the texture for this viewport.
    //
    // Rounding should be explicit!
    switch(currentVp->vpConfig->type) {
    case SLICE_XY:
        dcOffset = state->cubeSliceArea
                   //* (viewerState->currentPosition.z - state->cubeEdgeLength
                   * (currPosTrans.z - state->cubeEdgeLength
                   * currentPosition_dc.z);
        break;
    case SLICE_XZ:
        dcOffset = state->cubeEdgeLength
                   * (currPosTrans.y  - state->cubeEdgeLength
                   //     * (viewerState->currentPosition.y  - state->cubeEdgeLength
                   * currentPosition_dc.y);
        break;
    case SLICE_YZ:
        dcOffset = //viewerState->currentPosition.x - state->cubeEdgeLength
                   currPosTrans.x - state->cubeEdgeLength
                   * currentPosition_dc.x;
        break;
    default:
        qDebug("No such slice view: %d.", currentVp->vpConfig->type);
        return false;
    }
    // We iterate over the texture with x and y being in a temporary coordinate
    // system local to this texture.
    for(x_dc = 0; x_dc < currentVp->vpConfig->texture.usedTexLengthDc; x_dc++) {
        for(y_dc = 0; y_dc < currentVp->vpConfig->texture.usedTexLengthDc; y_dc++) {
            x_px = x_dc * state->cubeEdgeLength;
            y_px = y_dc * state->cubeEdgeLength;

            switch(currentVp->vpConfig->type) {
            // With an x/y-coordinate system in a viewport, we get the following
            // mapping from viewport (slice) coordinates to global (dc)
            // coordinates:
            // XY-slice: x local is x global, y local is y global
            // XZ-slice: x local is x global, y local is z global
            // YZ-slice: x local is y global, y local is z global.
            case SLICE_XY:
                SET_COORDINATE(currentDc,
                               upperLeftDc.x + x_dc,
                               upperLeftDc.y + y_dc,
                               upperLeftDc.z);
                break;
            case SLICE_XZ:
                SET_COORDINATE(currentDc,
                               upperLeftDc.x + x_dc,
                               upperLeftDc.y,
                               upperLeftDc.z + y_dc);
                break;
            case SLICE_YZ:
                SET_COORDINATE(currentDc,
                               upperLeftDc.x,
                               upperLeftDc.y + x_dc,
                               upperLeftDc.z + y_dc);
                break;
            default:
                LOG("No such slice type (%d) in vpGenerateTexture.", currentVp->vpConfig->type);
            }

            state->protectCube2Pointer->lock();
            datacube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
            overlayCube = Hashtable::ht_get(state->Oc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
            state->protectCube2Pointer->unlock();


            // Take care of the data textures.

            glBindTexture(GL_TEXTURE_2D,
                          currentVp->vpConfig->texture.texHandle);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // This is used to index into the texture. texData[index] is the first
            // byte of the datacube slice at position (x_dc, y_dc) in the texture.
            index = texIndex(x_dc, y_dc, 3, &(currentVp->vpConfig->texture));

            if(datacube == HT_FAILURE) {
                backlogAddElement(currentVp->backlog,
                                  currentDc,
                                  dcOffset,
                                  &(viewerState->texData[index]),
                                  x_px,
                                  y_px,
                                  CUBE_DATA);

                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                x_px,
                                y_px,
                                state->cubeEdgeLength,
                                state->cubeEdgeLength,
                                GL_RGB,
                                GL_UNSIGNED_BYTE,
                                viewerState->defaultTexData);
            } else {
                correct_cubes += 1;
                qDebug() << "vpGenerateTexture" <<correct_cubes;
                dcSliceExtract(datacube,
                               &(viewerState->texData[index]),
                               dcOffset,
                               currentVp->vpConfig);
                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                x_px,
                                y_px,
                                state->cubeEdgeLength,
                                state->cubeEdgeLength,
                                GL_RGB,
                                GL_UNSIGNED_BYTE,
                                &(viewerState->texData[index]));
            }

            //Take care of the overlay textures.
            if(state->overlay) {
                glBindTexture(GL_TEXTURE_2D,
                              currentVp->vpConfig->texture.overlayHandle);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                // This is used to index into the texture. texData[index] is the first
                // byte of the datacube slice at position (x_dc, y_dc) in the texture.
                index = texIndex(x_dc, y_dc, 4, &(currentVp->vpConfig->texture));

                if(overlayCube == HT_FAILURE) {
                    backlogAddElement(currentVp->backlog,
                                      currentDc,
                                      dcOffset * OBJID_BYTES,
                                      &(viewerState->overlayData[index]),
                                      x_px,
                                      y_px,
                                      CUBE_OVERLAY);

                    glTexSubImage2D(GL_TEXTURE_2D,
                                    0,
                                    x_px,
                                    y_px,
                                    state->cubeEdgeLength,
                                    state->cubeEdgeLength,
                                    GL_RGBA,
                                    GL_UNSIGNED_BYTE,
                                    viewerState->defaultOverlayData);
                }
                else {
                    ocSliceExtract(overlayCube,
                                   &(viewerState->overlayData[index]),
                                   dcOffset * OBJID_BYTES,
                                   currentVp->vpConfig);

                    glTexSubImage2D(GL_TEXTURE_2D,
                                    0,
                                    x_px,
                                    y_px,
                                    state->cubeEdgeLength,
                                    state->cubeEdgeLength,
                                    GL_RGBA,
                                    GL_UNSIGNED_BYTE,
                                    &(viewerState->overlayData[index]));
                }
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

 /* For downsample & upsamleVPTexture:
  * we read the texture to a CPU side - buffer,
  * and send it to graphicscard after the resampling. Using
  * OpenGL is certainly possible for the resampling
  * but the CPU implementation appears to be
  * more straightforward, with probably almost no
  * performance penalty. We use a simple
  * box filter for the downsampling */

static bool downsampleVPTexture(vpConfig *vpConfig) {
    /* allocate 2 texture buffers */
    //Byte *orig, *resampled;

    /* get the texture */

    /* downsample it */

    /* send it to the graphicscard again */

    return true;
}


static bool upsampleVPTexture(vpConfig *vpConfig) {
   return true;
}

/* this function calculates the mapping between the left upper texture pixel
 * and the real dataset pixel */
static bool calcLeftUpperTexAbsPx() {
    uint32_t i = 0;
    Coordinate currentPosition_dc, currPosTrans;
    viewerState *viewerState = state->viewerState;

    /* why div first by mag and then multiply again with it?? */
    CPY_COORDINATE(currPosTrans, viewerState->currentPosition);
    DIV_COORDINATE(currPosTrans, state->magnification);

    currentPosition_dc = Coordinate::Px2DcCoord(currPosTrans);

    //iterate over all viewports
    //this function has to be called after the texture changed or the user moved, in the sense of a
    //realignment of the data
    for (i = 0; i < viewerState->numberViewports; i++) {
        switch (viewerState->vpConfigs[i].type) {
        case VIEWPORT_XY:
            //Set the coordinate of left upper data pixel currently stored in the texture
            //viewerState->vpConfigs[i].texture.usedTexLengthDc is state->M and even int.. very funny
            // this guy is always in mag1, even if a different mag dataset is active!!!
            // this might be buggy for very high mags, test that!
            SET_COORDINATE(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx,
                           (currentPosition_dc.x - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           (currentPosition_dc.y - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           currentPosition_dc.z
                           * state->cubeEdgeLength * state->magnification);
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x >1000000){ LOG("uninit x %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x);}
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y > 1000000){ LOG("uninit y %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y);}
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z > 1000000){ LOG("uninit z %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z);}

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen,
                           viewerState->currentPosition.x
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.y -
                           (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.z);
            break;
        case VIEWPORT_XZ:
            //Set the coordinate of left upper data pixel currently stored in the texture
            SET_COORDINATE(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx,
                           (currentPosition_dc.x - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           currentPosition_dc.y * state->cubeEdgeLength  * state->magnification,
                           (currentPosition_dc.z - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification);

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen,
                           viewerState->currentPosition.x
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.y ,
                           viewerState->currentPosition.z
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx));
            break;
        case VIEWPORT_YZ:
            //Set the coordinate of left upper data pixel currently stored in the texture
            SET_COORDINATE(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx,
                           currentPosition_dc.x * state->cubeEdgeLength   * state->magnification,
                           (currentPosition_dc.y - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           (currentPosition_dc.z - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification);

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen,
                           viewerState->currentPosition.x ,
                           viewerState->currentPosition.y
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.z
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx));
            break;
        default:
            viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x = 0;
            viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y = 0;
            viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z = 0;
        }
    }
    return false;
}

/**
  * Initializes the viewer, is called only once after the viewing thread started
  *
  */
bool Viewer::initViewer() {
    qDebug() << "Viewer: initViewer begin";
    calcLeftUpperTexAbsPx();


    // init the skeletonizer
    if(Skeletonizer::initSkeletonizer() == false) {
        LOG("Error initializing the skeletonizer.");
        return false;
    }

    // init the gui
    /*
    if(MainWindow::initGUI() == false) {
        LOG("Error initializing the gui system");
        return false;
    }
    */

    // TDitem

    // Load the color map for the overlay

    if(state->overlay) {
        LOG("overlayColorMap at %p\n", &(state->viewerState->overlayColorMap[0][0]));
        if(Viewer::loadDatasetColorTable("stdOverlay.lut",
                          &(state->viewerState->overlayColorMap[0][0]),
                          GL_RGBA) == false) {
            LOG("Overlay color map stdOverlay.lut does not exist.");
            state->overlay = false;
        }
    }

    // This is the buffer that holds the actual texture data (for _all_ textures)

    state->viewerState->texData =
            (Byte *) malloc(TEXTURE_EDGE_LEN
               * TEXTURE_EDGE_LEN
               * sizeof(Byte)
               * 3);
    if(state->viewerState->texData == NULL) {
        LOG("Out of memory.");
        _Exit(false);
    }
    memset(state->viewerState->texData, '\0',
           TEXTURE_EDGE_LEN
           * TEXTURE_EDGE_LEN
           * sizeof(Byte)
           * 3);

    // This is the buffer that holds the actual overlay texture data (for _all_ textures)

    if(state->overlay) {
        state->viewerState->overlayData =
                (Byte *) malloc(TEXTURE_EDGE_LEN *
                   TEXTURE_EDGE_LEN *
                   sizeof(Byte) *
                   4);
        if(state->viewerState->overlayData == NULL) {
            LOG("Out of memory.");
            _Exit(false);
        }
        memset(state->viewerState->overlayData, '\0',
               TEXTURE_EDGE_LEN
               * TEXTURE_EDGE_LEN
               * sizeof(Byte)
               * 4);
    }

    // This is the data we use when the data for the
    //   slices is not yet available (hasn't yet been loaded).

    state->viewerState->defaultTexData = (Byte *) malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                         * sizeof(Byte)
                                                         * 3);
    if(state->viewerState->defaultTexData == NULL) {
        LOG("Out of memory.");
        _Exit(false);
    }
    memset(state->viewerState->defaultTexData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                     * sizeof(Byte)
                                                     * 3);

    // Default data for the overlays
    if(state->overlay) {
        state->viewerState->defaultOverlayData = (Byte *) malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                                 * sizeof(Byte)
                                                                 * 4);
        if(state->viewerState->defaultOverlayData == NULL) {
            LOG("Out of memory.");
            _Exit(false);
        }
        memset(state->viewerState->defaultOverlayData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                             * sizeof(Byte)
                                                             * 4);
    }

    /* temporarily moved to the run() Method */
    /*
    Viewer::initializeTextures();
    // init the rendering system

    if(Renderer::initRenderer() == false) {
        qDebug("Error initializing the rendering system.");
        return false;
    }

    SET_COORDINATE(state->viewerState->currentPosition, 830, 1000, 830)
    //viewerEventObj->sendLoadSignal(830, 1000, 830, NO_MAG_CHANGE);

    viewerEventObj->sendLoadSignal(state->viewerState->currentPosition.x,
                   state->viewerState->currentPosition.y,
                   state->viewerState->currentPosition.z,
                   NO_MAG_CHANGE);

    qDebug() << "Viewer: initViewer completed";
    */
    return true;
}



// from knossos-global.h
bool Viewer::loadDatasetColorTable(const char *path, GLuint *table, int32_t type) {
    FILE *lutFile = NULL;
    uint8_t lutBuffer[RGBA_LUTSIZE];
    uint32_t readBytes = 0, i = 0;
    uint32_t size = RGB_LUTSIZE;

    // The b is for compatibility with non-UNIX systems and denotes a
    // binary file.
    LOG("Reading Dataset LUT at %s\n", path);

    lutFile = fopen(path, "rb");
    if(lutFile == NULL) {
        LOG("Unable to open Dataset LUT at %s.", path);
        return false;
    }

    if(type == GL_RGB) {
        size = RGB_LUTSIZE;
    }
    else if(type == GL_RGBA) {
        size = RGBA_LUTSIZE;
    }
    else {
        LOG("Requested color type %x does not exist.", type);
        return false;
    }

    readBytes = (uint32_t)fread(lutBuffer, 1, size, lutFile);
    if(readBytes != size) {
        LOG("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, path, size);
        if(fclose(lutFile) != 0) {
            LOG("Additionally, an error occured closing the file.");
        }
        return false;
    }

    if(fclose(lutFile) != 0) {
        LOG("Error closing LUT file.");
        return false;
    }

    if(type == GL_RGB) {
        for(i = 0; i < 256; i++) {
            table[0 * 256 + i] = (GLuint)lutBuffer[i];
            table[1 * 256 + i] = (GLuint)lutBuffer[i + 256];
            table[2 * 256 + i] = (GLuint)lutBuffer[i + 512];
        }
    }
    else if(type == GL_RGBA) {
        for(i = 0; i < 256; i++) {
            table[0 * 256 + i] = (GLuint)lutBuffer[i];
            table[1 * 256 + i] = (GLuint)lutBuffer[i + 256];
            table[2 * 256 + i] = (GLuint)lutBuffer[i + 512];
            table[3 * 256 + i] = (GLuint)lutBuffer[i + 768];
        }
    }
    return true;
}

bool Viewer::loadTreeColorTable(const char *path, float *table, int32_t type) {
    FILE *lutFile = NULL;
    uint8_t lutBuffer[RGB_LUTSIZE];
    uint32_t readBytes = 0, i = 0;
    uint32_t size = RGB_LUTSIZE;

    // The b is for compatibility with non-UNIX systems and denotes a
    // binary file.
    LOG("Reading Tree LUT at %s\n", path);

    lutFile = fopen(path, "rb");
    if(lutFile == NULL) {
        LOG("Unable to open Tree LUT at %s.", path);
        return false;
    }
    if(type != GL_RGB) {
        /* AG_TextError("Tree colors only support RGB colors. Your color type is: %x", type); */
        LOG("Chosen color was of type %x, but expected GL_RGB", type);
        return false;
    }

    readBytes = (uint32_t)fread(lutBuffer, 1, size, lutFile);
    if(readBytes != size) {
        LOG("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, path, size);
        if(fclose(lutFile) != 0) {
            LOG("Additionally, an error occured closing the file.");
        }
        return false;
    }
    if(fclose(lutFile) != 0) {
        LOG("Error closing LUT file.");
        return false;
    }

    //Get RGB-Values in percent
    for(i = 0; i < 256; i++) {
        table[i]   = lutBuffer[i]/MAX_COLORVAL;
        table[i + 256] = lutBuffer[i+256]/MAX_COLORVAL;
        table[i + 512] = lutBuffer[i + 512]/MAX_COLORVAL;
    }

    MainWindow::treeColorAdjustmentsChanged();
    return true;
}

bool Viewer::updatePosition(int32_t serverMovement) {
    Coordinate jump;

    if(COMPARE_COORDINATE(tempConfig->viewerState->currentPosition, state->viewerState->currentPosition) != true) {
        jump.x = tempConfig->viewerState->currentPosition.x - state->viewerState->currentPosition.x;
        jump.y = tempConfig->viewerState->currentPosition.y - state->viewerState->currentPosition.y;
        jump.z = tempConfig->viewerState->currentPosition.z - state->viewerState->currentPosition.z;
        userMove(jump.x, jump.y, jump.z, serverMovement);
    }
    return true;
}

bool Viewer::calcDisplayedEdgeLength() {
    uint32_t i;
    float FOVinDCs;

    FOVinDCs = ((float)state->M) - 1.f;

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
        state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
            FOVinDCs * (float)state->cubeEdgeLength
            / (float) tempConfig->viewerState->vpConfigs[i].texture.edgeLengthPx;
    }
    return true;
}


/**
* takes care of all necessary changes inside the viewer and signals
* the loader to change the dataset
*/
/* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
bool Viewer::changeDatasetMag(uint32_t upOrDownFlag) {
    uint32_t i;

    if(state->viewerState->datasetMagLock) {
        return false;
    }

    switch(upOrDownFlag) {
    case MAG_DOWN:
        if(state->magnification > state->lowestAvailableMag) {
            state->magnification /= 2;
            for(i = 0; i < state->viewerState->numberViewports; i++) {
                if(state->viewerState->vpConfigs[i].type != (uint32_t)VIEWPORT_SKELETON) {
                    state->viewerState->vpConfigs[i].texture.zoomLevel *= 2.0;
                    upsampleVPTexture(&state->viewerState->vpConfigs[i]);
                    state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx *= 2.;
                }
            }
        }
        else return false;
        break;

    case MAG_UP:
        if(state->magnification < state->highestAvailableMag) {
            state->magnification *= 2;
            for(i = 0; i < state->viewerState->numberViewports; i++) {
                if(state->viewerState->vpConfigs[i].type != (uint32_t)VIEWPORT_SKELETON) {
                    state->viewerState->vpConfigs[i].texture.zoomLevel *= 0.5;
                    downsampleVPTexture(&state->viewerState->vpConfigs[i]);
                    state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx /= 2.;
                }
            }
        }
        else return false;
        break;
    }

    /* necessary? */
    state->viewerState->userMove = true;
    recalcTextureOffsets();

    /*for(i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != VIEWPORT_SKELETON) {
            LOG("left upper tex px of VP %d is: %d, %d, %d",i,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z);
        }
    }*/
    sendLoadSignal(state->viewerState->currentPosition.x,
                            state->viewerState->currentPosition.y,
                            state->viewerState->currentPosition.z,
                            upOrDownFlag);
    //refreshViewports();
    /* set flags to trigger the necessary renderer updates */
    //state->skeletonState->skeletonChanged = true;

    return true;
}

/**
  * @brief This method is used for the Viewer-Thread declared in main(knossos.cpp)
  * This is the old viewer() function from KNOSSOS 3.2
  * It works as follows:
  * - The viewer get initialized
  * - The rendering loop starts:
  *   - lists of pending texture parts are iterated and loaded if they are available
  *   - If not they are added to a backlog which is processed at a later time.
  *   - TODO: The Eventhandling in QT works asnyc, new concept are currently in progress
  * - the loadSignal occurs in three different locations:
  *   - initViewer
  *   - changeDatasetMag
  *   - userMove
  *
  */
//Entry point for viewer thread, general viewer coordination, "main loop"
void Viewer::run() {

    // Event and rendering loop.
    // What happens is that we go through lists of pending texture parts and load
    // them if they are available. If they aren't, they are added to a backlog
    // which is processed at a later time.
    // While we are loading the textures, we check for events. Some events
    // might cancel the current loading process. When all textures / backlogs
    // have been processed, we go into an idle state, in which we wait for events.
    qDebug() << "Viewer: start begin";
    struct viewerState *viewerState = state->viewerState;
    struct vpList *viewports = NULL;
    struct vpListElement *currentVp = NULL, *nextVp = NULL;
    uint32_t drawCounter = 0;

    state->viewerState->viewerReady = true;

    updateViewerState();
    recalcTextureOffsets();

    // Display info about skeleton save path here TODO
    // This creates a circular doubly linked list of
        // pending viewports (viewports for which the texture has not yet been
        // completely loaded) from the viewport-array in the viewerState
        // structure.
        // The idea is that we can easily remove the element representing a
        // pending viewport once its texture is completely loaded.
        viewports = vpListGenerate(viewerState);
        drawCounter = 0;

        currentVp = viewports->entry;

        while(viewports->elements > 0) {
            if(drawCounter == 0)
                vp->makeCurrent();
            else if(drawCounter == 1)
                vp2->makeCurrent();
            else if(drawCounter == 2)
                vp3->makeCurrent();


            nextVp = currentVp->next;
            // printf("currentVp at %p, nextVp at %p.\n", currentVp, nextVp);

            // We iterate over the list and either handle the backlog (a list
            // of datacubes and associated offsets, see headers) if there is
            // one or start loading everything from scratch if there is none.

            if(currentVp->vpConfig->type != VIEWPORT_SKELETON) {


                if(currentVp->backlog->elements == 0) {
                    // There is no backlog. That means we haven't yet attempted
                    // to load the texture for this viewport, which is what we
                    // do now. If we can't complete the texture because a Dc
                    // is missing, a backlog is generated.
                    vpGenerateTexture(currentVp, viewerState);
                }
                else {
                    // There is a backlog. We go through its elements
                    vpHandleBacklog(currentVp, viewerState);
                }

                if(currentVp->backlog->elements == 0) {
                    // There is no backlog after either handling the backlog
                    // or loading the whole texture. That means the texture is
                    // complete. We can remove the viewport/ from the list.

                    //  XXX TODO XXX
                    //  The Dc2Pointer hashtable locking is currently done at pretty high
                    //  frequency by vpHandleBacklog() and might slow down the
                    //  loader.
                    //  We might want to introduce a locked variable that says how many
                    //  yet "unused" (by the viewer) cubes the loader has provided.
                    //  Unfortunately, we can't non-busy wait on the loader _and_
                    //  events, unless the loader generates events itself... So if this
                    //  really is a bottleneck it might be worth to think about it
                    //  again.
                    vpListDelElement(viewports, currentVp);
                }
            }

            drawCounter++;
            if(drawCounter == 3) {
                drawCounter = 0;

                updateViewerState();
                recalcTextureOffsets();
                Skeletonizer::updateSkeletonState();
                Renderer::drawGUI();

                // TODO Crashes because of SDL
                //while(SDL_PollEvent(&event)) {
                //   if(EventModel::handleEvent(event) == false) {
                //       state->viewerState->viewerReady = false;
                //       //return true;
                //   }
                //}

                if(viewerState->userMove == true) {
                    break;
                }
            }

            // An incoming user movement event makes the current backlog &
            // viewport lists obsolete and we regenerate them dependant on
            // the new position

            // Leaves the loop that checks regularily for new available
            // texture parts because the whole texture has to be recreated
            // if the users moves

            currentVp = nextVp;

            emit now();
            emit now2();
            emit now3();

        }//end while(viewports->elements > 0)
        vpListDel(viewports);


        emit now4();
        //TODO Crashes because of SDL
        //if(viewerState->userMove == false) {
        //    if(SDL_WaitEvent(&event)) {
        //        if(EventModel::handleEvent(event) != true) {
        //            state->viewerState->viewerReady = false;
        //            //return true;
        //        }
        //    }
        //}
        viewerState->userMove = false;



    //QThread::currentThread()->quit();
    //emit finished();


    qDebug() << "Viewer: start ended";



}

//Initializes the window with the parameter given in viewerState
bool Viewer::createScreen() {
 // TODO what about all that outcommented code ???
 // initialize window
 //SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
 //SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
 //SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );

 //SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    if(state->viewerState->multisamplingOnOff) {
        //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    }
  //else { SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0); }

  /*
   At least on linux, the working directory is the directory from which
   knossos was called. So 'icon' will or will not be found depending on the
   directory from which knossos was started.
  */



  /*state->viewerState->screen = SDL_SetVideoMode(state->viewerState->screenSizeX,
                                 state->viewerState->screenSizeY, 24,
                                 SDL_OPENGL  | SDL_RESIZABLE);*/

  /*if(state->viewerState->screen == NULL) {
        printf("Unable to create screen: %s\n", SDL_GetError());
        return false;
    }*/

  //set clear color (background) and clear with it
    return true;
}

/**
*
* Transfers all (orthogonal viewports) textures completly from ram (*viewerState->vpConfigs[i].texture.data) to video memory
* @attention Calling makes only sense after full initialization of the OGL screen
* The functionality is moved into the initializeGL respectively initializeOverlayGL method of the
* GLWidgets
*/
bool Viewer::initializeTextures() {
    uint32_t i = 0;

    /*problem of deleting textures when calling again after resize?! TDitem */
    for(int i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != VIEWPORT_SKELETON) {
            //vp->makeCurrent();
            //state->viewerState->vpConfigs[i].displayList = glGenLists(1);
            //vp->context()->makeCurrent();

            glGenTextures(1, &state->viewerState->vpConfigs[i].texture.texHandle);
            if(state->overlay) {
                glGenTextures(1, &state->viewerState->vpConfigs[i].texture.overlayHandle);
            }
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type == VIEWPORT_SKELETON) {
            continue;
        }

        // Handle data textures.

        glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.texHandle);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        // loads an empty texture into video memory - during user movement, this
        // texture is updated via glTexSubImage2D in vpGenerateTexture & vpHandleBacklog
        // We need GL_RGB as texture internal format to color the textures

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     state->viewerState->vpConfigs[i].texture.edgeLengthPx,
                     state->viewerState->vpConfigs[i].texture.edgeLengthPx,
                     0,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     state->viewerState->defaultTexData);

        //Handle overlay textures.

        if(state->overlay) {
            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.overlayHandle);

            //Set the parameters for the texture.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         state->viewerState->vpConfigs[i].texture.edgeLengthPx,
                         state->viewerState->vpConfigs[i].texture.edgeLengthPx,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         state->viewerState->defaultOverlayData);
        }
    }
    return true;
}

//Frees allocated memory
bool Viewer::cleanUpViewer( viewerState *viewerState) {
/*
    int i;
    for(i = 0; i < viewerState->numberViewports; i++) {
        free(viewerState->vpConfigs[i].texture.data);
        free(viewerState->vpConfigs[i].texture.empty);
    }
    free(viewerState->vpConfigs);
*/
    return true;
}

bool Viewer::updateViewerState() {

    uint32_t i;

    /*if(!(state->viewerState->currentPosition.x == (tempConfig->viewerState->currentPosition.x - 1))) {
        state->viewerState->currentPosition.x = tempConfig->viewerState->currentPosition.x - 1;
    }
    if(!(state->viewerState->currentPosition.y == (tempConfig->viewerState->currentPosition.y - 1))) {
        state->viewerState->currentPosition.y = tempConfig->viewerState->currentPosition.y - 1;
    }
    if(!(state->viewerState->currentPosition.z == (tempConfig->viewerState->currentPosition.z - 1))) {
        state->viewerState->currentPosition.z = tempConfig->viewerState->currentPosition.z - 1;
    }*/

    if(state->viewerState->filterType != tempConfig->viewerState->filterType) {
        state->viewerState->filterType = tempConfig->viewerState->filterType;
        for(i = 0; i < state->viewerState->numberViewports; i++) {
            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.texHandle);
            // Set the parameters for the texture.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);
            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.overlayHandle);
            // Set the parameters for the texture.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    updateZoomCube();

    if(state->viewerState->workMode != tempConfig->viewerState->workMode) {
        state->viewerState->workMode = tempConfig->viewerState->workMode;
    }
    if(state->viewerState->dropFrames != tempConfig->viewerState->dropFrames) {
        state->viewerState->dropFrames = tempConfig->viewerState->dropFrames;
    }
    if(state->viewerState->stepsPerSec != tempConfig->viewerState->stepsPerSec) {
        state->viewerState->stepsPerSec = tempConfig->viewerState->stepsPerSec;

        //if(SDL_EnableKeyRepeat(200, (1000 / state->viewerState->stepsPerSec)) == FAIL) TODO Crashed
        //    LOG("Error setting key repeat parameters.");
    }

    if(state->viewerState->recenteringTime != tempConfig->viewerState->recenteringTime) {
        state->viewerState->recenteringTime = tempConfig->viewerState->recenteringTime;
    }
    if(state->viewerState->recenteringTimeOrth != tempConfig->viewerState->recenteringTimeOrth) {
        state->viewerState->recenteringTimeOrth = tempConfig->viewerState->recenteringTimeOrth;
    }
    return true;
}


bool Viewer::updateZoomCube() {
    uint32_t i;
    int32_t residue, max, currentZoomCube, oldZoomCube;

    /* Notice int division! */
    max = ((state->M/2)*2-1);
    oldZoomCube = state->viewerState->zoomCube;
    state->viewerState->zoomCube = 0;

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != (uint32_t)VIEWPORT_SKELETON) {
            residue = ((max*state->cubeEdgeLength)
            - ((int32_t)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
            / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)))
            / state->cubeEdgeLength;

            if(residue%2) {
                residue = residue / 2 + 1;
            }
            else if((residue%2 == 0) && (residue != 0)) {
                residue = (residue - 1) / 2 + 1;
            }
            currentZoomCube = (state->M/2)-residue;
            if(state->viewerState->zoomCube < currentZoomCube) {
                state->viewerState->zoomCube = currentZoomCube;
            }
            residue = ((max*state->cubeEdgeLength)
            - ((int32_t)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
            / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)))
            / state->cubeEdgeLength;

            if(residue%2) {
                residue = residue / 2 + 1;
            }
            else if((residue%2 == 0) && (residue != 0)) {
                residue = (residue - 1) / 2 + 1;
            }
            currentZoomCube = (state->M/2)-residue;
            if(state->viewerState->zoomCube < currentZoomCube) {
                state->viewerState->zoomCube = currentZoomCube;
            }
        }
    }
    if(oldZoomCube != state->viewerState->zoomCube) {
        state->skeletonState->skeletonChanged = true;
    }
    return true;
}

bool Viewer::userMove(int32_t x, int32_t y, int32_t z, int32_t serverMovement) {
    struct viewerState *viewerState = state->viewerState;

    Coordinate lastPosition_dc;
    Coordinate newPosition_dc;

    //The skeleton VP view has to be updated after a current pos change
    state->skeletonState->viewChanged = true;
    if(state->skeletonState->showIntersections) {
        state->skeletonState->skeletonSliceVPchanged = true;
    }
    // This determines whether the server will broadcast the coordinate change
    // to its client or not.

    lastPosition_dc = Coordinate::Px2DcCoord(viewerState->currentPosition);

    viewerState->userMove = true;

    if ((viewerState->currentPosition.x + x) >= 0 &&
        (viewerState->currentPosition.x + x) <= state->boundary.x &&
        (viewerState->currentPosition.y + y) >= 0 &&
        (viewerState->currentPosition.y + y) <= state->boundary.y &&
        (viewerState->currentPosition.z + z) >= 0 &&
        (viewerState->currentPosition.z + z) <= state->boundary.z) {
            viewerState->currentPosition.x += x;
            viewerState->currentPosition.y += y;
            viewerState->currentPosition.z += z;
    }
    else {
        LOG("Position (%d, %d, %d) out of bounds",
            viewerState->currentPosition.x + x + 1,
            viewerState->currentPosition.y + y + 1,
            viewerState->currentPosition.z + z + 1);
    }

    calcLeftUpperTexAbsPx();
    recalcTextureOffsets();
    newPosition_dc = Coordinate::Px2DcCoord(viewerState->currentPosition);

    if(serverMovement == TELL_COORDINATE_CHANGE &&
        state->clientState->connected == true &&
        state->clientState->synchronizePosition) {
        Client::broadcastPosition(viewerState->currentPosition.x,
                                  viewerState->currentPosition.y,
                                  viewerState->currentPosition.z);
    }

    /* TDitem
    printf("temp x: %d\n", tempConfig->viewerState->currentPosition.x);
    printf("temp x: %d\n", state->viewerState->currentPosition.x);
    */

    /*
    printf("temp y: %d\n", tempConfig->viewerState->currentPosition.y);
    printf("temp y: %d\n", state->viewerState->currentPosition.y);

    printf("temp z: %d\n", tempConfig->viewerState->currentPosition.z);
    printf("temp z: %d\n", state->viewerState->currentPosition.z);
    */

    tempConfig->viewerState->currentPosition.x = viewerState->currentPosition.x;
    tempConfig->viewerState->currentPosition.y = viewerState->currentPosition.y;
    tempConfig->viewerState->currentPosition.z = viewerState->currentPosition.z;

    if(!COMPARE_COORDINATE(newPosition_dc, lastPosition_dc)) {
        state->viewerState->superCubeChanged = true;

        sendLoadSignal(viewerState->currentPosition.x,
                                viewerState->currentPosition.y,
                                viewerState->currentPosition.z,
                                NO_MAG_CHANGE);
    }
    Remote::checkIdleTime();



    return true;
}

int32_t updatePosition(int32_t serverMovement) {
        Coordinate jump;

    if(COMPARE_COORDINATE(tempConfig->viewerState->currentPosition, state->viewerState->currentPosition) != true) {
        jump.x = tempConfig->viewerState->currentPosition.x - state->viewerState->currentPosition.x;
        jump.y = tempConfig->viewerState->currentPosition.y - state->viewerState->currentPosition.y;
        jump.z = tempConfig->viewerState->currentPosition.z - state->viewerState->currentPosition.z;
        Viewer::userMove(jump.x, jump.y, jump.z, serverMovement);
    }
    return true;
}


int32_t Viewer::findVPnumByWindowCoordinate(uint32_t xScreen, uint32_t yScreen) {
    uint32_t tempNum;

    tempNum = -1;
   /* TDitem
    for(i = 0; i < state->viewerState->numberViewports; i++) {
        if((xScreen >= state->viewerState->vpConfigs[i].lowerLeftCorner.x) && (xScreen <= (state->viewerState->vpConfigs[i].lowerLeftCorner.x + state->viewerState->vpConfigs[i].edgeLength))) {
            if((yScreen >= (((state->viewerState->vpConfigs[i].lowerLeftCorner.y - state->viewerState->screenSizeY) * -1) - state->viewerState->vpConfigs[i].edgeLength)) && (yScreen <= ((state->viewerState->vpConfigs[i].lowerLeftCorner.y - state->viewerState->screenSizeY) * -1))) {
                //Window coordinate lies in that VP
                tempNum = i;
            }
        }
    }
    //The VP on top (if there are multiple VPs on this coordinate) or -1 is returned.
    */
    return tempNum;
}

bool Viewer::recalcTextureOffsets() {
    uint32_t i;
    float midX, midY;

    midX = midY = 0.;

    /* Every time the texture offset coords change,
    the skeleton VP must be updated. */
    state->skeletonState->viewChanged = true;
    calcDisplayedEdgeLength();

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        /* Do this only for orthogonal VPs... */
        if (state->viewerState->vpConfigs[i].type == VIEWPORT_XY
            || state->viewerState->vpConfigs[i].type == VIEWPORT_XZ
            || state->viewerState->vpConfigs[i].type == VIEWPORT_YZ) {
            /*Don't remove /2 *2, integer division! */

            // old code for smaller FOV
            //state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
            //state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
            //    ((float)(((state->M / 2) * 2 - 1) * state->cubeEdgeLength))
            //    / ((float)state->viewerState->vpConfigs[i].texture.edgeLengthPx);


            // new code for larger FOV
            // displayedEdgeLength is in texture pixels, independent from the
            // currently active mag!



            //Multiply the zoom factor. (only truncation possible! 1 stands for minimal zoom)
            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX *=
                state->viewerState->vpConfigs[i].texture.zoomLevel;
            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *=
                state->viewerState->vpConfigs[i].texture.zoomLevel;

            //... and for the right orthogonal VP
            switch(state->viewerState->vpConfigs[i].type) {
            case VIEWPORT_XY:
                //Aspect ratio correction..
                if(state->viewerState->voxelXYRatio < 1) {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *=
                        state->viewerState->voxelXYRatio;
                }
                else {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /=
                        state->viewerState->voxelXYRatio;
                }
                //Display only entire pixels (only truncation possible!) WHY??

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
                    (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                                  / 2.
                                  / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                            * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                    * 2.;

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                    (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                                   / 2.
                                   / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                            * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                    * 2.;

                // Update screen pixel to data pixel mapping values
                state->viewerState->vpConfigs[i].screenPxXPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength
                    / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                    /  state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].screenPxYPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength
                    / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                // Pixels on the screen per 1 unit in the data coordinate system at the
                // original magnification. These guys appear to be unused!!! jk 14.5.12
                //state->viewerState->vpConfigs[i].screenPxXPerOrigMagUnit =
                //    state->viewerState->vpConfigs[i].screenPxXPerDataPx *
                //    state->magnification;

                //state->viewerState->vpConfigs[i].screenPxYPerOrigMagUnit =
                //    state->viewerState->vpConfigs[i].screenPxYPerDataPx *
                //    state->magnification;

                state->viewerState->vpConfigs[i].displayedlengthInNmX =
                    state->viewerState->voxelDimX
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].displayedlengthInNmY =
                    state->viewerState->voxelDimY
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                // scale to 0 - 1; midX is the current pos in tex coords
                // leftUpperPxInAbsPx is in always in mag1, independent of
                // the currently active mag
                midX = (float)(state->viewerState->currentPosition.x
                       - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x)
                    // / (float)state->viewerState->vpConfigs[i].texture.edgeLengthPx;
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;

                midY = (float)(state->viewerState->currentPosition.y
                       - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y)
                     //(float)state->viewerState->vpConfigs[i].texture.edgeLengthPx;
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;

                //Update state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen with this call
                calcLeftUpperTexAbsPx();

                //Offsets for crosshair
                state->viewerState->vpConfigs[i].texture.xOffset =
                    ((float)(state->viewerState->currentPosition.x
                    - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.x))
                    * state->viewerState->vpConfigs[i].screenPxXPerDataPx
                    + 0.5 * state->viewerState->vpConfigs[i].screenPxXPerDataPx;

                state->viewerState->vpConfigs[i].texture.yOffset =
                    ((float)(state->viewerState->currentPosition.y
                    - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.y))
                    * state->viewerState->vpConfigs[i].screenPxYPerDataPx
                    + 0.5 * state->viewerState->vpConfigs[i].screenPxYPerDataPx;

                break;
            case VIEWPORT_XZ:
                //Aspect ratio correction..
                if(state->viewerState->voxelXYtoZRatio < 1) {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *= state->viewerState->voxelXYtoZRatio;
                }
                else {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /= state->viewerState->voxelXYtoZRatio;
                }
                //Display only entire pixels (only truncation possible!)
                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
                        (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                                       / 2.
                                       / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                                * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                        * 2.;
                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                        (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                                       / 2.
                                       / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                                * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                        * 2.;

                //Update screen pixel to data pixel mapping values
                state->viewerState->vpConfigs[i].screenPxXPerDataPx =
                        (float)state->viewerState->vpConfigs[i].edgeLength
                        / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].screenPxYPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength
                    / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                // Pixels on the screen per 1 unit in the data coordinate system at the
                // original magnification.
                state->viewerState->vpConfigs[i].screenPxXPerOrigMagUnit =
                    state->viewerState->vpConfigs[i].screenPxXPerDataPx
                    * state->magnification;

                state->viewerState->vpConfigs[i].screenPxYPerOrigMagUnit =
                    state->viewerState->vpConfigs[i].screenPxYPerDataPx
                    * state->magnification;

                state->viewerState->vpConfigs[i].displayedlengthInNmX =
                    state->viewerState->voxelDimX
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].displayedlengthInNmY =
                    state->viewerState->voxelDimZ
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                midX = ((float)(state->viewerState->currentPosition.x
                               - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x))
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx; //scale to 0 - 1
                midY = ((float)(state->viewerState->currentPosition.z
                               - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z))
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx; //scale to 0 - 1

                //Update state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen with this call
                calcLeftUpperTexAbsPx();

                //Offsets for crosshair
                state->viewerState->vpConfigs[i].texture.xOffset = ((float)(state->viewerState->currentPosition.x - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.x)) * state->viewerState->vpConfigs[i].screenPxXPerDataPx + 0.5 * state->viewerState->vpConfigs[i].screenPxXPerDataPx;
                state->viewerState->vpConfigs[i].texture.yOffset = ((float)(state->viewerState->currentPosition.z - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.z)) * state->viewerState->vpConfigs[i].screenPxYPerDataPx + 0.5 * state->viewerState->vpConfigs[i].screenPxYPerDataPx;

                break;
            case VIEWPORT_YZ:
                //Aspect ratio correction..
                if(state->viewerState->voxelXYtoZRatio < 1) {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *= state->viewerState->voxelXYtoZRatio;
                }
                else {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /= state->viewerState->voxelXYtoZRatio;
                }

                //Display only entire pixels (only truncation possible!)
                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
                        (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                                       / 2.
                                       / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                                * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                        * 2.;
                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                        (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                                       / 2.
                                       / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                                * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                        * 2.;

                // Update screen pixel to data pixel mapping values
                // WARNING: YZ IS ROTATED AND MIRRORED! So screenPxXPerDataPx
                // corresponds to displayedEdgeLengthY and so on.
                state->viewerState->vpConfigs[i].screenPxXPerDataPx =
                        (float)state->viewerState->vpConfigs[i].edgeLength
                        / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].screenPxYPerDataPx =
                        (float)state->viewerState->vpConfigs[i].edgeLength
                        / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                        // Pixels on the screen per 1 unit in the data coordinate system at the
                        // original magnification.
                state->viewerState->vpConfigs[i].screenPxXPerOrigMagUnit =
                        state->viewerState->vpConfigs[i].screenPxXPerDataPx
                        * state->magnification;

                state->viewerState->vpConfigs[i].screenPxYPerOrigMagUnit =
                        state->viewerState->vpConfigs[i].screenPxYPerDataPx
                        * state->magnification;

                state->viewerState->vpConfigs[i].displayedlengthInNmX =
                        state->viewerState->voxelDimZ
                        * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].displayedlengthInNmY =
                        state->viewerState->voxelDimY
                        * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                midX = ((float)(state->viewerState->currentPosition.y
                                - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y))
                               // / (float)state->viewerState->vpConfigs[i].texture.edgeLengthPx; //scale to 0 - 1
                               * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;
                midY = ((float)(state->viewerState->currentPosition.z
                                - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z))
                               // / (float)state->viewerState->vpConfigs[i].texture.edgeLengthPx; //scale to 0 - 1
                               * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;

                //Update state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen with this call
                calcLeftUpperTexAbsPx();

                //Offsets for crosshair
                state->viewerState->vpConfigs[i].texture.xOffset =
                        ((float)(state->viewerState->currentPosition.z
                                 - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.z))
                        * state->viewerState->vpConfigs[i].screenPxXPerDataPx
                        + 0.5 * state->viewerState->vpConfigs[i].screenPxXPerDataPx;

                state->viewerState->vpConfigs[i].texture.yOffset =
                        ((float)(state->viewerState->currentPosition.y
                                 - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.y))
                        * state->viewerState->vpConfigs[i].screenPxYPerDataPx
                        + 0.5 * state->viewerState->vpConfigs[i].screenPxYPerDataPx;
                break;
            }

            //Calculate the vertices in texture coordinates
            // mid really means current pos inside the texture, in texture coordinates, relative to the texture origin 0., 0.
            state->viewerState->vpConfigs[i].texture.texLUx =
                    midX - (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.);
            state->viewerState->vpConfigs[i].texture.texLUy =
                    midY - (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.);
            state->viewerState->vpConfigs[i].texture.texRUx =
                    midX + (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.);
            state->viewerState->vpConfigs[i].texture.texRUy = state->viewerState->vpConfigs[i].texture.texLUy;
            state->viewerState->vpConfigs[i].texture.texRLx = state->viewerState->vpConfigs[i].texture.texRUx;
            state->viewerState->vpConfigs[i].texture.texRLy =
                    midY + (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.);
            state->viewerState->vpConfigs[i].texture.texLLx = state->viewerState->vpConfigs[i].texture.texLUx;
            state->viewerState->vpConfigs[i].texture.texLLy = state->viewerState->vpConfigs[i].texture.texRLy;
        }
    }
    //Reload the height/width-windows in viewports
    MainWindow::reloadDataSizeWin();
    return true;
}

bool Viewer::refreshViewports() {
    /*
    state->viewerState->gui->vpXyWin->updateGL();
    state->viewerState->gui->vpXzWin->updateGL();
    state->viewerState->gui->vpYzWin->updateGL();
    state->viewerState->gui->vpSkelWin->updateGL();
    */
    return true;
}

bool Viewer::sendLoadSignal(uint32_t x, uint32_t y, uint32_t z, int32_t magChanged) {
    qDebug() << "Viewer: sendLoadSignal begin";
    state->protectLoadSignal->lock();
    state->loadSignal = true;  
    state->datasetChangeSignal = magChanged;


    // Convert the coordinate to the right mag. The loader
    // is agnostic to the different dataset magnifications.
    // The int division is hopefully not too much of an issue here
    SET_COORDINATE(state->currentPositionX,
                   x / state->magnification,
                   y / state->magnification,
                   z / state->magnification);

    /*
    emit loadSignal();
    qDebug("I send a load signal to %i, %i, %i", x, y, z);
    */
    state->protectLoadSignal->unlock();

    state->conditionLoadSignal->wakeOne();
    qDebug() << "Viewer: sendLoadSignal ended";
    return true;
}
