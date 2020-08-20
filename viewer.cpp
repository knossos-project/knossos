/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "viewer.h"

#include "annotation/annotation.h"
#include "annotation/file_io.h"
#include "functions.h"
#include "loader.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "widgets/mainwindow.h"
#include "widgets/viewports/viewportbase.h"
#include "widgets/widgetcontainer.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QFutureSynchronizer>
#include <QtConcurrentRun>
#include <QVector3D>

#include <boost/container/static_vector.hpp>
#include <boost/range/combine.hpp>

#include <fstream>
#include <cmath>

ViewerState::ViewerState() {
    state->viewerState = this;
}

Viewer::Viewer() : evilHack{[this](){ state->viewer = this; return true; }()} {
    loadTreeLUT();

    recalcTextureOffsets();

    rewire();

    QObject::connect(&timer, &QTimer::timeout, this, &Viewer::run);// timer is started in main

    QObject::connect(&Segmentation::singleton(), &Segmentation::appendedRow, this, &Viewer::segmentation_changed);
    QObject::connect(&Segmentation::singleton(), &Segmentation::changedRow, this, &Viewer::segmentation_changed);
    QObject::connect(&Segmentation::singleton(), &Segmentation::changedRowSelection, this, &Viewer::segmentation_changed);
    QObject::connect(&Segmentation::singleton(), &Segmentation::removedRow, this, &Viewer::segmentation_changed);
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetData, this, &Viewer::segmentation_changed);
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetSelection, this, &Viewer::segmentation_changed);
    QObject::connect(&Segmentation::singleton(), &Segmentation::renderOnlySelectedObjsChanged, this, &Viewer::segmentation_changed);

    static auto regVBuff = [](){
        state->viewerState->AllTreesBuffers.regenVertBuffer = true;
        state->viewerState->selectedTreesBuffers.regenVertBuffer = true;
    };

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::guiModeLoaded, regVBuff);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeAddedSignal, regVBuff);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeChangedSignal, regVBuff);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeRemovedSignal, regVBuff);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::propertiesChanged, regVBuff);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeAddedSignal, regVBuff);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeChangedSignal, regVBuff);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeRemovedSignal, regVBuff);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treesMerged, regVBuff);

    static auto updateSelectionColor = [this](GLBuffers & glBuffers) {
        if(!glBuffers.regenVertBuffer) {
            auto & svp = glBuffers.pointVertBuffer;

            //restore old node color
            size_t offset = svp.colorBufferOffset[svp.lastSelectedNode];
            QColor color = getNodeColor(*state->skeletonState->nodesByNodeID[svp.lastSelectedNode]);

            svp.colors[offset] = decltype(glBuffers.lineVertBuffer.colors)::value_type{{static_cast<std::uint8_t>(color.red()), static_cast<std::uint8_t>(color.green()), static_cast<std::uint8_t>(color.blue()), static_cast<std::uint8_t>(color.alpha())}};

            svp.color_buffer.bind();
            svp.color_buffer.write(static_cast<int>(offset * sizeof(svp.colors[offset])), &svp.colors[offset], sizeof(svp.colors[offset]));
            svp.color_buffer.release();

            //colorize new active node
            svp.lastSelectedNode = state->skeletonState->selectedNodes.front()->nodeID;
            offset = svp.colorBufferOffset[svp.lastSelectedNode];

            color = Qt::green;
            decltype(glBuffers.lineVertBuffer.colors)::value_type activeNodeColor{{static_cast<std::uint8_t>(color.red()), static_cast<std::uint8_t>(color.green()), static_cast<std::uint8_t>(color.blue()), static_cast<std::uint8_t>(color.alpha())}};

            svp.color_buffer.bind();
            svp.color_buffer.write(static_cast<int>(offset * sizeof(activeNodeColor)), &activeNodeColor, sizeof(activeNodeColor));
            svp.color_buffer.release();
        } else {
            glBuffers.regenVertBuffer = true;
        }
    };
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeSelectionChangedSignal, []() {
        if (state->skeletonState->selectedNodes.size() == 1) {
            updateSelectionColor(state->viewerState->AllTreesBuffers);
            updateSelectionColor(state->viewerState->selectedTreesBuffers);
        }
    });
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeSelectionChangedSignal, regVBuff);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::resetData, regVBuff);

    QObject::connect(&Annotation::singleton(), &Annotation::movementAreaChanged, this, [this](){
        updateCurrentPosition();
        reslice_notify();
    });
    QObject::connect(this, &Viewer::movementAreaFactorChangedSignal, [this](){
        for (std::size_t layerId{0}; layerId < Dataset::datasets.size(); ++layerId) {
            if (!Dataset::datasets[layerId].isOverlay()) {
                reslice_notify(layerId);
            }
        }
    });
    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [](){
        state->viewerState->AllTreesBuffers.regenVertBuffer = true;
        state->viewerState->selectedTreesBuffers.regenVertBuffer = true;
    });

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
        mainWindow.forEachVPDo([&settings](ViewportBase & vp) {
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

int Viewer::highestMag() {
    return viewerState.datasetMagLock ? Dataset::current().magnification : Dataset::current().highestAvailableMag;
}

int Viewer::lowestMag() {
    return viewerState.datasetMagLock ? Dataset::current().magnification : Dataset::current().lowestAvailableMag;
}

float Viewer::highestScreenPxXPerDataPx(const bool ofCurrentMag) {
    const float texUnitsPerDataPx = 1. / viewerState.texEdgeLength / (ofCurrentMag ? lowestMag() : Dataset::current().lowestAvailableMag);
    auto * vp = viewportXY;
    float FOVinDCs = static_cast<float>(state->M) - 1.f;
    float displayedEdgeLen = (FOVinDCs * VPZOOMMAX * Dataset::current().cubeEdgeLength) / vp->texture.size;
    displayedEdgeLen = (std::ceil(displayedEdgeLen / 2. / texUnitsPerDataPx) * texUnitsPerDataPx) * 2.;
    return vp->edgeLength / (displayedEdgeLen / texUnitsPerDataPx);
}

float Viewer::lowestScreenPxXPerDataPx(const bool ofCurrentMag) {
    const float texUnitsPerDataPx = 1. / viewerState.texEdgeLength / (ofCurrentMag ? highestMag() : Dataset::current().highestAvailableMag);
    auto * vp = viewportXY;
    float FOVinDCs = static_cast<float>(state->M) - 1.f;
    float displayedEdgeLen = (FOVinDCs * Dataset::current().cubeEdgeLength) / vp->texture.size;
    displayedEdgeLen = (std::ceil(displayedEdgeLen / 2. / texUnitsPerDataPx) * texUnitsPerDataPx) * 2.;
    return vp->edgeLength / (displayedEdgeLen / texUnitsPerDataPx);
}

int Viewer::calcMag(const float screenPxXPerDataPx) {
    const float exactMag = Dataset::current().highestAvailableMag * lowestScreenPxXPerDataPx() / screenPxXPerDataPx;
    const auto roundedPower = std::ceil(std::log2(exactMag));
    return std::min(Dataset::current().highestAvailableMag, std::max(static_cast<int>(std::pow(2, roundedPower)), Dataset::current().lowestAvailableMag));
}

void Viewer::setMagnificationLock(const bool locked) {
    viewerState.datasetMagLock = locked;
    if (!locked) {
        const auto newMag = calcMag(viewportXY->screenPxXPerDataPx);
        if (newMag != Dataset::current().magnification) {
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

const auto datasetAdjustment = [](auto layerId, auto index){
    if (state->viewerState->datasetColortableOn) {
        return state->viewerState->datasetColortable[index];
    } else {
        const auto MAX_COLORVAL{std::numeric_limits<uint8_t>::max()};
        int dynIndex = ((index - state->viewerState->layerRenderSettings[layerId].bias * 255) / (state->viewerState->layerRenderSettings[layerId].rangeDelta));
        std::uint8_t val = std::min(static_cast<int>(MAX_COLORVAL), std::max(0, dynIndex));
        return std::tuple(val, val, val);
    }
};

void Viewer::dcSliceExtract(std::uint8_t * datacube, Coordinate cubePosInAbsPx, std::uint8_t * slice, ViewportOrtho & vp, const std::size_t layerId, const boost::optional<decltype(LayerRenderSettings::combineSlicesType)> combineType) {
    const auto cubeCoord = Dataset::current().global2cube(cubePosInAbsPx);
    const auto cubeMaxGlobalCoord = Dataset::current().cube2global(cubeCoord + CoordOfCube{1,1,1}) - Coordinate{1,1,1};
    const auto cubeEdgeLen = Dataset::current().cubeEdgeLength;
    const auto partlyOutsideMovementArea = Annotation::singleton().outsideMovementArea(Dataset::current().cube2global(cubeCoord))
            || Annotation::singleton().outsideMovementArea(cubeMaxGlobalCoord);
    // we traverse ZY column first because of better locailty of reference
    const std::size_t voxelIncrement = vp.viewportType == VIEWPORT_ZY ? cubeEdgeLen : 1;
    const std::size_t sliceIncrement = vp.viewportType == VIEWPORT_XY ? cubeEdgeLen : cubeEdgeLen * cubeEdgeLen;
    const std::size_t lineIncrement = vp.viewportType == VIEWPORT_ZY ? 0 : sliceIncrement - cubeEdgeLen;
    const std::size_t texNext = vp.viewportType == VIEWPORT_ZY ? cubeEdgeLen * 4 : 4;// RGBA per pixel
    const std::size_t texNextLine = vp.viewportType == VIEWPORT_ZY ? 4 - 4 * cubeEdgeLen * cubeEdgeLen : 0;

    const bool isDatasetAdjustment = state->viewerState->datasetColortableOn || state->viewerState->layerRenderSettings[layerId].bias > 0.0 || state->viewerState->layerRenderSettings[layerId].rangeDelta < 1.0;
    for (int yzz = 0; yzz < cubeEdgeLen; ++yzz) {
        for (int xxy = 0; xxy < cubeEdgeLen; ++xxy) {
            uint8_t r, g, b;
            if (isDatasetAdjustment) {
                std::tie(r, g, b) = datasetAdjustment(layerId, datacube[0]);
            } else {
                r = g = b = datacube[0];
            }
            if (partlyOutsideMovementArea) {
                const auto offsetx = Dataset::current().scaleFactor.componentMul(Coordinate(vp.viewportType == VIEWPORT_XY || vp.viewportType == VIEWPORT_XZ, vp.viewportType == VIEWPORT_ZY, 0) * xxy);
                const auto offsety = Dataset::current().scaleFactor.componentMul(Coordinate(0, vp.viewportType == VIEWPORT_XY, vp.viewportType == VIEWPORT_XZ || vp.viewportType == VIEWPORT_ZY) * yzz);
                if (Annotation::singleton().outsideMovementArea(cubePosInAbsPx + offsetx + offsety)) {
                    const double d = state->viewerState->outsideMovementAreaFactor / 100.0 + (1 - state->viewerState->outsideMovementAreaFactor / 100.0) * state->viewerState->showOnlyRawData;
                    r *= d; g *= d; b *= d;
                }
            }
            if (combineType) {
                if (combineType.get() == decltype(LayerRenderSettings::combineSlicesType)::min) {
                    slice[0] = std::min(slice[0], r);
                    slice[1] = std::min(slice[1], g);
                    slice[2] = std::min(slice[2], b);
                } else if (combineType.get() == decltype(LayerRenderSettings::combineSlicesType)::max) {
                    slice[0] = std::max(slice[0], r);
                    slice[1] = std::max(slice[1], g);
                    slice[2] = std::max(slice[2], b);
                }
            } else {
                slice[0] = r;
                slice[1] = g;
                slice[2] = b;
                slice[3] = 255;
            }
            datacube += voxelIncrement;
            slice += texNext;
        }
        datacube += lineIncrement;
        slice += texNextLine;
    }
}

void Viewer::dcSliceExtract(std::uint8_t * datacube, floatCoordinate *currentPxInDc_float, std::uint8_t * slice, int s, int *t, const floatCoordinate & v2, const std::size_t layerId, float usedSizeInCubePixels) {
    Coordinate currentPxInDc = {roundFloat(currentPxInDc_float->x), roundFloat(currentPxInDc_float->y), roundFloat(currentPxInDc_float->z)};
    const auto cubeEdgeLen = Dataset::current().cubeEdgeLength;
    if((currentPxInDc.x < 0) || (currentPxInDc.y < 0) || (currentPxInDc.z < 0) ||
       (currentPxInDc.x >= cubeEdgeLen) || (currentPxInDc.y >= cubeEdgeLen) || (currentPxInDc.z >= cubeEdgeLen)) {
        const int sliceIndex = 3 * ( s + *t * std::ceil(usedSizeInCubePixels));
        slice[sliceIndex] = slice[sliceIndex + 1] = slice[sliceIndex + 2] = 0;
        slice[sliceIndex + 3] = 255;
        (*t)++;
        if(*t < usedSizeInCubePixels) {
            // Actually, although currentPxInDc_float is passed by reference and updated here,
            // it is totally ignored (i.e. not read, then overwritten) by the calling function.
            // But to keep the functionality here compatible after this bugfix, we also replicate
            // this update here - from the originial below
            *currentPxInDc_float -= v2;
        }
        return;
    }

    while((0 <= currentPxInDc.x && currentPxInDc.x < cubeEdgeLen)
          && (0 <= currentPxInDc.y && currentPxInDc.y < cubeEdgeLen)
          && (0 <= currentPxInDc.z && currentPxInDc.z < cubeEdgeLen)) {

        const int sliceIndex = 4 * ( s + *t * std::ceil(usedSizeInCubePixels));
        const int dcIndex = currentPxInDc.x + currentPxInDc.y * cubeEdgeLen + currentPxInDc.z * state->cubeSliceArea;
        if(datacube == nullptr) {
            slice[sliceIndex] = slice[sliceIndex + 1] = slice[sliceIndex + 2] = 0;
        } else {
            if (layerId) {
                std::tie(slice[sliceIndex + 0], slice[sliceIndex + 1], slice[sliceIndex + 2]) = datasetAdjustment(layerId, datacube[dcIndex]);
            }
            else {
                slice[sliceIndex] = slice[sliceIndex + 1] = slice[sliceIndex + 2] = datacube[dcIndex];
            }
        }
        slice[sliceIndex + 3] = 255;
        (*t)++;
        if(*t >= usedSizeInCubePixels) {
            break;
        }
        *currentPxInDc_float -= v2;
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
void Viewer::ocSliceExtract(std::uint64_t * datacube, Coordinate cubePosInAbsPx, std::uint8_t * slice, ViewportOrtho & vp, const std::size_t layerId) {
    const auto cubeCoord = Dataset::current().global2cube(cubePosInAbsPx);
    const auto cubeMaxGlobalCoord = Dataset::current().cube2global(cubeCoord + CoordOfCube{1,1,1}) - Coordinate{1,1,1};
    const auto cubeEdgeLen = Dataset::current().cubeEdgeLength;
    const auto partlyOutsideMovementArea = Annotation::singleton().outsideMovementArea(Dataset::current().cube2global(cubeCoord))
            || Annotation::singleton().outsideMovementArea(cubeMaxGlobalCoord);
    // we traverse ZY column first because of better locailty of reference
    const std::size_t voxelIncrement = vp.viewportType == VIEWPORT_ZY ? cubeEdgeLen : 1;
    const std::size_t sliceIncrement = vp.viewportType == VIEWPORT_XY ? cubeEdgeLen : cubeEdgeLen * cubeEdgeLen;
    const std::size_t lineIncrement = vp.viewportType == VIEWPORT_ZY ? 0 : sliceIncrement - cubeEdgeLen;
    const std::size_t texNext = vp.viewportType == VIEWPORT_ZY ? cubeEdgeLen * 4 : 4;// RGBA per pixel
    const std::size_t texNextLine = vp.viewportType == VIEWPORT_ZY ? 4 - 4 * cubeEdgeLen * cubeEdgeLen : 0;

    const auto & seg = Segmentation::singleton();
    //cache
    uint64_t subobjectIdCache = Segmentation::singleton().getBackgroundId();
    bool selectedCache = seg.isSubObjectIdSelected(subobjectIdCache);
    auto colorCache = seg.colorObjectFromSubobjectId(subobjectIdCache);
    //first and last row boundaries
    const std::size_t min = cubeEdgeLen;
    const std::size_t max = cubeEdgeLen * (cubeEdgeLen - 1);
    std::size_t counter = 0;//slice position
    for (int yzz = 0; yzz < cubeEdgeLen; ++yzz) {
        for (int xxy = 0; xxy < cubeEdgeLen; ++xxy) {
            bool hide = false;
            if (partlyOutsideMovementArea) {
                const auto offsetx = Dataset::current().scaleFactor.componentMul(Coordinate(vp.viewportType == VIEWPORT_XY || vp.viewportType == VIEWPORT_XZ, vp.viewportType == VIEWPORT_ZY, 0) * xxy);
                const auto offsety = Dataset::current().scaleFactor.componentMul(Coordinate(0, vp.viewportType == VIEWPORT_XY, vp.viewportType == VIEWPORT_XZ || vp.viewportType == VIEWPORT_ZY) * yzz);
                if (Annotation::singleton().outsideMovementArea(cubePosInAbsPx + offsetx + offsety)) {
                    slice[3] = 0;
                    hide = true;
                }
            }

            if(hide == false) {
                const uint64_t subobjectId = datacube[0];

                const auto color = (subobjectIdCache == subobjectId) ? colorCache : seg.colorObjectFromSubobjectId(subobjectId);
                slice[0] = std::get<0>(color);
                slice[1] = std::get<1>(color);
                slice[2] = std::get<2>(color);
                slice[3] = std::get<3>(color);

                const bool selected = layerId == seg.layerId && ((subobjectIdCache == subobjectId) ? selectedCache : seg.isSubObjectIdSelected(subobjectId));
                const bool isPastFirstRow = counter >= min;
                const bool isBeforeLastRow = counter < max;
                const bool isNotFirstColumn = counter % cubeEdgeLen != 0;
                const bool isNotLastColumn = (counter + 1) % cubeEdgeLen != 0;

                // highlight edges where needed
                if(seg.highlightBorder) {
                    if(seg.hoverVersion) {
                        uint64_t objectId = seg.tryLargestObjectContainingSubobject(subobjectId);
                        if (selected && seg.mouseFocusedObjectId == objectId) {
                            if(isPastFirstRow && isBeforeLastRow && isNotFirstColumn && isNotLastColumn) {
                                const uint64_t left   = seg.tryLargestObjectContainingSubobject(*reinterpret_cast<uint64_t*>(datacube - voxelIncrement));
                                const uint64_t right  = seg.tryLargestObjectContainingSubobject(*reinterpret_cast<uint64_t*>(datacube + voxelIncrement));
                                const uint64_t top    = seg.tryLargestObjectContainingSubobject(*reinterpret_cast<uint64_t*>(datacube - sliceIncrement));
                                const uint64_t bottom = seg.tryLargestObjectContainingSubobject(*reinterpret_cast<uint64_t*>(datacube + sliceIncrement));
                                //enhance alpha of this voxel if any of the surrounding voxels belong to another object
                                if (objectId != left || objectId != right || objectId != top || objectId != bottom) {
                                    slice[3] = std::min(255, slice[3]*4);
                                }
                            }
                        }
                    }
                    else if (selected && isPastFirstRow && isBeforeLastRow && isNotFirstColumn && isNotLastColumn) {
                        const uint64_t left   = datacube[-voxelIncrement];
                        const uint64_t right  = datacube[+voxelIncrement];
                        const uint64_t top    = datacube[-sliceIncrement];
                        const uint64_t bottom = datacube[+sliceIncrement];
                        //enhance alpha of this voxel if any of the surrounding voxels belong to another subobject
                        if (subobjectId != left || subobjectId != right || subobjectId != top || subobjectId != bottom) {
                            slice[3] = std::min(255, slice[3]*4);
                        }
                    }
                }

                //fill cache
                subobjectIdCache = subobjectId;
                colorCache = color;
                selectedCache = selected;
            }
            ++counter;
            datacube += voxelIncrement;
            slice += texNext;
        }
        datacube += lineIncrement;
        slice += texNextLine;
    }
}

void Viewer::vpGenerateTexture(ViewportOrtho & vp, const std::size_t layerId) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.
    if (vp.viewportType == VIEWPORT_ARBITRARY) {
        vpGenerateTexture(static_cast<ViewportArb&>(vp), layerId);
        return;
    }
    const int multiSliceiMax = viewerState.layerRenderSettings[layerId].combineSlicesEnabled
            * viewerState.layerRenderSettings[layerId].combineSlices
            * ((vp.viewportType == VIEWPORT_XY) || !viewerState.layerRenderSettings[layerId].combineSlicesXyOnly);
    bool first{true};
    const auto cubeEdgeLen = Dataset::current().cubeEdgeLength;
    auto for_each_resliced_cube_do = [this, layerId, cubeEdgeLen, &vp](const CoordOfCube upperLeftDc, auto func){
        for (int x_dc = 0; x_dc < state->M; ++x_dc) {
            for (int y_dc = 0; y_dc < state->M; ++y_dc) {
                const auto v1dc = vp.v1 * x_dc, v2dc = vp.v2 * -y_dc;// v2 is negative
                CoordOfCube currentDc{upperLeftDc + CoordOfCube(v1dc.x, v1dc.y, v1dc.z) + CoordOfCube(v2dc.x, v2dc.y, v2dc.z)};
                if (!vp.resliceNecessary[layerId] && vp.resliceNecessaryCubes[layerId].find(currentDc) == std::end(vp.resliceNecessaryCubes[layerId])) {
                    continue;
                }
                const int index = 4 * (y_dc * viewerState.texEdgeLength * cubeEdgeLen + x_dc * cubeEdgeLen * cubeEdgeLen);
                func(x_dc, y_dc, currentDc, index);
            }
        }
    };
    for (int multiSlicei{-multiSliceiMax}; multiSlicei <= multiSliceiMax; ++multiSlicei) {
        const auto cubeEdgeLen = Dataset::current().cubeEdgeLength;
        const auto offset = vp.n.componentMul(Dataset::current().scaleFactor) * multiSlicei;
        const auto cpos = state->viewerState->currentPosition;
        const auto offsetCube = Dataset::datasets[layerId].global2cube(cpos + offset) - Dataset::datasets[layerId].global2cube(cpos);
        const auto & [min, max] = state->viewerState->showOnlyRawData ? std::pair(Coordinate(0, 0, 0), Dataset::current().boundary)
                                                                      : std::pair(Annotation::singleton().movementAreaMin, Annotation::singleton().movementAreaMax);
        const CoordInCube currentPosition_inside_dc = (cpos + offset)
                .capped(min, max)
                .insideCube(cubeEdgeLen, Dataset::current().scaleFactor);
        if (Annotation::singleton().outsideMovementArea(state->viewerState->currentPosition + offset) && !state->viewerState->showOnlyRawData) {
            continue;
        }

        // We iterate over the texture with x and y being in a temporary coordinate
        // system local to this texture.
        if (!vp.resliceNecessary[layerId] && vp.resliceNecessaryCubes[layerId].empty()) {
            return;
        }
        const CoordOfCube upperLeftDc = Dataset::datasets[layerId].global2cube(vp.texture.leftUpperPxInAbsPx) + offsetCube;
        QFutureSynchronizer<void> sync;
        for_each_resliced_cube_do(upperLeftDc, [this, cubeEdgeLen, layerId, &vp, &sync, currentPosition_inside_dc, first](auto, auto, auto currentDc, auto index){
            int slicePositionWithinCube = vp.n.componentMul(currentPosition_inside_dc.componentMul(Coordinate{1, cubeEdgeLen, state->cubeSliceArea})).length();
            Coordinate offsetCubeGlobal = vp.n.componentMul(vp.n.componentMul(currentPosition_inside_dc));// ensure n is positive by multiplying with itself

            state->protectCube2Pointer.lock();
            void * const cube = cubeQuery(state->cube2Pointer, layerId, Dataset::current().magIndex, currentDc);
            state->protectCube2Pointer.unlock();

            // Take care of the data textures.
            Coordinate slicePosInAbsPx = Dataset::datasets[layerId].cube2global(currentDc) + Dataset::datasets[layerId].scaleFactor.componentMul(offsetCubeGlobal);
            // This is used to index into the texture. overlayData[index] is the first
            // byte of the datacube slice at position (x_dc, y_dc) in the texture.
            sync.addFuture(QtConcurrent::run([this, &vp, cube, first, slicePositionWithinCube, slicePosInAbsPx, index, layerId, cubeEdgeLen]()  {
                if (cube != nullptr) {
                    if (Dataset::datasets[layerId].isOverlay()) {
                        ocSliceExtract(reinterpret_cast<std::uint64_t *>(cube) + slicePositionWithinCube, slicePosInAbsPx, vp.texture.texData[layerId].data() + index, vp, layerId);
                    } else {
                        const auto combine = boost::make_optional(!first, viewerState.layerRenderSettings[layerId].combineSlicesType);
                        dcSliceExtract(reinterpret_cast<std::uint8_t  *>(cube) + slicePositionWithinCube, slicePosInAbsPx, vp.texture.texData[layerId].data() + index, vp, layerId, combine);
                    }
                } else {
                    std::fill(vp.texture.texData[layerId].data() + index, vp.texture.texData[layerId].data() + index + 4 * cubeEdgeLen * cubeEdgeLen, 0);
                }
            }));
        });
        sync.waitForFinished();
        first = false;
    }
    vp.texture.texHandle[layerId].bind();
    const CoordOfCube upperLeftDc = Dataset::datasets[layerId].global2cube(vp.texture.leftUpperPxInAbsPx);
    for_each_resliced_cube_do(upperLeftDc, [cubeEdgeLen, layerId, &vp](auto x_dc, auto y_dc, auto, auto index){
        glTexSubImage2D(GL_TEXTURE_2D, 0, x_dc * cubeEdgeLen, y_dc * cubeEdgeLen, cubeEdgeLen, cubeEdgeLen, GL_RGBA, GL_UNSIGNED_BYTE, vp.texture.texData[layerId].data() + index);
    });
    vp.texture.texHandle[layerId].release();
    glBindTexture(GL_TEXTURE_2D, 0);
    vp.resliceNecessary[layerId] = false;
    vp.resliceNecessaryCubes[layerId].clear();
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

    const auto gpusupercube = (state->M - 1) * Dataset::current().cubeEdgeLength / gpucubeedge + 1;//remove cpu overlap and add gpu overlap
    const auto scroot = (state->viewerState->currentPosition / gpucubeedge) - gpusupercube / 2;
    floatCoordinate root = vp.texture.leftUpperPxInAbsPx / Dataset::current().magnification;
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
                const auto globalCoord = gpuCoord.cube2Global(gpucubeedge, Dataset::current().scaleFactor);
                for (auto & layer : layers) {
                    auto cubeIt = layer.textures.find(gpuCoord);
                    if (cubeIt != std::end(layer.textures)) {
                        cubeIt->second->vertices = /*std::move*/(points);
                    } else {
                        const auto cubeCoord = Dataset::current().global2cube(globalCoord);
                        const auto offset = globalCoord - Dataset::current().cube2global(cubeCoord);
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

void Viewer::vpGenerateTexture(ViewportArb &vp, const std::size_t layerId) {
    if (Dataset::datasets[layerId].isOverlay() || !vp.resliceNecessary[layerId]) {
        return;
    }
    vp.resliceNecessary[layerId] = false;

    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.
    floatCoordinate currentPxInDc_float, rowPx_float, currentPx_float;

    rowPx_float = vp.texture.leftUpperPxInAbsPx / Dataset::current().scaleFactor;
    currentPx_float = rowPx_float;

    static std::vector<std::uint8_t> texData;// reallocation for every run would be a waste
    texData.resize(4 * std::pow(std::ceil(vp.texture.usedSizeInCubePixels), 2), 0);

    int s = 0, t = 0, t_old = 0;
    while(s < vp.texture.usedSizeInCubePixels) {
        t = 0;
        while(t < vp.texture.usedSizeInCubePixels) {
            Coordinate currentPx = {roundFloat(currentPx_float.x), roundFloat(currentPx_float.y), roundFloat(currentPx_float.z)};
            Coordinate currentDc = currentPx / Dataset::current().cubeEdgeLength;

            if(currentPx.x < 0) { currentDc.x -= 1; }
            if(currentPx.y < 0) { currentDc.y -= 1; }
            if(currentPx.z < 0) { currentDc.z -= 1; }

            state->protectCube2Pointer.lock();
            void * const datacube = cubeQuery(state->cube2Pointer, layerId, Dataset::datasets[layerId].magIndex, {currentDc.x, currentDc.y, currentDc.z});
            state->protectCube2Pointer.unlock();

            currentPxInDc_float = currentPx_float - currentDc * Dataset::current().cubeEdgeLength;
            t_old = t;

            dcSliceExtract(reinterpret_cast<std::uint8_t *>(datacube), &currentPxInDc_float, texData.data(), s, &t, vp.v2, layerId, vp.texture.usedSizeInCubePixels);
            currentPx_float = currentPx_float - vp.v2 * (t - t_old);
        }
        s++;
        rowPx_float += vp.v1;
        currentPx_float = rowPx_float;
    }

    vp.texture.texHandle[layerId].bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, std::ceil(vp.texture.usedSizeInCubePixels), std::ceil(vp.texture.usedSizeInCubePixels), GL_RGBA, GL_UNSIGNED_BYTE, texData.data());
    vp.texture.texHandle[layerId].release();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Viewer::calcLeftUpperTexAbsPx() {
    window->forEachOrthoVPDo([this](ViewportOrtho & orthoVP) {
        const auto fov = orthoVP.texture.usedSizeInCubePixels;
        const CoordOfCube currentPosition_dc = viewerState.currentPosition.cube(Dataset::current().cubeEdgeLength, Dataset::current().scaleFactor);
        const auto xy = orthoVP.viewportType == VIEWPORT_XY;
        const auto xz = orthoVP.viewportType == VIEWPORT_XZ;
        const auto zy = orthoVP.viewportType == VIEWPORT_ZY;
        const auto leftUpperDc = currentPosition_dc - CoordOfCube{xy || xz, xy || zy, xz || zy} * state->M / 2;
        orthoVP.texture.leftUpperPxInAbsPx = Dataset::current().cube2global(leftUpperDc);
        if (orthoVP.viewportType == VIEWPORT_ARBITRARY) {
            auto & arbVP = static_cast<ViewportArb&>(orthoVP);
            arbVP.leftUpperPxInAbsPx_float = floatCoordinate{viewerState.currentPosition} - Dataset::current().scaleFactor.componentMul((orthoVP.v1 - orthoVP.v2) * 0.5 * fov);// broken arb slicing depends on this
            arbVP.texture.leftUpperPxInAbsPx = arbVP.leftUpperPxInAbsPx_float;
        }
    });
}

void Viewer::calcDisplayedEdgeLength() {
    window->forEachOrthoVPDo([](ViewportOrtho & vpOrtho){
        const auto & layer = Dataset::current();
        auto & texture = vpOrtho.texture;
        const auto voxelV1X = layer.scale.componentMul(vpOrtho.v1).length() / layer.scale.x;
        const auto voxelV2X = std::abs(layer.scale.componentMul(vpOrtho.v2).length()) / layer.scale.x;
        const auto voxelV1Xmag1 = layer.scales[0].componentMul(vpOrtho.v1).length() / layer.scales[0].x;
        const auto voxelV2Xmag1 = std::abs(layer.scales[0].componentMul(vpOrtho.v2).length()) / layer.scales[0].x;

        texture.texUsedX = texture.usedSizeInCubePixels / texture.size * texture.FOV / voxelV1X;
        texture.texUsedY = texture.usedSizeInCubePixels / texture.size * texture.FOV / voxelV2X;

        vpOrtho.displayedIsoPx = layer.scales[0].x * 0.5 * texture.usedSizeInCubePixels * texture.FOV * layer.magnification;// FOV is within current mag
        const auto dataPx = texture.usedSizeInCubePixels * texture.FOV * layer.magnification;
        vpOrtho.screenPxXPerDataPx = vpOrtho.edgeLength / dataPx * voxelV1X;
        vpOrtho.screenPxXPerMag1Px = vpOrtho.edgeLength / dataPx * voxelV1Xmag1;
        vpOrtho.screenPxYPerMag1Px = vpOrtho.edgeLength / dataPx * voxelV2Xmag1;

        vpOrtho.displayedlengthInNmX = layer.scales[0].componentMul(vpOrtho.v1).length() * (texture.texUsedX / texture.texUnitsPerDataPx);
    });
}

void Viewer::zoom(const float factor) {
    const auto & texture = viewportXY->texture;
    const bool reachedHighestMag = Dataset::current().magnification == Dataset::current().highestAvailableMag;
    const bool reachedLowestMag = Dataset::current().magnification == Dataset::current().lowestAvailableMag;
    const bool reachedMinZoom = texture.FOV * factor > VPZOOMMIN && reachedHighestMag;
    const bool reachedMaxZoom = texture.FOV * factor < VPZOOMMAX && reachedLowestMag;
    const bool magUp = texture.FOV == VPZOOMMIN && factor > 1 && !reachedHighestMag;
    const bool magDown = texture.FOV == 0.5 && factor < 1 && !reachedLowestMag;

    const auto updateFOV = [this](const float newFOV) {
        window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) {
            orthoVP.texture.FOV = newFOV;
        });
    };
    auto newMag = Dataset::current().magnification;
    if (reachedMinZoom) {
        updateFOV(VPZOOMMIN);
    } else if (reachedMaxZoom) {
        updateFOV(VPZOOMMAX);
    } else if (state->viewerState->datasetMagLock) {
        updateFOV(texture.FOV * factor > VPZOOMMIN ? VPZOOMMIN :
                  texture.FOV * factor < VPZOOMMAX ? VPZOOMMAX :
                  texture.FOV * factor);
    } else if (magUp) {
        newMag *= 2;
        updateFOV(0.5);
    } else if (magDown) {
        newMag /= 2;
        updateFOV(VPZOOMMIN);
    } else {
        const float zoomMax = Dataset::current().magnification == Dataset::current().lowestAvailableMag ? VPZOOMMAX : 0.5;
        updateFOV(std::max(std::min(texture.FOV * factor, static_cast<float>(VPZOOMMIN)), zoomMax));
    }

    if (newMag != Dataset::current().magnification) {
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

bool Viewer::updateDatasetMag(const int mag) {
    Loader::Controller::singleton().unloadCurrentMagnification(); //unload all the cubes
    if (mag != 0) {// change global mag after unloading
        const bool powerOf2 = mag > 0 && (mag & (mag - 1)) == 0;
        if (!powerOf2 || mag < Dataset::current().lowestAvailableMag || mag > Dataset::current().highestAvailableMag) {
            return false;
        }
        for (auto && layer : Dataset::datasets) {
            layer.magnification = mag;
            layer.magIndex = static_cast<std::size_t>(std::log2(mag));
            if (layer.scales.size() > layer.magIndex) {
                layer.scale = layer.scales[layer.magIndex];
                layer.scaleFactor = layer.scale / layer.scales[0];
            }
            window->forEachOrthoVPDo([mag](ViewportOrtho & orthoVP) {
                orthoVP.texture.texUnitsPerDataPx = 1.f / state->viewerState->texEdgeLength / mag;
            });
        }
    }
    //clear the viewports
    reslice_notify();

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
        timer.stop();
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
                    const auto globalCoord = pair.first.cube2Global(gpucubeedge, Dataset::current().scaleFactor);
                    const auto cubeCoord = globalCoord.cube(Dataset::current().cubeEdgeLength, Dataset::current().scaleFactor);
                    state->protectCube2Pointer.lock();
                    const auto * ptr = cubeQuery(state->cube2Pointer, layer.isOverlayData, Dataset::current().magIndex, cubeCoord);
                    state->protectCube2Pointer.unlock();
                    if (ptr != nullptr) {
                        layer.cubeSubArray(ptr, Dataset::current().cubeEdgeLength, gpucubeedge, pair.first, pair.second);
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

    window->forEachOrthoVPDo([](ViewportOrtho & vp) {
        vp.update();
    });
    window->viewport3D->update();
    window->updateTitlebar(); //display changes after filename
}

void Viewer::applyTextureFilterSetting(const QOpenGLTexture::Filter texFiltering) {
    viewerState.textureFilter = texFiltering;
    for (std::size_t layerId{0}; layerId < Dataset::datasets.size(); ++layerId) {
        if (!Dataset::datasets[layerId].isOverlay()) {
            viewerState.layerRenderSettings[layerId].textureFilter = texFiltering;
        } else {// overlay should have sharp edges by default
            viewerState.layerRenderSettings[layerId].textureFilter = QOpenGLTexture::Nearest;
        }
    }
    window->forEachOrthoVPDo([](ViewportOrtho & orthoVP) {
        orthoVP.applyTextureFilter();
    });
    emit layerRenderSettingsChanged();
}

void Viewer::updateCurrentPosition() {
    auto & annotation = Annotation::singleton();
    if (annotation.outsideMovementArea(state->viewerState->currentPosition)) {
        const Coordinate currPos = state->viewerState->currentPosition;
                const auto & [min, max] = state->viewerState->showOnlyRawData ? std::pair(Coordinate(0, 0, 0), Dataset::current().boundary)
                                                                              : std::pair(Annotation::singleton().movementAreaMin, Annotation::singleton().movementAreaMax);
        const Coordinate newPos = state->viewerState->currentPosition.capped(min, max);
        userMove(newPos - currPos, USERMOVE_NEUTRAL);
    }
}

void Viewer::setPosition(const floatCoordinate & pos, UserMoveType userMoveType, const Coordinate & viewportNormal) {
    const auto deltaPos = pos - state->viewerState->currentPosition;
    userMove(deltaPos, userMoveType, viewportNormal);
    userMoveRound(userMoveType, viewportNormal);
}

void Viewer::setPositionWithRecentering(const Coordinate &pos) {
    remote.process(pos);
}

void Viewer::setPositionWithRecenteringAndRotation(const Coordinate &pos) {
    remote.process(pos, true);
}

void Viewer::userMoveVoxels(const Coordinate & step, UserMoveType userMoveType, const floatCoordinate & viewportNormal) {
    auto & viewerState = *state->viewerState;
    boost::container::static_vector<ViewportOrtho*, 4> resliceVPs;
    if (step.z != 0) {
        resliceVPs.emplace_back(viewportXY);
    }
    if (step.x != 0) {
        resliceVPs.emplace_back(viewportZY);
    }
    if (step.y != 0) {
        resliceVPs.emplace_back(viewportXZ);
    }
    if (!resliceVPs.empty()) {
        resliceVPs.emplace_back(viewportArb);
    }
    for (auto && vp : resliceVPs) {
        for (auto && elem : vp->resliceNecessary) {
            elem = true;
        }
    }

    // This determines whether the server will broadcast the coordinate change
    // to its client or not.
    const auto lastPosition_dc = viewerState.currentPosition.cube(Dataset::current().cubeEdgeLength, Dataset::current().scaleFactor);
    const auto lastPosition_gpudc = viewerState.currentPosition.cube(gpucubeedge, Dataset::current().scaleFactor);

    const Coordinate movement = step;
    auto newPos = viewerState.currentPosition + movement;
    if (Annotation::singleton().outsideMovementArea(newPos) && !state->viewerState->showOnlyRawData) {
        const auto inc{state->skeletonState->displayMatlabCoordinates};
        qDebug() << tr("Position (%1, %2, %3) out of bounds").arg(newPos.x + inc).arg(newPos.y + inc).arg(newPos.z + inc);
    }
    const auto & [min, max] = state->viewerState->showOnlyRawData ? std::pair(Coordinate(0, 0, 0), Dataset::current().boundary)
                                                                  : std::pair(Annotation::singleton().movementAreaMin, Annotation::singleton().movementAreaMax);
    viewerState.currentPosition = newPos.capped(min, max);
    recalcTextureOffsets();

    const auto newPosition_dc = viewerState.currentPosition.cube(Dataset::current().cubeEdgeLength, Dataset::current().scaleFactor);
    const auto newPosition_gpudc = viewerState.currentPosition.cube(gpucubeedge, Dataset::current().scaleFactor);

    if (newPosition_dc != lastPosition_dc) {
        reslice_notify();
        // userMoveType How user movement was generated
        // Direction of user movement in case of drilling,
        // or normal to viewport plane in case of horizontal movement.
        // Left unset in neutral movement.
        const auto direction = (userMoveType == USERMOVE_DRILL) ? floatCoordinate{step} : (userMoveType == USERMOVE_HORIZONTAL) ? viewportNormal : floatCoordinate{};
        loader_notify(userMoveType, direction);
    }

    if (state->gpuSlicer && newPosition_gpudc != lastPosition_gpudc) {
        const auto supercubeedge = state->M * Dataset::current().cubeEdgeLength / gpucubeedge - (Dataset::current().cubeEdgeLength / gpucubeedge - 1);
        for (auto & layer : layers) {
            layer.ctx.makeCurrent(&layer.surface);
            std::vector<CoordOfGPUCube> obsoleteCubes;
            for (const auto & pair : layer.textures) {
                const auto pos = pair.first;
                const auto globalCoord = pos.cube2Global(gpucubeedge, Dataset::current().scaleFactor);
                if (!currentlyVisible(globalCoord, viewerState.currentPosition, supercubeedge, Dataset::current().scaleFactor * gpucubeedge)) {
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

    const auto gpusupercube = (state->M - 1) * Dataset::current().cubeEdgeLength / gpucubeedge + 1;//remove cpu overlap and add gpu overlap
    const int halfSupercube = gpusupercube * 0.5;
    auto edge = state->viewerState->currentPosition.cube(gpucubeedge, Dataset::current().scaleFactor) - halfSupercube;
    const auto end = edge + gpusupercube;
    edge = {std::max(0, edge.x), std::max(0, edge.y), std::max(0, edge.z)};//negative coords are calculated incorrectly and there are no cubes anyway
    for (int x = edge.x; x < end.x; ++x)
    for (int y = edge.y; y < end.y; ++y)
    for (int z = edge.z; z < end.z; ++z) {
        const auto gpuCoord = CoordOfGPUCube{x, y, z};
        const auto globalCoord = gpuCoord.cube2Global(gpucubeedge, Dataset::current().scaleFactor);
        if (currentlyVisible(globalCoord, state->viewerState->currentPosition, gpusupercube, Dataset::current().scaleFactor * gpucubeedge) && layer.textures.count(gpuCoord) == 0) {
            const auto cubeCoord = Dataset::current().global2cube(globalCoord);
            const auto offset = globalCoord - Dataset::current().cube2global(cubeCoord);
            layer.pendingOrthoCubes.emplace_back(gpuCoord, offset);
        }
    }
}

void Viewer::reslice_notify() {
    for (std::size_t layerId{0}; layerId < Dataset::datasets.size(); ++layerId) {
        reslice_notify(layerId);
    }
}

void Viewer::reslice_notify(const std::size_t layerId) {
    reslice_notify_all(layerId);
}

void Viewer::reslice_notify_all(const std::size_t layerId, boost::optional<CoordOfCube> cubeCoord) {
    if (layerId >= window->viewportArb->resliceNecessary.size()) {
        return;// loader may notify for layers that don’t exist anymore
    }
    window->forEachOrthoVPDo([this, layerId, cubeCoord](ViewportOrtho & vpOrtho) {
        if (!cubeCoord) {
            vpOrtho.resliceNecessary[layerId] = true;
        } else {
            QTimer::singleShot(0, this, [this, layerId, cubeCoord, &vpOrtho](){// some form of thread safety
                if (layerId >= window->viewportArb->resliceNecessary.size()) {
                    return;// loader may notify for layers that don’t exist anymore
                }
                vpOrtho.resliceNecessaryCubes[layerId].emplace(*cubeCoord);
            });
        }
    });
    window->viewportArb->resliceNecessary[layerId] = true;//arb visibility is not tested
    if (layerId == Segmentation::singleton().layerId) {
        // if anything has changed, update the volume texture data
        Segmentation::singleton().volume_update_required = true;
    }
}

void Viewer::segmentation_changed() {
    if (Segmentation::singleton().enabled) {
        reslice_notify(Segmentation::singleton().layerId);
    }
}

void Viewer::recalcTextureOffsets() {
    calcDisplayedEdgeLength();
    calcLeftUpperTexAbsPx();

    window->forEachOrthoVPDo([&](ViewportOrtho & orthoVP) {
        auto & texture = orthoVP.texture;
        float midX = texture.texUnitsPerDataPx;
        float midY = texture.texUnitsPerDataPx;
        float xFactor = 0.5 * texture.texUsedX;
        float yFactor = 0.5 * texture.texUsedY;
        const auto magCoordDiffF = floatCoordinate(state->viewerState->currentPosition - texture.leftUpperPxInAbsPx) / (Dataset::current().scaleFactor / Dataset::current().magnification);
        if (orthoVP.viewportType == VIEWPORT_XY) {
            midX *= magCoordDiffF.x;
            midY *= magCoordDiffF.y;
        } else if (orthoVP.viewportType == VIEWPORT_XZ) {
            midX *= magCoordDiffF.x;
            midY *= magCoordDiffF.z;
        } else if (orthoVP.viewportType == VIEWPORT_ZY) {
            midX *= magCoordDiffF.z;
            midY *= magCoordDiffF.y;
        } else {
            const auto texUsed = texture.usedSizeInCubePixels / texture.size;
            midX = 0.5 * texUsed;
            midY = 0.5 * texUsed;
        }
        // Calculate the vertices in texture coordinates
        // mid really means current pos inside the texture, in texture coordinates, relative to the texture origin 0., 0.
//        if (orthoVP.viewportType != VIEWPORT_ARBITRARY) {
            texture.texLUx = midX - xFactor;
            texture.texLUy = midY + yFactor;
            texture.texRUx = midX + xFactor;
            texture.texRUy = texture.texLUy;
            texture.texRLx = texture.texRUx;
            texture.texRLy = midY - yFactor;
            texture.texLLx = texture.texLUx;
            texture.texLLy = texture.texRLy;
//        } else {// arb should use the entirety of (a specific part) of the texture
//            const auto texUsed = 1;texture.fovPixel / texture.size * texture.FOV;
//            texture.texLUx = 0;
//            texture.texLUy = texUsed;
//            texture.texRUx = texUsed;
//            texture.texRUy = texUsed;
//            texture.texRLx = texUsed;
//            texture.texRLy = 0;
//            texture.texLLx = 0;
//            texture.texLLy = 0;
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
    for (std::size_t layerId{0}; layerId < Dataset::datasets.size(); ++layerId) {
        viewerState.layerRenderSettings[layerId].rangeDelta = state->viewerState->luminanceRangeDelta;
        viewerState.layerRenderSettings[layerId].bias = state->viewerState->luminanceBias;
    }
    reslice_notify();
    emit layerRenderSettingsChanged();
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

    for (auto && elem : viewportArb->resliceNecessary) {
        elem = true;
    }
    recalcTextureOffsets();
    viewportArb->sendCursorPosition();
}

void Viewer::resetRotation() {
    viewportArb->v1 = {1,  0,  0};
    viewportArb->v2 = {0, -1,  0};
    viewportArb->n  = {0,  0, -1};
    for (auto && elem : viewportArb->resliceNecessary) {
        elem = true;
    }
    recalcTextureOffsets();
    viewportArb->sendCursorPosition();
}

void Viewer::resizeTexEdgeLength(const int cubeEdge, const int superCubeEdge, const std::size_t layerCount) {
    int newTexEdgeLength = 512;
    while (newTexEdgeLength < cubeEdge * superCubeEdge) {
        newTexEdgeLength *= 2;
    }
    if (newTexEdgeLength != state->viewerState->texEdgeLength || layerCount != viewerState.layerRenderSettings.size()) {
        qDebug() << QString("cubeEdge = %1, sCubeEdge = %2, newTex = %3 (%4), size = %5 MiB")
                    .arg(cubeEdge).arg(superCubeEdge).arg(newTexEdgeLength).arg(state->viewerState->texEdgeLength)
                    .arg(layerCount * newTexEdgeLength * newTexEdgeLength * 4 * 2 * 3 / 1024. / 1024.).toStdString().c_str();
        viewerState.texEdgeLength = newTexEdgeLength;
        if (layerCount != viewerState.layerRenderSettings.size()) {
            viewerState.layerRenderSettings = decltype(viewerState.layerRenderSettings)(layerCount);
        }
        window->resetTextureProperties();
        window->forEachOrthoVPDo([layerCount](ViewportOrtho & vp) {
            vp.resetTexture(layerCount);
        });
        recalcTextureOffsets();
    }
    for (auto tup : boost::combine(viewerState.layerRenderSettings, Dataset::datasets)) {
        tup.get<0>().visible = tup.get<1>().loadingEnabled;// TODO multi layer
    }
}

void Viewer::loadNodeLUT(const QString & path) {
    state->viewerState->nodeColors = loadLookupTable(path);
}

void Viewer::loadTreeLUT(const QString & path) {
    state->viewerState->treeColors = loadLookupTable(path);
    Skeletonizer::singleton().updateTreeColors();
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
    viewerState.layerRenderSettings.at(index).visible = enabled;
    emit layerVisibilityChanged(index);
}

void Viewer::setMesh3dAlphaFactor(const float alpha) {
    state->viewerState->meshAlphaFactor3d = alpha;
    emit mesh3dAlphaFactorChanged(alpha);
}

void Viewer::setMeshSlicingAlphaFactor(const float alpha) {
    state->viewerState->meshAlphaFactorSlicing = alpha;
    emit meshSlicingAlphaFactorChanged(alpha);
}
