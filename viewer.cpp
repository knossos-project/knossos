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

#include "file_io.h"
#include "functions.h"
#include "segmentation/segmentation.h"
#include "session.h"
#include "skeleton/skeletonizer.h"
#include "widgets/mainwindow.h"
#include "widgets/viewport.h"
#include "widgets/widgetcontainer.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <qopengl.h>
#include <QtConcurrent/QtConcurrentRun>

#include <fstream>

#include <cmath>

Viewer::Viewer() {
    state->viewer = this;
    skeletonizer = &Skeletonizer::singleton();
    loadTreeLUT();
    window->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    vpUpperLeft = window->viewportXY.get();
    vpLowerLeft = window->viewportXZ.get();
    vpUpperRight = window->viewportYZ.get();

    initViewer();

    ViewportBase::initMesh(viewerState.lineVertBuffer, 1024);
    ViewportBase::initMesh(viewerState.pointVertBuffer, 1024);

    QDesktopWidget *desktop = QApplication::desktop();

    rewire();
    window->show();
    window->loadSettings();
    if(window->pos().x() <= 0 || window->pos().y() <= 0) {
        window->setGeometry(desktop->availableGeometry().topLeft().x() + 20,
                            desktop->availableGeometry().topLeft().y() + 50,
                            1024, 800);
    }

    state->viewerState->renderInterval = FAST;

    // for arbitrary viewport orientation
    window->forEachOrthoVPDo([&](ViewportOrtho & vp) {
        vp.s_max =  vp.t_max =
                (
                    (
                        (int)((state->M/2 + 1)
                              * state->cubeEdgeLength
                              / sqrt(2))
                        )
                    / 2)
                *2;
    });
    resetRotation();

    state->viewerState->movementAreaFactor = 80;
    state->viewerState->showOverlay = true;
    state->viewerState->autoTracingMode = navigationMode::recenter;
    state->viewerState->autoTracingDelay = 50;
    state->viewerState->autoTracingSteps = 10;

    state->viewerState->depthCutOff = state->viewerState->depthCutOff;
    state->viewerState->cumDistRenderThres = 7.f; //in screen pixels

    QObject::connect(&Segmentation::singleton(), &Segmentation::appendedRow, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::changedRow, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::removedRow, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetData, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetSelection, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::renderAllObjsChanged, this, &Viewer::oc_reslice_notify_visible);

    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, this, &Viewer::updateCurrentPosition);
    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, this, &Viewer::dc_reslice_notify_visible);
    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(this, &Viewer::movementAreaFactorChangedSignal, this, &Viewer::dc_reslice_notify_visible);

    if (state->gpuSlicer) {
//        gpucubeedge = 128;
        layers.emplace_back(*window->viewportXY->context());
        layers.back().createBogusCube(state->cubeEdgeLength, gpucubeedge);
        layers.emplace_back(*window->viewportXY->context());
//        layers.back().enabled = false;
        layers.back().isOverlayData = true;
        layers.back().createBogusCube(state->cubeEdgeLength, gpucubeedge);
    }

    baseTime.start();//keyRepeat timer
}

void Viewer::setMovementAreaFactor(float alpha) {
    state->viewerState->movementAreaFactor = alpha;
    emit movementAreaFactorChangedSignal();
}

bool Viewer::dcSliceExtract(char *datacube, Coordinate cubePosInAbsPx, char *slice, size_t dcOffset, ViewportOrtho & vp, bool useCustomLUT) {
    datacube += dcOffset;
    const auto & session = Session::singleton();
    const Coordinate areaMinCoord = {session.movementAreaMin.x,
                                     session.movementAreaMin.y,
                                     session.movementAreaMin.z};
    const Coordinate areaMaxCoord = {session.movementAreaMax.x,
                                     session.movementAreaMax.y,
                                     session.movementAreaMax.z};

    const auto partlyInMovementArea =
       areaMinCoord.x > cubePosInAbsPx.x || areaMaxCoord.x < cubePosInAbsPx.x + state->cubeEdgeLength * state->magnification ||
       areaMinCoord.y > cubePosInAbsPx.y || areaMaxCoord.y < cubePosInAbsPx.y + state->cubeEdgeLength * state->magnification ||
       areaMinCoord.z > cubePosInAbsPx.z || areaMaxCoord.z < cubePosInAbsPx.z + state->cubeEdgeLength * state->magnification;

    const std::size_t innerLoopBoundary = vp.viewportType == VIEWPORT_XZ ? state->cubeEdgeLength : state->cubeSliceArea;
    const std::size_t outerLoopBoundary = vp.viewportType == VIEWPORT_XZ ? state->cubeEdgeLength : 1;
    const std::size_t voxelIncrement = vp.viewportType == VIEWPORT_YZ ? state->cubeEdgeLength : 1;
    const std::size_t sliceIncrement = vp.viewportType == VIEWPORT_XY ? state->cubeEdgeLength : state->cubeSliceArea;
    const std::size_t sliceSubLineIncrement = sliceIncrement - state->cubeEdgeLength;

    int offsetX = 0, offsetY = 0;
    int pixelsPerLine = state->cubeEdgeLength*state->magnification;
    for(std::size_t y = 0; y < outerLoopBoundary; y++) {
        for(std::size_t x = 0; x < innerLoopBoundary; x++) {
            uint8_t r, g, b;
            if(useCustomLUT) {
                //extract data as unsigned number from the datacube
                const uint8_t adjustIndex = reinterpret_cast<uint8_t*>(datacube)[0];
                r = std::get<0>(state->viewerState->datasetAdjustmentTable[adjustIndex]);
                g = std::get<1>(state->viewerState->datasetAdjustmentTable[adjustIndex]);
                b = std::get<2>(state->viewerState->datasetAdjustmentTable[adjustIndex]);
            } else {
                r = g = b = reinterpret_cast<uint8_t*>(datacube)[0];
            }
            if(partlyInMovementArea) {
                bool factor = false;
                if((vp.viewportType == VIEWPORT_XY && (cubePosInAbsPx.y + offsetY < areaMinCoord.y || cubePosInAbsPx.y + offsetY > areaMaxCoord.y)) ||
                    ((vp.viewportType == VIEWPORT_XZ || vp.viewportType == VIEWPORT_YZ) && (cubePosInAbsPx.z + offsetY < areaMinCoord.z || cubePosInAbsPx.z + offsetY > areaMaxCoord.z))) {
                    // vertically out of movement area
                    factor = true;
                }
                else if(((vp.viewportType == VIEWPORT_XY || vp.viewportType == VIEWPORT_XZ) && (cubePosInAbsPx.x + offsetX < areaMinCoord.x || cubePosInAbsPx.x + offsetX > areaMaxCoord.x)) ||
                        (vp.viewportType == VIEWPORT_YZ && (cubePosInAbsPx.y + offsetX < areaMinCoord.y || cubePosInAbsPx.y + offsetX > areaMaxCoord.y))) {
                    // horizontally out of movement area
                    factor = true;
                }
                if (factor) {
                    float d = state->viewerState->movementAreaFactor * 1.0 / 100;
                    r *= d; g *= d; b *= d;
                }
            }
            reinterpret_cast<uint8_t*>(slice)[0] = r;
            reinterpret_cast<uint8_t*>(slice)[1] = g;
            reinterpret_cast<uint8_t*>(slice)[2] = b;

            datacube += voxelIncrement;
            slice += 3;
            offsetX = (offsetX + state->magnification) % pixelsPerLine;
            offsetY += (offsetX == 0)? state->magnification : 0; // at end of line increment to next line
        }
        datacube += sliceSubLineIncrement;
    }

    return true;
}

