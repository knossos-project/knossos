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
#include <fstream>

#include <QDebug>
#include <qopengl.h>
#include <QtConcurrent/QtConcurrentRun>

#include "functions.h"
#include "knossos.h"
#include "renderer.h"
#include "segmentation.h"
#include "skeletonizer.h"
#include "viewer.h"
#include "widgets/mainwindow.h"
#include "widgets/viewport.h"
#include "widgets/widgetcontainer.h"

#if defined(Q_OS_WIN)
#include <GL/wglext.h>
#elif defined(Q_OS_LINUX)
#define WINAPI
#include <GL/glx.h>
#include <GL/glxext.h>
#endif

static WINAPI int dummy(int) {
    return 0;
}

extern stateInfo *state;

Viewer::Viewer(QObject *parent) :
    QThread(parent)
{

    window = new MainWindow();
    window->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    vpUpperLeft = window->viewports[VIEWPORT_XY].get();
    vpLowerLeft = window->viewports[VIEWPORT_XZ].get();
    vpUpperRight = window->viewports[VIEWPORT_YZ].get();
    vpLowerRight = window->viewports[VIEWPORT_SKELETON].get();
    eventModel = new EventModel();
    vpUpperLeft->eventDelegate = vpLowerLeft->eventDelegate = vpUpperRight->eventDelegate = vpLowerRight->eventDelegate = eventModel;

    timer = new QTimer();

    /* order of the initialization of the rendering system is
     * 1. initViewer
     * 2. new Skeletonizer
     * 3. new Renderer
     * 4. Load the GUI-Settings (otherwise the initialization of the skeletonizer or the renderer would overwrite some variables)
    */

    initViewer();
    skeletonizer = new Skeletonizer();
    renderer = new Renderer();

    QDesktopWidget *desktop = QApplication::desktop();

    state->viewer = this;
    rewire();
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

    state->viewerState->renderInterval = FAST;
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

    if (
            (currentPxInDc.x < 0) ||
            (currentPxInDc.y < 0) ||
            (currentPxInDc.z < 0) ||
            (currentPxInDc.x >= state->cubeEdgeLength) ||
            (currentPxInDc.y >= state->cubeEdgeLength) ||
            (currentPxInDc.z >= state->cubeEdgeLength)
            )
    {
        sliceIndex = 3 * ( s + *t  *  state->cubeEdgeLength * state->M);
        slice[sliceIndex] = slice[sliceIndex + 1]
                          = slice[sliceIndex + 2]
                          = state->viewerState->defaultTexData[0];
        (*t)++;
        if(*t < viewPort->t_max) {
            // Actually, although currentPxInDc_float is passed by reference and updated here,
            // it is totally ignored (i.e. not read, then overwritten) by the calling function.
            // But to keep the functionality here compatible after this bugfix, we also replicate
            // this update here - from the originial below
            ADD_COORDINATE(*currentPxInDc_float, *v2);
        }
        return true;
    }

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

    if (
            (currentPxInDc.x < 0) ||
            (currentPxInDc.y < 0) ||
            (currentPxInDc.z < 0) ||
            (currentPxInDc.x >= state->cubeEdgeLength) ||
            (currentPxInDc.y >= state->cubeEdgeLength) ||
            (currentPxInDc.z >= state->cubeEdgeLength)
            )
    {
        sliceIndex = 3 * ( s + *t  *  state->cubeEdgeLength * state->M);
        slice[sliceIndex] = slice[sliceIndex + 1]
                          = slice[sliceIndex + 2]
                          = state->viewerState->defaultTexData[0];
        (*t)++;
        if(*t < viewPort->t_max) {
            // Actually, although currentPxInDc_float is passed by reference and updated here,
            // it is totally ignored (i.e. not read, then overwritten) by the calling function.
            // But to keep the functionality here compatible after this bugfix, we also replicate
            // this update here - from the originial below
            ADD_COORDINATE(*currentPxInDc_float, *v2);
        }
        return true;
    }

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
        else {
            slice[sliceIndex] = state->viewerState->datasetAdjustmentTable[0][datacube[dcIndex]];
            slice[sliceIndex + 1] = state->viewerState->datasetAdjustmentTable[1][datacube[dcIndex]];
            slice[sliceIndex + 2] = state->viewerState->datasetAdjustmentTable[2][datacube[dcIndex]];
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


/**
 * @brief Viewer::ocSliceExtract extracts subObject IDs from datacube
 *      and paints slice at the corresponding position with a color depending on the ID.
 * @param datacube pointer to the datacube for data extraction
 * @param slice pointer to a slice in which to draw the overlay
 *
 * In the first pass all pixels are filled with the color corresponding the subObject-ID.
 * In the second pass the datacube is traversed again to find edge voxels, i.e. all voxels
 * where at least one of their neighbors (left, right, top, bot) have a different ID than their own.
 * The opacity of these voxels is slightly increased to highlight the edges.
 *
 */
void Viewer::ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    Byte *cubePtr = datacube + dcOffset;
    Byte *slicePtr = slice;
    // configure variable increments depending on viewport type. Note that additional outer loop only exists for xz vp!
    // configuration for xy vp
    int innerLoopBoundary = state->cubeSliceArea;
    int outerLoopBoundary = 1;
    int outerCubeIncrement = 0;
    int innerCubeIncrement = OBJID_BYTES;
    int horizNBdistance = OBJID_BYTES;
    int vertNBdistance = state->cubeEdgeLength * OBJID_BYTES;
    switch(vpConfig->type) {
    case SLICE_XZ:
        outerLoopBoundary = state->cubeEdgeLength;
        innerLoopBoundary = state->cubeEdgeLength;
        outerCubeIncrement = state->cubeSliceArea*OBJID_BYTES - state->cubeEdgeLength*OBJID_BYTES;
        vertNBdistance = state->cubeSliceArea*OBJID_BYTES;
        break;
    case SLICE_YZ:
        horizNBdistance = state->cubeEdgeLength * OBJID_BYTES;
        vertNBdistance = state->cubeSliceArea * OBJID_BYTES;
        innerCubeIncrement = state->cubeEdgeLength * OBJID_BYTES;
        break;
    }

    // draw overlay in first pass
    auto &seg = Segmentation::singleton();
    uint64_t idCache = 0;
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> colorCache;
    for(int j = 0; j < outerLoopBoundary; j++) { // for xy and yz vp outer loop is only visited once
        for(int i = 0; i < innerLoopBoundary; i++) {
            uint64_t subObjectID;
            memcpy(&subObjectID, cubePtr, sizeof(subObjectID));

            std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> color;
            color = (idCache == subObjectID)? colorCache : seg.colorObjectFromId(subObjectID);

            slicePtr[0] = std::get<0>(color);
            slicePtr[1] = std::get<1>(color);
            slicePtr[2] = std::get<2>(color);
            slicePtr[3] = std::get<3>(color);

            idCache = subObjectID;
            colorCache = color;

            cubePtr += innerCubeIncrement;
            slicePtr += 4;
        }
        cubePtr += outerCubeIncrement;
    }
    // highlight edges in a second pass
    cubePtr = datacube + dcOffset;
    slicePtr = slice;
    Byte *datacubeEnd = datacube + state->cubeEdgeLength * state->cubeSliceArea * OBJID_BYTES;

    for(int j = 0; j < outerLoopBoundary; j++) {
        for(int i = 0; i < innerLoopBoundary; i++) {
            uint64_t subObjectID;
            memcpy(&subObjectID, cubePtr, sizeof(subObjectID));
            auto & segmentation = Segmentation::singleton();
            if (segmentation.subobjectExists(subObjectID)) {//don’t create objects from subobjects here
                const auto & subobject = segmentation.subobjectFromId(subObjectID);
                if (segmentation.isSelected(subobject)) {
                    uint64_t left = subObjectID;
                    uint64_t right = subObjectID;
                    uint64_t top = subObjectID;
                    uint64_t bot = subObjectID;

                    if(cubePtr - horizNBdistance - datacube >= 0) {
                        memcpy(&left, cubePtr - horizNBdistance, sizeof(left));
                    }
                    if(cubePtr + horizNBdistance - datacubeEnd <= 0) {
                        memcpy(&right, cubePtr + horizNBdistance, sizeof(right));
                    }
                    if(cubePtr - vertNBdistance - datacube >= 0) {
                        memcpy(&top, cubePtr - vertNBdistance, sizeof(top));
                    }
                    if(cubePtr + vertNBdistance - datacubeEnd <= 0) {
                        memcpy(&bot, cubePtr + vertNBdistance, sizeof(bot));
                    }
                    if(subObjectID !=left || subObjectID != right || subObjectID != top || subObjectID != bot) {
                        slicePtr[3] = (slicePtr[3]*4 > 255)? 255 : slicePtr[3]*4;
                    }
                }
            }
            cubePtr += innerCubeIncrement;
            slicePtr += 4;
        }
        cubePtr += outerCubeIncrement;
    }
}

void Viewer::ocSliceExtractUnique(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    datacube += dcOffset;

    switch(vpConfig->type) {
    case SLICE_XY:
        for(int i = 0; i < state->cubeSliceArea; i++) {
            uint64_t subObjectID;
            memcpy(&subObjectID, datacube, sizeof(subObjectID));

            const auto color = Segmentation::singleton().colorUniqueFromId(subObjectID);
            slice[0] = std::get<0>(color);
            slice[1] = std::get<1>(color);
            slice[2] = std::get<2>(color);
            slice[3] = std::get<3>(color);

            datacube += OBJID_BYTES;
            slice += 4;
        }
        break;
    case SLICE_XZ:
        for(int j = 0; j < state->cubeEdgeLength; j++) {
            for(int i = 0; i < state->cubeEdgeLength; i++) {
                uint64_t subObjectID;
                memcpy(&subObjectID, datacube, sizeof(subObjectID));

                const auto color = Segmentation::singleton().colorUniqueFromId(subObjectID);
                slice[0] = std::get<0>(color);
                slice[1] = std::get<1>(color);
                slice[2] = std::get<2>(color);
                slice[3] = std::get<3>(color);

                datacube += OBJID_BYTES;
                slice += 4;
            }

            datacube = datacube
                       + state->cubeSliceArea * OBJID_BYTES
                       - state->cubeEdgeLength * OBJID_BYTES;
        }
        break;
    case SLICE_YZ:
        for(int i = 0; i < state->cubeSliceArea; i++) {
            uint64_t subObjectID;
            memcpy(&subObjectID, datacube, sizeof(subObjectID));

            const auto color = Segmentation::singleton().colorUniqueFromId(subObjectID);
            slice[0] = std::get<0>(color);
            slice[1] = std::get<1>(color);
            slice[2] = std::get<2>(color);
            slice[3] = std::get<3>(color);

            datacube += state->cubeEdgeLength * OBJID_BYTES;
            slice += 4;
        }
        break;
    }
}

static int texIndex(uint x, uint y, uint colorMultiplicationFactor, viewportTexture *texture) {
    uint index = 0;

    index = x * state->cubeSliceArea + y
            * (texture->edgeLengthDc * state->cubeSliceArea)
            * colorMultiplicationFactor;

    return index;
}

bool Viewer::vpGenerateTexture(vpConfig &currentVp) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.

    uint x_px = 0, x_dc = 0, y_px = 0, y_dc = 0;
    Coordinate upperLeftDc, currentDc, currentPosition_dc;
    Coordinate currPosTrans, leftUpperPxInAbsPxTrans;

    Byte *datacube = NULL, *overlayCube = NULL;
    int dcOffset = 0, index = 0;

    CPY_COORDINATE(currPosTrans, state->viewerState->currentPosition);
    DIV_COORDINATE(currPosTrans, state->magnification);

    CPY_COORDINATE(leftUpperPxInAbsPxTrans, currentVp.texture.leftUpperPxInAbsPx);
    DIV_COORDINATE(leftUpperPxInAbsPxTrans, state->magnification);

    currentPosition_dc = Coordinate::Px2DcCoord(currPosTrans, state->cubeEdgeLength);

    upperLeftDc = Coordinate::Px2DcCoord(leftUpperPxInAbsPxTrans, state->cubeEdgeLength);

    // We calculate the coordinate of the DC that holds the slice that makes up the upper left
    // corner of our texture.
    // dcOffset is the offset by which we can index into a datacube to extract the first byte of
    // slice relevant to the texture for this viewport.
    //
    // Rounding should be explicit!
    switch(currentVp.type) {
    case SLICE_XY:
        dcOffset = state->cubeSliceArea
                   * (currPosTrans.z - state->cubeEdgeLength
                   * currentPosition_dc.z);
        break;
    case SLICE_XZ:
        dcOffset = state->cubeEdgeLength
                   * (currPosTrans.y  - state->cubeEdgeLength
                   * currentPosition_dc.y);
        break;
    case SLICE_YZ:
        dcOffset = currPosTrans.x - state->cubeEdgeLength
                   * currentPosition_dc.x;
        break;
    default:
        qDebug("No such slice view: %d.", currentVp.type);
        return false;
    }
    // We iterate over the texture with x and y being in a temporary coordinate
    // system local to this texture.
    for(x_dc = 0; x_dc < currentVp.texture.usedTexLengthDc; x_dc++) {
        for(y_dc = 0; y_dc < currentVp.texture.usedTexLengthDc; y_dc++) {
            x_px = x_dc * state->cubeEdgeLength;
            y_px = y_dc * state->cubeEdgeLength;

            switch(currentVp.type) {
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
                qDebug("No such slice type (%d) in vpGenerateTexture.", currentVp.type);
            }

            state->protectCube2Pointer->lock();
            datacube = Hashtable::ht_get(state->Dc2Pointer[int_log(state->magnification)], currentDc);
            overlayCube = Hashtable::ht_get(state->Oc2Pointer[int_log(state->magnification)], currentDc);
            state->protectCube2Pointer->unlock();


            // Take care of the data textures.

            glBindTexture(GL_TEXTURE_2D,
                          currentVp.texture.texHandle);

            // This is used to index into the texture. overlayData[index] is the first
            // byte of the datacube slice at position (x_dc, y_dc) in the texture.
            index = texIndex(x_dc, y_dc, 3, &(currentVp.texture));

            if(datacube == HT_FAILURE || state->viewerState->uniqueColorMode) {
                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                x_px,
                                y_px,
                                state->cubeEdgeLength,
                                state->cubeEdgeLength,
                                GL_RGB,
                                GL_UNSIGNED_BYTE,
                                state->viewerState->defaultTexData);
            } else {
                dcSliceExtract(datacube,
                               state->viewerState->texData + index,
                               dcOffset,
                               &currentVp);
                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                x_px,
                                y_px,
                                state->cubeEdgeLength,
                                state->cubeEdgeLength,
                                GL_RGB,
                                GL_UNSIGNED_BYTE,
                                state->viewerState->texData + index);
            }
            //Take care of the overlay textures.
            if(state->overlay) {
                glBindTexture(GL_TEXTURE_2D, currentVp.texture.overlayHandle);
                // This is used to index into the texture. texData[index] is the first
                // byte of the datacube slice at position (x_dc, y_dc) in the texture.
                index = texIndex(x_dc, y_dc, 4, &(currentVp.texture));

                if(overlayCube == HT_FAILURE) {
                    glTexSubImage2D(GL_TEXTURE_2D,
                                    0,
                                    x_px,
                                    y_px,
                                    state->cubeEdgeLength,
                                    state->cubeEdgeLength,
                                    GL_RGBA,
                                    GL_UNSIGNED_BYTE,
                                    state->viewerState->defaultOverlayData);
                }
                else {
                    if(state->viewerState->uniqueColorMode) {
                        ocSliceExtractUnique(overlayCube,
                                             state->viewerState->overlayData + index,
                                             dcOffset * OBJID_BYTES,
                                             &currentVp);
                    } else {
                        ocSliceExtract(overlayCube,
                                       state->viewerState->overlayData + index,
                                       dcOffset * OBJID_BYTES,
                                       &currentVp);
                    }

                    glTexSubImage2D(GL_TEXTURE_2D,
                                    0,
                                    x_px,
                                    y_px,
                                    state->cubeEdgeLength,
                                    state->cubeEdgeLength,
                                    GL_RGBA,
                                    GL_UNSIGNED_BYTE,
                                    state->viewerState->overlayData + index);
                }
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool Viewer::vpGenerateTexture_arb(vpConfig &currentVp) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.
    Coordinate currentDc, currentPx;
    floatCoordinate currentPxInDc_float, rowPx_float, currentPx_float;

    Byte *datacube = NULL;

    floatCoordinate *v1 = &currentVp.v1;
    floatCoordinate *v2 = &currentVp.v2;
    CPY_COORDINATE(rowPx_float, currentVp.texture.leftUpperPxInAbsPx);
    DIV_COORDINATE(rowPx_float, state->magnification);
    CPY_COORDINATE(currentPx_float, rowPx_float);

    glBindTexture(GL_TEXTURE_2D, currentVp.texture.texHandle);

    int s = 0, t = 0, t_old = 0;
    while(s < currentVp.s_max) {
        t = 0;
        while(t < currentVp.t_max) {
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
            datacube = Hashtable::ht_get(state->Dc2Pointer[int_log(state->magnification)], currentDc);
            state->protectCube2Pointer->unlock();

            SET_COORDINATE(currentPxInDc_float, currentPx_float.x-currentDc.x*state->cubeEdgeLength,
                                                currentPx_float.y-currentDc.y*state->cubeEdgeLength,
                                                currentPx_float.z-currentDc.z*state->cubeEdgeLength);
            t_old = t;
            dcSliceExtract_arb(datacube,
                               &currentVp,
                               &currentPxInDc_float,
                               s, &t);
            SET_COORDINATE(currentPx_float, currentPx_float.x + v2->x * (t - t_old),
                                            currentPx_float.y + v2->y * (t - t_old),
                                            currentPx_float.z + v2->z * (t - t_old));
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
                    currentVp.viewPortData);

    glBindTexture(GL_TEXTURE_2D, 0);

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

    currentPosition_dc = Coordinate::Px2DcCoord(currPosTrans, state->cubeEdgeLength);

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
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x >1000000){ qDebug("uninit x %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x);}
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y > 1000000){ qDebug("uninit y %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y);}
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z > 1000000){ qDebug("uninit z %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z);}

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
                           viewerState->currentPosition.x - state->magnification * (v1.x * viewerState->vpConfigs[i].s_max/2 + v2.x * viewerState->vpConfigs[i].t_max/2),
                           viewerState->currentPosition.y - state->magnification * (v1.y * viewerState->vpConfigs[i].s_max/2 + v2.y * viewerState->vpConfigs[i].t_max/2),
                           viewerState->currentPosition.z - state->magnification * (v1.z * viewerState->vpConfigs[i].s_max/2 + v2.z * viewerState->vpConfigs[i].t_max/2));

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

    if (state->overlay) {
        Segmentation::singleton().loadOverlayLutFromFile();
    }

    // This is the buffer that holds the actual texture data (for _all_ textures)

    state->viewerState->texData =
            (Byte *) malloc(TEXTURE_EDGE_LEN
               * TEXTURE_EDGE_LEN
               * sizeof(Byte)
               * 3);
    if(state->viewerState->texData == NULL) {
        qDebug() << "Out of memory.";
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
            qDebug() << "Out of memory.";
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
        qDebug() << "Out of memory.";
        _Exit(false);
    }
    memset(state->viewerState->defaultTexData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                     * sizeof(Byte)
                                                     * 3);

    /* @arb */
    for (std::size_t i = 0; i < state->viewerState->numberViewports; ++i){
        state->viewerState->vpConfigs[i].viewPortData = (Byte *)malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 3);
        if(state->viewerState->vpConfigs[i].viewPortData == NULL) {
            qDebug() << "Out of memory.";
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
            qDebug() << "Out of memory.";
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

    std::string path_stdstr = path.toStdString();
    const char *cpath = path_stdstr.c_str();

    qDebug("Reading Dataset LUT at %s\n", cpath);

    lutFile = fopen(cpath, "rb");
    if(lutFile == NULL) {
        qDebug("Unable to open Dataset LUT at %s.", cpath);
        return false;
    }

    if(type == GL_RGB) {
        size = RGB_LUTSIZE;
    }
    else if(type == GL_RGBA) {
        size = RGBA_LUTSIZE;
    }
    else {
        qDebug("Requested color type %x does not exist.", type);
        return false;
    }

    readBytes = (uint)fread(lutBuffer, 1, size, lutFile);
    if(readBytes != size) {
        qDebug("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, cpath, size);
        if(fclose(lutFile) != 0) {
            qDebug() << "Additionally, an error occured closing the file.";
        }
        return false;
    }

    if(fclose(lutFile) != 0) {
        qDebug() << "Error closing LUT file.";
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

    std::string path_stdstr = path.toStdString();
    const char *cpath = path_stdstr.c_str();
    // The b is for compatibility with non-UNIX systems and denotes a
    // binary file.
    qDebug("Reading Tree LUT at %s\n", cpath);

    lutFile = fopen(cpath, "rb");
    if(lutFile == NULL) {
        qDebug("Unable to open Tree LUT at %s.", cpath);
        return false;
    }
    if(type != GL_RGB) {
        /* AG_TextError("Tree colors only support RGB colors. Your color type is: %x", type); */
        qDebug("Chosen color was of type %x, but expected GL_RGB", type);
        return false;
    }

    readBytes = (uint)fread(lutBuffer, 1, size, lutFile);
    if(readBytes != size) {
        qDebug("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, cpath, size);
        if(fclose(lutFile) != 0) {
            qDebug() << "Additionally, an error occured closing the file.";
        }
        return false;
    }
    if(fclose(lutFile) != 0) {
        qDebug() << "Error closing LUT file.";
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
            if (static_cast<uint>(state->magnification) > state->lowestAvailableMag) {
                state->magnification /= 2;
                for(i = 0; i < state->viewerState->numberViewports; i++) {
                    if(state->viewerState->vpConfigs[i].type != (uint)VIEWPORT_SKELETON) {
                        state->viewerState->vpConfigs[i].texture.zoomLevel *= 2.0;
                        state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx *= 2.;
                    }
                }
            }
            else return false;
            break;

        case MAG_UP:
            if (static_cast<uint>(state->magnification)  < state->highestAvailableMag) {
                state->magnification *= 2;
                for(i = 0; i < state->viewerState->numberViewports; i++) {
                    if(state->viewerState->vpConfigs[i].type != (uint)VIEWPORT_SKELETON) {
                        state->viewerState->vpConfigs[i].texture.zoomLevel *= 0.5;
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
            qDebug("left upper tex px of VP %d is: %d, %d, %d",i,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z);
        }
    }*/
    sendLoadSignal(state->viewerState->currentPosition.x,
                            state->viewerState->currentPosition.y,
                            state->viewerState->currentPosition.z,
                            upOrDownFlag);

    /* set flags to trigger the necessary renderer updates */
    state->skeletonState->skeletonChanged = true;

    emit updateDatasetOptionsWidgetSignal();

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
    if (state->quitSignal) {//don’t do shit, when the rest is already going to sleep
        qDebug() << "viewer returned";
        return;
    }

    //start the timer before the rendering, else render interval and actual rendering time would accumulate
    timer->singleShot(state->viewerState->renderInterval, this, SLOT(run()));

    static QElapsedTimer baseTime;
    if (state->viewerKeyRepeat && (state->keyF || state->keyD)) {
        qint64 interval = 1000 / state->viewerState->stepsPerSec;
        if (baseTime.elapsed() >= interval) {
            baseTime.restart();
            if (state->viewerState->vpConfigs[0].type != VIEWPORT_ARBITRARY) {
                userMove(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2]);
            } else {
                userMove_arb(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2]);
            }
        }
    }
    // Event and rendering loop.
    // What happens is that we go through lists of pending texture parts and load
    // them if they are available.
    // While we are loading the textures, we check for events. Some events
    // might cancel the current loading process. When all textures
    // have been processed, we go into an idle state, in which we wait for events.

    state->viewerState->viewerReady = true;

    // Display info about skeleton save path here TODO

    // for arbitrary viewport orientation
    state->alpha += state->viewerState->alphaCache;
    state->beta += state->viewerState->betaCache;
    state->viewerState->alphaCache = state->viewerState->betaCache = 0;
    if (state->alpha >= 360) {
        state->alpha -= 360;
    } else if (state->alpha < 0) {
        state->alpha += 360;
    }
    if (state->beta >= 360) {
        state->beta -= 360;
    } else if (state->beta < 0) {
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

    for (std::size_t drawCounter = 0; drawCounter < 4 && !state->quitSignal; ++drawCounter) {
        vpConfig currentVp = state->viewerState->vpConfigs[drawCounter];

        if (currentVp.id == VP_UPPERLEFT) {
            vpUpperLeft->makeCurrent();
        } else if (currentVp.id == VP_LOWERLEFT) {
            vpLowerLeft->makeCurrent();
        } else if (currentVp.id == VP_UPPERRIGHT) {
            vpUpperRight->makeCurrent();
        }

        if(currentVp.type != VIEWPORT_SKELETON) {
            if(currentVp.type != VIEWPORT_ARBITRARY) {
                vpGenerateTexture(currentVp);
            } else {
                vpGenerateTexture_arb(currentVp);
            }
        }

        if(drawCounter == 3) {
            updateViewerState();
            recalcTextureOffsets();
            skeletonizer->autoSaveIfElapsed();
            window->updateTitlebar();//display changes after filename

            static auto disableVsync = [this](){
                void (*func)(int) = nullptr;
#if defined(Q_OS_WIN)
                func = (void(*)(int))wglGetProcAddress("wglSwapIntervalEXT");
#elif defined(Q_OS_LINUX)
                func = (void(*)(int))glXGetProcAddress((const GLubyte *)"glXSwapIntervalSGI");
#endif
                if (func != nullptr) {
#if defined(Q_OS_WIN)
                    return std::bind((PFNWGLSWAPINTERVALEXTPROC)func, 0);
#elif defined(Q_OS_LINUX)
                    return std::bind((PFNGLXSWAPINTERVALSGIPROC)func, 0);
#endif
                } else {
                    qDebug() << "disabling vsync not available";
                    return std::bind(&dummy, 0);
                }
            }();

            disableVsync();
            vpUpperLeft->updateGL();
            disableVsync();
            vpLowerLeft->updateGL();
            disableVsync();
            vpUpperRight->updateGL();
            disableVsync();
            vpLowerRight->updateGL();
            disableVsync();

            static uint call = 0;
            if (++call % 1000 == 0) {
                if(idlingExceeds(60000)) {
                    state->viewerState->renderInterval = SLOW;
                }
            }
            state->viewerState->userMove = false;
        }
    }
}

/** this method checks if the last call of the method checkIdleTime is longer than <treshold> msec ago.
 *  In this case, the render loop is slowed down to 1 calls per second (see timer->setSingleShot).
 *  Otherwise it stays in / switches to normal mode
*/
bool Viewer::idlingExceeds(uint msec) {
    QDateTime now = QDateTime::currentDateTimeUtc();
    if(state->viewerState->lastIdleTimeCall.msecsTo(now) >= msec) {
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

bool Viewer::userMove(int x, int y, int z) {
    struct viewerState *viewerState = state->viewerState;

    Coordinate lastPosition_dc;
    Coordinate newPosition_dc;

    //The skeleton VP view has to be updated after a current pos change
    state->skeletonState->viewChanged = true;
    /* @todo case decision for skeletonizer */
    if(state->skeletonState->showIntersections) {
        state->skeletonState->skeletonSliceVPchanged = true;
    }
    // This determines whether the server will broadcast the coordinate change
    // to its client or not.

    lastPosition_dc = Coordinate::Px2DcCoord(viewerState->currentPosition, state->cubeEdgeLength);

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
        qDebug("Position (%d, %d, %d) out of bounds",
            viewerState->currentPosition.x + x + 1,
            viewerState->currentPosition.y + y + 1,
            viewerState->currentPosition.z + z + 1);
    }

    calcLeftUpperTexAbsPx();
    recalcTextureOffsets();
    newPosition_dc = Coordinate::Px2DcCoord(viewerState->currentPosition, state->cubeEdgeLength);

    if(!COMPARE_COORDINATE(newPosition_dc, lastPosition_dc)) {
        state->viewerState->superCubeChanged = true;

        sendLoadSignal(viewerState->currentPosition.x,
                       viewerState->currentPosition.y,
                       viewerState->currentPosition.z,
                       NO_MAG_CHANGE);
    }

    QtConcurrent::run(this, &Viewer::updateCoordinatesSignal,
                      viewerState->currentPosition.x, viewerState->currentPosition.y, viewerState->currentPosition.z);

    return true;
}

bool Viewer::userMove_arb(float x, float y, float z) {
    Coordinate step;
    state->viewerState->moveCache.x += x;
    state->viewerState->moveCache.y += y;
    state->viewerState->moveCache.z += z;
    step.x = roundFloat(state->viewerState->moveCache.x);
    step.y = roundFloat(state->viewerState->moveCache.y);
    step.z = roundFloat(state->viewerState->moveCache.z);
    SUB_COORDINATE(state->viewerState->moveCache, step);
    return userMove(step.x, step.y, step.z);
}


int Viewer::findVPnumByWindowCoordinate(uint, uint) {
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
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx * (float)state->magnification;
                midY = state->viewerState->vpConfigs[i].t_max/2.
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx * (float)state->magnification;

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
/*
bool Viewer::moveVPonTop(uint currentVP) {
}
*/
/** Global interfaces  */
void Viewer::rewire() {
    // viewer signals
    connect(this, &Viewer::updateDatasetOptionsWidgetSignal,window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::update);
    QObject::connect(this, &Viewer::updateCoordinatesSignal, window, &MainWindow::updateCoordinateBar);
    // end viewer signals
    // skeletonizer signals
    QObject::connect(skeletonizer, &Skeletonizer::updateToolsSignal, window->widgetContainer->annotationWidget, &AnnotationWidget::updateLabels);
    QObject::connect(skeletonizer, &Skeletonizer::updateTreeviewSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::update);
    QObject::connect(skeletonizer, &Skeletonizer::userMoveSignal, this, &Viewer::userMove);
    QObject::connect(skeletonizer, &Skeletonizer::autosaveSignal, window, &MainWindow::autosaveSlot);
    QObject::connect(skeletonizer, &Skeletonizer::setSimpleTracing, window, &MainWindow::setSimpleTracing);
    QObject::connect(skeletonizer, &Skeletonizer::setSimpleTracing,
                     window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::setSimpleTracing);
    // end skeletonizer signals
    //event model signals
    QObject::connect(eventModel, &EventModel::treeAddedSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::treeAdded);
    QObject::connect(eventModel, &EventModel::nodeAddedSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::nodeAdded);
    QObject::connect(eventModel, &EventModel::nodeActivatedSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::nodeActivated);
    QObject::connect(eventModel, &EventModel::deleteSelectedNodesSignal, skeletonizer, &Skeletonizer::deleteSelectedNodes);
    QObject::connect(eventModel, &EventModel::nodeRadiusChangedSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::nodeRadiusChanged);
    QObject::connect(eventModel, &EventModel::nodePositionChangedSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::nodePositionChanged);
    QObject::connect(eventModel, &EventModel::updateTreeviewSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::update);
    QObject::connect(eventModel, &EventModel::unselectNodesSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::clearNodeTableSelection);
    QObject::connect(eventModel, &EventModel::userMoveSignal, this, &Viewer::userMove);
    QObject::connect(eventModel, &EventModel::userMoveArbSignal, this, &Viewer::userMove_arb);
    QObject::connect(eventModel, &EventModel::zoomOrthoSignal, vpUpperLeft, &Viewport::zoomOrthogonals);
    QObject::connect(eventModel, &EventModel::zoomInSkeletonVPSignal, vpLowerRight, &Viewport::zoomInSkeletonVP);
    QObject::connect(eventModel, &EventModel::zoomOutSkeletonVPSignal, vpLowerRight, &Viewport::zoomOutSkeletonVP);
    QObject::connect(eventModel, &EventModel::pasteCoordinateSignal, window, &MainWindow::pasteClipboardCoordinates);
    QObject::connect(eventModel, &EventModel::updateViewerStateSignal, this, &Viewer::updateViewerState);
    QObject::connect(eventModel, &EventModel::updateWidgetSignal, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::update);
    QObject::connect(eventModel, &EventModel::deleteActiveNodeSignal, &Skeletonizer::delActiveNode);
    QObject::connect(eventModel, &EventModel::genTestNodesSignal, skeletonizer, &Skeletonizer::genTestNodes);
    QObject::connect(eventModel, &EventModel::addSkeletonNodeSignal, skeletonizer, &Skeletonizer::UI_addSkeletonNode);
    QObject::connect(eventModel, &EventModel::addSkeletonNodeAndLinkWithActiveSignal, skeletonizer, &Skeletonizer::addSkeletonNodeAndLinkWithActive);
    QObject::connect(eventModel, &EventModel::setActiveNodeSignal, &Skeletonizer::setActiveNode);
    QObject::connect(eventModel, &EventModel::delSegmentSignal, &Skeletonizer::delSegment);
    QObject::connect(eventModel, &EventModel::addSegmentSignal, &Skeletonizer::addSegment);
    QObject::connect(eventModel, &EventModel::editNodeSignal, &Skeletonizer::editNode);
    QObject::connect(eventModel, &EventModel::findNodeInRadiusSignal, &Skeletonizer::findNodeInRadius);
    QObject::connect(eventModel, &EventModel::findSegmentByNodeIDSignal, &Skeletonizer::findSegmentByNodeIDs);
    QObject::connect(eventModel, &EventModel::findNodeByNodeIDSignal, &Skeletonizer::findNodeByNodeID);
    QObject::connect(eventModel, &EventModel::updateSlicePlaneWidgetSignal, window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, &VPSlicePlaneViewportWidget::updateIntersection);
    QObject::connect(eventModel, &EventModel::pushBranchNodeSignal, &Skeletonizer::pushBranchNode);
    QObject::connect(eventModel, &EventModel::setViewportOrientationSignal, vpUpperLeft, &Viewport::setOrientation);
    QObject::connect(eventModel, &EventModel::setViewportOrientationSignal, vpLowerLeft, &Viewport::setOrientation);
    QObject::connect(eventModel, &EventModel::setViewportOrientationSignal, vpUpperRight, &Viewport::setOrientation);
    QObject::connect(eventModel, &EventModel::compressionRatioToggled, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::updateCompressionRatioDisplay);
    //end event handler signals
    // mainwindow signals
    QObject::connect(window, &MainWindow::branchPushedSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::branchPushed);
    QObject::connect(window, &MainWindow::branchPoppedSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::branchPopped);
    QObject::connect(window, &MainWindow::nodeCommentChangedSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::nodeCommentChanged);
    QObject::connect(window, &MainWindow::updateToolsSignal, window->widgetContainer->annotationWidget, &AnnotationWidget::updateLabels);
    QObject::connect(window, &MainWindow::updateTreeviewSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::update);
    QObject::connect(window, &MainWindow::userMoveSignal, this, &Viewer::userMove);
    QObject::connect(window, &MainWindow::changeDatasetMagSignal, this, &Viewer::changeDatasetMag);
    QObject::connect(window, &MainWindow::recalcTextureOffsetsSignal, this, &Viewer::recalcTextureOffsets);
    QObject::connect(window, &MainWindow::updateTreeColorsSignal, &Skeletonizer::updateTreeColors);
    QObject::connect(window, &MainWindow::addTreeListElementSignal, skeletonizer, &Skeletonizer::addTreeListElement);
    QObject::connect(window, &MainWindow::stopRenderTimerSignal, timer, &QTimer::stop);
    QObject::connect(window, &MainWindow::startRenderTimerSignal, timer, static_cast<void(QTimer::*)(int)>(&QTimer::start));
    QObject::connect(window, &MainWindow::nextCommentSignal, skeletonizer, &Skeletonizer::nextComment);
    QObject::connect(window, &MainWindow::previousCommentSignal, skeletonizer, &Skeletonizer::previousComment);
    QObject::connect(window, &MainWindow::clearSkeletonSignal, &Skeletonizer::clearSkeleton);
    QObject::connect(window, &MainWindow::moveToNextNodeSignal, skeletonizer, &Skeletonizer::moveToNextNode);
    QObject::connect(window, &MainWindow::moveToPrevNodeSignal, skeletonizer, &Skeletonizer::moveToPrevNode);
    QObject::connect(window, &MainWindow::pushBranchNodeSignal, &Skeletonizer::pushBranchNode);
    QObject::connect(window, &MainWindow::popBranchNodeSignal, skeletonizer, &Skeletonizer::UI_popBranchNode);
    QObject::connect(window, &MainWindow::jumpToActiveNodeSignal, skeletonizer, &Skeletonizer::jumpToActiveNode);
    QObject::connect(window, &MainWindow::moveToPrevTreeSignal, skeletonizer, &Skeletonizer::moveToPrevTree);
    QObject::connect(window, &MainWindow::moveToNextTreeSignal, skeletonizer, &Skeletonizer::moveToNextTree);
    QObject::connect(window, &MainWindow::addCommentSignal, &Skeletonizer::addComment);
    QObject::connect(window, &MainWindow::editCommentSignal, &Skeletonizer::editComment);
    QObject::connect(window, &MainWindow::updateTaskDescriptionSignal, window->widgetContainer->taskManagementWidget->detailsTab, &TaskManagementDetailsTab::setDescription);
    QObject::connect(window, &MainWindow::updateTaskCommentSignal, window->widgetContainer->taskManagementWidget->detailsTab, &TaskManagementDetailsTab::setComment);
    QObject::connect(window, &MainWindow::treeAddedSignal, window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::treeAdded);
    //end mainwindow signals
    //viewport signals
    QObject::connect(vpUpperLeft, &Viewport::updateDatasetOptionsWidget, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::update);
    QObject::connect(vpLowerLeft, &Viewport::updateDatasetOptionsWidget, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::update);
    QObject::connect(vpUpperRight, &Viewport::updateDatasetOptionsWidget, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::update);
    QObject::connect(vpLowerRight, &Viewport::updateDatasetOptionsWidget, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::update);

    QObject::connect(vpUpperLeft, &Viewport::recalcTextureOffsetsSignal, this, &Viewer::recalcTextureOffsets);
    QObject::connect(vpLowerLeft, &Viewport::recalcTextureOffsetsSignal, this, &Viewer::recalcTextureOffsets);
    QObject::connect(vpUpperRight, &Viewport::recalcTextureOffsetsSignal, this, &Viewer::recalcTextureOffsets);
    QObject::connect(vpLowerRight, &Viewport::recalcTextureOffsetsSignal, this, &Viewer::recalcTextureOffsets);

    QObject::connect(vpUpperLeft, &Viewport::changeDatasetMagSignal, this, &Viewer::changeDatasetMag);
    QObject::connect(vpLowerLeft, &Viewport::changeDatasetMagSignal, this, &Viewer::changeDatasetMag);
    QObject::connect(vpUpperRight, &Viewport::changeDatasetMagSignal, this, &Viewer::changeDatasetMag);
    QObject::connect(vpLowerRight, &Viewport::changeDatasetMagSignal, this, &Viewer::changeDatasetMag);
    // end viewport signals

    // --- widget signals ---
    //  treeview tab signals
    QObject::connect(window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::setActiveNodeSignal, &Skeletonizer::setActiveNode);
    QObject::connect(window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::clearTreeSelectionSignal, &Skeletonizer::clearTreeSelection);
    QObject::connect(window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::clearNodeSelectionSignal, &Skeletonizer::clearNodeSelection);
    QObject::connect(window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::deleteSelectedNodesSignal, skeletonizer, &Skeletonizer::deleteSelectedNodes);
    QObject::connect(window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::delActiveNodeSignal, &Skeletonizer::delActiveNode);
    QObject::connect(window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::JumpToActiveNodeSignal, skeletonizer, &Skeletonizer::jumpToActiveNode);
    QObject::connect(window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::addSegmentSignal, &Skeletonizer::addSegment);
    QObject::connect(window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::delSegmentSignal, &Skeletonizer::delSegment);
    QObject::connect(window->widgetContainer->annotationWidget->treeviewTab, &ToolsTreeviewTab::deleteSelectedTreesSignal, skeletonizer, &Skeletonizer::deleteSelectedTrees);
    // commands tab signals
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::findTreeByTreeIDSignal, &Skeletonizer::findTreeByTreeID);
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::findNodeByNodeIDSignal, &Skeletonizer::findNodeByNodeID);
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::setActiveTreeSignal, &Skeletonizer::setActiveTreeByID);
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::setActiveNodeSignal, &Skeletonizer::setActiveNode);
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::jumpToNodeSignal, skeletonizer, &Skeletonizer::jumpToActiveNode);
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::addTreeListElement, skeletonizer, &Skeletonizer::addTreeListElement);
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::pushBranchNodeSignal, &Skeletonizer::pushBranchNode);
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::popBranchNodeSignal, skeletonizer, &Skeletonizer::UI_popBranchNode);
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::lockPositionSignal, &Skeletonizer::lockPosition);
    QObject::connect(window->widgetContainer->annotationWidget->commandsTab, &ToolsCommandsTab::unlockPositionSignal, &Skeletonizer::unlockPosition);
    //  -- end tools widget signals
    //  viewport settings widget signals --
    //  general vp settings tab signals
    QObject::connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, &VPGeneralTabWidget::updateViewerStateSignal, this, &Viewer::updateViewerState);
    //  slice plane vps tab signals
    QObject::connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, &VPSlicePlaneViewportWidget::treeColorAdjustmentsChangedSignal, window, &MainWindow::treeColorAdjustmentsChanged);
    QObject::connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, &VPSlicePlaneViewportWidget::loadTreeColorTableSignal, this, &Viewer::loadTreeColorTable);
    QObject::connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, &VPSlicePlaneViewportWidget::loadDataSetColortableSignal, &Viewer::loadDatasetColorTable);
    QObject::connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, &VPSlicePlaneViewportWidget::updateViewerStateSignal, this, &Viewer::updateViewerState);

    //  -- end viewport settings widget signals
    //  dataset options signals --
    QObject::connect(window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::zoomInSkeletonVPSignal, vpLowerRight, &Viewport::zoomInSkeletonVP);
    QObject::connect(window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::zoomOutSkeletonVPSignal, vpLowerRight, &Viewport::zoomOutSkeletonVP);
    //  -- end dataset options signals
    // dataset load signals --
    QObject::connect(window->widgetContainer->datasetLoadWidget, &DatasetLoadWidget::clearSkeletonSignalNoGUI, window, &MainWindow::clearSkeletonSlotNoGUI);
    QObject::connect(window->widgetContainer->datasetLoadWidget, &DatasetLoadWidget::clearSkeletonSignalGUI, window, &MainWindow::clearSkeletonSlotGUI);
    // -- end dataset load signals
    // task management signals --
    QObject::connect(window->widgetContainer->taskManagementWidget->mainTab, &TaskManagementMainTab::loadSkeletonSignal, window, &MainWindow::openFileDispatch);
    QObject::connect(window->widgetContainer->taskManagementWidget->mainTab, &TaskManagementMainTab::autosaveSignal, window, &MainWindow::autosaveSlot);
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
