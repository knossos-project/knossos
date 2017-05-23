/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "viewer.h"

#include "file_io.h"
#include "functions.h"
#include "loader.h"
#include "segmentation/segmentation.h"
#include "session.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "widgets/mainwindow.h"
#include "widgets/viewports/viewportbase.h"
#include "widgets/widgetcontainer.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QVector3D>

#include <fstream>
#include <cmath>

ViewerState::ViewerState() {
    state->viewerState = this;
}

Viewer::Viewer() : evilHack{[this](){ state->viewer = this; return true; }()} {
    viewerState.layerVisibility = std::vector<bool>(2, true);
    skeletonizer = &Skeletonizer::singleton();
    loadTreeLUT();

    recalcTextureOffsets();

    rewire();

    QObject::connect(&timer, &QTimer::timeout, this, &Viewer::run);// timer is started in main

    QObject::connect(&Segmentation::singleton(), &Segmentation::appendedRow, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::changedRow, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::changedRowSelection, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::removedRow, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetData, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetSelection, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(&Segmentation::singleton(), &Segmentation::renderOnlySelectedObjsChanged, this, &Viewer::oc_reslice_notify_visible);

    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, this, &Viewer::updateCurrentPosition);
    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, this, &Viewer::dc_reslice_notify_visible);
    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, this, &Viewer::oc_reslice_notify_visible);
    QObject::connect(this, &Viewer::movementAreaFactorChangedSignal, this, &Viewer::dc_reslice_notify_visible);

    keyRepeatTimer.start();
}

void Viewer::saveSettings() {
    QSettings settings;
    settings.beginGroup(VIEWER);
    // viewport position and sizes
    settings.setValue(VP_DEFAULT_POS_SIZE, state->viewerState->defaultVPSizeAndPos);

    mainWindow.forEachVPDo([&settings, this] (ViewportBase & vp) {
        settings.setValue(VP_I_DOCKED.arg(vp.viewportType), vp.isDocked);
        settings.setValue(VP_I_DOCKED_POS.arg(vp.viewportType), vp.dockPos);
        settings.setValue(VP_I_DOCKED_SIZE.arg(vp.viewportType), vp.dockSize);
        settings.setValue(VP_I_FLOAT_POS.arg(vp.viewportType), vp.floatParent.nonMaximizedPos ? vp.floatParent.nonMaximizedPos.get() : QVariant());
        settings.setValue(VP_I_FLOAT_SIZE.arg(vp.viewportType), vp.floatParent.nonMaximizedSize ? vp.floatParent.nonMaximizedSize.get() : QVariant());
        settings.setValue(VP_I_FLOAT_FULLSCREEN.arg(vp.viewportType), vp.floatParent.fullscreen);
        settings.setValue(VP_I_FLOAT_FULL_ORIG_DOCKED.arg(vp.viewportType), vp.isFullOrigDocked);
        settings.setValue(VP_I_VISIBLE.arg(vp.viewportType), vp.isVisibleTo(mainWindow.centralWidget())); // vps are visible only after mainwindow shows, but can already be marked for becoming visible
    });
    QList<QVariant> order;
    for (const auto & w : mainWindow.centralWidget()->children()) {
        mainWindow.forEachVPDo([&w, &order](ViewportBase & vp) {
            if (w == &vp) {
                order.append(vp.viewportType);
            }
        });
    }
    settings.setValue(VP_ORDER, order);
    settings.endGroup();
}

void Viewer::loadSettings() {
    QSettings settings;
    settings.beginGroup(VIEWER);
    state->viewerState->defaultVPSizeAndPos = settings.value(VP_DEFAULT_POS_SIZE, true).toBool();
    if (state->viewerState->defaultVPSizeAndPos) {
        mainWindow.resetViewports();
    } else {
        mainWindow.forEachVPDo([&settings, this](ViewportBase & vp) {
            const auto docked = settings.value(VP_I_DOCKED.arg(vp.viewportType), true).toBool();
            vp.isDocked = docked;
            vp.dockPos = settings.value(VP_I_DOCKED_POS.arg(vp.viewportType)).toPoint();
            vp.dockSize = settings.value(VP_I_DOCKED_SIZE.arg(vp.viewportType)).toSize();
            const auto posVariant = settings.value(VP_I_FLOAT_POS.arg(vp.viewportType));
            vp.floatParent.nonMaximizedPos = posVariant.isValid() ? boost::optional<QPoint>{posVariant.toPoint()} : boost::none;
            const auto sizeVariant = settings.value(VP_I_FLOAT_SIZE.arg(vp.viewportType));
            vp.floatParent.nonMaximizedSize = sizeVariant.isValid() ? boost::optional<QSize>{sizeVariant.toSize()} : boost::none;
            vp.floatParent.fullscreen = settings.value(VP_I_FLOAT_FULLSCREEN.arg(vp.viewportType), false).toBool();
            vp.isFullOrigDocked = settings.value(VP_I_FLOAT_FULL_ORIG_DOCKED.arg(vp.viewportType), false).toBool();
            if (docked) {
                vp.resize(vp.dockSize);
                vp.move(vp.dockPos);
                vp.setVisible(settings.value(VP_I_VISIBLE.arg(vp.viewportType), true).toBool());
            }
        });
    }
    emit changedDefaultVPSizeAndPos();
    for (const auto & i : settings.value(VP_ORDER).toList()) {
        mainWindow.viewport(static_cast<ViewportType>(i.toInt()))->raise();
    }
    settings.endGroup();
}

void Viewer::setMovementAreaFactor(float alpha) {
    state->viewerState->outsideMovementAreaFactor = alpha;
    emit movementAreaFactorChangedSignal();
}

uint Viewer::highestMag() {
    return viewerState.datasetMagLock ? state->magnification : state->highestAvailableMag;
}

uint Viewer::lowestMag() {
    return viewerState.datasetMagLock ? state->magnification : state->lowestAvailableMag;
}

float Viewer::highestScreenPxXPerDataPx(const bool ofCurrentMag) {
    const float texUnitsPerDataPx = 1. / viewerState.texEdgeLength / (ofCurrentMag ? lowestMag() : state->lowestAvailableMag);
    auto * vp = viewportXY;
    float FOVinDCs = static_cast<float>(state->M) - 1.f;
    float displayedEdgeLen = (FOVinDCs * VPZOOMMAX * state->cubeEdgeLength) / vp->texture.size;
    displayedEdgeLen = (std::ceil(displayedEdgeLen / 2. / texUnitsPerDataPx) * texUnitsPerDataPx) * 2.;
    return vp->edgeLength / (displayedEdgeLen / texUnitsPerDataPx);
}