bool Viewer::dcSliceExtract_arb(char *datacube, ViewportOrtho & vp, floatCoordinate *currentPxInDc_float, int s, int *t, bool useCustomLUT) {
    char *slice = vp.viewPortData;
    Coordinate currentPxInDc;
    int sliceIndex = 0, dcIndex = 0;
    floatCoordinate *v2 = &(vp.v2);

    currentPxInDc = {roundFloat(currentPxInDc_float->x), roundFloat(currentPxInDc_float->y), roundFloat(currentPxInDc_float->z)};

    if((currentPxInDc.x < 0) || (currentPxInDc.y < 0) || (currentPxInDc.z < 0) ||
       (currentPxInDc.x >= state->cubeEdgeLength) || (currentPxInDc.y >= state->cubeEdgeLength) || (currentPxInDc.z >= state->cubeEdgeLength)) {
        sliceIndex = 3 * ( s + *t  *  state->cubeEdgeLength * state->M);
        slice[sliceIndex] = slice[sliceIndex + 1]
                          = slice[sliceIndex + 2]
                          = state->viewerState->defaultTexData[0];
        (*t)++;
        if(*t < vp.t_max) {
            // Actually, although currentPxInDc_float is passed by reference and updated here,
            // it is totally ignored (i.e. not read, then overwritten) by the calling function.
            // But to keep the functionality here compatible after this bugfix, we also replicate
            // this update here - from the originial below
            *currentPxInDc_float += *v2;
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
                //extract data as unsigned number from the datacube
                const unsigned char adjustIndex = reinterpret_cast<unsigned char*>(datacube)[dcIndex];
                slice[sliceIndex + 0] = std::get<0>(state->viewerState->datasetAdjustmentTable[adjustIndex]);
                slice[sliceIndex + 1] = std::get<1>(state->viewerState->datasetAdjustmentTable[adjustIndex]);
                slice[sliceIndex + 2] = std::get<2>(state->viewerState->datasetAdjustmentTable[adjustIndex]);
            }
            else {
                slice[sliceIndex] = slice[sliceIndex + 1] = slice[sliceIndex + 2] = datacube[dcIndex];
            }
        }
        (*t)++;
        if(*t >= vp.t_max) {
            break;
        }
        *currentPxInDc_float += *v2;
        currentPxInDc = {roundFloat(currentPxInDc_float->x), roundFloat(currentPxInDc_float->y), roundFloat(currentPxInDc_float->z)};
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
void Viewer::ocSliceExtract(char *datacube, Coordinate cubePosInAbsPx, char *slice, size_t dcOffset, ViewportOrtho & vp) {
    datacube += dcOffset;

    const auto & session = Session::singleton();
    const Coordinate areaMinCoord = {session.movementAreaMin.x,
                                     session.movementAreaMin.y,
                                     session.movementAreaMin.z};
    const Coordinate areaMaxCoord = {session.movementAreaMax.x,
                                     session.movementAreaMax.y,
                                     session.movementAreaMax.z};

    const auto partlyInMovementArea =
       areaMinCoord.x > cubePosInAbsPx.x || areaMaxCoord.x < cubePosInAbsPx.x + state->cubeEdgeLength * state->magnification ||
       areaMinCoord.y > cubePosInAbsPx.y || areaMaxCoord.y < cubePosInAbsPx.y + state->cubeEdgeLength * state->magnification ||
       areaMinCoord.z > cubePosInAbsPx.z || areaMaxCoord.z < cubePosInAbsPx.z + state->cubeEdgeLength * state->magnification;

    const std::size_t innerLoopBoundary = vp.viewportType == VIEWPORT_XZ ? state->cubeEdgeLength : state->cubeSliceArea;
    const std::size_t outerLoopBoundary = vp.viewportType == VIEWPORT_XZ ? state->cubeEdgeLength : 1;
    const std::size_t voxelIncrement = vp.viewportType == VIEWPORT_YZ ? state->cubeEdgeLength * OBJID_BYTES : OBJID_BYTES;
    const std::size_t sliceIncrement = vp.viewportType == VIEWPORT_XY ? state->cubeEdgeLength * OBJID_BYTES : state->cubeSliceArea * OBJID_BYTES;
    const std::size_t sliceSubLineIncrement = sliceIncrement - state->cubeEdgeLength * OBJID_BYTES;

    auto & seg = Segmentation::singleton();
    //cache
    uint64_t subobjectIdCache = Segmentation::singleton().getBackgroundId();
    bool selectedCache = seg.isSubObjectIdSelected(subobjectIdCache);
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> colorCache = seg.colorObjectFromSubobjectId(subobjectIdCache);
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
                if((vp.viewportType == VIEWPORT_XY && (cubePosInAbsPx.y + offsetY < areaMinCoord.y || cubePosInAbsPx.y + offsetY > areaMaxCoord.y)) ||
                    ((vp.viewportType == VIEWPORT_XZ || vp.viewportType == VIEWPORT_YZ) && (cubePosInAbsPx.z + offsetY < areaMinCoord.z || cubePosInAbsPx.z + offsetY > areaMaxCoord.z))) {
                    // vertically out of movement area
                    reinterpret_cast<uint8_t*>(slice)[3] = 0;
                    hide = true;
                }
                else if(((vp.viewportType == VIEWPORT_XY || vp.viewportType == VIEWPORT_XZ) && (cubePosInAbsPx.x + offsetX < areaMinCoord.x || cubePosInAbsPx.x + offsetX > areaMaxCoord.x)) ||
                        (vp.viewportType == VIEWPORT_YZ && (cubePosInAbsPx.y + offsetX < areaMinCoord.y || cubePosInAbsPx.y + offsetX > areaMaxCoord.y))) {
                    // horizontally out of movement area
                    reinterpret_cast<uint8_t*>(slice)[3] = 0;
                    hide = true;
                }
            }

            if(hide == false) {
                uint64_t subobjectId = *reinterpret_cast<uint64_t*>(datacube);

                auto color = (subobjectIdCache == subobjectId) ? colorCache : seg.colorObjectFromSubobjectId(subobjectId);
                reinterpret_cast<uint8_t*>(slice)[0] = std::get<0>(color);
                reinterpret_cast<uint8_t*>(slice)[1] = std::get<1>(color);
                reinterpret_cast<uint8_t*>(slice)[2] = std::get<2>(color);
                reinterpret_cast<uint8_t*>(slice)[3] = std::get<3>(color);

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
                                reinterpret_cast<uint8_t*>(slice)[3] = std::min(255, reinterpret_cast<uint8_t*>(slice)[3]*4);
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
                        reinterpret_cast<uint8_t*>(slice)[3] = std::min(255, reinterpret_cast<uint8_t*>(slice)[3]*4);
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

static int texIndex(uint x, uint y, uint colorMultiplicationFactor, viewportTexture *texture) {
    uint index = 0;

    index = x * state->cubeSliceArea + y
            * (texture->edgeLengthDc * state->cubeSliceArea)
            * colorMultiplicationFactor;

    return index;
}

bool Viewer::vpGenerateTexture(ViewportOrtho & vp) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.

    const CoordInCube currentPosition_dc = state->viewerState->currentPosition.insideCube(state->cubeEdgeLength, state->magnification);

    bool dc_reslice, oc_reslice;
    int slicePositionWithinCube;
    switch(vp.viewportType) {
    case VIEWPORT_XY:
        slicePositionWithinCube = state->cubeSliceArea * currentPosition_dc.z;
        if(!dc_xy_changed && !oc_xy_changed) {
            return true;
        }
        dc_reslice = dc_xy_changed;
        oc_reslice = oc_xy_changed;

        dc_xy_changed = false;
        oc_xy_changed = false;
        break;
    case VIEWPORT_XZ:
        slicePositionWithinCube = state->cubeEdgeLength * currentPosition_dc.y;
        if(!dc_xz_changed && !oc_xz_changed) {
            return true;
        }
        dc_reslice = dc_xz_changed;
        oc_reslice = oc_xz_changed;

        dc_xz_changed = false;
        oc_xz_changed = false;
        break;
    case VIEWPORT_YZ:
        slicePositionWithinCube = currentPosition_dc.x;
        if(!dc_zy_changed && !oc_zy_changed) {
            return true;
        }
        dc_reslice = dc_zy_changed;
        oc_reslice = oc_zy_changed;

        dc_zy_changed = false;
        oc_zy_changed = false;
        break;
    default:
        qDebug("No such slice view: %d.", vp.viewportType);
        return false;
    }

    const CoordOfCube upperLeftDc = vp.texture.leftUpperPxInAbsPx.cube(state->cubeEdgeLength, state->magnification);

    // We iterate over the texture with x and y being in a temporary coordinate
    // system local to this texture.
    for(int x_dc = 0; x_dc < vp.texture.usedTexLengthDc; x_dc++) {
        for(int y_dc = 0; y_dc < vp.texture.usedTexLengthDc; y_dc++) {
            const int x_px = x_dc * state->cubeEdgeLength;
            const int y_px = y_dc * state->cubeEdgeLength;

            CoordOfCube currentDc;

            switch(vp.viewportType) {
            // With an x/y-coordinate system in a viewport, we get the following
            // mapping from viewport (slice) coordinates to global (dc)
            // coordinates:
            // XY-slice: x local is x global, y local is y global
            // XZ-slice: x local is x global, y local is z global
            // YZ-slice: x local is y global, y local is z global.
            case VIEWPORT_XY:
                currentDc = {upperLeftDc.x + x_dc, upperLeftDc.y + y_dc, upperLeftDc.z};
                break;
            case VIEWPORT_XZ:
                currentDc = {upperLeftDc.x + x_dc, upperLeftDc.y, upperLeftDc.z + y_dc};
                break;
            case VIEWPORT_YZ:
                currentDc = {upperLeftDc.x, upperLeftDc.y + x_dc, upperLeftDc.z + y_dc};
                break;
            default:
                qDebug("No such slice type (%d) in vpGenerateTexture.", vp.viewportType);
            }
            state->protectCube2Pointer.lock();
            char * const datacube = Coordinate2BytePtr_hash_get_or_fail(state->Dc2Pointer[int_log(state->magnification)], currentDc);
            char * const overlayCube = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[int_log(state->magnification)], currentDc);
            state->protectCube2Pointer.unlock();

            // Take care of the data textures.

            Coordinate cubePosInAbsPx = {currentDc.x * state->magnification * state->cubeEdgeLength,
                                         currentDc.y * state->magnification * state->cubeEdgeLength,
                                         currentDc.z * state->magnification * state->cubeEdgeLength};
            if (dc_reslice) {
                glBindTexture(GL_TEXTURE_2D, vp.texture.texHandle);

                // This is used to index into the texture. overlayData[index] is the first
                // byte of the datacube slice at position (x_dc, y_dc) in the texture.
                const int index = texIndex(x_dc, y_dc, 3, &(vp.texture));

                if(datacube == nullptr) {
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
                                   slicePositionWithinCube,
                                   vp,
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
            }
            //Take care of the overlay textures.
            if (state->overlay && oc_reslice && state->viewerState->showOverlay) {
                glBindTexture(GL_TEXTURE_2D, vp.texture.overlayHandle);
                // This is used to index into the texture. texData[index] is the first
                // byte of the datacube slice at position (x_dc, y_dc) in the texture.
                const int index = texIndex(x_dc, y_dc, 4, &(vp.texture));

                if (overlayCube == nullptr) {
                    glTexSubImage2D(GL_TEXTURE_2D,
                                    0,
                                    x_px,
                                    y_px,
                                    state->cubeEdgeLength,
                                    state->cubeEdgeLength,
                                    GL_RGBA,
                                    GL_UNSIGNED_BYTE,
                                    state->viewerState->defaultOverlayData);
                } else {
                    ocSliceExtract(overlayCube,
                                   cubePosInAbsPx,
                                   state->viewerState->overlayData + index,
                                   slicePositionWithinCube * OBJID_BYTES,
                                   vp);

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

void Viewer::vpGenerateTexture_arb(ViewportOrtho & vp) {
    if (!dc_xy_changed && !dc_xz_changed && !dc_zy_changed) {
        return;
    }
    switch(vp.id) {
    case VP_UPPERLEFT: dc_xy_changed = false; break;
    case VP_LOWERLEFT: dc_xz_changed = false; break;
    case VP_UPPERRIGHT: dc_zy_changed = false; break;
    }

    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.
    floatCoordinate currentPxInDc_float, rowPx_float, currentPx_float;

    char *datacube = NULL;

    rowPx_float = vp.texture.leftUpperPxInAbsPx / state->magnification;
    currentPx_float = rowPx_float;

    glBindTexture(GL_TEXTURE_2D, vp.texture.texHandle);

    int s = 0, t = 0, t_old = 0;
    while(s < vp.s_max) {
        t = 0;
        while(t < vp.t_max) {
            Coordinate currentPx = {roundFloat(currentPx_float.x), roundFloat(currentPx_float.y), roundFloat(currentPx_float.z)};
            Coordinate currentDc = currentPx / state->cubeEdgeLength;

            if(currentPx.x < 0) { currentDc.x -= 1; }
            if(currentPx.y < 0) { currentDc.y -= 1; }
            if(currentPx.z < 0) { currentDc.z -= 1; }

            state->protectCube2Pointer.lock();
            datacube = Coordinate2BytePtr_hash_get_or_fail(state->Dc2Pointer[int_log(state->magnification)], {currentDc.x, currentDc.y, currentDc.z});
            state->protectCube2Pointer.unlock();

            currentPxInDc_float = currentPx_float - currentDc * state->cubeEdgeLength;
            t_old = t;
            dcSliceExtract_arb(datacube,
                               vp,
                               &currentPxInDc_float,
                               s, &t,
                               state->viewerState->datasetAdjustmentOn);
            currentPx_float = currentPx_float + vp.v2 * (t - t_old);
        }
        s++;
        rowPx_float += vp.v1;
        currentPx_float = rowPx_float;
    }

    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    state->M*state->cubeEdgeLength,
                    state->M*state->cubeEdgeLength,
                    GL_RGB,
                    GL_UNSIGNED_BYTE,
                    vp.viewPortData);

    glBindTexture(GL_TEXTURE_2D, 0);
}

/* this function calculates the mapping between the left upper texture pixel
 * and the real dataset pixel */
bool Viewer::calcLeftUpperTexAbsPx() {
    auto & viewerState = *state->viewerState;

    CoordOfCube currentPosition_dc = viewerState.currentPosition.cube(state->cubeEdgeLength, state->magnification);

    //iterate over all viewports
    //this function has to be called after the texture changed or the user moved, in the sense of a
    //realignment of the data
    window->forEachOrthoVPDo([&viewerState, &currentPosition_dc](ViewportOrtho & orthoVP) {
        switch (orthoVP.viewportType) {
        case VIEWPORT_XY:
            //Set the coordinate of left upper data pixel currently stored in the texture
            //orthoVP.texture.usedTexLengthDc is state->M and even int.. very funny
            // this guy is always in mag1, even if a different mag dataset is active!!!
            // this might be buggy for very high mags, test that!
            orthoVP.texture.leftUpperPxInAbsPx = {
                currentPosition_dc.x - static_cast<int>(orthoVP.texture.usedTexLengthDc / 2)
                , currentPosition_dc.y - static_cast<int>(orthoVP.texture.usedTexLengthDc / 2)
                , currentPosition_dc.z
            };
            orthoVP.texture.leftUpperPxInAbsPx *= state->cubeEdgeLength * state->magnification;
            //if(orthoVP.texture.leftUpperPxInAbsPx.x >1000000){ qDebug("uninit x %d", orthoVP.texture.leftUpperPxInAbsPx.x);}
            //if(orthoVP.texture.leftUpperPxInAbsPx.y > 1000000){ qDebug("uninit y %d", orthoVP.texture.leftUpperPxInAbsPx.y);}
            //if(orthoVP.texture.leftUpperPxInAbsPx.z > 1000000){ qDebug("uninit z %d", orthoVP.texture.leftUpperPxInAbsPx.z);}

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            orthoVP.leftUpperDataPxOnScreen = {
                viewerState.currentPosition.x - static_cast<int>((orthoVP.texture.displayedEdgeLengthX / 2.) / orthoVP.texture.texUnitsPerDataPx)
                , viewerState.currentPosition.y - static_cast<int>((orthoVP.texture.displayedEdgeLengthY / 2.) / orthoVP.texture.texUnitsPerDataPx)
                , viewerState.currentPosition.z
            };
            break;
        case VIEWPORT_XZ:
            //Set the coordinate of left upper data pixel currently stored in the texture
            orthoVP.texture.leftUpperPxInAbsPx = {
                currentPosition_dc.x - static_cast<int>(orthoVP.texture.usedTexLengthDc / 2)
                , currentPosition_dc.y
                , currentPosition_dc.z - static_cast<int>(orthoVP.texture.usedTexLengthDc / 2)
            };
            orthoVP.texture.leftUpperPxInAbsPx *= state->cubeEdgeLength * state->magnification;
            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            orthoVP.leftUpperDataPxOnScreen = {
                viewerState.currentPosition.x - static_cast<int>((orthoVP.texture.displayedEdgeLengthX / 2.) / orthoVP.texture.texUnitsPerDataPx)
                , viewerState.currentPosition.y
                , viewerState.currentPosition.z - static_cast<int>((orthoVP.texture.displayedEdgeLengthY / 2.) / orthoVP.texture.texUnitsPerDataPx)
            };
            break;
        case VIEWPORT_YZ:
            //Set the coordinate of left upper data pixel currently stored in the texture
            orthoVP.texture.leftUpperPxInAbsPx = {
                currentPosition_dc.x
                , currentPosition_dc.y - static_cast<int>(orthoVP.texture.usedTexLengthDc / 2)
                , currentPosition_dc.z - static_cast<int>(orthoVP.texture.usedTexLengthDc / 2)
            };
            orthoVP.texture.leftUpperPxInAbsPx *= state->cubeEdgeLength   * state->magnification;

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            orthoVP.leftUpperDataPxOnScreen = {
                viewerState.currentPosition.x
                , viewerState.currentPosition.y - static_cast<int>((orthoVP.texture.displayedEdgeLengthX / 2.) / orthoVP.texture.texUnitsPerDataPx)
                , viewerState.currentPosition.z - static_cast<int>((orthoVP.texture.displayedEdgeLengthY / 2.) / orthoVP.texture.texUnitsPerDataPx)
            };
            break;
        case VIEWPORT_ARBITRARY:
            orthoVP.leftUpperPxInAbsPx_float = viewerState.currentPosition - (orthoVP.v1 * orthoVP.s_max/2 + orthoVP.v2 * orthoVP.t_max/2) * state->magnification;

            orthoVP.texture.leftUpperPxInAbsPx = orthoVP.leftUpperPxInAbsPx_float;

            orthoVP.leftUpperDataPxOnScreen_float =
                viewerState.currentPosition
                - orthoVP.v1 * ((orthoVP.texture.displayedEdgeLengthX / 2.) / orthoVP.texture.texUnitsPerDataPx)
                - orthoVP.v2 * ((orthoVP.texture.displayedEdgeLengthY / 2.) / orthoVP.texture.texUnitsPerDataPx)
            ;

            orthoVP.leftUpperDataPxOnScreen = {
                roundFloat(orthoVP.leftUpperDataPxOnScreen.x)
                , roundFloat(orthoVP.leftUpperDataPxOnScreen.y)
                , roundFloat(orthoVP.leftUpperDataPxOnScreen.z)
            };
            break;
        default:
            orthoVP.texture.leftUpperPxInAbsPx.x = 0;
            orthoVP.texture.leftUpperPxInAbsPx.y = 0;
            orthoVP.texture.leftUpperPxInAbsPx.z = 0;
        }
    });
    return false;
}

void Viewer::initViewer() {
    // This is the buffer that holds the actual texture data (for _all_ textures)

    state->viewerState->texData =
            (char *) malloc(TEXTURE_EDGE_LEN
               * TEXTURE_EDGE_LEN
               * sizeof(char)
               * 3);
    if(state->viewerState->texData == NULL) {
        qDebug() << "Out of memory.";
        _Exit(false);
    }
    memset(state->viewerState->texData, '\0',
           TEXTURE_EDGE_LEN
           * TEXTURE_EDGE_LEN
           * sizeof(char)
           * 3);

    // This is the buffer that holds the actual overlay texture data (for _all_ textures)

    state->viewerState->overlayData =
            (char *) malloc(TEXTURE_EDGE_LEN *
               TEXTURE_EDGE_LEN *
               sizeof(char) *
               4);
    if(state->viewerState->overlayData == NULL) {
        qDebug() << "Out of memory.";
        _Exit(false);
    }
    memset(state->viewerState->overlayData, '\0',
           TEXTURE_EDGE_LEN
           * TEXTURE_EDGE_LEN
           * sizeof(char)
           * 4);

    // This is the data we use when the data for the
    //   slices is not yet available (hasn't yet been loaded).

    state->viewerState->defaultTexData = (char *) malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                         * sizeof(char)
                                                         * 3);
    if(state->viewerState->defaultTexData == NULL) {
        qDebug() << "Out of memory.";
        _Exit(false);
    }
    memset(state->viewerState->defaultTexData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                     * sizeof(char)
                                                     * 3);

    /* @arb */
    window->forEachOrthoVPDo([this](ViewportOrtho & orthoVP) {
        orthoVP.viewPortData = (char *)malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(char) * 3);
        if(orthoVP.viewPortData == NULL) {
            qDebug() << "Out of memory.";
            exit(0);
        }
        memset(orthoVP.viewPortData, state->viewerState->defaultTexData[0], TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(char) * 3);
    });

    // Default data for the overlays
    state->viewerState->defaultOverlayData = (char *) malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                             * sizeof(char)
                                                             * 4);
    if(state->viewerState->defaultOverlayData == NULL) {
        qDebug() << "Out of memory.";
        _Exit(false);
    }
    memset(state->viewerState->defaultOverlayData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                         * sizeof(char)
                                                         * 4);

    recalcTextureOffsets();
}

bool Viewer::calcDisplayedEdgeLength() {
    float FOVinDCs;

    FOVinDCs = ((float)state->M) - 1.f;

    window->forEachOrthoVPDo([&](ViewportOrtho & vpOrtho) {
        if (vpOrtho.viewportType==VIEWPORT_ARBITRARY) {
            vpOrtho.texture.displayedEdgeLengthX = vpOrtho.s_max / (float) vpOrtho.texture.edgeLengthPx;
            vpOrtho.texture.displayedEdgeLengthY = vpOrtho.t_max / (float) vpOrtho.texture.edgeLengthPx;
        }
        else {
            vpOrtho.texture.displayedEdgeLengthX = vpOrtho.texture.displayedEdgeLengthY = FOVinDCs * (float)state->cubeEdgeLength / (float) vpOrtho.texture.edgeLengthPx;
        }
    });
    return true;
}

void Viewer::zoom(const float factor) {
    int triggerMagChange = false;
    if (vpUpperLeft->displayedEdgeLenghtXForZoomFactor(vpUpperLeft->texture.zoomLevel * factor) == 0
            || vpUpperLeft->screenPxXPerDataPxForZoomFactor(vpUpperLeft->texture.zoomLevel * factor) < VPZOOMMIN) {
        return;
    }
    bool magUp = std::floor((vpUpperLeft->texture.zoomLevel*2)+0.5)/2 == 1 && factor > 1;
    bool magDown = std::floor((vpUpperLeft->texture.zoomLevel*2)+0.5)/2 == 0.5 && factor < 1;
    state->viewer->window->forEachOrthoVPDo([&factor, &triggerMagChange, &magUp, &magDown](ViewportOrtho & orthoVP) {
        if(state->viewerState->datasetMagLock) {
            orthoVP.texture.zoomLevel *= factor;
        }
        else {
            if (magDown && (static_cast<uint>(state->magnification) != state->lowestAvailableMag)) {
                orthoVP.texture.zoomLevel = 1;
                triggerMagChange = MAG_DOWN;
            }
            else if (magUp && (static_cast<uint>(state->magnification) != state->highestAvailableMag)) {
                orthoVP.texture.zoomLevel = 0.5;
                triggerMagChange = MAG_UP;
            }
            else {
                orthoVP.texture.zoomLevel *= factor;
            }
        }
    });

    if (triggerMagChange) {
        changeDatasetMag(triggerMagChange);
    }
    recalcTextureOffsets();
    emit zoomChanged();
}

void Viewer::zoomReset() {
    state->viewer->window->forEachOrthoVPDo([](ViewportOrtho & orthoVP){
        orthoVP.texture.zoomLevel = 1;
    });
    recalcTextureOffsets();
    emit zoomChanged();
}

/**
* takes care of all necessary changes inside the viewer and signals
* the loader to change the dataset
*/
/* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
bool Viewer::changeDatasetMag(uint upOrDownFlag) {
    if (DATA_SET != upOrDownFlag) {

        if(state->viewerState->datasetMagLock) {
            return false;
        }

        switch(upOrDownFlag) {
        case MAG_DOWN:
            if (static_cast<uint>(state->magnification) > state->lowestAvailableMag) {
                state->magnification /= 2;
                window->forEachOrthoVPDo([](ViewportOrtho & orthoVP) {
                    orthoVP.texture.texUnitsPerDataPx *= 2.;
                });
            }
            else return false;
            break;

        case MAG_UP:
            if (static_cast<uint>(state->magnification)  < state->highestAvailableMag) {
                state->magnification *= 2;
                window->forEachOrthoVPDo([](ViewportOrtho & orthoVP) {
                    orthoVP.texture.texUnitsPerDataPx /= 2.;
                });
            }
            else return false;
            break;
        }
    }

    Loader::Controller::singleton().unloadCurrentMagnification();//unload all the cubes
    //clear the viewports
    dc_reslice_notify_visible();
    oc_reslice_notify_visible();

    loader_notify();//start loading

    emit zoomChanged();

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
    if (state->quitSignal) {//don’t do anything, when the rest is already going to sleep
        qDebug() << "viewer returned";
        return;
    }

    //start the timer before the rendering, else render interval and actual rendering time would accumulate
    timer.singleShot(QApplication::activeWindow() != nullptr ? state->viewerState->renderInterval : SLOW, this, &Viewer::run);

    if (state->viewerKeyRepeat && (state->keyF || state->keyD)) {
        qint64 interval = 1000 / state->viewerState->stepsPerSec;
        if (baseTime.elapsed() >= interval) {
            baseTime.restart();
            if (vpUpperLeft->viewportType != VIEWPORT_ARBITRARY) {
                userMove(Coordinate(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2]), USERMOVE_DRILL);
            } else {
                userMove_arb({state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2]}, USERMOVE_DRILL);
            }
        }
    }
    // Event and rendering loop.
    // What happens is that we go through lists of pending texture parts and load
    // them if they are available.
    // While we are loading the textures, we check for events. Some events
    // might cancel the current loading process. When all textures
    // have been processed, we go into an idle state, in which we wait for events.

    // for arbitrary viewport orientation
    if(rotation.alpha != 0) {
        rotateAndNormalize(v1, rotation.axis, rotation.alpha);
        rotateAndNormalize(v2, rotation.axis, rotation.alpha);
        rotateAndNormalize(v3, rotation.axis, rotation.alpha);
        vpUpperLeft->v1 = v1;
        vpUpperLeft->v2 = v2;
        vpUpperLeft->n = v3;
        vpLowerLeft->v1 = v1;
        vpLowerLeft->v2 = v3;
        vpLowerLeft->n = v2;
        vpUpperRight->v1 = v3;
        vpUpperRight->v2 = v2;
        vpUpperRight->n = v1;
        rotation = Rotation();
        alphaCache = 0;
    }

    if (state->gpuSlicer && gpuRendering) {
        QElapsedTimer timer;
        timer.start();
        for (auto & layer : layers) {
            calculateMissingGPUCubes(layer);
            while (!layer.pendingCubes.empty() && !timer.hasExpired(3)) {
                const auto pair = layer.pendingCubes.back();
                layer.pendingCubes.pop_back();
                const auto globalCoord = pair.first.cube2Global(gpucubeedge, state->magnification);
                const auto cubeCoord = globalCoord.cube(state->cubeEdgeLength, state->magnification);
                state->protectCube2Pointer.lock();
                const auto * ptr = Coordinate2BytePtr_hash_get_or_fail((layer.isOverlayData ? state->Oc2Pointer : state->Dc2Pointer)[int_log(state->magnification)], cubeCoord);
                state->protectCube2Pointer.unlock();
                if (ptr != nullptr) {
                    layer.cubeSubArray(ptr, state->cubeEdgeLength, gpucubeedge, pair.first, pair.second);
                }
            }
        }
    }

    recalcTextureOffsets();//should be in userMove and setVPOrientation but that’s infeasable because vp update is async
    window->forEachOrthoVPDo([this](ViewportOrtho & vp) {
        if (vp.isVisible()) {
            vp.makeCurrent();
        }
        if (!gpuRendering) {
            if (!ViewportOrtho::arbitraryOrientation) {
                vpGenerateTexture(vp);
            } else {
                vpGenerateTexture_arb(vp);
            }
        }
        vp.update();
    });
    window->viewport3D.get()->update();
    window->updateTitlebar(); //display changes after filename
}

void Viewer::applyTextureFilterSetting(const GLint texFiltering) {
    window->forEachOrthoVPDo([&texFiltering](ViewportOrtho & orthoVP) {
        orthoVP.makeCurrent();
        glBindTexture(GL_TEXTURE_2D, orthoVP.texture.texHandle);
        // Set the parameters for the texture.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texFiltering);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFiltering);
        glBindTexture(GL_TEXTURE_2D, orthoVP.texture.overlayHandle);
        // Set the parameters for the texture.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texFiltering);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFiltering);
    });
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Viewer::updateCurrentPosition() {
    auto & session = Session::singleton();
    if (session.outsideMovementArea(state->viewerState->currentPosition)) {
        const Coordinate currPos = state->viewerState->currentPosition;
        const Coordinate newPos = state->viewerState->currentPosition.capped(session.movementAreaMin, session.movementAreaMax);
        userMove(newPos - currPos, USERMOVE_NEUTRAL);
    }
}

void Viewer::userMove(const Coordinate & step, UserMoveType userMoveType, const Coordinate & viewportNormal) {
    auto & viewerState = *state->viewerState;

    if (ViewportOrtho::arbitraryOrientation && (step.z != 0 || step.x != 0 || step.y != 0)) {//slices are arbitrary…
        dc_xy_changed = oc_xy_changed = dc_zy_changed = oc_zy_changed = dc_xz_changed = oc_xz_changed = true;
    } else {
        if (step.z != 0) {
            dc_xy_changed = oc_xy_changed = true;
        }
        if (step.x != 0) {
            dc_zy_changed = oc_zy_changed = true;
        }
        if (step.y != 0) {
            dc_xz_changed = oc_xz_changed = true;
        }
    }

    // This determines whether the server will broadcast the coordinate change
    // to its client or not.

    const auto lastPosition_dc = viewerState.currentPosition.cube(state->cubeEdgeLength, state->magnification);
    const auto lastPosition_gpudc = viewerState.currentPosition.cube(gpucubeedge, state->magnification);

    const Coordinate movement = step;
    auto newPos = viewerState.currentPosition + movement;
    if (!Session::singleton().outsideMovementArea(newPos)) {
            viewerState.currentPosition = newPos;
    } else {
        qDebug() << tr("Position (%1, %2, %3) out of bounds").arg(newPos.x + 1).arg(newPos.y + 1).arg(newPos.z + 1);
    }

    const auto newPosition_dc = viewerState.currentPosition.cube(state->cubeEdgeLength, state->magnification);
    const auto newPosition_gpudc = viewerState.currentPosition.cube(gpucubeedge, state->magnification);

    if (newPosition_dc != lastPosition_dc) {
        dc_reslice_notify_visible();
        oc_reslice_notify_visible();

        state->loaderUserMoveType = userMoveType;
        Coordinate direction = (userMoveType == USERMOVE_DRILL)? step : (userMoveType == USERMOVE_HORIZONTAL)? viewportNormal : Coordinate(0, 0, 0);
        state->loaderUserMoveViewportDirection = direction;
        loader_notify();
    }

    if (state->gpuSlicer && newPosition_gpudc != lastPosition_gpudc) {
        const auto supercubeedge = state->M * state->cubeEdgeLength / gpucubeedge - (state->cubeEdgeLength / gpucubeedge - 1);
        for (auto & layer : layers) {
            layer.ctx.makeCurrent(&layer.surface);
            std::vector<CoordOfGPUCube> obsoleteCubes;
            for (const auto & pair : layer.textures) {
                const auto pos = pair.first;
                const auto globalCoord = pos.cube2Global(gpucubeedge, state->magnification);
                if (!currentlyVisible(globalCoord, viewerState.currentPosition, supercubeedge, gpucubeedge)) {
                    obsoleteCubes.emplace_back(pos);
                }
            }
            for (const auto & pos : obsoleteCubes) {
                layer.textures.erase(pos);
            }
            calculateMissingGPUCubes(layer);
        }
    }

    emit coordinateChangedSignal(viewerState.currentPosition);
}

void Viewer::userMove_arb(const floatCoordinate & floatStep, UserMoveType userMoveType, const Coordinate & viewportNormal) {
    moveCache += floatStep;
    Coordinate step = {static_cast<int>(moveCache.x), static_cast<int>(moveCache.y), static_cast<int>(moveCache.z)};
    moveCache -= step;
    userMove(step, userMoveType, viewportNormal);
}

void Viewer::calculateMissingGPUCubes(TextureLayer & layer) {
    layer.pendingCubes.clear();

    const auto gpusupercube = (state->M - 1) * state->cubeEdgeLength / gpucubeedge + 1;//remove cpu overlap and add gpu overlap
    const int halfSupercube = gpusupercube * 0.5;
    const auto edge = state->viewerState->currentPosition.cube(gpucubeedge, state->magnification) - halfSupercube;
    const auto end = edge + gpusupercube;
    for (int x = edge.x; x < end.x; ++x)
    for (int y = edge.y; y < end.y; ++y)
    for (int z = edge.z; z < end.z; ++z) {
        const auto gpuCoord = CoordOfGPUCube{x, y, z};
        const auto globalCoord = gpuCoord.cube2Global(gpucubeedge, state->magnification);
        if (currentlyVisible(globalCoord, state->viewerState->currentPosition, gpusupercube, gpucubeedge) && layer.textures.count(gpuCoord) == 0) {
            const auto cubeCoord = globalCoord.cube(state->cubeEdgeLength, state->magnification);
            const auto offset = globalCoord - cubeCoord.cube2Global(state->cubeEdgeLength, state->magnification);
            layer.pendingCubes.emplace_back(gpuCoord, offset);
        }
    }
}

void Viewer::dc_reslice_notify_all(const Coordinate coord) {
    if (currentlyVisibleWrapWrap(state->viewerState->currentPosition, coord)) {
        dc_reslice_notify_visible();
    }
}

void Viewer::dc_reslice_notify_visible() {
    dc_xy_changed = true;
    dc_xz_changed = true;
    dc_zy_changed = true;
}

void Viewer::oc_reslice_notify_all(const Coordinate coord) {
    if (currentlyVisibleWrapWrap(state->viewerState->currentPosition, coord)) {
        oc_reslice_notify_visible();
    }
    // if anything has changed, update the volume texture data
    Segmentation::singleton().volume_update_required = true;
}

void Viewer::oc_reslice_notify_visible() {
    oc_xy_changed = true;
    oc_xz_changed = true;
    oc_zy_changed = true;
    // if anything has changed, update the volume texture data
    Segmentation::singleton().volume_update_required = true;
}

bool Viewer::recalcTextureOffsets() {
    float midX = 0.,midY = 0.;

    calcDisplayedEdgeLength();

    window->forEachOrthoVPDo([&](ViewportOrtho & orthoVP) {
        orthoVP.texture.displayedEdgeLengthX *= orthoVP.texture.zoomLevel;
        orthoVP.texture.displayedEdgeLengthY *= orthoVP.texture.zoomLevel;
        switch(orthoVP.viewportType) {
        case VIEWPORT_XY:
            //Aspect ratio correction..
            if(state->viewerState->voxelXYRatio < 1) {
                orthoVP.texture.displayedEdgeLengthY *= state->viewerState->voxelXYRatio;
            }
            else {
                orthoVP.texture.displayedEdgeLengthX /= state->viewerState->voxelXYRatio;
            }
            //Display only entire pixels (only truncation possible!) WHY??
            orthoVP.texture.displayedEdgeLengthX =
                (std::ceil(orthoVP.texture.displayedEdgeLengthX / 2. / orthoVP.texture.texUnitsPerDataPx) * orthoVP.texture.texUnitsPerDataPx) * 2.;
            orthoVP.texture.displayedEdgeLengthY =
                ((std::ceil(orthoVP.texture.displayedEdgeLengthY / 2. / orthoVP.texture.texUnitsPerDataPx)) * orthoVP.texture.texUnitsPerDataPx) * 2.;
            // Update screen pixel to data pixel mapping values
            orthoVP.screenPxXPerDataPx = (float)orthoVP.edgeLength / (orthoVP.texture.displayedEdgeLengthX /  orthoVP.texture.texUnitsPerDataPx);
            orthoVP.screenPxYPerDataPx = (float)orthoVP.edgeLength / (orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.displayedlengthInNmX = state->viewerState->voxelDimX * (orthoVP.texture.displayedEdgeLengthX / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.displayedlengthInNmY = state->viewerState->voxelDimY * (orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx);
            //Update orthoVP.leftUpperDataPxOnScreen with this call
            calcLeftUpperTexAbsPx();

            // scale to 0 - 1; midX is the current pos in tex coords
            // leftUpperPxInAbsPx is in always in mag1, independent of
            // the currently active mag
            midX = (float)(state->viewerState->currentPosition.x - orthoVP.texture.leftUpperPxInAbsPx.x) * orthoVP.texture.texUnitsPerDataPx;
            midY = (float)(state->viewerState->currentPosition.y - orthoVP.texture.leftUpperPxInAbsPx.y) * orthoVP.texture.texUnitsPerDataPx;

            //Offsets for crosshair
            orthoVP.texture.xOffset = ((float)(state->viewerState->currentPosition.x - orthoVP.leftUpperDataPxOnScreen.x)) * orthoVP.screenPxXPerDataPx + 0.5 * orthoVP.screenPxXPerDataPx;
            orthoVP.texture.yOffset = ((float)(state->viewerState->currentPosition.y - orthoVP.leftUpperDataPxOnScreen.y)) * orthoVP.screenPxYPerDataPx + 0.5 * orthoVP.screenPxYPerDataPx;
            break;
        case VIEWPORT_XZ:
            //Aspect ratio correction..
            if(state->viewerState->voxelXYtoZRatio < 1) {
                orthoVP.texture.displayedEdgeLengthY *= state->viewerState->voxelXYtoZRatio;
            }
            else {
                orthoVP.texture.displayedEdgeLengthX /= state->viewerState->voxelXYtoZRatio;
            }
            //Display only entire pixels (only truncation possible!)
            orthoVP.texture.displayedEdgeLengthX = ((std::ceil(orthoVP.texture.displayedEdgeLengthX / 2. / orthoVP.texture.texUnitsPerDataPx)) * orthoVP.texture.texUnitsPerDataPx) * 2.;
            orthoVP.texture.displayedEdgeLengthY = ((std::ceil(orthoVP.texture.displayedEdgeLengthY / 2. / orthoVP.texture.texUnitsPerDataPx)) * orthoVP.texture.texUnitsPerDataPx) * 2.;

            //Update screen pixel to data pixel mapping values
            orthoVP.screenPxXPerDataPx = (float)orthoVP.edgeLength / (orthoVP.texture.displayedEdgeLengthX / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.screenPxYPerDataPx = (float)orthoVP.edgeLength / (orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.displayedlengthInNmX = state->viewerState->voxelDimX * (orthoVP.texture.displayedEdgeLengthX / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.displayedlengthInNmY = state->viewerState->voxelDimZ * (orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx);

            //Update orthoVP.leftUpperDataPxOnScreen with this call
            calcLeftUpperTexAbsPx();

            midX = ((float)(state->viewerState->currentPosition.x - orthoVP.texture.leftUpperPxInAbsPx.x)) * orthoVP.texture.texUnitsPerDataPx; //scale to 0 - 1
            midY = ((float)(state->viewerState->currentPosition.z - orthoVP.texture.leftUpperPxInAbsPx.z)) * orthoVP.texture.texUnitsPerDataPx; //scale to 0 - 1

            //Offsets for crosshair
            orthoVP.texture.xOffset = ((float)(state->viewerState->currentPosition.x - orthoVP.leftUpperDataPxOnScreen.x)) * orthoVP.screenPxXPerDataPx + 0.5 * orthoVP.screenPxXPerDataPx;
            orthoVP.texture.yOffset = ((float)(state->viewerState->currentPosition.z - orthoVP.leftUpperDataPxOnScreen.z)) * orthoVP.screenPxYPerDataPx + 0.5 * orthoVP.screenPxYPerDataPx;
            break;
        case VIEWPORT_YZ:
            //Aspect ratio correction..
            if(state->viewerState->voxelXYtoZRatio < 1) {
                orthoVP.texture.displayedEdgeLengthY *= state->viewerState->voxelXYtoZRatio;
            }
            else {
                orthoVP.texture.displayedEdgeLengthX /= state->viewerState->voxelXYtoZRatio;
            }

            //Display only entire pixels (only truncation possible!)
            orthoVP.texture.displayedEdgeLengthX = ((std::ceil(orthoVP.texture.displayedEdgeLengthX / 2. / orthoVP.texture.texUnitsPerDataPx)) * orthoVP.texture.texUnitsPerDataPx) * 2.;
            orthoVP.texture.displayedEdgeLengthY = ((std::ceil(orthoVP.texture.displayedEdgeLengthY / 2. / orthoVP.texture.texUnitsPerDataPx)) * orthoVP.texture.texUnitsPerDataPx) * 2.;

            // Update screen pixel to data pixel mapping values
            // WARNING: YZ IS ROTATED AND MIRRORED! So screenPxXPerDataPx
            // corresponds to displayedEdgeLengthY and so on.
            orthoVP.screenPxXPerDataPx = (float)orthoVP.edgeLength / (orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.screenPxYPerDataPx = (float)orthoVP.edgeLength / (orthoVP.texture.displayedEdgeLengthX / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.displayedlengthInNmX = state->viewerState->voxelDimZ * (orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.displayedlengthInNmY = state->viewerState->voxelDimY * (orthoVP.texture.displayedEdgeLengthX / orthoVP.texture.texUnitsPerDataPx);

            //Update orthoVP.leftUpperDataPxOnScreen with this call
            calcLeftUpperTexAbsPx();

            midX = ((float)(state->viewerState->currentPosition.y - orthoVP.texture.leftUpperPxInAbsPx.y)) * orthoVP.texture.texUnitsPerDataPx;
            midY = ((float)(state->viewerState->currentPosition.z - orthoVP.texture.leftUpperPxInAbsPx.z)) * orthoVP.texture.texUnitsPerDataPx;

            //Offsets for crosshair
            orthoVP.texture.xOffset = ((float)(state->viewerState->currentPosition.z - orthoVP.leftUpperDataPxOnScreen.z)) * orthoVP.screenPxXPerDataPx + 0.5 * orthoVP.screenPxXPerDataPx;
            orthoVP.texture.yOffset = ((float)(state->viewerState->currentPosition.y - orthoVP.leftUpperDataPxOnScreen.y)) * orthoVP.screenPxYPerDataPx + 0.5 * orthoVP.screenPxYPerDataPx;
            break;
        case VIEWPORT_ARBITRARY:
            //v1: vector in Viewport x-direction, parameter s corresponds to v1
            //v2: vector in Viewport y-direction, parameter t corresponds to v2
            orthoVP.s_max = orthoVP.t_max = (((int)((state->M / 2 + 1) * state->cubeEdgeLength / sqrt(2.))) / 2) * 2;
            float voxelV1X = sqrtf(powf(orthoVP.v1.x, 2.0) + powf(orthoVP.v1.y / state->viewerState->voxelXYRatio, 2.0) + powf(orthoVP.v1.z / state->viewerState->voxelXYRatio / state->viewerState->voxelXYtoZRatio , 2.0));
            float voxelV2X = sqrtf((powf(orthoVP.v2.x, 2.0) + powf(orthoVP.v2.y / state->viewerState->voxelXYRatio, 2.0) + powf(orthoVP.v2.z / state->viewerState->voxelXYRatio / state->viewerState->voxelXYtoZRatio , 2.0)));

            orthoVP.texture.displayedEdgeLengthX /= voxelV1X;
            orthoVP.texture.displayedEdgeLengthY /= voxelV2X;

            //Display only entire pixels (only truncation possible!) WHY??
            orthoVP.texture.displayedEdgeLengthX = (std::ceil(orthoVP.texture.displayedEdgeLengthX / 2. / orthoVP.texture.texUnitsPerDataPx) * orthoVP.texture.texUnitsPerDataPx) * 2.;
            orthoVP.texture.displayedEdgeLengthY = (std::ceil(orthoVP.texture.displayedEdgeLengthY / 2. / orthoVP.texture.texUnitsPerDataPx) * orthoVP.texture.texUnitsPerDataPx) * 2.;

            // Update screen pixel to data pixel mapping values
            orthoVP.screenPxXPerDataPx = (float)orthoVP.edgeLength / (orthoVP.texture.displayedEdgeLengthX / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.screenPxYPerDataPx = (float)orthoVP.edgeLength / (orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx);
            orthoVP.displayedlengthInNmX =
                sqrtf(powf(state->viewerState->voxelDimX * orthoVP.v1.x,2.)
                      + powf(state->viewerState->voxelDimY * orthoVP.v1.y,2.)
                      + powf(state->viewerState->voxelDimZ * orthoVP.v1.z,2.)
                ) * (orthoVP.texture.displayedEdgeLengthX / orthoVP.texture.texUnitsPerDataPx);

            orthoVP.displayedlengthInNmY =
                sqrtf(powf(state->viewerState->voxelDimX * orthoVP.v2.x,2.)
                      + powf(state->viewerState->voxelDimY * orthoVP.v2.y,2.)
                      + powf(state->viewerState->voxelDimZ * orthoVP.v2.z,2.)
                ) * (orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx);

            //Update orthoVP.leftUpperDataPxOnScreen with this call
            calcLeftUpperTexAbsPx();

            midX = orthoVP.s_max / 2. * orthoVP.texture.texUnitsPerDataPx * (float)state->magnification;
            midY = orthoVP.t_max / 2. * orthoVP.texture.texUnitsPerDataPx * (float)state->magnification;

            //Offsets for crosshair
            orthoVP.texture.xOffset = midX / orthoVP.texture.texUnitsPerDataPx * orthoVP.screenPxXPerDataPx + 0.5 * orthoVP.screenPxXPerDataPx;
            orthoVP.texture.yOffset = midY / orthoVP.texture.texUnitsPerDataPx * orthoVP.screenPxYPerDataPx + 0.5 * orthoVP.screenPxYPerDataPx;
            break;
        }
        //Calculate the vertices in texture coordinates
        // mid really means current pos inside the texture, in texture coordinates, relative to the texture origin 0., 0.
        orthoVP.texture.texLUx = midX - (orthoVP.texture.displayedEdgeLengthX / 2.);
        orthoVP.texture.texLUy = midY - (orthoVP.texture.displayedEdgeLengthY / 2.);
        orthoVP.texture.texRUx = midX + (orthoVP.texture.displayedEdgeLengthX / 2.);
        orthoVP.texture.texRUy = orthoVP.texture.texLUy;
        orthoVP.texture.texRLx = orthoVP.texture.texRUx;
        orthoVP.texture.texRLy = midY + (orthoVP.texture.displayedEdgeLengthY / 2.);
        orthoVP.texture.texLLx = orthoVP.texture.texLUx;
        orthoVP.texture.texLLy = orthoVP.texture.texRLy;
    });
    return true;
}

void Viewer::loader_notify() {
    Loader::Controller::singleton().startLoading(state->viewerState->currentPosition);
}

void Viewer::defaultDatasetLUT() {
    state->viewerState->datasetColortableOn = false;
    datasetColorAdjustmentsChanged();
}

void Viewer::loadDatasetLUT(const QString & path) {
    state->viewerState->datasetColortable = loadLookupTable(path);
    state->viewerState->datasetColortableOn = true;
    datasetColorAdjustmentsChanged();
}

void Viewer::datasetColorAdjustmentsChanged() {
    if (state->viewerState->datasetColortableOn) {
        state->viewerState->datasetAdjustmentTable = state->viewerState->datasetColortable;
    } else {
        state->viewerState->datasetAdjustmentTable.resize(256);
        for (std::size_t i = 0; i < state->viewerState->datasetAdjustmentTable.size(); ++i) {//identity adjustment
            state->viewerState->datasetAdjustmentTable[i] = std::make_tuple(i, i, i);
        }
    }
    //Apply the dynamic range settings to the adjustment table
    if (state->viewerState->luminanceBias != 0 || state->viewerState->luminanceRangeDelta != MAX_COLORVAL) {
        const auto originalAdjustment = state->viewerState->datasetAdjustmentTable;
        for (int i = 0; i < 256; ++i) {
            int dynIndex = ((i - state->viewerState->luminanceBias) / (state->viewerState->luminanceRangeDelta / MAX_COLORVAL));
            dynIndex = std::min(static_cast<int>(MAX_COLORVAL), std::max(0, dynIndex));
            state->viewerState->datasetAdjustmentTable[i] = originalAdjustment[dynIndex];
        }
    }
    state->viewerState->datasetAdjustmentOn = state->viewerState->datasetColortableOn || state->viewerState->luminanceBias != 0 || state->viewerState->luminanceRangeDelta != MAX_COLORVAL;

    dc_reslice_notify_visible();
}

/** Global interfaces  */
void Viewer::rewire() {
    // viewer signals
    QObject::connect(this, &Viewer::zoomChanged, &window->widgetContainer.datasetOptionsWidget, &DatasetOptionsWidget::update);
    QObject::connect(this, &Viewer::coordinateChangedSignal, [this](const Coordinate & pos) { window->updateCoordinateBar(pos.x, pos.y, pos.z); });
    QObject::connect(this, &Viewer::coordinateChangedSignal, [this](const Coordinate &) {
        if (window->viewport3D->hasCursor) {
            window->updateCursorLabel(Coordinate(), VIEWPORT_SKELETON);
        } else {
            window->forEachOrthoVPDo([](ViewportOrtho & orthoVP) {
                if (orthoVP.hasCursor) {
                    orthoVP.sendCursorPosition();
                }
            });
        }
    });
    // end viewer signals
    //viewport signals
    window->forEachVPDo([this](ViewportBase & vp) {
        QObject::connect(&vp, &ViewportBase::pasteCoordinateSignal, window, &MainWindow::pasteClipboardCoordinates);
        QObject::connect(&vp, &ViewportBase::compressionRatioToggled, &window->widgetContainer.datasetOptionsWidget, &DatasetOptionsWidget::updateCompressionRatioDisplay);
        QObject::connect(&vp, &ViewportBase::rotationSignal, this, &Viewer::setRotation);
        QObject::connect(&vp, &ViewportBase::updateDatasetOptionsWidget, &window->widgetContainer.datasetOptionsWidget, &DatasetOptionsWidget::update);
    });

    window->forEachOrthoVPDo([this](ViewportOrtho & orthoVP) {
        QObject::connect(&orthoVP, &ViewportOrtho::changeDatasetMagSignal, this, &Viewer::changeDatasetMag);
        QObject::connect(&orthoVP, &ViewportOrtho::recalcTextureOffsetsSignal, this, &Viewer::recalcTextureOffsets);
    });

    // end viewport signals

    // --- widget signals ---
    //  appearance widget signals --
    QObject::connect(&window->widgetContainer.appearanceWidget.viewportTab, &ViewportTab::setVPOrientationSignal, this, &Viewer::setVPOrientation);
    //  -- end appearance widget signals
    // dataset load signals --
    QObject::connect(&window->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::clearSkeletonSignalNoGUI, window, &MainWindow::clearSkeletonSlotNoGUI);
    QObject::connect(&window->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::clearSkeletonSignalGUI, window, &MainWindow::clearSkeletonSlotGUI);
    QObject::connect(&window->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::updateDatasetCompression, &window->widgetContainer.datasetOptionsWidget, &DatasetOptionsWidget::updateCompressionRatioDisplay);
     // -- end dataset load signals
    // --- end widget signals
}

void Viewer::setRotation(const floatCoordinate & axis, const float angle) {
    alphaCache += angle; // angles are added up here until they are processed in the thread loop
    rotation = Rotation(axis, alphaCache);
    dc_xy_changed = oc_xy_changed = dc_zy_changed = oc_zy_changed = dc_xz_changed = oc_xz_changed = true;
}

void Viewer::setVPOrientation(const bool arbitrary) {
    if (arbitrary) {
         window->forEachOrthoVPDo([this](ViewportOrtho & orthoVP) { orthoVP.setOrientation(VIEWPORT_ARBITRARY); });
    } else {
        window->viewportXY->setOrientation(VIEWPORT_XY);
        window->viewportXZ->setOrientation(VIEWPORT_XZ);
        window->viewportYZ->setOrientation(VIEWPORT_YZ);
        resetRotation();
    }
    dc_xy_changed = oc_xy_changed = dc_zy_changed = oc_zy_changed = dc_xz_changed = oc_xz_changed = true;
}

void Viewer::resetRotation() {
    alphaCache = 0;
    rotation = Rotation();
    v1 = {1, 0, 0};
    v2 = {0, 1, 0};
    v3 = {0, 0, 1};
    vpUpperLeft->v1 = v1;
    vpUpperLeft->v2 = v2;
    vpUpperLeft->n = v3;
    vpLowerLeft->v1 = v1;
    vpLowerLeft->v2 = v3;
    vpLowerLeft->n = v2;
    vpUpperRight->v1 = v3;
    vpUpperRight->v2 = v2;
    vpUpperRight->n = v1;
}

void Viewer::loadTreeLUT(const QString & path) {
    state->viewerState->treeColors = loadLookupTable(path);
    skeletonizer->updateTreeColors();
}

void Viewer::loadNodeLUT(const QString & path) {
    state->viewerState->nodeColors = loadLookupTable(path);
}

color4F Viewer::getNodeColor(const nodeListElement & node) const {
    const auto property = state->viewerState->highlightedNodePropertyByColor;
    const auto range = state->viewerState->nodePropertyColorMapMax - state->viewerState->nodePropertyColorMapMin;
    const auto & nodeColors = state->viewerState->nodeColors;
    if (!property.isEmpty() && node.properties.contains(property) && range > 0) {
        const int index = node.properties[property].toDouble() / range * MAX_COLORVAL;
        return {std::get<0>(nodeColors[index])/255.f, std::get<1>(nodeColors[index])/255.f, std::get<2>(nodeColors[index])/255.f, 1.f};
    }
    if (node.isBranchNode) { //branch nodes are always blue
        return {0.f, 0.f, 1.f, 1.f};
    }
    if (CommentSetting::useCommentNodeColor && node.comment && strlen(node.comment->content) != 0) {
        // default color for comment nodes
        auto newColor = CommentSetting::getColor(QString(node.comment->content));
        return color4F(newColor.red()/255., newColor.green()/255., newColor.blue()/255., newColor.alpha()/255.);
    }
    if (node.correspondingTree == state->skeletonState->activeTree && state->viewerState->highlightActiveTree) {
        return {1.f, 0., 0., 1.f};
    }
    return node.correspondingTree->color;
}
