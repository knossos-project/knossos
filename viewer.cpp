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
#include "skeletonizer.h"
#include "renderer.h"
#include "widgets/widgetcontainer.h"

#include "sleeper.h"
#include "widgets/mainwindow.h"
#include "widgets/viewport.h"
#include "functions.h"
#include <qopengl.h>
#include <QtConcurrent/QtConcurrentRun>

extern stateInfo *state;

Viewer::Viewer(QObject *parent) :
    QThread(parent)
{

    window = new MainWindow();
    window->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    state->console = window->widgetContainer->console;
    vpUpperLeft = window->viewports[VIEWPORT_XY];
    vpLowerLeft = window->viewports[VIEWPORT_XZ];
    vpUpperRight = window->viewports[VIEWPORT_YZ];
    vpLowerRight = window->viewports[VIEWPORT_SKELETON];
    eventModel = new EventModel();
    vpUpperLeft->eventDelegate = vpLowerLeft->eventDelegate = vpUpperRight->eventDelegate = vpLowerRight->eventDelegate = eventModel;

    timer = new QTimer();

    /* order of the initialization of the rendering system is
     * 1. initViewer
     * 2. new Skeletonizer
     * 3. new Renderer
     * 4. Load the GUI-Settings (otherwise the initialization of the skeletonizer or the renderer would overwrite some variables)
    */
    SET_COORDINATE(state->viewerState->currentPosition, state->boundary.x / 2, state->boundary.y / 2, state->boundary.z / 2);
	SET_COORDINATE(state->viewerState->lastRecenteringPosition, state->viewerState->currentPosition.x, state->viewerState->currentPosition.y, state->viewerState->currentPosition.z);

    initViewer();
    skeletonizer = new Skeletonizer();
    renderer = new Renderer();

    QDesktopWidget *desktop = QApplication::desktop();

    window->loadSettings();
    window->show();
    if(window->pos().x() <= 0 or window->pos().y() <= 0) {
        window->setGeometry(desktop->availableGeometry().topLeft().x() + 20,
                            desktop->availableGeometry().topLeft().y() + 50,
                            1024, 800);
    }


    // This is needed for the viewport text rendering
    renderer->refVPXY = vpUpperLeft;
    renderer->refVPXZ = vpLowerLeft;
    renderer->refVPYZ = vpUpperRight;
    renderer->refVPSkel = vpLowerRight;

    rewire();
    // TODO: to be removed in release version. Jumps to center of e1088_large dataset
    sendLoadSignal(state->viewerState->currentPosition.x,
                   state->viewerState->currentPosition.y,
                   state->viewerState->currentPosition.z,
                   NO_MAG_CHANGE);
    updateCoordinatesSignal(state->viewerState->currentPosition.x,
                           state->viewerState->currentPosition.y,
                           state->viewerState->currentPosition.z);
    frames = 0;
    state->alpha = 0;
    state->beta = 0;

    for (uint i = 0; i < state->viewerState->numberViewports; i++){
        state->viewerState->vpConfigs[i].s_max =  state->viewerState->vpConfigs[i].t_max =
                (
                    (
                        (int)((state->M/2 + 1)
                              * state->cubeEdgeLength
                              / sqrt(2))
                        )
                    / 2)
                *2;
    }

    floatCoordinate v1, v2, v3;
    getDirectionalVectors(state->alpha, state->beta, &v1, &v2, &v3);

    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XY].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XY].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XY].n , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XZ].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XZ].v2 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XZ].n , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_YZ].v1 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_YZ].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_YZ].n , v1);

    timer->singleShot(10, this, SLOT(run()));
    delay.start();
}

vpList *Viewer::vpListNew() {
    vpList *newVpList = NULL;

    newVpList = (vpList *) malloc(sizeof(vpList));
    if(newVpList == NULL) {
        LOG("Out of memory.\n")
        return NULL;
    }

    newVpList->entry = NULL;
    newVpList->elements = 0;

    return newVpList;
}

int Viewer::vpListAddElement(vpList *vpList, vpConfig *vpConfig) {
    vpListElement *newElement = (vpListElement *) malloc(sizeof(vpListElement));
    if(newElement == NULL) {
        qDebug("Out of memory\n");
        exit(EXIT_FAILURE);
    }

    newElement->vpConfig = vpConfig;
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

vpList *Viewer::vpListGenerate(viewerState *viewerState) {
    vpList *newVpList = NULL;
    uint i = 0;

    newVpList = vpListNew();
    if(newVpList == NULL) {
        LOG("Error generating new vpList.")
        _Exit(false);
    }

    for(i = 0; i < viewerState->numberViewports; i++) {
        if(viewerState->vpConfigs[i].type == VIEWPORT_SKELETON) {
            continue;
        }
        vpListAddElement(newVpList, &(viewerState->vpConfigs[i]));
    }
    return newVpList;
}

int Viewer::vpListDelElement(vpList *list,  vpListElement *element) {
    if(element->next == element) {
        // This is the only element in the list
        list->entry = NULL;
    } else {
        element->next->previous = element->previous;
        element->previous->next = element->next;
        list->entry = element->next;
    }
    free(element);
    list->elements = list->elements - 1;
    return list->elements;
}

bool Viewer::vpListDel(vpList *list) {
    while(list->elements > 0) {
        if(vpListDelElement(list, list->entry) < 0) {
            LOG("Error deleting element at %p from the slot list %d elements remain in the list.",
                   list->entry, list->elements);
            return false;
        }
    }

    return true;
}

bool Viewer::resetViewPortData(vpConfig *viewport) {
    // for arbitrary vp orientation
    memset(viewport->viewPortData,
           state->viewerState->defaultTexData[0],
           TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 3);
    viewport->s_max = viewport->t_max  = -1;
    return true;
}

bool Viewer::sliceExtract_standard(Byte *datacube, Byte *slice, vpConfig *vpConfig) {
    int i, j;
    switch(vpConfig->type) {
    case SLICE_XY:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = slice[1] = slice[2] = *datacube;
            datacube++;
            slice += 3;
        }
        break;
    case SLICE_XZ:
        for(j = 0; j < state->cubeEdgeLength; j++) {
            for(i = 0; i < state->cubeEdgeLength; i++) {
                slice[0] = slice[1] = slice[2] = *datacube;
                datacube++;
                slice += 3;
            }
            datacube = datacube + state->cubeSliceArea - state->cubeEdgeLength;
        }
        break;
    case SLICE_YZ:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = slice[1] = slice[2] = *datacube;
            datacube += state->cubeEdgeLength;
            slice += 3;
        }
        break;
    default:
        return false;
    }
    return true;
}

