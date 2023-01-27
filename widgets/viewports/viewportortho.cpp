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

#include "viewportortho.h"

#include "dataset.h"
#include "stateInfo.h"
#include "viewer.h"

ViewportOrtho::ViewportOrtho(QWidget *parent, ViewportType viewportType) : ViewportBase(parent, viewportType) {
    // v2 is negative because it goes from top to bottom on screen
    // n is positive if we want to look towards 0 and negative to look towards infinity
    switch(viewportType) {
    case VIEWPORT_XZ:
        v1 = {1, 0,  0};
        v2 = {0, 0, -1};
        n  = {0, 1,  0};
        break;
    case VIEWPORT_ZY:
        v1 = {0,  0, 1};
        v2 = {0, -1, 0};
        n  = {1,  0, 0};
        break;
    case VIEWPORT_XY:
    case VIEWPORT_ARBITRARY:
        v1 = {1,  0,  0};
        v2 = {0, -1,  0};
        n  = {0,  0, -1};
        break;
    default:
        throw std::runtime_error("ViewportOrtho::ViewportOrtho unknown vp");
    }

    zoomResetAction.setShortcut(QKeySequence("Ctrl+0"));
    zoomResetAction.setShortcutContext(Qt::WidgetShortcut);
    QObject::connect(&zoomResetAction, &QAction::triggered, state->viewer, &Viewer::zoomReset);
    menuButton.menu()->insertAction(zoomEndSeparator, &zoomResetAction);
    addAction(&zoomResetAction);
}

ViewportOrtho::~ViewportOrtho() {
    makeCurrent();
    for (auto & elem : texture.texHandle) {
        elem.destroy();
    }
}

void ViewportOrtho::initializeGL() {
    ViewportBase::initializeGL();
    resetTexture(Dataset::datasets.size());
}

void ViewportOrtho::paintGL() {
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (state->gpuSlicer && state->viewer->gpuRendering) {
        renderViewportFast();
    } else {
        renderViewport();
    }
    renderViewportFrontFace();
}

void ViewportOrtho::mouseMoveEvent(QMouseEvent *event) {
    ViewportBase::mouseMoveEvent(event);
    Segmentation::singleton().brush.setView(static_cast<brush_t::view_t>(viewportType), v1, v2, n);
}

void ViewportOrtho::mousePressEvent(QMouseEvent *event) {
    Segmentation::singleton().brush.setView(static_cast<brush_t::view_t>(viewportType), v1, v2, n);
    ViewportBase::mousePressEvent(event);
}

void ViewportOrtho::resetTexture(const std::size_t layerCount) {
    resliceNecessary = decltype(resliceNecessary)(layerCount);
    resliceNecessaryCubes = decltype(resliceNecessaryCubes)(layerCount);
    for (auto && elem : resliceNecessary) {
        elem = true;// can’t use vector init ctor for atomics
    }
    const bool changedLayerCount{layerCount != texture.texHandle.size()};
    const bool changedTextureSize{!texture.texHandle.empty() && texture.size != texture.texHandle.front().width()};
    makeCurrent();
    if (context() != nullptr && (changedLayerCount || changedTextureSize)) {
        texture.texHandle = decltype(texture.texHandle)(layerCount);
        texture.texData = decltype(texture.texData)(layerCount);
        for (std::size_t i{0}; i < layerCount; ++i) {
            auto & elem = texture.texHandle[i];
            auto & texData = texture.texData[i];
            elem.destroy();
            elem.setSize(texture.size, texture.size);
            elem.setFormat(QOpenGLTexture::RGBA8_UNorm);
            elem.setWrapMode(QOpenGLTexture::ClampToBorder);
            QColor borderColor{0, 0, 255, 51};
            elem.setBorderColor(borderColor);
            elem.allocateStorage();
            texData.resize(4 * std::pow(texture.size, 2));
            for (std::size_t i = 3; i < texData.size(); i += 4) {
                texData[i-3] = borderColor.red();
                texData[i-2] = borderColor.green();
                texData[i-1] = borderColor.blue();
                texData[i] = borderColor.alpha();
            }
            const auto & cdata = texData;
            elem.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, cdata.data());
            elem.release();
        }
        if (state->gpuSlicer && viewportType == ViewportType::VIEWPORT_XY) {
            state->viewer->layers.clear();
            for (const auto & dset : Dataset::datasets) {
                state->viewer->layers.emplace_back(*context());
                state->viewer->layers.back().enabled = dset.loadingEnabled;
                state->viewer->layers.back().isOverlayData = dset.isOverlay();
                state->viewer->layers.back().createBogusCube(dset.cubeShape, dset.gpuCubeShape);
                makeCurrent();// reset the surface
            }
        }
        applyTextureFilter();
    }
}

void ViewportOrtho::applyTextureFilter() {
    for (std::size_t layerId{0}; layerId < texture.texHandle.size(); ++layerId) {
        setTextureFilter(layerId, Dataset::datasets[layerId].renderSettings.textureFilter);
    }
}

void ViewportOrtho::setTextureFilter(std::size_t layerId, const QOpenGLTexture::Filter textureFilter) {
    auto & elem = texture.texHandle[layerId];
    if (elem.isCreated()) {
        elem.setMinMagFilters(textureFilter, textureFilter);
    }
}

void ViewportOrtho::sendCursorPosition() {
    if (hasCursor) {
        emit cursorPositionChanged(getCoordinateFromOrthogonalClick(mapFromGlobal(QCursor::pos()), *this), viewportType);
    }
}

float ViewportOrtho::displayedEdgeLenghtXForZoomFactor(const float zoomFactor) const {
    float FOVinDCs = ((float)state->M) - 1.f;
    float result = FOVinDCs * Dataset::current().cubeShape.x / static_cast<float>(texture.size);
    return (std::floor((result * zoomFactor) / 2. / texture.texUnitsPerDataPx) * texture.texUnitsPerDataPx)*2;
}

void ViewportOrtho::zoom(const float step) {
    state->viewer->zoom(step);
}

void ViewportOrtho::takeSnapshotDatasetSize(SnapshotOptions o) {
    o.size = width() / screenPxXPerDataPx / Dataset::current().scaleFactor.componentMul(v1).length();
    takeSnapshot(o);
}
