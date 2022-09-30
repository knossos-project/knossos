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

float ViewportArb::displayedEdgeLenghtXForZoomFactor(const float zoomFactor) const {
    float result = displayedIsoPx / static_cast<float>(texture.size);
    return (std::floor((result * zoomFactor) / 2. / texture.texUnitsPerDataPx) * texture.texUnitsPerDataPx)*2;
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
    const int width = (state->M - 1) * std::max({Dataset::current().cubeEdgeLength.x, Dataset::current().cubeEdgeLength.y, Dataset::current().cubeEdgeLength.z}) / std::sqrt(2);
    const int height = width;
    const auto begin = leftUpperPxInAbsPx_float;
    auto & texData = texture.texData[Segmentation::singleton().layerId];
    texData.resize(4 * width * height, 0);
    boost::multi_array_ref<uint8_t, 3> viewportView(texData.data(), boost::extents[width][height][4]);
    // cache
    auto subobjectIdCache = Segmentation::singleton().getBackgroundId();
    auto colorCache = Segmentation::singleton().colorObjectFromSubobjectId(subobjectIdCache);
    for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
        const auto dataPos = static_cast<Coordinate>(begin + v1 * Dataset::current().magnification * x - v2 * Dataset::current().magnification * y);
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
    texture.texHandle[Segmentation::singleton().layerId].bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texData.data());
    texture.texHandle[Segmentation::singleton().layerId].release();
}