bool Viewer::sliceExtract_standard_arb(Byte *datacube, struct vpConfig *viewPort, floatCoordinate *currentPxInDc_float,
                                       int s, int *t) {
    Byte *slice = viewPort->viewPortData;
    Coordinate currentPxInDc;
    int sliceIndex = 0, dcIndex = 0;
    floatCoordinate *v2 = &(viewPort->v2);

    SET_COORDINATE(currentPxInDc, roundFloat(currentPxInDc_float->x),
                                  roundFloat(currentPxInDc_float->y),
                                  roundFloat(currentPxInDc_float->z));
    // rounding may lead to values outside of cube edge, i.e.  values >= cubeEdgeLength + 0.5 or values <= -0.5
    if(currentPxInDc.x < 0) { currentPxInDc.x = 0; }
    if(currentPxInDc.y < 0) { currentPxInDc.y = 0; }
    if(currentPxInDc.z < 0) { currentPxInDc.z = 0; }
    if(currentPxInDc.x == state->cubeEdgeLength) { currentPxInDc.x = state->cubeEdgeLength - 1; }
    if(currentPxInDc.y == state->cubeEdgeLength) { currentPxInDc.y = state->cubeEdgeLength - 1; }
    if(currentPxInDc.z == state->cubeEdgeLength) { currentPxInDc.z = state->cubeEdgeLength - 1; }

    while((0 <= currentPxInDc.x && currentPxInDc.x < state->cubeEdgeLength)
          && (0 <= currentPxInDc.y && currentPxInDc.y < state->cubeEdgeLength)
          && (0 <= currentPxInDc.z && currentPxInDc.z < state->cubeEdgeLength)) {

        sliceIndex = 3 * ( s + *t  *  state->cubeEdgeLength * state->M);
        dcIndex = currentPxInDc.x + currentPxInDc.y * state->cubeEdgeLength + currentPxInDc.z * state->cubeSliceArea;
        if(datacube == NULL) {
            slice[sliceIndex] = slice[sliceIndex + 1]
                              = slice[sliceIndex + 2]
                              = state->viewerState->defaultTexData[dcIndex];
        }
        else {
            slice[sliceIndex] = slice[sliceIndex + 1]
                              = slice[sliceIndex + 2]
                              = datacube[dcIndex];
        }
        (*t)++;
        if(*t >= viewPort->t_max) {
            break;
        }
        ADD_COORDINATE(*currentPxInDc_float, *v2);
        SET_COORDINATE(currentPxInDc, roundFloat(currentPxInDc_float->x),
                                      roundFloat(currentPxInDc_float->y),
                                      roundFloat(currentPxInDc_float->z));
    }
    return true;
}


bool Viewer::sliceExtract_adjust(Byte *datacube,
                                     Byte *slice,
                                     vpConfig *vpConfig) {
    int i, j;
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
            datacube  = datacube + state->cubeSliceArea - state->cubeEdgeLength;
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

bool Viewer::sliceExtract_adjust_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float,
                                     int s, int *t) {
    Byte *slice = viewPort->viewPortData;
    Coordinate currentPxInDc;
    int sliceIndex = 0, dcIndex = 0;
    floatCoordinate *v2 = &(viewPort->v2);

    SET_COORDINATE(currentPxInDc, (roundFloat(currentPxInDc_float->x)),
                                  (roundFloat(currentPxInDc_float->y)),
                                  (roundFloat(currentPxInDc_float->z)));
    // rounding may lead to values outside of cube edge, i.e.  values >= cubeEdgeLength + 0.5 or values <= -0.5
    if(currentPxInDc.x < 0) { currentPxInDc.x = 0; }
    if(currentPxInDc.y < 0) { currentPxInDc.y = 0; }
    if(currentPxInDc.z < 0) { currentPxInDc.z = 0; }
    if(currentPxInDc.x == state->cubeEdgeLength) { currentPxInDc.x = state->cubeEdgeLength - 1; }
    if(currentPxInDc.y == state->cubeEdgeLength) { currentPxInDc.y = state->cubeEdgeLength - 1; }
    if(currentPxInDc.z == state->cubeEdgeLength) { currentPxInDc.z = state->cubeEdgeLength - 1; }

    while((0 <= currentPxInDc.x && currentPxInDc.x < state->cubeEdgeLength)
          && (0 <= currentPxInDc.y && currentPxInDc.y < state->cubeEdgeLength)
          && (0 <= currentPxInDc.z && currentPxInDc.z < state->cubeEdgeLength)) {

        sliceIndex = 3 * ( s + *t  * state->cubeEdgeLength * state->M);

        dcIndex = currentPxInDc.x + currentPxInDc.y * state->cubeEdgeLength + currentPxInDc.z * state->cubeSliceArea;
        if(datacube == NULL) {
            slice[sliceIndex] = slice[sliceIndex + 1]
                              = slice[sliceIndex + 2]
                              = state->viewerState->defaultTexData[dcIndex];
        }
        slice[sliceIndex] = state->viewerState->datasetAdjustmentTable[0][datacube[dcIndex]];
        slice[sliceIndex + 1] = state->viewerState->datasetAdjustmentTable[1][datacube[dcIndex]];
        slice[sliceIndex + 2] = state->viewerState->datasetAdjustmentTable[2][datacube[dcIndex]];

        (*t)++;
        if(*t >= viewPort->t_max) {
            break;
        }
        ADD_COORDINATE(*currentPxInDc_float, *v2);
        SET_COORDINATE(currentPxInDc, roundFloat(currentPxInDc_float->x),
                                      roundFloat(currentPxInDc_float->y),
                                      roundFloat(currentPxInDc_float->z));
    }
    return true;
}

bool Viewer::dcSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    datacube += dcOffset;

    if(state->viewerState->datasetAdjustmentOn) {
        /* Texture type GL_RGB and we need to adjust coloring */
        //QFuture<bool> future = QtConcurrent::run(this, &Viewer::sliceExtract_adjust, datacube, slice, vpConfig);
        //future.waitForFinished();
        sliceExtract_adjust(datacube, slice, vpConfig);
    }
    else {
        /* Texture type GL_RGB and we don't need to adjust anything*/
       //QFuture<bool> future = QtConcurrent::run(this, &Viewer::sliceExtract_adjust, datacube, slice, vpConfig);
       //future.waitForFinished();
       sliceExtract_standard(datacube, slice, vpConfig);
    }
    return true;
}

bool Viewer::dcSliceExtract_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t) {
    if(state->viewerState->datasetAdjustmentOn) {
        // Texture type GL_RGB and we need to adjust coloring
        sliceExtract_adjust_arb(datacube, viewPort, currentPxInDc_float, s, t);
    } else {
        // Texture type GL_RGB and we don't need to adjust anything
        sliceExtract_standard_arb(datacube, viewPort, currentPxInDc_float, s, t);
    }
    return true;
}

bool Viewer::ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    int i, j;
    int objId, *objIdP;

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

static int texIndex(uint x,
                        uint y,
                        uint colorMultiplicationFactor,
                        viewportTexture *texture) {
    uint index = 0;

    index = x * state->cubeSliceArea + y
            * (texture->edgeLengthDc * state->cubeSliceArea)
            * colorMultiplicationFactor;

    return index;
}

bool Viewer::vpGenerateTexture(vpListElement *currentVp, viewerState *viewerState) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.

    uint x_px = 0, x_dc = 0, y_px = 0, y_dc = 0;
    Coordinate upperLeftDc, currentDc, currentPosition_dc;
    Coordinate currPosTrans, leftUpperPxInAbsPxTrans;

    Byte *datacube = NULL, *overlayCube = NULL;
    int dcOffset = 0, index = 0;

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
                qDebug("No such slice type (%d) in vpGenerateTexture.", currentVp->vpConfig->type);
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

