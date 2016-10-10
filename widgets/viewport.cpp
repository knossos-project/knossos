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
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "viewport.h"

#include "functions.h"
#include "GuiConstants.h"
#include "profiler.h"
#include "scriptengine/scripting.h"
#include "segmentation/cubeloader.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QOpenGLFramebufferObject>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

bool ViewportBase::oglDebug = false;
bool Viewport3D::showBoundariesInUm = false;
bool ViewportOrtho::showNodeComments = false;

RenderOptions::RenderOptions() : drawBoundaryAxes(true), drawBoundaryBox(true), drawCrosshairs(state->viewerState->drawVPCrosshairs), drawOverlay(Segmentation::enabled && state->viewerState->showOverlay),
drawSkeleton(true), drawViewportPlanes(true), enableSkeletonDownsampling(true), enableTextScaling(false), highlightActiveNode(true), highlightSelection(true), selectionPass(SelectionPass::NoSelection), pointCloudPicking{false} {}

RenderOptions::RenderOptions(const bool drawBoundaryAxes, const bool drawBoundaryBox, const bool drawOverlay, const bool drawSkeleton, const bool drawViewportPlanes)
    : drawBoundaryAxes(drawBoundaryAxes), drawBoundaryBox(drawBoundaryBox), drawCrosshairs(false), drawOverlay(drawOverlay), drawSkeleton(drawSkeleton),
      drawViewportPlanes(drawViewportPlanes), enableSkeletonDownsampling(false), enableTextScaling(true), highlightActiveNode(false), highlightSelection(false), selectionPass(SelectionPass::NoSelection) {}

void ResizeButton::mouseMoveEvent(QMouseEvent * event) {
    emit vpResize(event->globalPos());
}

QViewportFloatWidget::QViewportFloatWidget(QWidget *parent, ViewportType vpType) : QWidget(parent) {
    setWindowFlags(Qt::Window);
    const std::array<const char * const, ViewportBase::numberViewports> VP_TITLES{{"XY", "XZ", "ZY", "Arbitrary", "3D"}};
    setWindowTitle(VP_TITLES[vpType]);
    new QVBoxLayout(this);
}

ViewportBase::ViewportBase(QWidget *parent, ViewportType viewportType) :
    QOpenGLWidget(parent), resizeButton(this), viewportType(viewportType), edgeLength(width()) {
    dockParent = parent;
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);

    QObject::connect(&snapshotAction, &QAction::triggered, [this]() { emit snapshotTriggered(this->viewportType); });
    QObject::connect(&floatingWindowAction, &QAction::triggered, [this]() { setDock(!isDocked); });
    menuButton.setText("…");
    menuButton.setCursor(Qt::ArrowCursor);
    menuButton.setMinimumSize(35, 20);
    menuButton.setMaximumSize(menuButton.minimumSize());
    menuButton.addAction(&snapshotAction);
    menuButton.addAction(&floatingWindowAction);
    menuButton.setPopupMode(QToolButton::InstantPopup);
    resizeButton.setCursor(Qt::SizeFDiagCursor);
    resizeButton.setIcon(QIcon(":/resources/icons/resize.gif"));
    resizeButton.setMinimumSize(20, 20);
    resizeButton.setMaximumSize(resizeButton.minimumSize());
    QObject::connect(&resizeButton, &ResizeButton::vpResize, [this](const QPoint & globalPos) {
        if (!isDocked) {
            // Floating viewports are resized indirectly by container window
            return;
        }
        raise();//we come from the resize button
        //»If you move the widget as a result of the mouse event, use the global position returned by globalPos() to avoid a shaking motion.«
        const auto desiredSize = mapFromGlobal(globalPos);
        sizeAdapt(desiredSize);
        state->viewerState->defaultVPSizeAndPos = false;
    });

    vpLayout.setMargin(0);//attach buttons to vp border
    vpLayout.addStretch(1);
    vpHeadLayout.setAlignment(Qt::AlignTop | Qt::AlignRight);
    vpHeadLayout.addWidget(&menuButton);
    vpHeadLayout.setSpacing(0);
    vpLayout.insertLayout(0, &vpHeadLayout);
    vpLayout.addWidget(&resizeButton, 0, Qt::AlignBottom | Qt::AlignRight);
    setLayout(&vpLayout);
}