float Viewer::lowestScreenPxXPerDataPx(const bool ofCurrentMag) {
    const float texUnitsPerDataPx = 1. / viewerState.texEdgeLength / (ofCurrentMag ? highestMag() : state->highestAvailableMag);
    auto * vp = viewportXY;
    float FOVinDCs = static_cast<float>(state->M) - 1.f;
    float displayedEdgeLen = (FOVinDCs * state->cubeEdgeLength) / vp->texture.size;
    displayedEdgeLen = (std::ceil(displayedEdgeLen / 2. / texUnitsPerDataPx) * texUnitsPerDataPx) * 2.;
    return vp->edgeLength / (displayedEdgeLen / texUnitsPerDataPx);
}

uint Viewer::calcMag(const float screenPxXPerDataPx) {
    const float exactMag = state->highestAvailableMag * lowestScreenPxXPerDataPx() / screenPxXPerDataPx;
    const uint roundedPower = std::ceil(std::log(exactMag) / std::log(2));
    return std::min(state->highestAvailableMag, std::max(static_cast<uint>(std::pow(2, roundedPower)), state->lowestAvailableMag));
}

void Viewer::setMagnificationLock(const bool locked) {
    viewerState.datasetMagLock = locked;
    if (!locked) {
        const uint newMag = calcMag(viewportXY->screenPxXPerDataPx);
        if (newMag != static_cast<uint>(state->magnification)) {
            updateDatasetMag(newMag);
            float newFOV = viewportXY->screenPxXPerDataPxForZoomFactor(1.f) / viewportXY->screenPxXPerDataPx;
            window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) {
                orthoVP.texture.FOV = newFOV;
            });
        }
    }
    recalcTextureOffsets();
    emit magnificationLockChanged(locked);
}

void Viewer::dcSliceExtract(char *datacube, Coordinate cubePosInAbsPx, char *slice, ViewportOrtho & vp, bool useCustomLUT) {
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

    const std::size_t voxelIncrement = vp.viewportType == VIEWPORT_ZY ? state->cubeEdgeLength : 1;
    const std::size_t sliceIncrement = vp.viewportType == VIEWPORT_XY ? state->cubeEdgeLength : state->cubeSliceArea;
    const std::size_t sliceSubLineIncrement = vp.viewportType == VIEWPORT_ZY ? 0 : sliceIncrement - state->cubeEdgeLength;
    const std::size_t texNextLine = vp.viewportType == VIEWPORT_ZY ? state->cubeEdgeLength * 3 : 3;// RGB per pixel
    const std::size_t texRevertToFirstLine = vp.viewportType == VIEWPORT_ZY ? (state->cubeSliceArea - 1) * 3 : 0;

    int offsetX = 0, offsetY = 0;
    const int coordsPerLine = state->cubeEdgeLength * state->magnification;
    for (auto y = state->cubeEdgeLength; y != 0; --y) {
        for (auto x = state->cubeEdgeLength; x != 0; --x) {// x and y’s values are not used
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
                    ((vp.viewportType == VIEWPORT_XZ || vp.viewportType == VIEWPORT_ZY) && (cubePosInAbsPx.z + offsetY < areaMinCoord.z || cubePosInAbsPx.z + offsetY > areaMaxCoord.z))) {
                    // vertically out of movement area
                    factor = true;
                }
                else if(((vp.viewportType == VIEWPORT_XY || vp.viewportType == VIEWPORT_XZ) && (cubePosInAbsPx.x + offsetX < areaMinCoord.x || cubePosInAbsPx.x + offsetX > areaMaxCoord.x)) ||
                        (vp.viewportType == VIEWPORT_ZY && (cubePosInAbsPx.y + offsetX < areaMinCoord.y || cubePosInAbsPx.y + offsetX > areaMaxCoord.y))) {
                    // horizontally out of movement area
                    factor = true;
                }
                if (factor) {
                    float d = state->viewerState->outsideMovementAreaFactor * 1.0 / 100;
                    r *= d; g *= d; b *= d;
                }
            }
            reinterpret_cast<uint8_t*>(slice)[0] = r;
            reinterpret_cast<uint8_t*>(slice)[1] = g;
            reinterpret_cast<uint8_t*>(slice)[2] = b;

            datacube += voxelIncrement;
            slice += texNextLine;
            offsetX = (offsetX + state->magnification) % coordsPerLine;
            offsetY += (offsetX == 0)? state->magnification : 0; // at end of line increment to next line
        }
        datacube += sliceSubLineIncrement;
        slice -= texRevertToFirstLine;
    }
}