bool Viewer::vpGenerateTexture_arb(struct vpListElement *currentVp) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.
    Coordinate currentDc, currentPx;
    floatCoordinate currentPxInDc_float, rowPx_float, currentPx_float;

    Byte *datacube = NULL, *overlayCube = NULL;

    floatCoordinate *v1 = &currentVp->vpConfig->v1;
    floatCoordinate *v2 = &currentVp->vpConfig->v2;
    CPY_COORDINATE(rowPx_float, currentVp->vpConfig->texture.leftUpperPxInAbsPx);
    DIV_COORDINATE(rowPx_float, state->magnification);
    CPY_COORDINATE(currentPx_float, rowPx_float);

    glBindTexture(GL_TEXTURE_2D, currentVp->vpConfig->texture.texHandle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int s = 0, t = 0, t_old = 0;
    while(s < currentVp->vpConfig->s_max) {
        t = 0;
        while(t < currentVp->vpConfig->t_max) {
            SET_COORDINATE(currentPx, roundFloat(currentPx_float.x),
                                      roundFloat(currentPx_float.y),
                                      roundFloat(currentPx_float.z));
            SET_COORDINATE(currentDc, currentPx.x/state->cubeEdgeLength,
                                      currentPx.y/state->cubeEdgeLength,
                                      currentPx.z/state->cubeEdgeLength);

            if(currentPx.x < 0) { currentDc.x -= 1; }
            if(currentPx.y < 0) { currentDc.y -= 1; }
            if(currentPx.z < 0) { currentDc.z -= 1; }

            state->protectCube2Pointer->lock();
            datacube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
            overlayCube = Hashtable::ht_get(state->Oc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
            state->protectCube2Pointer->unlock();

            SET_COORDINATE(currentPxInDc_float, currentPx_float.x-currentDc.x*state->cubeEdgeLength,
                                                currentPx_float.y-currentDc.y*state->cubeEdgeLength,
                                                currentPx_float.z-currentDc.z*state->cubeEdgeLength);
            t_old = t;
            dcSliceExtract_arb(datacube,
                               currentVp->vpConfig,
                               &currentPxInDc_float,
                               s, &t);
            SET_COORDINATE(currentPx_float, currentPx_float.x + v2->x * (t - t_old),
                                            currentPx_float.y + v2->y * (t - t_old),
                                            currentPx_float.z + v2->z * (t - t_old));
             //  Take care of the overlay textures.
            if(state->overlay) {
                //TDITEM handle overlay
            }
        }
        s++;
        ADD_COORDINATE(rowPx_float, *v1);
        CPY_COORDINATE(currentPx_float, rowPx_float);
    }

    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    state->M*state->cubeEdgeLength,
                    state->M*state->cubeEdgeLength,
                    GL_RGB,
                    GL_UNSIGNED_BYTE,
                    currentVp->vpConfig->viewPortData);

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
bool Viewer::calcLeftUpperTexAbsPx() {
    uint i = 0;
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
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x >1000000){ LOG("uninit x %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x)}
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y > 1000000){ LOG("uninit y %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y)}
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z > 1000000){ LOG("uninit z %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z)}

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
        case VIEWPORT_ARBITRARY:
            floatCoordinate v1, v2;
            CPY_COORDINATE(v1, viewerState->vpConfigs[i].v1);
            CPY_COORDINATE(v2, viewerState->vpConfigs[i].v2);
            SET_COORDINATE(viewerState->vpConfigs[i].leftUpperPxInAbsPx_float,
                           viewerState->currentPosition.x - v1.x * viewerState->vpConfigs[i].s_max/2 - v2.x * viewerState->vpConfigs[i].t_max/2,
                           viewerState->currentPosition.y - v1.y * viewerState->vpConfigs[i].s_max/2 - v2.y * viewerState->vpConfigs[i].t_max/2,
                           viewerState->currentPosition.z - v1.z * viewerState->vpConfigs[i].s_max/2 - v2.z * viewerState->vpConfigs[i].t_max/2);

            SET_COORDINATE(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx,
                           roundFloat(viewerState->vpConfigs[i].leftUpperPxInAbsPx_float.x),
                           roundFloat(viewerState->vpConfigs[i].leftUpperPxInAbsPx_float.y),
                           roundFloat(viewerState->vpConfigs[i].leftUpperPxInAbsPx_float.z));

            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen_float,
                           viewerState->currentPosition.x
                           - v1.x * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx)

                           - v2.x * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.y

                           - v1.y * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                           - v2.y * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.z

                           - v1.z * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                           - v2.z * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx));

            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen,
                           roundFloat(viewerState->vpConfigs[i].leftUpperDataPxOnScreen.x),
                           roundFloat(viewerState->vpConfigs[i].leftUpperDataPxOnScreen.y),
                           roundFloat(viewerState->vpConfigs[i].leftUpperDataPxOnScreen.z));
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
    calcLeftUpperTexAbsPx();

    if(state->overlay) {
        LOG("overlayColorMap at %p\n", &(state->viewerState->overlayColorMap[0][0]))
        if(loadDatasetColorTable("stdOverlay.lut",
                          &(state->viewerState->overlayColorMap[0][0]),
                          GL_RGBA) == false) {
            LOG("Overlay color map stdOverlay.lut does not exist.")
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
        LOG("Out of memory.")
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
            LOG("Out of memory.")
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
        LOG("Out of memory.")
        _Exit(false);
    }
    memset(state->viewerState->defaultTexData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                     * sizeof(Byte)
                                                     * 3);

    /* @arb */
    for(int i = 0; i < state->viewerState->numberViewports; i++){
        state->viewerState->vpConfigs[i].viewPortData = (Byte *)malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 3);
        if(state->viewerState->vpConfigs[i].viewPortData == NULL) {
            LOG("Out of memory.")
            exit(0);
        }
        memset(state->viewerState->vpConfigs[i].viewPortData, state->viewerState->defaultTexData[0], TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 3);
    }


    // Default data for the overlays
    if(state->overlay) {
        state->viewerState->defaultOverlayData = (Byte *) malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                                 * sizeof(Byte)
                                                                 * 4);
        if(state->viewerState->defaultOverlayData == NULL) {
            LOG("Out of memory.")
            _Exit(false);
        }
        memset(state->viewerState->defaultOverlayData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                             * sizeof(Byte)
                                                             * 4);
    }

    updateViewerState();
    recalcTextureOffsets();

    return true;
}