ViewportBase::~ViewportBase() {
    if (oglDebug && oglLogger.isLogging()) {
        makeCurrent();
        oglLogger.stopLogging();
    }
}

void ViewportBase::setDock(bool isDock) {
    bool wasVisible = isVisible();
    isDocked = isDock;
    if (isDock) {
        setParent(dockParent);
        if (nullptr != floatParent) {
            delete floatParent;
            floatParent = nullptr;
        }
        move(dockPos);
        resize(dockSize);
        dockPos = {};
        dockSize = {};
        floatingWindowAction.setText("Undock viewport");
    } else {
        dockPos = pos();
        dockSize = size();
        floatParent = new QViewportFloatWidget(dockParent, viewportType);
        floatParent->layout()->addWidget(this);
        floatParent->resize(size());
        if (wasVisible) {
            floatParent->show();
        }
        state->viewerState->defaultVPSizeAndPos = false;
        floatingWindowAction.setText("Dock viewport");
    }
    if (wasVisible) {
        show();
        setFocus();
    }
}

void ViewportBase::moveVP(const QPoint & globalPos) {
    if (!isDocked) {
        // Moving viewports is relevant only when docked
        return;
    }
    //»If you move the widget as a result of the mouse event, use the global position returned by globalPos() to avoid a shaking motion.«
    const auto position = mapFromGlobal(globalPos);
    const auto desiredX = x() + position.x() - mouseDown.x();
    const auto desiredY = y() + position.y() - mouseDown.y();
    posAdapt({desiredX, desiredY});
    state->viewerState->defaultVPSizeAndPos = false;
}

ViewportOrtho::ViewportOrtho(QWidget *parent, ViewportType viewportType) : ViewportBase(parent, viewportType) {
    switch(viewportType) {
    case VIEWPORT_XZ:
        v1 = {1, 0, 0};
        v2 = {0, 0, 1};
        n = {0, 1, 0};
        break;
    case VIEWPORT_ZY:
        v1 = {0, 0, 1};
        v2 = {0, 1, 0};
        n = {1, 0, 0};
        break;
    case VIEWPORT_XY:
    case VIEWPORT_ARBITRARY:
        v1 = {1, 0, 0};
        v2 = {0, 1, 0};
        n = {0, 0, 1};
        break;
    default:
        throw std::runtime_error("ViewportOrtho::ViewportOrtho unknown vp");
    }
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
    const auto size = texture.edgeLengthPx;
    if (texture.texHandle != 0) {
        glBindTexture(GL_TEXTURE_2D, texture.texHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    }
    if (texture.overlayHandle != 0) {
        glBindTexture(GL_TEXTURE_2D, texture.overlayHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
}

Viewport3D::Viewport3D(QWidget *parent, ViewportType viewportType) : ViewportBase(parent, viewportType) {
    for (auto * button : {&xyButton, &xzButton, &zyButton}) {
        button->setMinimumSize(30, 20);
    }
    r90Button.setMinimumSize(35, 20);
    r180Button.setMinimumSize(40, 20);
    resetButton.setMinimumSize(45, 20);

    for (auto * button : {&resetButton, &r180Button, &r90Button, &zyButton, &xzButton, &xyButton}) {
        button->setMaximumSize(button->minimumSize());
        button->setCursor(Qt::ArrowCursor);
        vpHeadLayout.insertWidget(0, button);
    }

    connect(&xyButton, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_XY_VIEW;
        }
    });
    connect(&xzButton, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_XZ_VIEW;
        }
    });
    connect(&zyButton, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_ZY_VIEW;
        }
    });
    connect(&r90Button, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_R90;
        }
    });
    connect(&r180Button, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_R180;
        }
    });
    connect(&resetButton, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_RESET;
        }
    });
}

