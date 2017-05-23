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

#include "viewportortho.h"

#include "stateInfo.h"
#include "viewer.h"

bool ViewportOrtho::showNodeComments = false;

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
    if (texture.texHandle != 0) {
        glDeleteTextures(1, &texture.texHandle);
    }
    if (texture.overlayHandle != 0) {
        glDeleteTextures(1, &texture.overlayHandle);
    }
    texture.texHandle = texture.overlayHandle = 0;
}

void ViewportOrtho::initializeGL() {
    ViewportBase::initializeGL();
    // data texture
    glGenTextures(1, &texture.texHandle);

    glBindTexture(GL_TEXTURE_2D, texture.texHandle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture.textureFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.textureFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // overlay texture
    glGenTextures(1, &texture.overlayHandle);

    glBindTexture(GL_TEXTURE_2D, texture.overlayHandle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    resetTexture();// allocates textures

    if (state->gpuSlicer) {
        if (viewportType == ViewportType::VIEWPORT_XY) {
//            state->viewer->gpucubeedge = 128;
            state->viewer->layers.emplace_back(*context());
            state->viewer->layers.back().createBogusCube(state->cubeEdgeLength, state->viewer->gpucubeedge);
            state->viewer->layers.emplace_back(*context());
//            state->viewer->layers.back().enabled = false;
            state->viewer->layers.back().isOverlayData = true;
            state->viewer->layers.back().createBogusCube(state->cubeEdgeLength, state->viewer->gpucubeedge);
        }

        glEnable(GL_TEXTURE_3D);

        auto vertex_shader_code = R"shaderSource(
        #version 110
        uniform mat4 model_matrix;
        uniform mat4 view_matrix;
        uniform mat4 projection_matrix;
        attribute vec3 vertex;
        attribute vec3 texCoordVertex;
        varying vec3 texCoordFrag;
        void main() {
            mat4 mvp_mat = projection_matrix * view_matrix * model_matrix;
            gl_Position = mvp_mat * vec4(vertex, 1.0);
            texCoordFrag = texCoordVertex;
        })shaderSource";

        raw_data_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_code);
        raw_data_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, R"shaderSource(
        #version 110
        uniform float textureOpacity;
        uniform sampler3D texture;
        varying vec3 texCoordFrag;//in
        //varying vec4 gl_FragColor;//out
        void main() {
            gl_FragColor = vec4(vec3(texture3D(texture, texCoordFrag).r), textureOpacity);
        })shaderSource");

        raw_data_shader.link();

        overlay_data_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_code);
        overlay_data_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, R"shaderSource(
        #version 110
        uniform float textureOpacity;
        uniform sampler3D indexTexture;
        uniform sampler1D textureLUT;
        uniform float lutSize;//float(textureSize1D(textureLUT, 0));
        uniform float factor;//expand float to uint8 range
        varying vec3 texCoordFrag;//in
        void main() {
            float index = texture3D(indexTexture, texCoordFrag).r;
            index *= factor;
            gl_FragColor = texture1D(textureLUT, (index + 0.5) / lutSize);
            gl_FragColor.a = textureOpacity;
        })shaderSource");

        overlay_data_shader.link();

        glDisable(GL_TEXTURE_3D);
    }
}

void ViewportOrtho::paintGL() {
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (state->gpuSlicer && state->viewer->gpuRendering) {
        renderViewportFast();
    } else {
        state->viewer->vpGenerateTexture(*this);
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

void ViewportOrtho::resetTexture() {
    if (texture.texHandle != 0) {
        glBindTexture(GL_TEXTURE_2D, texture.texHandle);
        std::vector<char> texData(static_cast<std::size_t>(3 * std::pow(texture.size, 2)));// RGB
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.size, texture.size, 0, GL_RGB, GL_UNSIGNED_BYTE, texData.data());
    }
    if (texture.overlayHandle != 0) {
        glBindTexture(GL_TEXTURE_2D, texture.overlayHandle);
        std::vector<char> texData(static_cast<std::size_t>(4 * std::pow(texture.size, 2)));// RGBA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.size, texture.size, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData.data());
    }
}

void ViewportOrtho::sendCursorPosition() {
    if (hasCursor) {
        emit cursorPositionChanged(getCoordinateFromOrthogonalClick(mapFromGlobal(QCursor::pos()), *this), viewportType);
    }
}

float ViewportOrtho::displayedEdgeLenghtXForZoomFactor(const float zoomFactor) const {
    float FOVinDCs = ((float)state->M) - 1.f;
    float result = FOVinDCs * state->cubeEdgeLength / static_cast<float>(texture.size);
    return (std::floor((result * zoomFactor) / 2. / texture.texUnitsPerDataPx) * texture.texUnitsPerDataPx)*2;
}

void ViewportOrtho::zoom(const float step) {
    state->viewer->zoom(step);
}