// from knossos-global.h
bool Viewer::loadDatasetColorTable(QString path, GLuint *table, int type) {
    FILE *lutFile = NULL;
    uint8_t lutBuffer[RGBA_LUTSIZE];
    uint readBytes = 0, i = 0;
    uint size = RGB_LUTSIZE;

    // The b is for compatibility with non-UNIX systems and denotes a
    // binary file.

    const char *cpath = path.toStdString().c_str();

    LOG("Reading Dataset LUT at %s\n", cpath);

    lutFile = fopen(cpath, "rb");
    if(lutFile == NULL) {
        LOG("Unable to open Dataset LUT at %s.", cpath)
        return false;
    }

    if(type == GL_RGB) {
        size = RGB_LUTSIZE;
    }
    else if(type == GL_RGBA) {
        size = RGBA_LUTSIZE;
    }
    else {
        LOG("Requested color type %x does not exist.", type)
        return false;
    }

    readBytes = (uint)fread(lutBuffer, 1, size, lutFile);
    if(readBytes != size) {
        LOG("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, cpath, size)
        if(fclose(lutFile) != 0) {
            LOG("Additionally, an error occured closing the file.")
        }
        return false;
    }

    if(fclose(lutFile) != 0) {
        LOG("Error closing LUT file.")
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

bool Viewer::loadTreeColorTable(QString path, float *table, int type) {
    FILE *lutFile = NULL;
    uint8_t lutBuffer[RGB_LUTSIZE];
    uint readBytes = 0, i = 0;
    uint size = RGB_LUTSIZE;

    const char *cpath = path.toStdString().c_str();
    // The b is for compatibility with non-UNIX systems and denotes a
    // binary file.
    LOG("Reading Tree LUT at %s\n", cpath)

    lutFile = fopen(cpath, "rb");
    if(lutFile == NULL) {
        LOG("Unable to open Tree LUT at %s.", cpath)
        return false;
    }
    if(type != GL_RGB) {
        /* AG_TextError("Tree colors only support RGB colors. Your color type is: %x", type); */
        LOG("Chosen color was of type %x, but expected GL_RGB", type)
        return false;
    }

    readBytes = (uint)fread(lutBuffer, 1, size, lutFile);
    if(readBytes != size) {
        LOG("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, cpath, size)
        if(fclose(lutFile) != 0) {
            LOG("Additionally, an error occured closing the file.")
        }
        return false;
    }
    if(fclose(lutFile) != 0) {
        LOG("Error closing LUT file.")
        return false;
    }

    //Get RGB-Values in percent
    for(i = 0; i < 256; i++) {
        table[i]   = lutBuffer[i]/MAX_COLORVAL;
        table[i + 256] = lutBuffer[i+256]/MAX_COLORVAL;
        table[i + 512] = lutBuffer[i + 512]/MAX_COLORVAL;
    }

    window->treeColorAdjustmentsChanged();
    return true;
}

bool Viewer::updatePosition(int serverMovement) {
    Coordinate jump;

    return true;
}

bool Viewer::calcDisplayedEdgeLength() {
    uint i;
    float FOVinDCs;

    FOVinDCs = ((float)state->M) - 1.f;


    for(i = 0; i < state->viewerState->numberViewports; i++) {
        if (state->viewerState->vpConfigs[i].type==VIEWPORT_ARBITRARY){
            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
                state->viewerState->vpConfigs[i].s_max / (float) state->viewerState->vpConfigs[i].texture.edgeLengthPx;
            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                state->viewerState->vpConfigs[i].t_max / (float) state->viewerState->vpConfigs[i].texture.edgeLengthPx;
        }
        else {
            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                FOVinDCs * (float)state->cubeEdgeLength
                / (float) state->viewerState->vpConfigs[i].texture.edgeLengthPx;
        }
    }
    return true;
}


/**
* takes care of all necessary changes inside the viewer and signals
* the loader to change the dataset
*/
/* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
bool Viewer::changeDatasetMag(uint upOrDownFlag) {
    uint i;
    if (DATA_SET != upOrDownFlag) {

        if(state->viewerState->datasetMagLock) {
            return false;
        }

        switch(upOrDownFlag) {
        case MAG_DOWN:
            if(state->magnification > state->lowestAvailableMag) {
                state->magnification /= 2;
                for(i = 0; i < state->viewerState->numberViewports; i++) {
                    if(state->viewerState->vpConfigs[i].type != (uint)VIEWPORT_SKELETON) {
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
                    if(state->viewerState->vpConfigs[i].type != (uint)VIEWPORT_SKELETON) {
                        state->viewerState->vpConfigs[i].texture.zoomLevel *= 0.5;
                        downsampleVPTexture(&state->viewerState->vpConfigs[i]);
                        state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx /= 2.;
                    }
                }
            }
            else return false;
            break;
        }
    }

    /* necessary? */
    state->viewerState->userMove = true;
    recalcTextureOffsets();

    /*for(i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != VIEWPORT_SKELETON) {
            LOG("left upper tex px of VP %d is: %d, %d, %d",i,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z)
        }
    }*/
    sendLoadSignal(state->viewerState->currentPosition.x,
                            state->viewerState->currentPosition.y,
                            state->viewerState->currentPosition.z,
                            upOrDownFlag);

    /* set flags to trigger the necessary renderer updates */
    state->skeletonState->skeletonChanged = true;
    skeletonizer->skeletonChanged = true;

    emit updateZoomAndMultiresWidgetSignal();

    return true;
}

/**
  * @brief This method is used for the Viewer-Thread declared in main(knossos.cpp)
  * This is the old viewer() function from KNOSSOS 3.2
  * It works as follows:
  * - The viewer get initialized
  * - The rendering loop starts:
  *   - lists of pending texture parts are iterated and loaded if they are available
  *   - TODO: The Eventhandling in QT works asnyc, new concept are currently in progress
  * - the loadSignal occurs in three different locations:
  *   - initViewer
  *   - changeDatasetMag
  *   - userMove
  *
  */
//Entry point for viewer thread, general viewer coordination, "main loop"
void Viewer::run() {
    static uint call = 0;
    processUserMove();
    /*
    if(!state->viewerState->userMove) {
        if(QApplication::hasPendingEvents()) {
            QApplication::processEvents();
        }
        timer->singleShot(20 , this, SLOT(run()));
        return;

    }*/

    // Event and rendering loop.
    // What happens is that we go through lists of pending texture parts and load
    // them if they are available.
    // While we are loading the textures, we check for events. Some events
    // might cancel the current loading process. When all textures
    // have been processed, we go into an idle state, in which we wait for events.
    struct viewerState *viewerState = state->viewerState;
    struct vpListElement *currentVp = NULL, *nextVp = NULL;
    uint drawCounter = 0;

    state->viewerState->viewerReady = true;

    // Display info about skeleton save path here TODO
    // This creates a circular doubly linked list of
    // pending viewports (viewports for which the texture has not yet been
    // completely loaded) from the viewport-array in the viewerState
    // structure.
    // The idea is that we can easily remove the element representing a
    // pending viewport once its texture is completely loaded.
    viewports = vpListGenerate(viewerState);
    currentVp = viewports->entry;

    // for arbitrary viewport orientation
    state->alpha += state->viewerState->alphaCache;
    state->beta += state->viewerState->betaCache;
    state->viewerState->alphaCache = state->viewerState->betaCache = 0;
    if (state->alpha >= 360) {
        state->alpha -= 360;
    }
    else if (state->alpha < 0) {
        state->alpha += 360;
    }
    if (state->beta >= 360) {
        state->beta -= 360;
    }
    else if (state->beta < 0) {
        state->beta += 360;
    }

    getDirectionalVectors(state->alpha, state->beta, &v1, &v2, &v3);

    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERLEFT].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERLEFT].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERLEFT].n , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_LOWERLEFT].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_LOWERLEFT].v2 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_LOWERLEFT].n , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERRIGHT].v1 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERRIGHT].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERRIGHT].n , v1);
    recalcTextureOffsets();

    while(viewports->elements > 0) {

        switch(currentVp->vpConfig->id) {

        case VP_UPPERLEFT:
            vpUpperLeft->makeCurrent();
            break;
        case VP_LOWERLEFT:
            vpLowerLeft->makeCurrent();
            break;
        case VP_UPPERRIGHT:
            vpUpperRight->makeCurrent();
            break;
        }
        nextVp = currentVp->next;

        if(currentVp->vpConfig->type != VIEWPORT_SKELETON) {
            if(currentVp->vpConfig->type != VIEWPORT_ARBITRARY) {
                 vpGenerateTexture(currentVp, viewerState);
            }
            else {
                vpGenerateTexture_arb(currentVp);
            }
        }
        drawCounter++;
        if(drawCounter == 3) {
            drawCounter = 0;

            updateViewerState();
            recalcTextureOffsets();
            skeletonizer->updateSkeletonState();

            vpUpperLeft->updateGL();
            vpLowerLeft->updateGL();
            vpUpperRight->updateGL();
            vpLowerRight->updateGL();


            if(call % 1000 == 0) {
                if(idlingExceeds(10000)) {
                    state->viewerState->renderInterval = 1000;
                }
            }
            timer->singleShot(state->viewerState->renderInterval, this, SLOT(run()));

            vpListDel(viewports);
            viewerState->userMove = false;
            call += 1;
            return;
        }
        currentVp = nextVp;
    } //end while(viewports->elements > 0)
}

/** this method checks if the last call of the method checkIdleTime is longer than <treshold> msec ago.
 *  In this case, the render loop is slowed down to 2 calls per second (see timer->setSingleShot).
 *  Otherwise it stays in / switches to normal mode
*/
bool Viewer::idlingExceeds(uint msec) {
    QDateTime now = QDateTime::currentDateTimeUtc();
    if(now.msecsTo(state->viewerState->lastIdleTimeCall) <= -msec) {
        return true;
    }
    return false;
}