void ViewportBase::initializeGL() {
    static bool printed = false;
    if (!printed) {
        qDebug() << reinterpret_cast<const char*>(::glGetString(GL_VERSION))
                 << reinterpret_cast<const char*>(::glGetString(GL_VENDOR))
                 << reinterpret_cast<const char*>(::glGetString(GL_RENDERER));
        printed = true;
    }
    if (!initializeOpenGLFunctions()) {
        qDebug() << "initializeOpenGLFunctions failed";
    }
    QObject::connect(&oglLogger, &QOpenGLDebugLogger::messageLogged, [](const QOpenGLDebugMessage & msg){
        if (msg.type() == QOpenGLDebugMessage::ErrorType) {
            qWarning() << msg;
        } else {
            qDebug() << msg;
        }
    });
    if (oglDebug && oglLogger.initialize()) {
        oglLogger.startLogging(QOpenGLDebugLogger::SynchronousLogging);
    }

    // The following code configures openGL to draw into the current VP
    //set the drawing area in the window to our actually processed viewport.
    glViewport(upperLeftCorner.x, upperLeftCorner.y, edgeLength, edgeLength);
    //select the projection matrix
    glMatrixMode(GL_PROJECTION);
    //reset it
    glLoadIdentity();
    //define coordinate system for our viewport: left right bottom top near far
    //coordinate values
    glOrtho(0, edgeLength, edgeLength, 0, 25, -25);
    //select the modelview matrix for modification
    glMatrixMode(GL_MODELVIEW);
    //reset it
    glLoadIdentity();
    //glBlendFunc(GL_ONE_MINUS_DST_ALPHA,GL_DST_ALPHA);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    /* performance tricks from mesa3d  */

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glShadeModel(GL_FLAT);
    glDisable(GL_DITHER);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_COLOR_MATERIAL);
}

void ViewportOrtho::initializeGL() {
    ViewportBase::initializeGL();
    glGenTextures(1, &texture.texHandle);

    glBindTexture(GL_TEXTURE_2D, texture.texHandle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture.textureFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.textureFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // loads an empty texture into video memory - during user movement, this
    // texture is updated via glTexSubImage2D in vpGenerateTexture
    // We need GL_RGB as texture internal format to color the textures

    std::vector<char> texData(4 * std::pow(state->viewerState->texEdgeLength, 2));
    glTexImage2D(GL_TEXTURE_2D,
                0,
                GL_RGB,
                texture.edgeLengthPx,
                texture.edgeLengthPx,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                texData.data());

    createOverlayTextures();

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
        // glEnable(GL_DEPTH_TEST);
        // glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

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

void ViewportOrtho::createOverlayTextures() {
    glGenTextures(1, &texture.overlayHandle);

    glBindTexture(GL_TEXTURE_2D, texture.overlayHandle);

    //Set the parameters for the texture.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    const auto size = texture.edgeLengthPx;
    std::vector<char> texData(4 * std::pow(state->viewerState->texEdgeLength, 2));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData.data());
}

void ViewportBase::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat x = (GLfloat)width() / height();

    glFrustum(-x, +x, -1.0, + 1.0, 0.1, 10.0);
    glMatrixMode(GL_MODELVIEW);

    upperLeftCorner = {geometry().topLeft().x(), geometry().topLeft().y(), 0};
    edgeLength = width();
    state->viewer->recalcTextureOffsets();
}

void Viewport3D::paintGL() {
    glClear(GL_DEPTH_BUFFER_BIT);
    renderViewport();
    renderViewportFrontFace();
}

void ViewportOrtho::paintGL() {
    glClear(GL_DEPTH_BUFFER_BIT);
    if (state->gpuSlicer && state->viewer->gpuRendering) {
        renderViewportFast();
    } else {
        state->viewer->vpGenerateTexture(*this);
        renderViewport();
    }
    renderViewportFrontFace();
}

void ViewportBase::enterEvent(QEvent *) {
    hasCursor = true;
}

