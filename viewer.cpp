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
#include "session.h"
#include "skeletonizer.h"
#include "viewer.h"
#include "widgets/mainwindow.h"
#include "widgets/viewport.h"
#include "widgets/viewportsettings/vpgeneraltabwidget.h"
#include "widgets/viewportsettings/vpsliceplaneviewportwidget.h"
#include "widgets/widgetcontainer.h"

#if defined(Q_OS_WIN)
#include <GL/wglext.h>
#elif defined(Q_OS_LINUX)
#define WINAPI
#include <GL/glx.h>
#include <GL/glxext.h>
#else
#define WINAPI
#endif

static WINAPI int dummy(int) {
    return 0;
}

Viewer::Viewer(QObject *parent) : QThread(parent) {
    state->viewer = this;
    skeletonizer = &Skeletonizer::singleton();
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
    renderer = new Renderer();

    QDesktopWidget *desktop = QApplication::desktop();

    rewire();
    window->show();
    window->loadSettings();
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
    state->viewerState->renderInterval = FAST;

    // for arbitrary viewport orientation
    for (uint i = 0; i < Viewport::numberViewports; i++){
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
    SET_COORDINATE(moveCache, 0, 0, 0);
    resetRotation();
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XY].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XY].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XY].n , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XZ].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XZ].v2 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_XZ].n , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_YZ].v1 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_YZ].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VIEWPORT_YZ].n , v1);

    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, this, &Viewer::updateCurrentPosition);

    baseTime.start();//keyRepeat timer
}

bool Viewer::resetViewPortData(vpConfig *viewport) {
    // for arbitrary vp orientation
    memset(viewport->viewPortData,
           state->viewerState->defaultTexData[0],
           TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 3);
    viewport->s_max = viewport->t_max  = -1;
    return true;
}

bool Viewer::dcSliceExtract(Byte *datacube, Coordinate cubePosInAbsPx, Byte *slice, size_t dcOffset, vpConfig *vpConfig, bool useCustomLUT) {
    datacube += dcOffset;
    const auto & session = Session::singleton();
    const Coordinate areaMinCoord = {session.movementCenter.x - session.movementRange.x,
                                     session.movementCenter.y - session.movementRange.y,
                                     session.movementCenter.z - session.movementRange.z};
    const Coordinate areaMaxCoord = {session.movementCenter.x + session.movementRange.x,
                                     session.movementCenter.y + session.movementRange.y,
                                     session.movementCenter.z + session.movementRange.z};

    const auto partlyInMovementArea =
       areaMinCoord.x > cubePosInAbsPx.x || areaMaxCoord.x < cubePosInAbsPx.x + state->cubeEdgeLength * state->magnification ||
       areaMinCoord.y > cubePosInAbsPx.y || areaMaxCoord.y < cubePosInAbsPx.y + state->cubeEdgeLength * state->magnification ||
       areaMinCoord.z > cubePosInAbsPx.z || areaMaxCoord.z < cubePosInAbsPx.z + state->cubeEdgeLength * state->magnification;

    const std::size_t innerLoopBoundary = vpConfig->type == SLICE_XZ ? state->cubeEdgeLength : state->cubeSliceArea;
    const std::size_t outerLoopBoundary = vpConfig->type == SLICE_XZ ? state->cubeEdgeLength : 1;
    const std::size_t voxelIncrement = vpConfig->type == SLICE_YZ ? state->cubeEdgeLength : 1;
    const std::size_t sliceIncrement = vpConfig->type == SLICE_XY ? state->cubeEdgeLength : state->cubeSliceArea;
    const std::size_t sliceSubLineIncrement = sliceIncrement - state->cubeEdgeLength;

    int offsetX = 0, offsetY = 0;
    int pixelsPerLine = state->cubeEdgeLength*state->magnification;
    for(std::size_t y = 0; y < outerLoopBoundary; y++) {
        for(std::size_t x = 0; x < innerLoopBoundary; x++) {
            Byte r, g, b;
            if(useCustomLUT) {
                r = state->viewerState->datasetAdjustmentTable[0][*datacube];
                g = state->viewerState->datasetAdjustmentTable[1][*datacube];
                b = state->viewerState->datasetAdjustmentTable[2][*datacube];
            }
            else {
                r = g = b = *datacube;
            }
            if(partlyInMovementArea) {
                if((vpConfig->type == SLICE_XY && (cubePosInAbsPx.y + offsetY < areaMinCoord.y || cubePosInAbsPx.y + offsetY > areaMaxCoord.y)) ||
                    ((vpConfig->type == SLICE_XZ || vpConfig->type == SLICE_YZ) && (cubePosInAbsPx.z + offsetY < areaMinCoord.z || cubePosInAbsPx.z + offsetY > areaMaxCoord.z))) {
                    // vertically out of movement area
                    r /= 2, g /= 2, b /= 2;
                }
                else if(((vpConfig->type == SLICE_XY || vpConfig->type == SLICE_XZ) && (cubePosInAbsPx.x + offsetX < areaMinCoord.x || cubePosInAbsPx.x + offsetX > areaMaxCoord.x)) ||
                        (vpConfig->type == SLICE_YZ && (cubePosInAbsPx.y + offsetX < areaMinCoord.y || cubePosInAbsPx.y + offsetX > areaMaxCoord.y))) {
                    // horizontally out of movement area
                    r /= 2, g /= 2, b /= 2;
                }
            }
            slice[0] = r, slice[1] = g, slice[2] = b;

            datacube += voxelIncrement;
            slice += 3;
            offsetX = (offsetX + state->magnification) % pixelsPerLine;
            offsetY += (offsetX == 0)? state->magnification : 0; // at end of line increment to next line
        }
        datacube += sliceSubLineIncrement;
    }

    return true;
}