bool Viewer::updateViewerState() {
   uint i;

    for(i = 0; i < state->viewerState->numberViewports; i++) {

        if(i == VP_UPPERLEFT) {
            vpUpperLeft->makeCurrent();
        }
        else if(i == VP_LOWERLEFT) {
            vpLowerLeft->makeCurrent();
        }
        else if(i == VP_UPPERRIGHT) {
            vpUpperRight->makeCurrent();
        }
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
    updateZoomCube();
    return true;
}


bool Viewer::updateZoomCube() {
    uint i;
    int residue, max, currentZoomCube, oldZoomCube;

    /* Notice int division! */
    max = ((state->M/2)*2-1);
    oldZoomCube = state->viewerState->zoomCube;
    state->viewerState->zoomCube = 0;

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != (uint)VIEWPORT_SKELETON) {
            residue = ((max*state->cubeEdgeLength)
            - ((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
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
            - ((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
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
        //skeletonizer->skeletonChanged = true;
    }
    return true;
}

bool Viewer::userMove(int x, int y, int z, int serverMovement) {
    struct viewerState *viewerState = state->viewerState;

    Coordinate lastPosition_dc;
    Coordinate newPosition_dc;

    //The skeleton VP view has to be updated after a current pos change
    state->skeletonState->viewChanged = true;
    skeletonizer->viewChanged = true;
    /* @todo case decision for skeletonizer */
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
            SET_COORDINATE(state->currentDirections[state->currentDirectionsIndex], x, y, z);
            state->currentDirectionsIndex = (state->currentDirectionsIndex + 1) % LL_CURRENT_DIRECTIONS_SIZE;
    }
    else {
        LOG("Position (%d, %d, %d) out of bounds",
            viewerState->currentPosition.x + x + 1,
            viewerState->currentPosition.y + y + 1,
            viewerState->currentPosition.z + z + 1)
    }

    //qDebug() << state->viewerState->currentPosition.x << " " << state->viewerState->currentPosition.y;

    calcLeftUpperTexAbsPx();
    recalcTextureOffsets();
    newPosition_dc = Coordinate::Px2DcCoord(viewerState->currentPosition);

    if(serverMovement == TELL_COORDINATE_CHANGE &&
        state->clientState->connected == true &&
        state->clientState->synchronizePosition) {
        emit broadcastPosition(viewerState->currentPosition.x,
                                  viewerState->currentPosition.y,
                                  viewerState->currentPosition.z);
    }


    if(!COMPARE_COORDINATE(newPosition_dc, lastPosition_dc)) {
        state->viewerState->superCubeChanged = true;

        sendLoadSignal(viewerState->currentPosition.x,
                       viewerState->currentPosition.y,
                       viewerState->currentPosition.z,
                       NO_MAG_CHANGE);
    }

    QtConcurrent::run(this, &Viewer::updateCoordinatesSignal,
                      viewerState->currentPosition.x, viewerState->currentPosition.y, viewerState->currentPosition.z);
    emit idleTimeSignal();

    return true;
}

bool Viewer::userMove_arb(float x, float y, float z, int serverMovement) {
    Coordinate step;
    state->viewerState->moveCache.x += x;
    state->viewerState->moveCache.y += y;
    state->viewerState->moveCache.z += z;
    step.x = roundFloat(state->viewerState->moveCache.x);
    step.y = roundFloat(state->viewerState->moveCache.y);
    step.z = roundFloat(state->viewerState->moveCache.z);
    SUB_COORDINATE(state->viewerState->moveCache, step);
    return userMove(step.x, step.y, step.z, serverMovement);
    return false;
}


int Viewer::findVPnumByWindowCoordinate(uint xScreen, uint yScreen) {
    uint tempNum;

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
    uint i;
    float midX = 0.,midY = 0.;

    /* Every time the texture offset coords change,
    the skeleton VP must be updated. */
    state->skeletonState->viewChanged = true;
    //skeletonizer->viewChanged = true;
    calcDisplayedEdgeLength();

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        /* Do this only for orthogonal VPs... */
        if (state->viewerState->vpConfigs[i].type == VIEWPORT_XY
            || state->viewerState->vpConfigs[i].type == VIEWPORT_XZ
            || state->viewerState->vpConfigs[i].type == VIEWPORT_YZ
            || state->viewerState->vpConfigs[i].type == VIEWPORT_ARBITRARY) {

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
                state->viewerState->vpConfigs[i].texture.xOffset =
                        ((float)(state->viewerState->currentPosition.x
                                 - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.x))
                        * state->viewerState->vpConfigs[i].screenPxXPerDataPx
                        + 0.5 * state->viewerState->vpConfigs[i].screenPxXPerDataPx;
                state->viewerState->vpConfigs[i].texture.yOffset =
                        ((float)(state->viewerState->currentPosition.z
                                 - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.z))
                        * state->viewerState->vpConfigs[i].screenPxYPerDataPx
                        + 0.5 * state->viewerState->vpConfigs[i].screenPxYPerDataPx;

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
            case VIEWPORT_ARBITRARY:
                //v1: vector in Viewport x-direction, parameter s corresponds to v1
                //v2: vector in Viewport y-direction, parameter t corresponds to v2

                state->viewerState->vpConfigs[i].s_max = state->viewerState->vpConfigs[i].t_max =
                        (((int)((state->M/2+1)*state->cubeEdgeLength/sqrt(2.)))/2)*2;

                floatCoordinate *v1 = &(state->viewerState->vpConfigs[i].v1);
                floatCoordinate *v2 = &(state->viewerState->vpConfigs[i].v2);
                //Aspect ratio correction..

                //Calculation of new Ratio V1 to V2, V1 is along x

//               float voxelV1V2Ratio;

//                voxelV1V2Ratio =  sqrtf((powf(state->viewerState->voxelXYtoZRatio * state->viewerState->voxelXYRatio * v1->x, 2)

//                                 +  powf(state->viewerState->voxelXYtoZRatio * v1->y, 2) + powf(v1->z, 2))

//                                 / (powf(state->viewerState->voxelXYtoZRatio * state->viewerState->voxelXYRatio * v2->x, 2)

//                                 +  powf(state->viewerState->voxelXYtoZRatio * v2->y, 2) + powf(v2->z, 2)));

//                if(voxelV1V2Ratio < 1)
//                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *= voxelV1V2Ratio;
//                else
//                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /= voxelV1V2Ratio;
                float voxelV1X =
                        sqrtf(powf(v1->x, 2.0) + powf(v1->y / state->viewerState->voxelXYRatio, 2.0)
                        + powf(v1->z / state->viewerState->voxelXYRatio / state->viewerState->voxelXYtoZRatio , 2.0));
                float voxelV2X =
                        sqrtf((powf(v2->x, 2.0) + powf(v2->y / state->viewerState->voxelXYRatio, 2.0)
                        + powf(v2->z / state->viewerState->voxelXYRatio / state->viewerState->voxelXYtoZRatio , 2.0)));

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /= voxelV1X;
                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY /= voxelV2X;

                //Display only entire pixels (only truncation possible!) WHY??

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
                    (float)(
                        (int)(
                            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                            / 2.
                            / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                        )
                        * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                    )
                    * 2.;

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                    (float)(
                        (int)(
                             state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                             / 2.
                             / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                        )
                        * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                    )
                    * 2.;

                // Update screen pixel to data pixel mapping values
                state->viewerState->vpConfigs[i].screenPxXPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength /
                    (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /
                     state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].screenPxYPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength /
                    (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY /
                     state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                // Pixels on the screen per 1 unit in the data coordinate system at the
                // original magnification. These guys appear to be unused!!! jk 14.5.12
                state->viewerState->vpConfigs[i].screenPxXPerOrigMagUnit =
                    state->viewerState->vpConfigs[i].screenPxXPerDataPx *
                    state->magnification;

                state->viewerState->vpConfigs[i].screenPxYPerOrigMagUnit =
                    state->viewerState->vpConfigs[i].screenPxYPerDataPx *
                    state->magnification;

                state->viewerState->vpConfigs[i].displayedlengthInNmX =
                    sqrtf(powf(state->viewerState->voxelDimX * v1->x,2.)
                          + powf(state->viewerState->voxelDimY * v1->y,2.)
                          + powf(state->viewerState->voxelDimZ * v1->z,2.)
                    )
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /
                     state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].displayedlengthInNmY =
                    sqrtf(powf(state->viewerState->voxelDimX * v2->x,2.)
                          + powf(state->viewerState->voxelDimY * v2->y,2.)
                          + powf(state->viewerState->voxelDimZ * v2->z,2.)
                    )
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY /
                    state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                midX = state->viewerState->vpConfigs[i].s_max/2.
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;
                midY = state->viewerState->vpConfigs[i].t_max/2.
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;

                //Update state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen with this call
                calcLeftUpperTexAbsPx();

                //Offsets for crosshair
                state->viewerState->vpConfigs[i].texture.xOffset =
                        midX
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                        * state->viewerState->vpConfigs[i].screenPxXPerDataPx
                        + 0.5
                        * state->viewerState->vpConfigs[i].screenPxXPerDataPx;
                state->viewerState->vpConfigs[i].texture.yOffset =
                        midY
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                        * state->viewerState->vpConfigs[i].screenPxYPerDataPx
                        + 0.5
                        * state->viewerState->vpConfigs[i].screenPxYPerDataPx;
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

bool Viewer::sendLoadSignal(uint x, uint y, uint z, int magChanged) {
    state->protectLoadSignal->lock();
    state->loadSignal = true;  
    state->datasetChangeSignal = magChanged;

    state->previousPositionX = state->currentPositionX;

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
    return true;
}

bool Viewer::moveVPonTop(uint currentVP) {

}

/** Global interfaces  */
void Viewer::rewire() {
    // viewer signals
    connect(this, SIGNAL(updateZoomAndMultiresWidgetSignal()),window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(this, SIGNAL(updateCoordinatesSignal(int,int,int)), window, SLOT(updateCoordinateBar(int,int,int)));
    connect(this, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));
    // end viewer signals
    // skeletonizer signals
    //connect(skeletonizer, SIGNAL(updateToolsSignal()), window->widgetContainer->toolsWidget, SLOT(updateToolsSlot()));
    connect(skeletonizer, SIGNAL(updateToolsSignal()), window->widgetContainer->annotationWidget, SLOT(update()));
    connect(skeletonizer, SIGNAL(updateTreeviewSignal()), window->widgetContainer->annotationWidget->treeviewTab, SLOT(update()));
    connect(skeletonizer, SIGNAL(userMoveSignal(int,int,int,int)), this, SLOT(userMove(int,int,int,int)));
    connect(skeletonizer, SIGNAL(displayModeChangedSignal()),
                    window->widgetContainer->viewportSettingsWidget->skeletonViewportWidget, SLOT(updateDisplayModeRadio()));
    connect(skeletonizer, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));
    connect(skeletonizer, SIGNAL(saveSkeletonSignal()), window, SLOT(saveSlot()));
    // end skeletonizer signals
    //event model signals
   // connect(eventModel, SIGNAL(updateTools()), window->widgetContainer->toolsWidget, SLOT(updateToolsSlot()));
    connect(eventModel, SIGNAL(updateTools()), window->widgetContainer->annotationWidget, SLOT(update()));
    connect(eventModel, SIGNAL(updateTreeviewSignal()), window->widgetContainer->annotationWidget->treeviewTab, SLOT(update()));
    connect(eventModel, SIGNAL(unselectNodesSignal()),
                    window->widgetContainer->annotationWidget->treeviewTab->nodeTable, SLOT(clearSelection()));
    connect(eventModel, SIGNAL(userMoveSignal(int,int,int,int)), this, SLOT(userMove(int,int,int,int)));
    connect(eventModel, SIGNAL(userMoveArbSignal(float,float,float,int)), this, SLOT(userMove_arb(float,float,float,int)));
    connect(eventModel, SIGNAL(zoomOrthoSignal(float)), vpUpperLeft, SLOT(zoomOrthogonals(float)));
    connect(eventModel, SIGNAL(zoomInSkeletonVPSignal()), vpLowerRight, SLOT(zoomInSkeletonVP()));
    connect(eventModel, SIGNAL(zoomOutSkeletonVPSignal()), vpLowerRight, SLOT(zoomOutSkeletonVP()));
    connect(eventModel, SIGNAL(pasteCoordinateSignal()), window, SLOT(pasteClipboardCoordinates())); // TIENITODO event handler??
    connect(eventModel, SIGNAL(updateViewerStateSignal()), this, SLOT(updateViewerState()));
    connect(eventModel, SIGNAL(updatePositionSignal(int)), this, SLOT(updatePosition(int)));
    connect(eventModel, SIGNAL(updateWidgetSignal()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(eventModel, SIGNAL(workModeAddSignal()), window, SLOT(addNodeSlot()));
    connect(eventModel, SIGNAL(workModeLinkSignal()), window, SLOT(linkWithActiveNodeSlot()));
    connect(eventModel, SIGNAL(deleteActiveNodeSignal()), skeletonizer, SLOT(delActiveNode()));
    connect(eventModel, SIGNAL(genTestNodesSignal(uint)), skeletonizer, SLOT(genTestNodes(uint)));
    connect(eventModel, SIGNAL(addSkeletonNodeSignal(Coordinate*,Byte)), skeletonizer, SLOT(UI_addSkeletonNode(Coordinate*,Byte)));
    connect(eventModel, SIGNAL(addSkeletonNodeAndLinkWithActiveSignal(Coordinate*,Byte,int)),
                    skeletonizer, SLOT(addSkeletonNodeAndLinkWithActive(Coordinate*,Byte,int)));
    connect(eventModel, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)),
                    skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(eventModel, SIGNAL(previousCommentlessNodeSignal()), skeletonizer, SLOT(previousCommentlessNode()));
    connect(eventModel, SIGNAL(nextCommentSignal(char*)), skeletonizer, SLOT(nextComment(char*)));
    connect(eventModel, SIGNAL(previousCommentSignal(char*)), skeletonizer, SLOT(previousComment(char*)));
    connect(eventModel, SIGNAL(saveSkeletonSignal()), window, SLOT(saveSlot()));
    connect(eventModel, SIGNAL(delSegmentSignal(int,int,int,segmentListElement*,int)),
                    skeletonizer, SLOT(delSegment(int,int,int,segmentListElement*,int)));
    connect(eventModel, SIGNAL(addSegmentSignal(int,int,int,int)), skeletonizer, SLOT(addSegment(int,int,int,int)));
    connect(eventModel, SIGNAL(editNodeSignal(int,int,nodeListElement*,float,int,int,int,int)),
                    skeletonizer, SLOT(editNode(int,int,nodeListElement*,float,int,int,int,int)));
    connect(eventModel, SIGNAL(findNodeInRadiusSignal(Coordinate)), skeletonizer, SLOT(findNodeInRadius(Coordinate)));
    connect(eventModel, SIGNAL(findSegmentByNodeIDSignal(int,int)), skeletonizer, SLOT(findSegmentByNodeIDs(int,int)));
    connect(eventModel, SIGNAL(findNodeByNodeIDSignal(int)), skeletonizer, SLOT(findNodeByNodeID(int)));
    connect(eventModel, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));
    connect(eventModel, SIGNAL(updateSlicePlaneWidgetSignal()),
                    window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SLOT(updateIntersection()));
    connect(eventModel, SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int,int)),
                    skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int,int)));
    connect(eventModel, SIGNAL(retrieveVisibleObjectBeneathSquareSignal(uint,uint,uint,uint)),
            renderer, SLOT(retrieveVisibleObjectBeneathSquare(uint,uint,uint,uint)));
    connect(eventModel, SIGNAL(undoSignal()), skeletonizer, SLOT(undo()));
    connect(eventModel, SIGNAL(setViewportOrientationSignal(int)), vpUpperLeft, SLOT(setOrientation(int)));
    connect(eventModel, SIGNAL(setViewportOrientationSignal(int)), vpLowerLeft, SLOT(setOrientation(int)));
    connect(eventModel, SIGNAL(setViewportOrientationSignal(int)), vpUpperRight, SLOT(setOrientation(int)));
    //end event handler signals
    // mainwindow signals
    //connect(window, SIGNAL(updateToolsSignal()), window->widgetContainer->toolsWidget, SLOT(updateToolsSlot()));
    connect(window, SIGNAL(updateToolsSignal()), window->widgetContainer->annotationWidget, SLOT(update()));
    connect(window, SIGNAL(updateTreeviewSignal()), window->widgetContainer->annotationWidget->treeviewTab, SLOT(update()));
    connect(window, SIGNAL(userMoveSignal(int, int, int, int)), this, SLOT(userMove(int,int,int,int)));
//    connect(window, SIGNAL(updateCommentsTableSignal()),
//                    window->widgetContainer->commentsWidget->nodeCommentsTab, SLOT(updateCommentsTable()));
    connect(window, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));
    connect(window, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));
    connect(window, SIGNAL(saveSkeletonSignal(QString)), skeletonizer, SLOT(saveXmlSkeleton(QString)));
    connect(window, SIGNAL(loadSkeletonSignal(QString)), skeletonizer, SLOT(loadXmlSkeleton(QString)));
    connect(window, SIGNAL(updateTreeColorsSignal()), skeletonizer, SLOT(updateTreeColors()));
    connect(window, SIGNAL(addTreeListElementSignal(int,int,int,color4F,int)),
                    skeletonizer, SLOT(addTreeListElement(int,int,int,color4F,int)));
    connect(window, SIGNAL(stopRenderTimerSignal()), timer, SLOT(stop()));
    connect(window, SIGNAL(startRenderTimerSignal(int)), timer, SLOT(start(int)));
    connect(window, SIGNAL(nextCommentSignal(char*)), skeletonizer, SLOT(nextComment(char*)));
    connect(window, SIGNAL(previousCommentSignal(char*)), skeletonizer, SLOT(previousComment(char*)));
    connect(window, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));
    connect(window, SIGNAL(clearSkeletonSignal(int,int)), skeletonizer, SLOT(clearSkeleton(int,int)));
    connect(window, SIGNAL(updateSkeletonFileNameSignal(int,int,char*)),
                    skeletonizer, SLOT(updateSkeletonFileName(int,int,char*)));
    connect(window, SIGNAL(moveToNextNodeSignal()), skeletonizer, SLOT(moveToNextNode()));
    connect(window, SIGNAL(moveToPrevNodeSignal()), skeletonizer, SLOT(moveToPrevNode()));
    connect(window, SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int,int)),
                    skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int,int)));
    connect(window, SIGNAL(popBranchNodeSignal()), skeletonizer, SLOT(UI_popBranchNode()));
    connect(window, SIGNAL(jumpToActiveNodeSignal()), skeletonizer, SLOT(jumpToActiveNode()));
    connect(window, SIGNAL(moveToPrevTreeSignal()), skeletonizer, SLOT(moveToPrevTree()));
    connect(window, SIGNAL(moveToNextTreeSignal()), skeletonizer, SLOT(moveToNextTree()));
    connect(window, SIGNAL(addCommentSignal(int,const char*,nodeListElement*,int,int)),
                    skeletonizer, SLOT(addComment(int,const char*,nodeListElement*,int,int)));
    connect(window, SIGNAL(editCommentSignal(int,commentListElement*,int,char*,nodeListElement*,int,int)),
                    skeletonizer, SLOT(editComment(int,commentListElement*,int,char*,nodeListElement*,int,int)));
    connect(window, SIGNAL(updateTaskDescriptionSignal(QString)),
                    window->widgetContainer->taskManagementWidget->detailsTab, SLOT(setDescription(QString)));
    connect(window, SIGNAL(updateTaskCommentSignal(QString)),
                    window->widgetContainer->taskManagementWidget->detailsTab, SLOT(setComment(QString)));
    //end mainwindow signals
    //viewport signals
    connect(vpUpperLeft, SIGNAL(updateZoomAndMultiresWidget()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(vpLowerLeft, SIGNAL(updateZoomAndMultiresWidget()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(vpUpperRight, SIGNAL(updateZoomAndMultiresWidget()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(vpLowerRight, SIGNAL(updateZoomAndMultiresWidget()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));

    connect(vpUpperLeft, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));
    connect(vpLowerLeft, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));
    connect(vpUpperRight, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));
    connect(vpLowerRight, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));

    connect(vpUpperLeft, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));
    connect(vpLowerLeft, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));
    connect(vpUpperRight, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));
    connect(vpLowerRight, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));
    // end viewport signals

    // --- widget signals ---
    //  tools widget signals --