void ViewportBase::leaveEvent(QEvent *) {
    hasCursor = false;
    emit cursorPositionChanged(Coordinate(), VIEWPORT_UNDEFINED);
}

void ViewportBase::keyPressEvent(QKeyEvent *event) {
    const auto ctrl = event->modifiers().testFlag(Qt::ControlModifier);
    const auto alt = event->modifiers().testFlag(Qt::AltModifier);
    if (ctrl && alt) {
        setCursor(Qt::OpenHandCursor);
    } else if (ctrl) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }
    handleKeyPress(event);
}

void ViewportBase::mouseMoveEvent(QMouseEvent *event) {
    const auto mouseBtn = event->buttons();
    const auto penmode = state->viewerState->penmode;

    if((!penmode && mouseBtn == Qt::LeftButton) || (penmode && mouseBtn == Qt::RightButton)) {
        Qt::KeyboardModifiers modifiers = event->modifiers();
        bool ctrl = modifiers.testFlag(Qt::ControlModifier);
        bool alt = modifiers.testFlag(Qt::AltModifier);

        if(ctrl && alt) { // drag viewport around
            moveVP(event->globalPos());
        } else {
            handleMouseMotionLeftHold(event);
        }
    } else if(mouseBtn == Qt::MidButton) {
        handleMouseMotionMiddleHold(event);
    } else if( (!penmode && mouseBtn == Qt::RightButton) || (penmode && mouseBtn == Qt::LeftButton)) {
        handleMouseMotionRightHold(event);
    }
    handleMouseHover(event);

    prevMouseMove = event->pos();
}

void ViewportBase::mousePressEvent(QMouseEvent *event) {
    raise(); //bring this viewport to front
    mouseDown = event->pos();
    auto penmode = state->viewerState->penmode;
    if((penmode && event->button() == Qt::RightButton) || (!penmode && event->button() == Qt::LeftButton)) {
        Qt::KeyboardModifiers modifiers = event->modifiers();
        const auto ctrl = modifiers.testFlag(Qt::ControlModifier);
        const auto alt = modifiers.testFlag(Qt::AltModifier);

        if(ctrl && alt) { // user wants to drag vp
            setCursor(Qt::ClosedHandCursor);
        } else {
            handleMouseButtonLeft(event);
        }
    } else if(event->button() == Qt::MiddleButton) {
        handleMouseButtonMiddle(event);
    } else if((penmode && event->button() == Qt::LeftButton) || (!penmode && event->button() == Qt::RightButton)) {
        handleMouseButtonRight(event);
    }
}

void ViewportBase::mouseReleaseEvent(QMouseEvent *event) {
    EmitOnCtorDtor eocd(&SignalRelay::Signal_Viewort_mouseReleaseEvent, state->signalRelay, this, event);

    Qt::KeyboardModifiers modifiers = event->modifiers();
    const auto ctrl = modifiers.testFlag(Qt::ControlModifier);
    const auto alt = modifiers.testFlag(Qt::AltModifier);

    if (ctrl && alt) {
        setCursor(Qt::OpenHandCursor);
    } else if (ctrl) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }

    if(event->button() == Qt::LeftButton) {
        handleMouseReleaseLeft(event);
    }
    if(event->button() == Qt::RightButton) {
        handleMouseReleaseRight(event);
    }
    if(event->button() == Qt::MiddleButton) {
        handleMouseReleaseMiddle(event);
    }
}

void ViewportBase::keyReleaseEvent(QKeyEvent *event) {
    Qt::KeyboardModifiers modifiers = event->modifiers();
    const auto ctrl = modifiers.testFlag(Qt::ControlModifier);
    const auto alt = modifiers.testFlag(Qt::AltModifier);

    if (ctrl && alt) {
        setCursor(Qt::OpenHandCursor);
    } else if (ctrl) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }

    if (event->key() == Qt::Key_Shift) {
        state->viewerState->repeatDirection /= 10;// decrease movement speed
        Segmentation::singleton().brush.setInverse(false);
    } else if (!event->isAutoRepeat()) {// don’t let shift break keyrepeat
        state->viewer->userMoveRound();
        state->viewerState->keyRepeat = false;
        state->viewerState->notFirstKeyRepeat = false;
    }
    handleKeyRelease(event);
}