bool Viewer::dcSliceExtract_arb(Byte *datacube, struct vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t, bool useCustomLUT) {
    Byte *slice = viewPort->viewPortData;
    Coordinate currentPxInDc;
    int sliceIndex = 0, dcIndex = 0;
    floatCoordinate *v2 = &(viewPort->v2);

    SET_COORDINATE(currentPxInDc, roundFloat(currentPxInDc_float->x),
                                  roundFloat(currentPxInDc_float->y),
                                  roundFloat(currentPxInDc_float->z));

    if((currentPxInDc.x < 0) || (currentPxInDc.y < 0) || (currentPxInDc.z < 0) ||
       (currentPxInDc.x >= state->cubeEdgeLength) || (currentPxInDc.y >= state->cubeEdgeLength) || (currentPxInDc.z >= state->cubeEdgeLength)) {
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
            slice[sliceIndex] = slice[sliceIndex + 1] = slice[sliceIndex + 2] = state->viewerState->defaultTexData[dcIndex];
        }
        else {
            if(useCustomLUT) {
                slice[sliceIndex] = state->viewerState->datasetAdjustmentTable[0][datacube[dcIndex]];
                slice[sliceIndex + 1] = state->viewerState->datasetAdjustmentTable[1][datacube[dcIndex]];
                slice[sliceIndex + 2] = state->viewerState->datasetAdjustmentTable[2][datacube[dcIndex]];
            }
            else {
                slice[sliceIndex] = slice[sliceIndex + 1] = slice[sliceIndex + 2] = datacube[dcIndex];
            }
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

/**
 * @brief Viewer::ocSliceExtract extracts subObject IDs from datacube
 *      and paints slice at the corresponding position with a color depending on the ID.
 * @param datacube pointer to the datacube for data extraction
 * @param cubePosInAbsPx smallest coordinates inside the datacube in dataset pixels
 * @param slice pointer to a slice in which to draw the overlay
 *
 * In the first pass all pixels are filled with the color corresponding the subObject-ID.
 * In the second pass the datacube is traversed again to find edge voxels, i.e. all voxels
 * where at least one of their neighbors (left, right, top, bot) have a different ID than their own.
 * The opacity of these voxels is slightly increased to highlight the edges.
 *
 * Before extraction we determine if the cube is partly outside of the movement area in which case
 * each pixel is tested for its position and is omitted if outside of the area.
 *
 */
void Viewer::ocSliceExtract(Byte *datacube, Coordinate cubePosInAbsPx, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    datacube += dcOffset;

    const auto & session = Session::singleton();
    const Coordinate areaMinCoord = {session.movementCenter.x - session.movementRange.x,
                                     session.movementCenter.y - session.movementRange.y,
                                     session.movementCenter.z - session.movementRange.z};
    const Coordinate areaMaxCoord = {session.movementCenter.x + session.movementRange.x,
                                     session.movementCenter.y + session.movementRange.y,
                                     session.movementCenter.z + session.movementRange.z};

    const auto partlyInMovementArea =
       areaMinCoord.x > cubePosInAbsPx.x || areaMaxCoord.x < cubePosInAbsPx.x + state->cubeEdgeLength * state->magnification ||
       areaMinCoord.y > cubePosInAbsPx.y || areaMaxCoord.y < cubePosInAbsPx.y + state->cubeEdgeLength * state->magnification ||
       areaMinCoord.z > cubePosInAbsPx.z || areaMaxCoord.z < cubePosInAbsPx.z + state->cubeEdgeLength * state->magnification;

    const std::size_t innerLoopBoundary = vpConfig->type == SLICE_XZ ? state->cubeEdgeLength : state->cubeSliceArea;
    const std::size_t outerLoopBoundary = vpConfig->type == SLICE_XZ ? state->cubeEdgeLength : 1;
    const std::size_t voxelIncrement = vpConfig->type == SLICE_YZ ? state->cubeEdgeLength * OBJID_BYTES : OBJID_BYTES;
    const std::size_t sliceIncrement = vpConfig->type == SLICE_XY ? state->cubeEdgeLength * OBJID_BYTES : state->cubeSliceArea * OBJID_BYTES;
    const std::size_t sliceSubLineIncrement = sliceIncrement - state->cubeEdgeLength * OBJID_BYTES;

    auto & seg = Segmentation::singleton();
    //cache
    uint64_t subobjectIdCache = 0;
    bool selectedCache = false;
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> colorCache;
    //first and last row boundaries
    const std::size_t min = state->cubeEdgeLength;
    const std::size_t max = state->cubeEdgeLength * (state->cubeEdgeLength - 1);
    std::size_t counter = 0;//slice position
    int offsetX = 0, offsetY = 0; // current texel's horizontal and vertical dataset pixel offset inside cube
    int pixelsPerLine = state->cubeEdgeLength*state->magnification;
    for (std::size_t y = 0; y < outerLoopBoundary; ++y) {
        for (std::size_t x = 0; x < innerLoopBoundary; ++x) {
            bool hide = false;
            if(partlyInMovementArea) {
                if((vpConfig->type == SLICE_XY && (cubePosInAbsPx.y + offsetY < areaMinCoord.y || cubePosInAbsPx.y + offsetY > areaMaxCoord.y)) ||
                    ((vpConfig->type == SLICE_XZ || vpConfig->type == SLICE_YZ) && (cubePosInAbsPx.z + offsetY < areaMinCoord.z || cubePosInAbsPx.z + offsetY > areaMaxCoord.z))) {
                    // vertically out of movement area
                    slice[3] = 0;
                    hide = true;
                }
                else if(((vpConfig->type == SLICE_XY || vpConfig->type == SLICE_XZ) && (cubePosInAbsPx.x + offsetX < areaMinCoord.x || cubePosInAbsPx.x + offsetX > areaMaxCoord.x)) ||
                        (vpConfig->type == SLICE_YZ && (cubePosInAbsPx.y + offsetX < areaMinCoord.y || cubePosInAbsPx.y + offsetX > areaMaxCoord.y))) {
                    // horizontally out of movement area
                    slice[3] = 0;
                    hide = true;
                }
            }

            if(hide == false) {
                uint64_t subobjectId = *reinterpret_cast<uint64_t*>(datacube);

                auto color = (subobjectIdCache == subobjectId) ? colorCache : seg.colorObjectFromId(subobjectId);
                slice[0] = std::get<0>(color);
                slice[1] = std::get<1>(color);
                slice[2] = std::get<2>(color);
                slice[3] = std::get<3>(color);

                const bool selected = (subobjectIdCache == subobjectId) ? selectedCache : seg.isSubObjectIdSelected(subobjectId);
                const bool isPastFirstRow = counter >= min;
                const bool isBeforeLastRow = counter < max;
                const bool isNotFirstColumn = counter % state->cubeEdgeLength != 0;
                const bool isNotLastColumn = (counter + 1) % state->cubeEdgeLength != 0;

                // highlight edges where needed
                if(seg.hoverVersion) {
                    uint64_t objectId = seg.tryLargestObjectContainingSubobject(subobjectId);
                    if (selected && (seg.renderAllObjs || (!seg.renderAllObjs && seg.mouseFocusedObjectId == objectId))) {
                        if(isPastFirstRow && isBeforeLastRow && isNotFirstColumn && isNotLastColumn) {
                            const uint64_t left = seg.tryLargestObjectContainingSubobject(*reinterpret_cast<uint64_t*>(datacube - voxelIncrement));
                            const uint64_t right = seg.tryLargestObjectContainingSubobject(*reinterpret_cast<uint64_t*>(datacube + voxelIncrement));
                            const uint64_t top = seg.tryLargestObjectContainingSubobject(*reinterpret_cast<uint64_t*>(datacube - sliceIncrement));
                            const uint64_t bottom = seg.tryLargestObjectContainingSubobject(*reinterpret_cast<uint64_t*>(datacube + sliceIncrement));
                            //enhance alpha of this voxel if any of the surrounding voxels belong to another object
                            if (objectId != left || objectId != right || objectId != top || objectId != bottom) {
                                slice[3] = std::min(255, slice[3]*4);
                            }
                        }
                    }
                }
                else if (selected && isPastFirstRow && isBeforeLastRow && isNotFirstColumn && isNotLastColumn) {
                    const uint64_t left = *reinterpret_cast<uint64_t*>(datacube - voxelIncrement);
                    const uint64_t right = *reinterpret_cast<uint64_t*>(datacube + voxelIncrement);
                    const uint64_t top = *reinterpret_cast<uint64_t*>(datacube - sliceIncrement);
                    const uint64_t bottom = *reinterpret_cast<uint64_t*>(datacube + sliceIncrement);;
                    //enhance alpha of this voxel if any of the surrounding voxels belong to another subobject
                    if (subobjectId != left || subobjectId != right || subobjectId != top || subobjectId != bottom) {
                        slice[3] = std::min(255, slice[3]*4);
                    }
                }

                //fill cache
                subobjectIdCache = subobjectId;
                colorCache = color;
                selectedCache = selected;
            }
            ++counter;
            slice += 4;//RGBA per pixel
            datacube += voxelIncrement;
            offsetX = (offsetX + state->magnification) % pixelsPerLine;
            offsetY += (offsetX == 0)? state->magnification : 0; // at end of line increment to next line
        }
        datacube += sliceSubLineIncrement;
    }
}

void Viewer::ocSliceExtractUnique(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    datacube += dcOffset;
    const std::size_t innerLoopBoundary = vpConfig->type == SLICE_XZ ? state->cubeEdgeLength : state->cubeSliceArea;
    const std::size_t outerLoopBoundary = vpConfig->type == SLICE_XZ ? state->cubeEdgeLength : 1;
    const std::size_t voxelIncrement = vpConfig->type == SLICE_YZ ? state->cubeEdgeLength * OBJID_BYTES : OBJID_BYTES;
    const std::size_t sliceIncrement = vpConfig->type != SLICE_XY ? state->cubeSliceArea * OBJID_BYTES : state->cubeEdgeLength * OBJID_BYTES;
    const std::size_t sliceSubLineIncrement = sliceIncrement - state->cubeEdgeLength * OBJID_BYTES;

    for (std::size_t y = 0; y < outerLoopBoundary; ++y) {
        for (std::size_t x = 0; x < innerLoopBoundary; ++x) {
            uint64_t subObjectID = *reinterpret_cast<uint64_t*>(datacube);

            const auto color = Segmentation::singleton().colorUniqueFromId(subObjectID);
            slice[0] = std::get<0>(color);
            slice[1] = std::get<1>(color);
            slice[2] = std::get<2>(color);
            slice[3] = std::get<3>(color);

            slice += 4;//RGBA per pixel
            datacube += voxelIncrement;
        }
        datacube += sliceSubLineIncrement;
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
            datacube = Coordinate2BytePtr_hash_get_or_fail(state->Dc2Pointer[int_log(state->magnification)], currentDc);
            overlayCube = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[int_log(state->magnification)], currentDc);
            state->protectCube2Pointer->unlock();


            // Take care of the data textures.

            glBindTexture(GL_TEXTURE_2D,
                          currentVp.texture.texHandle);

            // This is used to index into the texture. overlayData[index] is the first
            // byte of the datacube slice at position (x_dc, y_dc) in the texture.
            index = texIndex(x_dc, y_dc, 3, &(currentVp.texture));
            Coordinate cubePosInAbsPx = {currentDc.x * state->magnification * state->cubeEdgeLength,
                                         currentDc.y * state->magnification * state->cubeEdgeLength,
                                         currentDc.z * state->magnification * state->cubeEdgeLength};

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
                               cubePosInAbsPx,
                               state->viewerState->texData + index,
                               dcOffset,
                               &currentVp,
                               state->viewerState->datasetAdjustmentOn);
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
                                       cubePosInAbsPx,
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
            datacube = Coordinate2BytePtr_hash_get_or_fail(state->Dc2Pointer[int_log(state->magnification)], currentDc);
            state->protectCube2Pointer->unlock();

            SET_COORDINATE(currentPxInDc_float, currentPx_float.x-currentDc.x*state->cubeEdgeLength,
                                                currentPx_float.y-currentDc.y*state->cubeEdgeLength,
                                                currentPx_float.z-currentDc.z*state->cubeEdgeLength);
            t_old = t;
            dcSliceExtract_arb(datacube,
                               &currentVp,
                               &currentPxInDc_float,
                               s, &t,
                               state->viewerState->datasetAdjustmentOn);
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
    for (i = 0; i < Viewport::numberViewports; i++) {
        floatCoordinate v1, v2;
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

    Segmentation::singleton().loadOverlayLutFromFile();

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
    for (std::size_t i = 0; i < Viewport::numberViewports; ++i){
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


    for(i = 0; i < Viewport::numberViewports; i++) {
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
                for(i = 0; i < Viewport::numberViewports; i++) {
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
                for(i = 0; i < Viewport::numberViewports; i++) {
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

    /*for(i = 0; i < Viewport::numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != VIEWPORT_SKELETON) {
            qDebug("left upper tex px of VP %d is: %d, %d, %d",i,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z);
        }
    }*/
    sendLoadSignal(upOrDownFlag);

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

    if (state->viewerKeyRepeat && (state->keyF || state->keyD)) {
        qint64 interval = 1000 / state->viewerState->stepsPerSec;
        if (baseTime.elapsed() >= interval) {
            baseTime.restart();
            const auto type = state->viewerState->vpConfigs[0].type;
            if (type != VIEWPORT_ARBITRARY) {
                userMove(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2],
                        USERMOVE_DRILL, type);
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
    if(rotation.alpha != 0) {
        rotateAndNormalize(v1, rotation.axis, rotation.alpha);
        rotateAndNormalize(v2, rotation.axis, rotation.alpha);
        rotateAndNormalize(v3, rotation.axis, rotation.alpha);
        CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERLEFT].v1 , v1);
        CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERLEFT].v2 , v2);
        CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERLEFT].n , v3);
        CPY_COORDINATE(state->viewerState->vpConfigs[VP_LOWERLEFT].v1 , v1);
        CPY_COORDINATE(state->viewerState->vpConfigs[VP_LOWERLEFT].v2 , v3);
        CPY_COORDINATE(state->viewerState->vpConfigs[VP_LOWERLEFT].n , v2);
        CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERRIGHT].v1 , v3);
        CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERRIGHT].v2 , v2);
        CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERRIGHT].n , v1);
        rotation = Rotation();
        alphaCache = 0;
    }
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

            state->viewerState->userMove = false;
        }
    }
}

