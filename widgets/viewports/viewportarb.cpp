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

#include "viewportarb.h"

#include "dataset.h"
#include "segmentation/cubeloader.h"
#include "stateInfo.h"
#include "viewer.h"

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
    const int width = (state->M - 1) * Dataset::current().cubeEdgeLength / std::sqrt(2);
    const int height = width;
    const auto begin = leftUpperPxInAbsPx_float;
    std::vector<char> texData(4 * std::pow(state->viewerState->texEdgeLength, 2));
    boost::multi_array_ref<uint8_t, 3> viewportView(reinterpret_cast<uint8_t *>(texData.data()), boost::extents[width][height][4]);
    for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
        const auto dataPos = static_cast<Coordinate>(begin + v1 * Dataset::current().magnification * x - v2 * Dataset::current().magnification * y);
        if (dataPos.x < 0 || dataPos.y < 0 || dataPos.z < 0) {
            viewportView[y][x][0] = viewportView[y][x][1] = viewportView[y][x][2] = viewportView[y][x][3] = 0;
        } else {
            const auto soid = readVoxel(dataPos);
            const auto color = Segmentation::singleton().colorObjectFromSubobjectId(soid);
            viewportView[y][x][0] = std::get<0>(color);
            viewportView[y][x][1] = std::get<1>(color);
            viewportView[y][x][2] = std::get<2>(color);
            viewportView[y][x][3] = std::get<3>(color);
        }
    }
    glBindTexture(GL_TEXTURE_2D, texture.overlayHandle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}