void ViewportOrtho::zoom(const float step) {
    state->viewer->zoom(step);
}

float Viewport3D::zoomStep() const {
    return (0.1* (0.5 - state->skeletonState->zoomLevel));
}

void Viewport3D::zoom(const float step) {
    auto & zoomLvl = state->skeletonState->zoomLevel;
    zoomLvl = std::min(std::max(zoomLvl + step, SKELZOOMMIN), SKELZOOMMAX);
    emit updateDatasetOptionsWidget();
}

void Viewport3D::showHideButtons(bool isShow) {
    ViewportBase::showHideButtons(isShow);
    xyButton.setVisible(isShow);
    xzButton.setVisible(isShow);
    zyButton.setVisible(isShow);
    r90Button.setVisible(isShow);
    r180Button.setVisible(isShow);
    resetButton.setVisible(isShow);
}

void Viewport3D::updateVolumeTexture() {
    auto& seg = Segmentation::singleton();
    int texLen = seg.volume_tex_len;
    if(seg.volume_tex_id == 0) {
        glGenTextures(1, &seg.volume_tex_id);
        glBindTexture(GL_TEXTURE_3D, seg.volume_tex_id);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, texLen, texLen, texLen, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    static Profiler tex_gen_profiler;
    static Profiler dcfetch_profiler;
    static Profiler colorfetch_profiler;
    static Profiler occlusion_profiler;
    static Profiler tex_transfer_profiler;

    tex_gen_profiler.start(); // ----------------------------------------------------------- profiling
    auto currentPosDc = state->viewerState->currentPosition / state->magnification / state->cubeEdgeLength;
    int cubeLen = state->cubeEdgeLength;
    int M = state->M;
    int M_radius = (M - 1) / 2;
    GLubyte* colcube = new GLubyte[4*texLen*texLen*texLen];
    std::tuple<uint64_t, std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>> lastIdColor;

    state->protectCube2Pointer.lock();

    dcfetch_profiler.start(); // ----------------------------------------------------------- profiling
    uint64_t** rawcubes = new uint64_t*[M*M*M];
    for(int z = 0; z < M; ++z)
    for(int y = 0; y < M; ++y)
    for(int x = 0; x < M; ++x) {
        auto cubeIndex = z*M*M + y*M + x;
        Coordinate cubeCoordRelative{x - M_radius, y - M_radius, z - M_radius};
        rawcubes[cubeIndex] = reinterpret_cast<uint64_t*>(
            Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[int_log(state->magnification)],
            {currentPosDc.x + cubeCoordRelative.x, currentPosDc.y + cubeCoordRelative.y, currentPosDc.z + cubeCoordRelative.z}));
    }
    dcfetch_profiler.end(); // ----------------------------------------------------------- profiling

    colorfetch_profiler.start(); // ----------------------------------------------------------- profiling

    for(int z = 0; z < texLen; ++z)
    for(int y = 0; y < texLen; ++y)
    for(int x = 0; x < texLen; ++x) {
        Coordinate DcCoord{(x * M)/cubeLen, (y * M)/cubeLen, (z * M)/cubeLen};
        auto cubeIndex = DcCoord.z*M*M + DcCoord.y*M + DcCoord.x;
        auto& rawcube = rawcubes[cubeIndex];

        if(rawcube != nullptr) {
            auto indexInDc  = ((z * M)%cubeLen)*cubeLen*cubeLen + ((y * M)%cubeLen)*cubeLen + (x * M)%cubeLen;
            auto indexInTex = z*texLen*texLen + y*texLen + x;
            auto subobjectId = rawcube[indexInDc];
            if(subobjectId == std::get<0>(lastIdColor)) {
                auto idColor = std::get<1>(lastIdColor);
                colcube[4*indexInTex+0] = std::get<0>(idColor);
                colcube[4*indexInTex+1] = std::get<1>(idColor);
                colcube[4*indexInTex+2] = std::get<2>(idColor);
                colcube[4*indexInTex+3] = std::get<3>(idColor);
            } else if (seg.isSubObjectIdSelected(subobjectId)) {
                auto idColor = seg.colorObjectFromSubobjectId(subobjectId);
                std::get<3>(idColor) = 255; // ignore color alpha
                colcube[4*indexInTex+0] = std::get<0>(idColor);
                colcube[4*indexInTex+1] = std::get<1>(idColor);
                colcube[4*indexInTex+2] = std::get<2>(idColor);
                colcube[4*indexInTex+3] = std::get<3>(idColor);
                lastIdColor = std::make_tuple(subobjectId, idColor);
            } else {
                colcube[4*indexInTex+0] = 0;
                colcube[4*indexInTex+1] = 0;
                colcube[4*indexInTex+2] = 0;
                colcube[4*indexInTex+3] = 0;
            }
        } else {
            auto indexInTex = z*texLen*texLen + y*texLen + x;
            colcube[4*indexInTex+0] = 0;
            colcube[4*indexInTex+1] = 0;
            colcube[4*indexInTex+2] = 0;
            colcube[4*indexInTex+3] = 0;
        }
    }

    delete[] rawcubes;

    state->protectCube2Pointer.unlock();

    colorfetch_profiler.end(); // ----------------------------------------------------------- profiling

    occlusion_profiler.start(); // ----------------------------------------------------------- profiling
    for(int z = 1; z < texLen - 1; ++z)
    for(int y = 1; y < texLen - 1; ++y)
    for(int x = 1; x < texLen - 1; ++x) {
        auto indexInTex = (z)*texLen*texLen + (y)*texLen + x;
        if(colcube[4*indexInTex+3] != 0) {
            for(int xi = -1; xi <= 1; ++xi) {
                auto othrIndexInTex = (z)*texLen*texLen + (y)*texLen + x+xi;
                if((xi != 0) && colcube[4*othrIndexInTex+3] != 0) {
                    colcube[4*indexInTex+0] *= 0.95f;
                    colcube[4*indexInTex+1] *= 0.95f;
                    colcube[4*indexInTex+2] *= 0.95f;
                }
            }
        }
    }

    for(int z = 1; z < texLen - 1; ++z)
    for(int y = 1; y < texLen - 1; ++y)
    for(int x = 1; x < texLen - 1; ++x) {
        auto indexInTex = (z)*texLen*texLen + (y)*texLen + x;
        if(colcube[4*indexInTex+3] != 0) {
            for(int yi = -1; yi <= 1; ++yi) {
                auto othrIndexInTex = (z)*texLen*texLen + (y+yi)*texLen + x;
                if((yi != 0) && colcube[4*othrIndexInTex+3] != 0) {
                    colcube[4*indexInTex+0] *= 0.95f;
                    colcube[4*indexInTex+1] *= 0.95f;
                    colcube[4*indexInTex+2] *= 0.95f;
                }
            }
        }
    }

    for(int z = 1; z < texLen - 1; ++z)
    for(int y = 1; y < texLen - 1; ++y)
    for(int x = 1; x < texLen - 1; ++x) {
        auto indexInTex = (z)*texLen*texLen + (y)*texLen + x;
        if(colcube[4*indexInTex+3] != 0) {
            for(int zi = -1; zi <= 1; ++zi) {
                auto othrIndexInTex = (z+zi)*texLen*texLen + (y)*texLen + x;
                if((zi != 0) && colcube[4*othrIndexInTex+3] != 0) {
                    colcube[4*indexInTex+0] *= 0.95f;
                    colcube[4*indexInTex+1] *= 0.95f;
                    colcube[4*indexInTex+2] *= 0.95f;
                }
            }
        }
    }
    occlusion_profiler.end(); // ----------------------------------------------------------- profiling

    tex_transfer_profiler.start(); // ----------------------------------------------------------- profiling
    glBindTexture(GL_TEXTURE_3D, seg.volume_tex_id);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, texLen, texLen, texLen, 0, GL_RGBA, GL_UNSIGNED_BYTE, colcube);
    delete[] colcube;
    tex_transfer_profiler.end(); // ----------------------------------------------------------- profiling

    tex_gen_profiler.end(); // ----------------------------------------------------------- profiling

    // --------------------- display some profiling information ------------------------
    // qDebug() << "tex gen avg time: " << tex_gen_profiler.average_time()*1000 << "ms";
    // qDebug() << "    dc fetch    : " << dcfetch_profiler.average_time()*1000 << "ms";
    // qDebug() << "    color fetch : " << colorfetch_profiler.average_time()*1000 << "ms";
    // qDebug() << "    occlusion   : " << occlusion_profiler.average_time()*1000 << "ms";
    // qDebug() << "    tex transfer: " << tex_transfer_profiler.average_time()*1000 << "ms";
    // qDebug() << "---------------------------------------------";
}