//    connect(window->widgetContainer->toolsWidget, SIGNAL(findTreeByTreeIDSignal(int)), skeletonizer, SLOT(findTreeByTreeID(int)));
//    connect(window->widgetContainer->toolsWidget, SIGNAL(findNodeByNodeIDSignal(int)), skeletonizer, SLOT(findNodeByNodeID(int)));
//    connect(window->widgetContainer->toolsWidget, SIGNAL(setActiveTreeSignal(int)), skeletonizer, SLOT(setActiveTreeByID(int)));
//    connect(window->widgetContainer->toolsWidget, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)),
//                    skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
//    connect(window->widgetContainer->toolsWidget, SIGNAL(addCommentSignal(int,const char*,nodeListElement*,int,int)),
//                    skeletonizer, SLOT(addComment(int,const char*,nodeListElement*,int,int)));
//    connect(window->widgetContainer->toolsWidget,
//                    SIGNAL(editCommentSignal(int,commentListElement*,int,char*,nodeListElement*,int,int)),
//                    skeletonizer, SLOT(editComment(int,commentListElement*,int,char*,nodeListElement*,int,int)));
//    connect(window->widgetContainer->toolsWidget, SIGNAL(nextCommentSignal(char*)), skeletonizer, SLOT(nextComment(char*)));
//    connect(window->widgetContainer->toolsWidget, SIGNAL(previousCommentSignal(char*)),
//                    skeletonizer, SLOT(previousComment(char*)));
//    //  tools quick tab signals
//    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(popBranchNodeSignal()),
//                    skeletonizer, SLOT(UI_popBranchNode()));
//    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget,
//                    SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int,int)),
//                    skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int,int)));
//    //  tools trees tab signals
//    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(delActiveTreeSignal()),
//                    skeletonizer, SLOT(delActiveTree()));
//    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(restoreDefaultTreeColorSignal()),
//                    skeletonizer, SLOT(restoreDefaultTreeColor()));
//    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(splitConnectedComponent(int,int, int)),
//                    skeletonizer, SLOT(splitConnectedComponent(int,int,int)));
//    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(addTreeListElement(int,int,int,color4F, int)),
//                    skeletonizer, SLOT(addTreeListElement(int,int,int,color4F,int)));
//    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(addTreeComment(int,int,char*)),
//                    skeletonizer, SLOT(addTreeComment(int,int,char*)));
//    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(mergeTrees(int,int,int,int)),
//                    skeletonizer, SLOT(mergeTrees(int,int,int,int)));
//    //  tools nodes tab signals
//    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(jumpToNodeSignal()),
//                    skeletonizer, SLOT(jumpToActiveNode()));
//    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(lockPositionSignal(Coordinate)),
//                    skeletonizer, SLOT(lockPosition(Coordinate)));
//    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(unlockPositionSignal()),
//                    skeletonizer, SLOT(unlockPosition()));
//    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(updatePositionSignal(int)),
//                    this, SLOT(updatePosition(int)));
//    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(deleteActiveNodeSignal()),
//                    skeletonizer, SLOT(delActiveNode()));
//    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget,
//                    SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)),
//                    skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
//    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(findNodeByNodeIDSignal(int)),
//                    skeletonizer, SLOT(findNodeByNodeID(int)));
    //  -- end tools widget signals
    //  tools widget signals --
    //  treeview tab signals
    connect(window->widgetContainer->annotationWidget->treeviewTab, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)),
            skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(window->widgetContainer->annotationWidget->treeviewTab->nodeTable, SIGNAL(delActiveNodeSignal()),
                    skeletonizer, SLOT(delActiveNode()));
    connect(window->widgetContainer->annotationWidget->treeviewTab, SIGNAL(JumpToActiveNodeSignal()),
                    skeletonizer, SLOT(jumpToActiveNode()));
    connect(window->widgetContainer->annotationWidget->treeviewTab, SIGNAL(addSegmentSignal(int,int,int,int)),
                    skeletonizer, SLOT(addSegment(int,int,int,int)));
    // commands tab signals
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(findTreeByTreeIDSignal(int)),
                    skeletonizer, SLOT(findTreeByTreeID(int)));
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(findNodeByNodeIDSignal(int)),
                    skeletonizer, SLOT(findNodeByNodeID(int)));
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(setActiveTreeSignal(int)),
                    skeletonizer, SLOT(setActiveTreeByID(int)));
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)),
                    skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(jumpToNodeSignal()),
                    skeletonizer, SLOT(jumpToActiveNode()));
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(updateTreeviewSignal()),
                    window->widgetContainer->annotationWidget->treeviewTab, SLOT(update()));
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(addTreeListElement(int,int,int,color4F,int)),
                    skeletonizer, SLOT(addTreeListElement(int,int,int,color4F,int)));
    connect(window->widgetContainer->annotationWidget->commandsTab,
                    SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int,int)),
                    skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int,int)));
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(popBranchNodeSignal()),
                    skeletonizer, SLOT(UI_popBranchNode()));
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(lockPositionSignal(Coordinate)),
                    skeletonizer, SLOT(lockPosition(Coordinate)));
    connect(window->widgetContainer->annotationWidget->commandsTab, SIGNAL(unlockPositionSignal()),
                    skeletonizer, SLOT(unlockPosition()));
    //  -- end tools widget signals
    //  viewport settings widget signals --
    //  general vp settings tab signals
    connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, SIGNAL(overrideNodeRadiusSignal(bool)),
                    skeletonizer, SLOT(setOverrideNodeRadius(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, SIGNAL(segRadiusToNodeRadiusSignal(float)),
                    skeletonizer, SLOT(setSegRadiusToNodeRadius(float)));
    connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, SIGNAL(skeletonChangedSignal(bool)),
                    skeletonizer, SLOT(setSkeletonChanged(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, SIGNAL(showNodeID(bool)),
                    skeletonizer, SLOT(setShowNodeIDs(bool)));
    //  slice plane vps tab signals
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SIGNAL(showIntersectionsSignal(bool)),
                    skeletonizer, SLOT(setShowIntersections(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget,
                    SIGNAL(treeColorAdjustmentsChangedSignal()),
                    window, SLOT(treeColorAdjustmentsChanged()));
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget,
                    SIGNAL(loadTreeColorTableSignal(QString,float*,int)),
                    this, SLOT(loadTreeColorTable(QString,float*,int)));
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget,
                    SIGNAL(loadDataSetColortableSignal(QString,GLuint*,int)),
                    this, SLOT(loadDatasetColorTable(QString,GLuint*,int)));
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget,
                    SIGNAL(updateViewerStateSignal()),
                    this, SLOT(updateViewerState()));
    //  skeleton vp tab signals
    connect(window->widgetContainer->viewportSettingsWidget->skeletonViewportWidget,SIGNAL(showXYPlaneSignal(bool)),
                    skeletonizer, SLOT(setShowXyPlane(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->skeletonViewportWidget,
                    SIGNAL(rotateAroundActiveNodeSignal(bool)),
                    skeletonizer, SLOT(setRotateAroundActiveNode(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->skeletonViewportWidget, SIGNAL(updateViewerStateSignal()),
                    this, SLOT(updateViewerState()));
    //  -- end viewport settings widget signals
    //  zoom and multires signals --
    connect(window->widgetContainer->zoomAndMultiresWidget, SIGNAL(zoomInSkeletonVPSignal()), vpLowerRight, SLOT(zoomInSkeletonVP()));
    connect(window->widgetContainer->zoomAndMultiresWidget, SIGNAL(zoomOutSkeletonVPSignal()), vpLowerRight, SLOT(zoomOutSkeletonVP()));
    connect(window->widgetContainer->zoomAndMultiresWidget, SIGNAL(zoomLevelSignal(float)),
                    skeletonizer, SLOT(setZoomLevel(float)));
    //  -- end zoom and multires signals
    // comments widget signals --
//    connect(window->widgetContainer->commentsWidget->nodeCommentsTab, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)),
//                    skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
//    connect(window->widgetContainer->commentsWidget->nodeCommentsTab, SIGNAL(setJumpToActiveNodeSignal()),
//                    skeletonizer, SLOT(jumpToActiveNode()));
//    connect(window->widgetContainer->commentsWidget->nodeCommentsTab, SIGNAL(findNodeByNodeIDSignal(int)),
//                    skeletonizer, SLOT(findNodeByNodeID(int)));
    // -- end comments widget signals
    // dataset property signals --
    connect(window->widgetContainer->datasetPropertyWidget, SIGNAL(clearSkeletonSignal()), window, SLOT(clearSkeletonSlot()));
    // -- end dataset property signals
    // task management signals --
    connect(window->widgetContainer->taskManagementWidget->mainTab, SIGNAL(loadSkeletonSignal(const QString)),
                    window, SLOT(loadSkeletonAfterUserDecision(const QString)));
    connect(window->widgetContainer->taskManagementWidget->mainTab, SIGNAL(saveSkeletonSignal()), window, SLOT(saveSlot()));
    // -- end task management signals
    // --- end widget signals
}

bool Viewer::getDirectionalVectors(float alpha, float beta, floatCoordinate *v1, floatCoordinate *v2, floatCoordinate *v3) {

        //alpha: rotation around z-axis
        //beta: rotation around new (rotated) y-axis
        alpha = alpha * (float)PI/180;
        beta = beta * (float)PI/180;

        SET_COORDINATE((*v1), (cos(alpha)*cos(beta)), (sin(alpha)*cos(beta)), (-sin(beta)));
        SET_COORDINATE((*v2), (-sin(alpha)), (cos(alpha)), (0));
        SET_COORDINATE((*v3), (cos(alpha)*sin(beta)), (sin(alpha)*sin(beta)), (cos(beta)));

        return true;
}

/** The platform decisions are unfortunately neccessary because the behaviour of the event handling
 *  differs from platform to platform.
 *  The details here are that key events are recognized by settings flags in the event handler. At the begin of the method run()
 *  we are checking if the keys F or D are still pressed. In this case the new coordinates (which were already calculated in the event-handler) will be passed
 *  to the the method userMove which is from here a direct call, that has no signal and slot delay. This prevents two things. First of all it prevents a strange effect unter windows
 *  that pressing F or D and fast mouse movements had led to delayed mouse event processing. The second thing is that there was a delay between the userMoveSignal from the eventhandler
 *  and the processing of the userMove Slot. The run method was called but the new position was first available in the next frame. Thus rendering an "empty" frame could be prevented.
 *
*/
bool Viewer::processUserMove() {
    if(state->keyF or state->keyD) {
        int time = delay.elapsed();

#ifndef Q_OS_MAC
        if(time > 200) {
            delay.restart();
        }
#endif
#ifdef Q_OS_MAC
        state->autorepeat = true;
#endif
        if(time >= 200 and !state->autorepeat) {
            userMove(state->newCoord[0], state->newCoord[1], state->newCoord[2], TELL_COORDINATE_CHANGE);
        } else if(state->autorepeat) {
            userMove(state->newCoord[0], state->newCoord[1], state->newCoord[2], TELL_COORDINATE_CHANGE);
        }
    }
}