void Viewer::dcSliceExtract(char *datacube, floatCoordinate *currentPxInDc_float, char *slice, int s, int *t, ViewportArb &vp, bool useCustomLUT) {
    Coordinate currentPxInDc = {roundFloat(currentPxInDc_float->x), roundFloat(currentPxInDc_float->y), roundFloat(currentPxInDc_float->z)};

    if((currentPxInDc.x < 0) || (currentPxInDc.y < 0) || (currentPxInDc.z < 0) ||
       (currentPxInDc.x >= state->cubeEdgeLength) || (currentPxInDc.y >= state->cubeEdgeLength) || (currentPxInDc.z >= state->cubeEdgeLength)) {
        const int sliceIndex = 3 * ( s + *t  *  state->cubeEdgeLength * state->M);
        slice[sliceIndex] = slice[sliceIndex + 1] = slice[sliceIndex + 2] = 0;
        (*t)++;
        if(*t < vp.texture.usedSizeInCubePixels) {
            // Actually, although currentPxInDc_float is passed by reference and updated here,
            // it is totally ignored (i.e. not read, then overwritten) by the calling function.
            // But to keep the functionality here compatible after this bugfix, we also replicate
            // this update here - from the originial below
            *currentPxInDc_float -= vp.v2;
        }
        return;
    }

    while((0 <= currentPxInDc.x && currentPxInDc.x < state->cubeEdgeLength)
          && (0 <= currentPxInDc.y && currentPxInDc.y < state->cubeEdgeLength)
          && (0 <= currentPxInDc.z && currentPxInDc.z < state->cubeEdgeLength)) {

        const int sliceIndex = 3 * ( s + *t  *  state->cubeEdgeLength * state->M);
        const int dcIndex = currentPxInDc.x + currentPxInDc.y * state->cubeEdgeLength + currentPxInDc.z * state->cubeSliceArea;
        if(datacube == NULL) {
            slice[sliceIndex] = slice[sliceIndex + 1] = slice[sliceIndex + 2] = 0;
        } else {
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
        if(*t >= vp.texture.usedSizeInCubePixels) {
            break;
        }
        *currentPxInDc_float -= vp.v2;
        currentPxInDc = {roundFloat(currentPxInDc_float->x), roundFloat(currentPxInDc_float->y), roundFloat(currentPxInDc_float->z)};
    }
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
void Viewer::ocSliceExtract(char *datacube, Coordinate cubePosInAbsPx, char *slice, ViewportOrtho & vp) {
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

    const std::size_t voxelIncrement = vp.viewportType == VIEWPORT_ZY ? state->cubeEdgeLength * OBJID_BYTES : OBJID_BYTES;
    const std::size_t sliceIncrement = vp.viewportType == VIEWPORT_XY ? state->cubeEdgeLength * OBJID_BYTES : state->cubeSliceArea * OBJID_BYTES;
    const std::size_t sliceSubLineIncrement = vp.viewportType == VIEWPORT_ZY ? 0 : sliceIncrement - state->cubeEdgeLength * OBJID_BYTES;
    const std::size_t texNextLine = vp.viewportType == VIEWPORT_ZY ? state->cubeEdgeLength * 4 : 4;// RGBA per pixel
    const std::size_t textRevertToFirstLine = vp.viewportType == VIEWPORT_ZY ? (state->cubeSliceArea - 1) * 4 : 0;

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
    int coordsPerLine = state->cubeEdgeLength*state->magnification;
    for (auto y = state->cubeEdgeLength; y != 0; --y) {
        for (auto x = state->cubeEdgeLength; x != 0; --x) {// x and y’s values are not used
            bool hide = false;
            if(partlyInMovementArea) {
                if((vp.viewportType == VIEWPORT_XY && (cubePosInAbsPx.y + offsetY < areaMinCoord.y || cubePosInAbsPx.y + offsetY > areaMaxCoord.y)) ||
                    ((vp.viewportType == VIEWPORT_XZ || vp.viewportType == VIEWPORT_ZY) && (cubePosInAbsPx.z + offsetY < areaMinCoord.z || cubePosInAbsPx.z + offsetY > areaMaxCoord.z))) {
                    // vertically out of movement area
                    reinterpret_cast<uint8_t*>(slice)[3] = 0;
                    hide = true;
                }
                else if(((vp.viewportType == VIEWPORT_XY || vp.viewportType == VIEWPORT_XZ) && (cubePosInAbsPx.x + offsetX < areaMinCoord.x || cubePosInAbsPx.x + offsetX > areaMaxCoord.x)) ||
                        (vp.viewportType == VIEWPORT_ZY && (cubePosInAbsPx.y + offsetX < areaMinCoord.y || cubePosInAbsPx.y + offsetX > areaMaxCoord.y))) {
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
                    if (selected && seg.mouseFocusedObjectId == objectId) {
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
            datacube += voxelIncrement;
            slice += texNextLine;
            offsetX = (offsetX + state->magnification) % coordsPerLine;
            offsetY += (offsetX == 0)? state->magnification : 0; // at end of line increment to next line
        }
        datacube += sliceSubLineIncrement;
        slice -= textRevertToFirstLine;
    }
}

static int texIndex(uint x, uint y, uint colorMultiplicationFactor, viewportTexture *texture) {
    uint index = 0;

    index = x * state->cubeSliceArea + y
            * (texture->size * state->cubeEdgeLength)
            * colorMultiplicationFactor;

    return index;
}

bool Viewer::vpGenerateTexture(ViewportOrtho & vp) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.
    if (vp.viewportType == VIEWPORT_ARBITRARY) {
        vpGenerateTexture(static_cast<ViewportArb&>(vp));
        return true;
    }
    const bool dc_reslice = vp.dcResliceNecessary;
    const bool oc_reslice = vp.ocResliceNecessary;
    vp.dcResliceNecessary = vp.ocResliceNecessary = false;
    const CoordInCube currentPosition_dc = state->viewerState->currentPosition.insideCube(state->cubeEdgeLength, state->magnification);

    int slicePositionWithinCube;
    switch(vp.viewportType) {
    case VIEWPORT_XY:
        slicePositionWithinCube = state->cubeSliceArea * currentPosition_dc.z;
        break;
    case VIEWPORT_XZ:
        slicePositionWithinCube = state->cubeEdgeLength * currentPosition_dc.y;
        break;
    case VIEWPORT_ZY:
        slicePositionWithinCube = currentPosition_dc.x;
        break;
    default:
        qDebug("No such slice view: %d.", vp.viewportType);
        return false;
    }

    const CoordOfCube upperLeftDc = Coordinate(vp.texture.leftUpperPxInAbsPx).cube(state->cubeEdgeLength, state->magnification);

    std::vector<char> texData(4 * std::pow(state->viewerState->texEdgeLength, 2));
    // We iterate over the texture with x and y being in a temporary coordinate
    // system local to this texture.
    for(int x_dc = 0; x_dc < state->M; x_dc++) {
        for(int y_dc = 0; y_dc < state->M; y_dc++) {
            const int x_px = x_dc * state->cubeEdgeLength;
            const int y_px = y_dc * state->cubeEdgeLength;

            CoordOfCube currentDc;

            switch(vp.viewportType) {
            // With an x/y-coordinate system in a viewport, we get the following
            // mapping from viewport (slice) coordinates to global (dc)
            // coordinates:
            // XY-slice: x local is x global, y local is y global
            // XZ-slice: x local is x global, y local is z global
            // ZY-slice: x local is z global, y local is y global.
            case VIEWPORT_XY:
                currentDc = {upperLeftDc.x + x_dc, upperLeftDc.y + y_dc, upperLeftDc.z};
                break;
            case VIEWPORT_XZ:
                currentDc = {upperLeftDc.x + x_dc, upperLeftDc.y, upperLeftDc.z + y_dc};
                break;
            case VIEWPORT_ZY:
                currentDc = {upperLeftDc.x, upperLeftDc.y + y_dc, upperLeftDc.z + x_dc};
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

                if (datacube != nullptr) {
                    dcSliceExtract(datacube + slicePositionWithinCube,
                                   cubePosInAbsPx,
                                   texData.data() + index,
                                   vp,
                                   state->viewerState->datasetAdjustmentOn);
                } else {
                    std::fill(std::begin(texData), std::end(texData), 0);
                }
                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                x_px,
                                y_px,
                                state->cubeEdgeLength,
                                state->cubeEdgeLength,
                                GL_RGB,
                                GL_UNSIGNED_BYTE,
                                texData.data() + index);
            }
            //Take care of the overlay textures.
            if (Segmentation::enabled && oc_reslice && state->viewerState->showOnlyRawData == false) {
                glBindTexture(GL_TEXTURE_2D, vp.texture.overlayHandle);
                // This is used to index into the texture. texData[index] is the first
                // byte of the datacube slice at position (x_dc, y_dc) in the texture.
                const int index = texIndex(x_dc, y_dc, 4, &(vp.texture));

                if (overlayCube != nullptr) {
                    ocSliceExtract(overlayCube + slicePositionWithinCube * OBJID_BYTES,
                                   cubePosInAbsPx,
                                   texData.data() + index,
                                   vp);
                } else {
                    std::fill(std::begin(texData), std::end(texData), 0);
                }
                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                x_px,
                                y_px,
                                state->cubeEdgeLength,
                                state->cubeEdgeLength,
                                GL_RGBA,
                                GL_UNSIGNED_BYTE,
                                texData.data() + index);
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Viewer::arbCubes(ViewportArb & vp) {
    const auto pointInCube = [this](const Coordinate currentDC, const floatCoordinate point) {
       return currentDC.x * gpucubeedge <= point.x && point.x <= currentDC.x * gpucubeedge + gpucubeedge &&
              currentDC.y * gpucubeedge <= point.y && point.y <= currentDC.y * gpucubeedge + gpucubeedge &&
              currentDC.z * gpucubeedge <= point.z && point.z <= currentDC.z * gpucubeedge + gpucubeedge;
    };

    const floatCoordinate xAxis = {1, 0, 0}; const floatCoordinate yAxis = {0, 1, 0}; const floatCoordinate zAxis = {0, 0, 1};
    const auto normal = vp.n;// the normal vector direction is not important here because it doesn’t change the plane

    for (auto & layer : layers) {
        layer.pendingArbCubes.clear();
        for (auto & pair : layer.textures) {
            pair.second->vertices.clear();
        }
    }

    const auto gpusupercube = (state->M - 1) * state->cubeEdgeLength / gpucubeedge + 1;//remove cpu overlap and add gpu overlap
    const auto scroot = (state->viewerState->currentPosition / gpucubeedge) - gpusupercube / 2;
    floatCoordinate root = vp.texture.leftUpperPxInAbsPx / state->magnification;
    for (int z = 0; z < gpusupercube; ++z)
    for (int y = 0; y < gpusupercube; ++y)
    for (int x = 0; x < gpusupercube; ++x) {
        Coordinate currentGPUDc = scroot + Coordinate{x, y, z};

        if (currentGPUDc.x < 0 || currentGPUDc.y < 0 || currentGPUDc.z < 0) {
            continue;
        }

        floatCoordinate topPlaneUpVec = currentGPUDc * gpucubeedge;
        floatCoordinate bottomPlaneUpVec = currentGPUDc * gpucubeedge + floatCoordinate(gpucubeedge, gpucubeedge, 0);
        floatCoordinate leftPlaneUpVec = currentGPUDc * gpucubeedge + floatCoordinate(0, gpucubeedge, gpucubeedge);
        floatCoordinate rightPlaneUpVec = currentGPUDc * gpucubeedge + floatCoordinate(gpucubeedge, 0, gpucubeedge);

        std::vector<floatCoordinate> points;
        auto addPoints = [&](const floatCoordinate & plane, const floatCoordinate & axis){
            floatCoordinate point;
            if (intersectLineAndPlane(normal, root, plane, axis, point)) {
                if (pointInCube(currentGPUDc, point)) {
                    points.emplace_back(point);
                }
            }
        };
        addPoints(topPlaneUpVec, xAxis);
        addPoints(topPlaneUpVec, yAxis);
        addPoints(topPlaneUpVec, zAxis);
        addPoints(bottomPlaneUpVec, xAxis * -1);
        addPoints(bottomPlaneUpVec, yAxis * -1);
        addPoints(bottomPlaneUpVec, zAxis);
        addPoints(leftPlaneUpVec, xAxis);
        addPoints(leftPlaneUpVec, yAxis * -1);
        addPoints(leftPlaneUpVec, zAxis * -1);
        addPoints(rightPlaneUpVec, xAxis * -1);
        addPoints(rightPlaneUpVec, yAxis);
        addPoints(rightPlaneUpVec, zAxis * -1);

        if (!points.empty()) {
            const auto center = std::accumulate(std::begin(points), std::end(points), floatCoordinate{}) / points.size();
            const auto first = points.front() - center;
            std::sort(std::begin(points), std::end(points), [=](const floatCoordinate & lhs, const floatCoordinate & rhs){
                // http://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors#16544330
                const auto line0 = lhs - center;
                const auto line1 = rhs - center;
                // move radiant angle range from [-ⲡ, ⲡ] into positive (4 > ⲡ) for < to work correctly
                const auto radiant0 = std::atan2(normal.dot(first.cross(line0)), first.dot(line0)) + 4;
                const auto radiant1 = std::atan2(normal.dot(first.cross(line1)), first.dot(line1)) + 4;
                return radiant0 < radiant1;
            });
            const auto prevSize = points.size();
            points.erase(std::unique(std::begin(points), std::end(points)), std::end(points));
            if (prevSize != points.size()) {
                qDebug() << prevSize << points.size();
            }

            // if the arb plane is parallel to one of the coordinate axis
            // the intersection test will be true for adjacent cubes on cube boundaries
            // because the edge coordinates at the end of a cube are inclusive to render across the last pixel
            bool moreThanXEndBorder{false}, moreThanYEndBorder{false}, moreThanZEndBorder{false};
            for (auto & point : points) {
                if (point.x != rightPlaneUpVec.x) {
                    moreThanXEndBorder = true;
                }
                if (point.y != bottomPlaneUpVec.y) {
                    moreThanYEndBorder = true;
                }
                if (point.z != leftPlaneUpVec.z) {
                    moreThanZEndBorder = true;
                }
            }
            if (moreThanXEndBorder && moreThanYEndBorder && moreThanZEndBorder) {
                const CoordOfGPUCube gpuCoord{currentGPUDc.x, currentGPUDc.y, currentGPUDc.z};
                const auto globalCoord = gpuCoord.cube2Global(gpucubeedge, state->magnification);
                for (auto & layer : layers) {
                    auto cubeIt = layer.textures.find(gpuCoord);
                    if (cubeIt != std::end(layer.textures)) {
                        cubeIt->second->vertices = /*std::move*/(points);
                    } else {
                        const auto cubeCoord = globalCoord.cube(state->cubeEdgeLength, state->magnification);
                        const auto offset = globalCoord - cubeCoord.cube2Global(state->cubeEdgeLength, state->magnification);
                        layer.pendingArbCubes.emplace_back(gpuCoord, offset);
                    }
                }
            }
        }
    }
}

void Viewer::setEnableArbVP(const bool on) {
    viewerState.enableArbVP = on;
    emit enabledArbVP(on);
}

void Viewer::setDefaultVPSizeAndPos(const bool on) {
    state->viewerState->defaultVPSizeAndPos = on;
    emit changedDefaultVPSizeAndPos();
}

void Viewer::vpGenerateTexture(ViewportArb &vp) {
    if (!vp.dcResliceNecessary) {
        return;
    }
    vp.dcResliceNecessary = false;

    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.
    floatCoordinate currentPxInDc_float, rowPx_float, currentPx_float;

    char *datacube = NULL;

    rowPx_float = vp.texture.leftUpperPxInAbsPx / state->magnification;
    currentPx_float = rowPx_float;

    std::vector<char> texData(3 * std::pow(state->viewerState->texEdgeLength, 2));

    glBindTexture(GL_TEXTURE_2D, vp.texture.texHandle);

    int s = 0, t = 0, t_old = 0;
    while(s < vp.texture.usedSizeInCubePixels) {
        t = 0;
        while(t < vp.texture.usedSizeInCubePixels) {
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

            dcSliceExtract(datacube,
                           &currentPxInDc_float,
                           texData.data(),
                           s, &t,
                           vp,
                           state->viewerState->datasetAdjustmentOn);
            currentPx_float = currentPx_float - vp.v2 * (t - t_old);
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
                    texData.data());

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Viewer::calcLeftUpperTexAbsPx() {
    window->forEachOrthoVPDo([this](ViewportOrtho & orthoVP) {
        const auto fov = orthoVP.texture.usedSizeInCubePixels;
        const CoordOfCube currentPosition_dc = viewerState.currentPosition.cube(state->cubeEdgeLength, state->magnification);
        const auto xy = orthoVP.viewportType == VIEWPORT_XY;
        const auto xz = orthoVP.viewportType == VIEWPORT_XZ;
        const auto zy = orthoVP.viewportType == VIEWPORT_ZY;
        orthoVP.texture.leftUpperPxInAbsPx = (currentPosition_dc - CoordOfCube{(xy || xz) * state->M / 2, (xy || zy) * state->M / 2, (xz || zy) * state->M / 2}).cube2Global(state->cubeEdgeLength, state->magnification);
        if (orthoVP.viewportType == VIEWPORT_ARBITRARY) {
            auto & arbVP = static_cast<ViewportArb&>(orthoVP);
            arbVP.leftUpperPxInAbsPx_float = floatCoordinate{viewerState.currentPosition} - (orthoVP.v1 - orthoVP.v2) * 0.5 * fov * state->magnification;// broken arb slicing depends on this
            arbVP.texture.leftUpperPxInAbsPx = arbVP.leftUpperPxInAbsPx_float;
        }
    });
}

void Viewer::calcDisplayedEdgeLength() {
    window->forEachOrthoVPDo([](ViewportOrtho & vpOrtho){
        float voxelV1X = state->scale.componentMul(vpOrtho.v1).length() / state->scale.x;
        float voxelV2X = std::abs(state->scale.componentMul(vpOrtho.v2).length()) / state->scale.x;

        vpOrtho.texture.texUsedX = vpOrtho.texture.usedSizeInCubePixels / vpOrtho.texture.size * vpOrtho.texture.FOV / voxelV1X;
        vpOrtho.texture.texUsedY = vpOrtho.texture.usedSizeInCubePixels / vpOrtho.texture.size * vpOrtho.texture.FOV / voxelV2X;

        vpOrtho.displayedIsoPx = state->scale.x * 0.5 * vpOrtho.texture.usedSizeInCubePixels * vpOrtho.texture.FOV * state->magnification;// FOV is within current mag
        const auto dataPx = vpOrtho.texture.usedSizeInCubePixels * vpOrtho.texture.FOV * state->magnification;
        vpOrtho.screenPxXPerDataPx = vpOrtho.edgeLength / dataPx * voxelV1X;
        vpOrtho.screenPxYPerDataPx = vpOrtho.edgeLength / dataPx * voxelV2X;

        vpOrtho.displayedlengthInNmX = state->scale.componentMul(vpOrtho.v1).length() * (vpOrtho.texture.texUsedX / vpOrtho.texture.texUnitsPerDataPx);
        vpOrtho.displayedlengthInNmY = state->scale.componentMul(vpOrtho.v2).length() * (vpOrtho.texture.texUsedY / vpOrtho.texture.texUnitsPerDataPx);
    });
}

void Viewer::zoom(const float factor) {
    const bool reachedHighestMag = static_cast<uint>(state->magnification) == state->highestAvailableMag;
    const bool reachedLowestMag = static_cast<uint>(state->magnification) == state->lowestAvailableMag;
    const bool reachedMinZoom = viewportXY->texture.FOV * factor > VPZOOMMIN && reachedHighestMag;
    const bool reachedMaxZoom = viewportXY->texture.FOV * factor  < VPZOOMMAX && reachedLowestMag;
    const bool magUp = viewportXY->texture.FOV == VPZOOMMIN && factor > 1 && !reachedHighestMag;
    const bool magDown = viewportXY->texture.FOV == 0.5 && factor < 1 && !reachedLowestMag;

    const auto updateFOV = [this](const float newFOV) {
        window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) { orthoVP.texture.FOV = newFOV; });
    };
    uint newMag = state->magnification;
    if (reachedMinZoom) {
        updateFOV(VPZOOMMIN);
    } else if (reachedMaxZoom) {
        updateFOV(VPZOOMMAX);
    } else if (state->viewerState->datasetMagLock) {
        updateFOV(viewportXY->texture.FOV * factor > VPZOOMMIN ? VPZOOMMIN :
                  viewportXY->texture.FOV * factor < VPZOOMMAX ? VPZOOMMAX :
                  viewportXY->texture.FOV * factor);
    } else if (magUp) {
        newMag *= 2;
        updateFOV(0.5);
    } else if (magDown) {
        newMag /= 2;
        updateFOV(VPZOOMMIN);
    } else {
        const float zoomMax = static_cast<float>(state->magnification) == state->lowestAvailableMag ? VPZOOMMAX : 0.5;
        updateFOV(std::max(std::min(viewportXY->texture.FOV * factor, static_cast<float>(VPZOOMMIN)), zoomMax));
    }

    if (newMag != static_cast<uint>(state->magnification)) {
        updateDatasetMag(newMag);
    }
    recalcTextureOffsets();
    emit zoomChanged();
}

void Viewer::zoomReset() {
    state->viewer->window->forEachOrthoVPDo([](ViewportOrtho & orthoVP){
        orthoVP.texture.FOV = 1;
    });
    recalcTextureOffsets();
    emit zoomChanged();
}

bool Viewer::updateDatasetMag(const uint mag) {
    Loader::Controller::singleton().unloadCurrentMagnification(); //unload all the cubes
    if (mag != 0) {// change global mag after unloading
        const bool powerOf2 = mag > 0 && (mag & (mag - 1)) == 0;
        if (!powerOf2 || mag < state->lowestAvailableMag || mag > state->highestAvailableMag) {
            return false;
        }
        state->magnification = mag;
        window->forEachOrthoVPDo([mag](ViewportOrtho & orthoVP) {
            orthoVP.texture.texUnitsPerDataPx = 1.f / state->viewerState->texEdgeLength / mag;
        });
    }
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
  *   - userMove
  *
  */
//Entry point for viewer thread, general viewer coordination, "main loop"
void Viewer::run() {
    if (state->quitSignal) {//don’t do anything, when the rest is already going to sleep
        qDebug() << "viewer returned";
        return;
    }

    // start the timer before the rendering, else render interval and actual rendering time would accumulate
    if (timer.isActive()) {
        timer.stop();
        timer.start(QApplication::activeWindow() != nullptr ? 10 : 1000);// only update once per second if K isn’t the active application
    }

    if (viewerState.keyRepeat) {
        const double interval = 1000.0 / viewerState.movementSpeed;
        if (keyRepeatTimer.elapsed() >= interval) {
            const auto multiplier = viewerState.notFirstKeyRepeat * keyRepeatTimer.elapsed() / interval;
            viewerState.notFirstKeyRepeat = true;// first repeat should only move 1 slice
            keyRepeatTimer.restart();
            userMove(viewerState.repeatDirection * multiplier, USERMOVE_DRILL);
        }
    }
    // Event and rendering loop.
    // What happens is that we go through lists of pending texture parts and load
    // them if they are available.
    // While we are loading the textures, we check for events. Some events
    // might cancel the current loading process. When all textures
    // have been processed, we go into an idle state, in which we wait for events.
    if (state->gpuSlicer && gpuRendering) {
        const auto & loadPendingCubes = [&](TextureLayer & layer, std::vector<std::pair<CoordOfGPUCube, Coordinate>> & pendingCubes, QElapsedTimer & timer) {
            while (!pendingCubes.empty() && !timer.hasExpired(3)) {
                const auto pair = pendingCubes.back();
                pendingCubes.pop_back();
                if (layer.textures.find(pair.first) == std::end(layer.textures)) {
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
        };

        QElapsedTimer timer;
        timer.start();
        for (auto & layer : layers) {
            calculateMissingOrthoGPUCubes(layer);
            loadPendingCubes(layer, layer.pendingOrthoCubes, timer);
            loadPendingCubes(layer, layer.pendingArbCubes, timer);
        }
    }

    window->forEachOrthoVPDo([this](ViewportOrtho & vp) {
        vp.update();
    });
    window->viewport3D.get()->update();
    window->updateTitlebar(); //display changes after filename
}

void Viewer::applyTextureFilterSetting(const GLint texFiltering) {
    window->forEachOrthoVPDo([&texFiltering](ViewportOrtho & orthoVP) {
        orthoVP.texture.textureFilter = texFiltering;
        if (orthoVP.texture.texHandle != 0) {// 0 is an invalid tex id
            orthoVP.makeCurrent();
            glBindTexture(GL_TEXTURE_2D, orthoVP.texture.texHandle);
            // Set the parameters for the texture.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texFiltering);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFiltering);
        }
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

void Viewer::setPosition(const floatCoordinate & pos, UserMoveType userMoveType, const Coordinate & viewportNormal) {
    const auto deltaPos = pos - state->viewerState->currentPosition;
    userMove(deltaPos, userMoveType, viewportNormal);
}

void Viewer::setPositionWithRecentering(const Coordinate &pos) {
    remote.process(pos);
}

void Viewer::setPositionWithRecenteringAndRotation(const Coordinate &pos, const floatCoordinate normal) {
    remote.process(pos, normal);
}

void Viewer::userMoveVoxels(const Coordinate & step, UserMoveType userMoveType, const floatCoordinate & viewportNormal) {
    auto & viewerState = *state->viewerState;
    if (step.z != 0) {
        viewportXY->dcResliceNecessary = viewportXY->ocResliceNecessary = viewportArb->dcResliceNecessary = viewportArb->ocResliceNecessary = true;
    }
    if (step.x != 0) {
        viewportZY->dcResliceNecessary = viewportZY->ocResliceNecessary = viewportArb->dcResliceNecessary = viewportArb->ocResliceNecessary = true;
    }
    if (step.y != 0) {
        viewportXZ->dcResliceNecessary = viewportXZ->ocResliceNecessary = viewportArb->dcResliceNecessary = viewportArb->ocResliceNecessary = true;
    }

    // This determines whether the server will broadcast the coordinate change
    // to its client or not.

    const auto lastPosition_dc = viewerState.currentPosition.cube(state->cubeEdgeLength, state->magnification);
    const auto lastPosition_gpudc = viewerState.currentPosition.cube(gpucubeedge, state->magnification);

    const Coordinate movement = step;
    auto newPos = viewerState.currentPosition + movement;
    if (!Session::singleton().outsideMovementArea(newPos)) {
        viewerState.currentPosition = newPos;
        recalcTextureOffsets();
    } else {
        qDebug() << tr("Position (%1, %2, %3) out of bounds").arg(newPos.x + 1).arg(newPos.y + 1).arg(newPos.z + 1);
    }

    const auto newPosition_dc = viewerState.currentPosition.cube(state->cubeEdgeLength, state->magnification);
    const auto newPosition_gpudc = viewerState.currentPosition.cube(gpucubeedge, state->magnification);

    if (newPosition_dc != lastPosition_dc) {
        dc_reslice_notify_visible();
        oc_reslice_notify_visible();
        // userMoveType How user movement was generated
        // Direction of user movement in case of drilling,
        // or normal to viewport plane in case of horizontal movement.
        // Left unset in neutral movement.
        const auto direction = (userMoveType == USERMOVE_DRILL) ? floatCoordinate{step} : (userMoveType == USERMOVE_HORIZONTAL) ? viewportNormal : floatCoordinate{};
        loader_notify(userMoveType, direction);
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
            calculateMissingOrthoGPUCubes(layer);
        }
    }

    emit coordinateChangedSignal(viewerState.currentPosition);
}

void Viewer::userMove(const floatCoordinate & floatStep, UserMoveType userMoveType, const floatCoordinate & viewportNormal) {
    moveCache += floatStep;
    const Coordinate step(moveCache);
    moveCache -= step;
    userMoveVoxels(step, userMoveType, viewportNormal);
}

void Viewer::userMoveRound(UserMoveType userMoveType, const floatCoordinate & viewportNormal) {
    const Coordinate rounded(std::lround(moveCache.x), std::lround(moveCache.y), std::lround(moveCache.z));
    Viewer::userMoveClear();
    userMoveVoxels(rounded, userMoveType, viewportNormal);
}

void Viewer::userMoveClear() {
    moveCache = {};
}

void Viewer::calculateMissingOrthoGPUCubes(TextureLayer & layer) {
    layer.pendingOrthoCubes.clear();

    const auto gpusupercube = (state->M - 1) * state->cubeEdgeLength / gpucubeedge + 1;//remove cpu overlap and add gpu overlap
    const int halfSupercube = gpusupercube * 0.5;
    auto edge = state->viewerState->currentPosition.cube(gpucubeedge, state->magnification) - halfSupercube;
    const auto end = edge + gpusupercube;
    edge = {std::max(0, edge.x), std::max(0, edge.y), std::max(0, edge.z)};//negative coords are calculated incorrectly and there are no cubes anyway
    for (int x = edge.x; x < end.x; ++x)
    for (int y = edge.y; y < end.y; ++y)
    for (int z = edge.z; z < end.z; ++z) {
        const auto gpuCoord = CoordOfGPUCube{x, y, z};
        const auto globalCoord = gpuCoord.cube2Global(gpucubeedge, state->magnification);
        if (currentlyVisible(globalCoord, state->viewerState->currentPosition, gpusupercube, gpucubeedge) && layer.textures.count(gpuCoord) == 0) {
            const auto cubeCoord = globalCoord.cube(state->cubeEdgeLength, state->magnification);
            const auto offset = globalCoord - cubeCoord.cube2Global(state->cubeEdgeLength, state->magnification);
            layer.pendingOrthoCubes.emplace_back(gpuCoord, offset);
        }
    }
}

void Viewer::dc_reslice_notify_all(const Coordinate coord) {
    if (currentlyVisibleWrapWrap(state->viewerState->currentPosition, coord)) {
        dc_reslice_notify_visible();
    }
    window->viewportArb->dcResliceNecessary = true;//arb visibility is not tested
}

void Viewer::dc_reslice_notify_visible() {
    window->forEachOrthoVPDo([](ViewportOrtho & vpOrtho) {
        vpOrtho.dcResliceNecessary = true;
    });
}

void Viewer::oc_reslice_notify_all(const Coordinate coord) {
    if (currentlyVisibleWrapWrap(state->viewerState->currentPosition, coord)) {
        oc_reslice_notify_visible();
    }
    window->viewportArb->ocResliceNecessary = true;//arb visibility is not tested
    // if anything has changed, update the volume texture data
    Segmentation::singleton().volume_update_required = true;
}

void Viewer::oc_reslice_notify_visible() {
    window->forEachOrthoVPDo([](ViewportOrtho & vpOrtho) {
        vpOrtho.ocResliceNecessary = true;
    });
    // if anything has changed, update the volume texture data
    Segmentation::singleton().volume_update_required = true;
}

void Viewer::recalcTextureOffsets() {
    calcDisplayedEdgeLength();
    calcLeftUpperTexAbsPx();

    window->forEachOrthoVPDo([&](ViewportOrtho & orthoVP) {
        float midX = orthoVP.texture.texUnitsPerDataPx;
        float midY = orthoVP.texture.texUnitsPerDataPx;
        float xFactor = 0.5 * orthoVP.texture.texUsedX;
        float yFactor = 0.5 * orthoVP.texture.texUsedY;
        if (orthoVP.viewportType == VIEWPORT_XY) {
            midX *= state->viewerState->currentPosition.x - orthoVP.texture.leftUpperPxInAbsPx.x;
            midY *= state->viewerState->currentPosition.y - orthoVP.texture.leftUpperPxInAbsPx.y;
        } else if (orthoVP.viewportType == VIEWPORT_XZ) {
            midX *= state->viewerState->currentPosition.x - orthoVP.texture.leftUpperPxInAbsPx.x;
            midY *= state->viewerState->currentPosition.z - orthoVP.texture.leftUpperPxInAbsPx.z;
        } else if (orthoVP.viewportType == VIEWPORT_ZY) {
            midX *= state->viewerState->currentPosition.z - orthoVP.texture.leftUpperPxInAbsPx.z;
            midY *= state->viewerState->currentPosition.y - orthoVP.texture.leftUpperPxInAbsPx.y;
        } else {
            const auto texUsed = orthoVP.texture.usedSizeInCubePixels / orthoVP.texture.size;
            midX = 0.5 * texUsed;
            midY = 0.5 * texUsed;
        }
        // Calculate the vertices in texture coordinates
        // mid really means current pos inside the texture, in texture coordinates, relative to the texture origin 0., 0.
//        if (orthoVP.viewportType != VIEWPORT_ARBITRARY) {
            orthoVP.texture.texLUx = midX - xFactor;
            orthoVP.texture.texLUy = midY + yFactor;
            orthoVP.texture.texRUx = midX + xFactor;
            orthoVP.texture.texRUy = orthoVP.texture.texLUy;
            orthoVP.texture.texRLx = orthoVP.texture.texRUx;
            orthoVP.texture.texRLy = midY - yFactor;
            orthoVP.texture.texLLx = orthoVP.texture.texLUx;
            orthoVP.texture.texLLy = orthoVP.texture.texRLy;
//        } else {// arb should use the entirety of (a specific part) of the texture
//            const auto texUsed = 1;orthoVP.texture.fovPixel / orthoVP.texture.size * orthoVP.texture.FOV;
//            orthoVP.texture.texLUx = 0;
//            orthoVP.texture.texLUy = texUsed;
//            orthoVP.texture.texRUx = texUsed;
//            orthoVP.texture.texRUy = texUsed;
//            orthoVP.texture.texRLx = texUsed;
//            orthoVP.texture.texRLy = 0;
//            orthoVP.texture.texLLx = 0;
//            orthoVP.texture.texLLy = 0;
//        }
    });
}

void Viewer::loader_notify(const UserMoveType userMoveType, const floatCoordinate & direction) {
    Loader::Controller::singleton().startLoading(state->viewerState->currentPosition, userMoveType, direction);
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
    const uint MAX_COLORVAL{255};
    if (state->viewerState->luminanceBias > 0 || state->viewerState->luminanceRangeDelta < MAX_COLORVAL) {
        const auto originalAdjustment = state->viewerState->datasetAdjustmentTable;
        for (std::size_t i = 0; i < 256; ++i) {
            int dynIndex = ((i - state->viewerState->luminanceBias) / (state->viewerState->luminanceRangeDelta / static_cast<double>(MAX_COLORVAL)));
            dynIndex = std::min(static_cast<int>(MAX_COLORVAL), std::max(0, dynIndex));
            state->viewerState->datasetAdjustmentTable[i] = originalAdjustment[dynIndex];
        }
    }
    state->viewerState->datasetAdjustmentOn = state->viewerState->datasetColortableOn || state->viewerState->luminanceBias > 0 || state->viewerState->luminanceRangeDelta < MAX_COLORVAL;

    dc_reslice_notify_visible();
}

/** Global interfaces  */
void Viewer::rewire() {
    // viewer signals
    QObject::connect(this, &Viewer::zoomChanged, &window->widgetContainer.zoomWidget, &ZoomWidget::update);
    QObject::connect(this, &Viewer::coordinateChangedSignal, [this](const Coordinate & pos) { window->updateCoordinateBar(pos.x, pos.y, pos.z); });
    QObject::connect(this, &Viewer::coordinateChangedSignal, [this](const Coordinate &) {
        if (window->viewport3D->hasCursor) {
            window->updateCursorLabel(Coordinate(), VIEWPORT_SKELETON);
        } else {
            window->forEachOrthoVPDo([](ViewportOrtho & orthoVP) {
                orthoVP.sendCursorPosition();
            });
        }
    });
    // end viewer signals
    //viewport signals
    window->forEachVPDo([this](ViewportBase & vp) {
        QObject::connect(&vp, &ViewportBase::pasteCoordinateSignal, window, &MainWindow::pasteClipboardCoordinates);
        QObject::connect(&vp, &ViewportBase::updateZoomWidget, &window->widgetContainer.zoomWidget, &ZoomWidget::update);
    });
    // end viewport signals
}

void Viewer::addRotation(const QQuaternion & quaternion) {
    const auto prevRotation = QQuaternion::fromAxes(viewportArb->v1, viewportArb->v2, viewportArb->n);
    const auto rotation = quaternion.normalized() * prevRotation.normalized();

    const auto coord = [](auto qvec){
        return floatCoordinate{qvec.x(), qvec.y(), qvec.z()};
    };
    viewportArb->v1 = coord(rotation.rotatedVector({1, 0, 0}).normalized());
    viewportArb->v2 = coord(rotation.rotatedVector({0, 1, 0}).normalized());
    viewportArb->n  = coord(rotation.rotatedVector({0, 0, 1}).normalized());

    viewportArb->ocResliceNecessary = viewportArb->dcResliceNecessary = true;
    recalcTextureOffsets();
    viewportArb->sendCursorPosition();
}

void Viewer::resetRotation() {
    viewportArb->v1 = {1,  0,  0};
    viewportArb->v2 = {0, -1,  0};
    viewportArb->n  = {0,  0, -1};
    viewportArb->ocResliceNecessary = viewportArb->dcResliceNecessary = true;
    recalcTextureOffsets();
    viewportArb->sendCursorPosition();
}

void Viewer::resizeTexEdgeLength(const int cubeEdge, const int superCubeEdge) {
    int newTexEdgeLength = 512;
    while (newTexEdgeLength < cubeEdge * superCubeEdge) {
        newTexEdgeLength *= 2;
    }
    if (newTexEdgeLength != state->viewerState->texEdgeLength) {
        qDebug() << QString("cubeEdge = %1, sCubeEdge = %2, newTex = %3 (%4)").arg(cubeEdge).arg(superCubeEdge).arg(newTexEdgeLength).arg(state->viewerState->texEdgeLength).toStdString().c_str();
        viewerState.texEdgeLength = newTexEdgeLength;
        window->resetTextureProperties();
        window->forEachOrthoVPDo([](ViewportOrtho & vp) {
            vp.resetTexture();
        });
        recalcTextureOffsets();
    }
}

void Viewer::loadNodeLUT(const QString & path) {
    state->viewerState->nodeColors = loadLookupTable(path);
}

void Viewer::loadTreeLUT(const QString & path) {
    state->viewerState->treeColors = loadLookupTable(path);
    skeletonizer->updateTreeColors();
}

QColor Viewer::getNodeColor(const nodeListElement & node) const {
    const auto property = state->viewerState->highlightedNodePropertyByColor;
    const auto range = state->viewerState->nodePropertyColorMapMax - state->viewerState->nodePropertyColorMapMin;
    const auto & nodeColors = state->viewerState->nodeColors;
    QColor color;
    if (!property.isEmpty() && node.properties.contains(property) && range > 0) {
        const int index = (node.properties[property].toDouble() / range * nodeColors.size()) - 1;
        color = QColor::fromRgb(std::get<0>(nodeColors[index]), std::get<1>(nodeColors[index]), std::get<2>(nodeColors[index]));
    }
    else if (node.isBranchNode) { //branch nodes are always blue
        color = Qt::blue;
    }
    else if (CommentSetting::useCommentNodeColor && node.getComment().isEmpty() == false) {
        color = CommentSetting::getColor(node.getComment());
    }
    else if (node.correspondingTree == state->skeletonState->activeTree && state->viewerState->highlightActiveTree) {
        color = Qt::red;
    } else {
        color = node.correspondingTree->color;
    }
    // if focused on synapse, darken rest of skeleton
    const auto * activeTree = state->skeletonState->activeTree;
    const auto * activeNode = state->skeletonState->activeNode;
    const auto * activeSynapse = (activeNode && activeNode->isSynapticNode) ? activeNode->correspondingSynapse :
                                 (activeTree && activeTree->isSynapticCleft) ? activeTree->correspondingSynapse :
                                                                               nullptr;
    const auto synapseBuilding = state->skeletonState->synapseState != Synapse::State::PreSynapse;
    auto partOfSynapse = [&node](const Synapse * synapse) {
        return node.correspondingSynapse == synapse || synapse->getCleft() == node.correspondingTree;
    };
    if ((activeSynapse && !partOfSynapse(activeSynapse)) || (synapseBuilding && !partOfSynapse(&state->skeletonState->temporarySynapse))) {
        color.setAlpha(Synapse::darkenedAlpha);
    }

    return color;
}

void Viewer::setLayerVisibility(const int index, const bool enabled) {
    viewerState.layerVisibility.at(index) = enabled;
    emit layerVisibilityChanged(index);
}