void Viewport3D::addTreePointcloud(int tree_id, QVector<float> & verts, QVector<float> & normals, QVector<unsigned int> & indices, const QVector<float> & color, int draw_mode) {
    // temporary, color information might be switched to per-object rather than per-vertex
    auto col = color;
    if(col.size() == 3) {
        col.append(1.0f);
    }

    std::vector<std::array<GLfloat, 4>> colors;
    for(int i = 0; i < verts.size(); ++i) {
        colors.push_back({{col[0], col[1], col[2], col[3]}});
        // tmp? scale vertices down by dataset scale
        verts[i] /= (i%3==0)?state->scale.x:(i%3==1)?state->scale.y:state->scale.z;
    }

    std::vector<int> vertex_face_count(verts.size() / 3);
    for(int i = 0; i < indices.size(); ++i) {
        ++vertex_face_count[indices[i]];
        // check index validity (can be removed if causing performance issues)
        if(indices[i] > verts.size()) {
            qDebug() << "index wrong: " << indices[i] << "(should be less than " << verts.size() << ")";
        }
    }

    // generate normals of indexed vertices
    if(normals.empty() && !indices.empty()) {
        normals.resize(verts.size());
        for(int i = 0; i < indices.size()-2; i += 3) {
            QVector3D p1{verts[indices[i]*3]  , verts[indices[i]*3+1]  , verts[indices[i]*3+2]};
            QVector3D p2{verts[indices[i+1]*3], verts[indices[i+1]*3+1], verts[indices[i+1]*3+2]};
            QVector3D p3{verts[indices[i+2]*3], verts[indices[i+2]*3+1], verts[indices[i+2]*3+2]};
            QVector3D e1{p2 - p1};
            QVector3D e2{p3 - p1};

            QVector3D normal{QVector3D::normal(e1, e2)};

            for(int j = 0; j < 3; ++j) {
                normals[indices[i+j]*3] += normal.x() / vertex_face_count[indices[i+j]];
            }
            for(int j = 0; j < 3; ++j) {
                normals[indices[i+j]*3+1] += normal.y() / vertex_face_count[indices[i+j]];
            }
            for(int j = 0; j < 3; ++j) {
                normals[indices[i+j]*3+2] += normal.z() / vertex_face_count[indices[i+j]];
            }
        }
    }

    PointcloudBuffer buf{static_cast<GLenum>(draw_mode)};
    buf.vertex_count = verts.size() / 3;
    buf.index_count = indices.size();
    buf.position_buf.bind();
    buf.position_buf.allocate(verts.data(), verts.size() * sizeof(GLfloat));
    buf.normal_buf.bind();
    buf.normal_buf.allocate(normals.data(), normals.size() * sizeof(GLfloat));
    buf.color_buf.bind();
    buf.color_buf.allocate(colors.data(), colors.size() * 4 * sizeof(GLfloat));
    buf.index_buf.bind();
    buf.index_buf.allocate(indices.data(), indices.size() * sizeof(GLuint));
    buf.index_buf.release();
    buf.vertex_coords = verts;
    buf.indices = indices;
    pointcloudBuffers.emplace(tree_id, buf);
}