bool Viewer::updateViewerState() {
   uint i;

    for(i = 0; i < Viewport::numberViewports; i++) {

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

    for(i = 0; i < Viewport::numberViewports; i++) {
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

void Viewer::updateCurrentPosition() {
    auto & session = Session::singleton();
    if(session.outsideMovementArea(state->viewerState->currentPosition)) {
        Coordinate currPos = state->viewerState->currentPosition;
        userMove(session.movementCenter.x - currPos.x, session.movementCenter.y - currPos.y, session.movementCenter.z - currPos.z,
                 USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
    }
}

bool Viewer::userMove(int x, int y, int z, UserMoveType userMoveType, ViewportType viewportType) {
    struct viewerState *viewerState = state->viewerState;

    Coordinate lastPosition_dc;
    Coordinate newPosition_dc;

    // This determines whether the server will broadcast the coordinate change
    // to its client or not.

    lastPosition_dc = Coordinate::Px2DcCoord(viewerState->currentPosition, state->cubeEdgeLength);

    viewerState->userMove = true;
    auto newPos = Coordinate(viewerState->currentPosition.x + x, viewerState->currentPosition.y + y, viewerState->currentPosition.z + z);
    if (Session::singleton().outsideMovementArea(newPos) == false) {
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
        state->loaderUserMoveType = userMoveType;
        Coordinate direction;
        switch (userMoveType) {
        case USERMOVE_DRILL:
            SET_COORDINATE(direction, x, y, z);
            break;
        case USERMOVE_HORIZONTAL:
            switch (viewportType) {
            case VIEWPORT_XZ:
                SET_COORDINATE(direction, 0, 1, 0);
                break;
            case VIEWPORT_YZ:
                SET_COORDINATE(direction, 1, 0, 0);
                break;
            case VIEWPORT_XY:
            default:
                SET_COORDINATE(direction, 0, 0, 1);
                break;
            }
            break;
        case USERMOVE_NEUTRAL:
            SET_COORDINATE(direction, 0, 0, 0);
            break;
        default:
            break;
        }
        CPY_COORDINATE(state->loaderUserMoveViewportDirection, direction);
        sendLoadSignal(NO_MAG_CHANGE);
    }

    emit coordinateChangedSignal(viewerState->currentPosition.x, viewerState->currentPosition.y, viewerState->currentPosition.z);

    return true;
}

bool Viewer::userMove_arb(float x, float y, float z) {
    Coordinate step;
    moveCache.x += x;
    moveCache.y += y;
    moveCache.z += z;
    step.x = roundFloat(moveCache.x);
    step.y = roundFloat(moveCache.y);
    step.z = roundFloat(moveCache.z);
    SUB_COORDINATE(moveCache, step);
    // In fact the movement most likely is not neutral, but since arbitrary horizontal or drilling movement
    // makes little difference for the (currently) orthogonal loading order, we leave it as such
    return userMove(step.x, step.y, step.z, USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
}

bool Viewer::recalcTextureOffsets() {
    uint i;
    float midX = 0.,midY = 0.;

    calcDisplayedEdgeLength();

    for(i = 0; i < Viewport::numberViewports; i++) {
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

bool Viewer::sendLoadSignal(int magChanged) {
    state->protectLoadSignal->lock();
    state->loadSignal = true;
    state->datasetChangeSignal = magChanged;

    state->previousPositionX = state->currentPositionX;

    // Convert the coordinate to the right mag. The loader
    // is agnostic to the different dataset magnifications.
    // The int division is hopefully not too much of an issue here
    SET_COORDINATE(state->currentPositionX,
                   state->viewerState->currentPosition.x / state->magnification,
                   state->viewerState->currentPosition.y / state->magnification,
                   state->viewerState->currentPosition.z / state->magnification);

    state->conditionLoadSignal->wakeOne();//wake up loader if it’s sleeping
    state->protectLoadSignal->unlock();

    return true;
}

/** Global interfaces  */
void Viewer::rewire() {
    // viewer signals
    connect(this, &Viewer::updateDatasetOptionsWidgetSignal,window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::update);
    QObject::connect(this, &Viewer::coordinateChangedSignal, window, &MainWindow::updateCoordinateBar);
    // end viewer signals
    // skeletonizer signals
    QObject::connect(skeletonizer, &Skeletonizer::userMoveSignal, this, &Viewer::userMove);
    // end skeletonizer signals
    //event model signals
    QObject::connect(eventModel, &EventModel::userMoveSignal, this, &Viewer::userMove);
    QObject::connect(eventModel, &EventModel::userMoveArbSignal, this, &Viewer::userMove_arb);
    QObject::connect(eventModel, &EventModel::zoomReset, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::zoomDefaultsClicked);
    QObject::connect(eventModel, &EventModel::zoomOrthoSignal, vpUpperLeft, &Viewport::zoomOrthogonals);
    QObject::connect(eventModel, &EventModel::zoomOrthoSignal, renderer, &Renderer::invalidatePickingBuffer);
    QObject::connect(eventModel, &EventModel::zoomInSkeletonVPSignal, vpLowerRight, &Viewport::zoomInSkeletonVP);
    QObject::connect(eventModel, &EventModel::zoomOutSkeletonVPSignal, vpLowerRight, &Viewport::zoomOutSkeletonVP);
    QObject::connect(eventModel, &EventModel::pasteCoordinateSignal, window, &MainWindow::pasteClipboardCoordinates);
    QObject::connect(eventModel, &EventModel::updateViewerStateSignal, this, &Viewer::updateViewerState);
    QObject::connect(eventModel, &EventModel::updateWidgetSignal, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::update);
    QObject::connect(eventModel, &EventModel::genTestNodesSignal, skeletonizer, &Skeletonizer::genTestNodes);
    QObject::connect(eventModel, &EventModel::delSegmentSignal, &Skeletonizer::delSegment);
    QObject::connect(eventModel, &EventModel::addSegmentSignal, &Skeletonizer::addSegment);
    QObject::connect(eventModel, &EventModel::updateSlicePlaneWidgetSignal, window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, &VPSlicePlaneViewportWidget::updateIntersection);
    QObject::connect(eventModel, &EventModel::compressionRatioToggled, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::updateCompressionRatioDisplay);
    QObject::connect(eventModel, &EventModel::rotationSignal, this, &Viewer::setRotation);
    //end event handler signals
    // mainwindow signals
    QObject::connect(window, &MainWindow::resetRotationSignal, this, &Viewer::resetRotation);
    QObject::connect(window, &MainWindow::userMoveSignal, this, &Viewer::userMove);
    QObject::connect(window, &MainWindow::changeDatasetMagSignal, this, &Viewer::changeDatasetMag);
    QObject::connect(window, &MainWindow::recalcTextureOffsetsSignal, this, &Viewer::recalcTextureOffsets);
    QObject::connect(window, &MainWindow::updateTreeColorsSignal, &Skeletonizer::updateTreeColors);
    QObject::connect(window, &MainWindow::addTreeListElementSignal, skeletonizer, &Skeletonizer::addTreeListElement);
    QObject::connect(window, &MainWindow::stopRenderTimerSignal, timer, &QTimer::stop);
    QObject::connect(window, &MainWindow::startRenderTimerSignal, timer, static_cast<void(QTimer::*)(int)>(&QTimer::start));
    QObject::connect(window, &MainWindow::updateTaskDescriptionSignal, window->widgetContainer->taskManagementWidget, &TaskManagementWidget::setDescription);
    QObject::connect(window, &MainWindow::updateTaskCommentSignal, window->widgetContainer->taskManagementWidget, &TaskManagementWidget::setComment);
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
    //  viewport settings widget signals --
    //  general vp settings tab signals
    QObject::connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, &VPGeneralTabWidget::updateViewerStateSignal, this, &Viewer::updateViewerState);
    //  slice plane vps tab signals
    QObject::connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, &VPSlicePlaneViewportWidget::setVPOrientationSignal, this, &Viewer::setVPOrientation);
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
    QObject::connect(window->widgetContainer->datasetLoadWidget, &DatasetLoadWidget::datasetChanged, &Session::singleton(), &Session::updateMovementArea);
    QObject::connect(window->widgetContainer->datasetLoadWidget, &DatasetLoadWidget::clearSkeletonSignalNoGUI, window, &MainWindow::clearSkeletonSlotNoGUI);
    QObject::connect(window->widgetContainer->datasetLoadWidget, &DatasetLoadWidget::clearSkeletonSignalGUI, window, &MainWindow::clearSkeletonSlotGUI);
    QObject::connect(window->widgetContainer->datasetLoadWidget, &DatasetLoadWidget::updateDatasetCompression, window->widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::updateCompressionRatioDisplay);
     // -- end dataset load signals
    // task management signals --
    QObject::connect(window->widgetContainer->taskManagementWidget, &TaskManagementWidget::loadAnnotationFiles, window, &MainWindow::openFileDispatch);
    QObject::connect(window->widgetContainer->taskManagementWidget, &TaskManagementWidget::autosaveSignal, window, &MainWindow::autosaveSlot);
    // -- end task management signals
    // navigation widget signals --
    QObject::connect(window->widgetContainer->navigationWidget, &NavigationWidget::sendLoadSignal, this, &Viewer::sendLoadSignal);
    QObject::connect(window->widgetContainer->navigationWidget, &NavigationWidget::movementAreaChanged, this, &Viewer::updateCurrentPosition);
    // --- end widget signals
}

void Viewer::setRotation(float x, float y, float z, float angle) {
    alphaCache += angle; // angles are added up here until they are processed in the thread loop
    rotation = Rotation(x, y, z, alphaCache);
}

void Viewer::setVPOrientation(bool arbitrary) {
    if(arbitrary) {
        window->viewports[VP_UPPERLEFT]->setOrientation(VIEWPORT_ARBITRARY);
        window->viewports[VP_LOWERLEFT]->setOrientation(VIEWPORT_ARBITRARY);
        window->viewports[VP_UPPERRIGHT]->setOrientation(VIEWPORT_ARBITRARY);
    }
    else {
        window->viewports[VP_UPPERLEFT]->setOrientation(VIEWPORT_XY);
        window->viewports[VP_LOWERLEFT]->setOrientation(VIEWPORT_XZ);
        window->viewports[VP_UPPERRIGHT]->setOrientation(VIEWPORT_YZ);
        resetRotation();
    }
}

void Viewer::resetRotation() {
    alphaCache = 0;
    rotation = Rotation();
    SET_COORDINATE(v1, 1, 0, 0);
    SET_COORDINATE(v2, 0, 1, 0);
    SET_COORDINATE(v3, 0, 0, 1);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERLEFT].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERLEFT].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERLEFT].n , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_LOWERLEFT].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_LOWERLEFT].v2 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_LOWERLEFT].n , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERRIGHT].v1 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERRIGHT].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[VP_UPPERRIGHT].n , v1);
}
