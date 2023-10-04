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

#include "viewportarb.h"

#include "dataset.h"
#include "segmentation/cubeloader.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QOpenGLPixelTransferOptions>

ViewportArb::ViewportArb(QWidget *parent, ViewportType viewportType) : ViewportOrtho(parent, viewportType) {
    menuButton.menu()->addAction(&resetAction);
    connect(&resetAction, &QAction::triggered, []() {
        state->viewer->resetRotation();
    });
}

void ViewportArb::hideVP() {
    ViewportBase::hideVP();
    state->viewer->setEnableArbVP(false);
}

void ViewportArb::paintGL() {
    if (state->gpuSlicer && state->viewer->gpuRendering) {
        state->viewer->arbCubes(*this);
    } else if (Segmentation::singleton().enabled && state->viewerState->showOnlyRawData == false) {
        updateOverlayTexture();
    }
    ViewportOrtho::paintGL();
}

void ViewportArb::updateOverlayTexture() {
    if (!Segmentation::singleton().enabled || !resliceNecessary[Segmentation::singleton().layerId]) {
        return;
    }
    resliceNecessary[Segmentation::singleton().layerId] = false;
    const int width = (state->M - 1) * Dataset::current().cubeEdgeLength / std::sqrt(2);
    const int height = width;
    const auto begin = leftUpperPxInAbsPx_float;
    auto & texData = textures[Segmentation::singleton().layerId].texData;
    texData.resize(4 * width * height, 0);
    boost::multi_array_ref<uint8_t, 3> viewportView(texData.data(), boost::extents[width][height][4]);
    // cache
    auto subobjectIdCache = Segmentation::singleton().getBackgroundId();
    auto colorCache = Segmentation::singleton().colorObjectFromSubobjectId(subobjectIdCache);
    for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
        const auto dataPos = static_cast<Coordinate>(begin + Dataset::current().scaleFactor.componentMul(v1) * x - Dataset::current().scaleFactor.componentMul(v2) * y);
        if (dataPos.x < 0 || dataPos.y < 0 || dataPos.z < 0) {
            viewportView[y][x][0] = viewportView[y][x][1] = viewportView[y][x][2] = viewportView[y][x][3] = 0;
        } else {
            const auto soid = readVoxel(dataPos);
            const auto color = (subobjectIdCache == soid) ? colorCache : Segmentation::singleton().colorObjectFromSubobjectId(soid);
            subobjectIdCache = soid;
            colorCache = color;
            viewportView[y][x][0] = std::get<0>(color);
            viewportView[y][x][1] = std::get<1>(color);
            viewportView[y][x][2] = std::get<2>(color);
            viewportView[y][x][3] = std::get<3>(color);
        }
    }
    textures[Segmentation::singleton().layerId].texHandle.bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texData.data());
    textures[Segmentation::singleton().layerId].texHandle.release();
}