void Viewport3D::deleteTreePointcloud(int tree_id) {
    pointcloudBuffers.erase(tree_id);
}

void ViewportBase::takeSnapshot(const QString & path, const int size, const bool withAxes, const bool withBox, const bool withOverlay, const bool withSkeleton, const bool withScale, const bool withVpPlanes) {
    makeCurrent();
    glPushAttrib(GL_VIEWPORT_BIT); // remember viewport setting
    glViewport(0, 0, size, size);
    QOpenGLFramebufferObject fbo(size, size, QOpenGLFramebufferObject::CombinedDepthStencil);
    const RenderOptions options(withAxes, withBox, withOverlay, withSkeleton, withVpPlanes);
    fbo.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Qt does not clear it?
    renderViewport(options);
    if(withScale) {
        setFrontFacePerspective();
        renderScaleBar();
    }
    QImage fboImage(fbo.toImage());
    QImage image(fboImage.constBits(), fboImage.width(), fboImage.height(), QImage::Format_RGB32);
    image.save(path);
    glPopAttrib(); // restore viewport setting
    fbo.release();
}

void ViewportOrtho::sendCursorPosition() {
    if (hasCursor) {
        const auto cursorPos = mapFromGlobal(QCursor::pos());
        emit cursorPositionChanged(getCoordinateFromOrthogonalClick(cursorPos.x(), cursorPos.y(), *this), viewportType);
    }
}

float ViewportOrtho::displayedEdgeLenghtXForZoomFactor(const float zoomFactor) const {
    float FOVinDCs = ((float)state->M) - 1.f;
    float result = FOVinDCs * state->cubeEdgeLength / static_cast<float>(texture.edgeLengthPx);
    return (std::floor((result * zoomFactor) / 2. / texture.texUnitsPerDataPx) * texture.texUnitsPerDataPx)*2;
}

float ViewportArb::displayedEdgeLenghtXForZoomFactor(const float zoomFactor) const {
    float result = vpLenghtInDataPx / static_cast<float>(texture.edgeLengthPx);
    return (std::floor((result * zoomFactor) / 2. / texture.texUnitsPerDataPx) * texture.texUnitsPerDataPx)*2;
}


ViewportArb::ViewportArb(QWidget *parent, ViewportType viewportType) : ViewportOrtho(parent, viewportType), vpLenghtInDataPx((static_cast<int>((state->M / 2 + 1) * state->cubeEdgeLength / std::sqrt(2))  / 2) * 2), vpHeightInDataPx(vpLenghtInDataPx) {
    menuButton.addAction(&resetAction);
    connect(&resetAction, &QAction::triggered, [this]() {
        state->viewer->resetRotation();
    });
}

void ViewportArb::paintGL() {
    glClear(GL_DEPTH_BUFFER_BIT);
    if (state->gpuSlicer && state->viewer->gpuRendering) {
        state->viewer->arbCubes(*this);
    } else if (Segmentation::enabled && state->viewerState->showOverlay) {
        updateOverlayTexture();
    }
    ViewportOrtho::paintGL();
}

void ViewportArb::showHideButtons(bool isShow) {
    ViewportBase::showHideButtons(isShow);
}

void ViewportArb::updateOverlayTexture() {
    if (!ocResliceNecessary) {
        return;
    }
    ocResliceNecessary = false;
    const int width = (state->M - 1) * state->cubeEdgeLength / std::sqrt(2);
    const int height = width;
    const auto begin = leftUpperPxInAbsPx_float;
    std::vector<char> texData(4 * std::pow(state->viewerState->texEdgeLength, 2));
    boost::multi_array_ref<uint8_t, 3> viewportView(reinterpret_cast<uint8_t *>(texData.data()), boost::extents[width][height][4]);
    for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
        const auto dataPos = static_cast<Coordinate>(begin + v1 * state->magnification * x + v2 * state->magnification * y);
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
