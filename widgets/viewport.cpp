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
bool ViewportOrtho::arbitraryOrientation = false;
bool ViewportOrtho::showNodeComments = false;

void ResizeButton::mouseMoveEvent(QMouseEvent * event) {
    emit vpResize(event->globalPos());
}

QViewportFloatWidget::QViewportFloatWidget(QWidget *parent, int id) : QWidget(parent) {
    setWindowFlags(Qt::Window);
    const std::array<const char * const, ViewportBase::numberViewports> VP_TITLES{{"XY", "XZ", "ZY", "3D"}};
    setWindowTitle(VP_TITLES[id]);
    new QVBoxLayout(this);
}

ViewportBase::ViewportBase(QWidget *parent, ViewportType viewportType, const uint id) :
    QOpenGLWidget(parent), resizeButton(this), viewportType(viewportType), id(id) {
    dockParent = parent;
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);

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
    vpLayout.addWidget(&resizeButton, 0, Qt::AlignBottom | Qt::AlignRight);
    setLayout(&vpLayout);
}

void ViewportBase::setDock(bool isDock) {
    bool wasVisible = isVisible();
    isDocked = isDock;
    if (isDock) {
        setParent(dockParent);
        if (NULL != floatParent) {
            delete floatParent;
            floatParent = NULL;
        }
        move(dockPos);
        resize(dockSize);
        dockPos = {};
        dockSize = {};
    } else {
        dockPos = pos();
        dockSize = size();
        floatParent = new QViewportFloatWidget(dockParent, id);
        floatParent->layout()->addWidget(this);
        floatParent->resize(size());
        if (wasVisible) {
            floatParent->show();
        }
        state->viewerState->defaultVPSizeAndPos = false;
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

ViewportOrtho::ViewportOrtho(QWidget *parent, ViewportType viewportType, const uint id) : ViewportBase(parent, viewportType, id) {
    switch(viewportType) {
    case VIEWPORT_XY:
        v1 = {1, 0, 0};
        v2 = {0, 1, 0};
        n = {0, 0, 1};
        break;
    case VIEWPORT_XZ:
        v1 = {1, 0, 0};
        v2 = {0, 0, 1};
        n = {0, 1, 0};
        break;
    case VIEWPORT_YZ:
        v1 = {0, 0, 1};
        v2 = {0, 1, 0};
        n = {1, 0, 0};
        break;
    default: break;
    }
    timeDBase.start();
    timeFBase.start();
}

Viewport3D::Viewport3D(QWidget *parent, ViewportType viewportType, const uint id) : ViewportBase(parent, viewportType, id) {
    const auto svpLayout = new QHBoxLayout();
    svpLayout->setSpacing(0);
    svpLayout->setAlignment(Qt::AlignTop | Qt::AlignRight);

    for (auto * button : {&xyButton, &xzButton, &yzButton}) {
        button->setMinimumSize(30, 20);
    }
    r90Button.setMinimumSize(35, 20);
    r180Button.setMinimumSize(40, 20);
    resetButton.setMinimumSize(45, 20);

    for (auto * button : {&xyButton, &xzButton, &yzButton, &r90Button, &r180Button, &resetButton}) {
        button->setMaximumSize(button->minimumSize());
        button->setCursor(Qt::ArrowCursor);
        svpLayout->addWidget(button);
    }
    vpLayout.insertLayout(0, svpLayout);

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
    connect(&yzButton, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_YZ_VIEW;
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
    if (!initializeOpenGLFunctions()) {
        qDebug() << "initializeOpenGLFunctions failed";
    }
    oglLogger.initialize();
    QObject::connect(&oglLogger, &QOpenGLDebugLogger::messageLogged, [](const QOpenGLDebugMessage & msg){
        qDebug() << msg;
    });
    if (oglDebug) {
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // loads an empty texture into video memory - during user movement, this
    // texture is updated via glTexSubImage2D in vpGenerateTexture
    // We need GL_RGB as texture internal format to color the textures

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 texture.edgeLengthPx,
                 texture.edgeLengthPx,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 state->viewerState->defaultTexData);

    createOverlayTextures();

    if (state->gpuSlicer) {
        GLint iUnits, texture_units, max_tu;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS, &iUnits);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texture_units);
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_tu);
        std::cout << "MultiTexture: " << iUnits << ' ' << texture_units << ' ' << max_tu << std::endl;

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    const auto size = texture.edgeLengthPx;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, state->viewerState->defaultOverlayData);
}

void ViewportOrtho::setOrientation(ViewportType orientation) {
    viewportType = orientation;
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
}

void Viewport3D::paintGL() {
    renderViewport();
    renderViewportFrontFace();
}

void ViewportOrtho::paintGL() {
    if (viewportType == VIEWPORT_ARBITRARY) {
        updateOverlayTexture();
    }
    if (state->gpuSlicer && state->viewer->gpuRendering) {
        renderViewportFast();
    } else {
        renderViewport(RenderOptions(false, false, state->viewerState->drawVPCrosshairs, state->overlay && state->viewerState->showOverlay));
    }
    renderViewportFrontFace();
}

void ViewportBase::enterEvent(QEvent *) {
    hasCursor = true;
    if (QApplication::activeWindow() != 0) {
        activateWindow();//steal keyboard from other active windows
    }
    setFocus();//get keyboard focus for this widget for viewport specific shortcuts
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

    Segmentation::singleton().brush.setView(static_cast<brush_t::view_t>(viewportType));

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

void ViewportOrtho::mouseReleaseEvent(QMouseEvent *event) {
    ViewportBase::mouseReleaseEvent(event);
    userMouseSlide = {};
    arbNodeDragCache = {};
    draggedNode = nullptr;
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

    if (event->key() == Qt::Key_D) {
        state->keyD = false;
    } else if (event->key() == Qt::Key_F) {
        state->keyF = false;
    } else if (event->key() == Qt::Key_Shift) {//decrease movement speed
        state->repeatDirection[0] /= 10;
        state->repeatDirection[1] /= 10;
        state->repeatDirection[2] /= 10;

        Segmentation::singleton().brush.setInverse(false);
    } else {
        handleKeyRelease(event);
    }
}

void ViewportOrtho::zoom(const float step) {
    int triggerMagChange = false;
    state->viewer->window->forEachOrthoVPDo([&step, &triggerMagChange](ViewportOrtho & orthoVP) {
        if(state->viewerState->datasetMagLock) {
            if(!(orthoVP.texture.zoomLevel + step < VPZOOMMAX) &&
               !(orthoVP.texture.zoomLevel + step > VPZOOMMIN)) {
                orthoVP.texture.zoomLevel += step;
            }
        }
        else {
            /* trigger a mag change when possible */
            if((orthoVP.texture.zoomLevel + step < 0.5)
                && (orthoVP.texture.zoomLevel >= 0.5)
                && (static_cast<uint>(state->magnification) != state->lowestAvailableMag)) {
                orthoVP.texture.zoomLevel += step;
                triggerMagChange = MAG_DOWN;
            }
            if((orthoVP.texture.zoomLevel + step > 1.0)
                && (orthoVP.texture.zoomLevel <= 1.0)
                && (static_cast<uint>(state->magnification) != state->highestAvailableMag)) {
                orthoVP.texture.zoomLevel += step;
                triggerMagChange = MAG_UP;
            }
            /* performe normal zooming otherwise. This case also covers
            * the special case of zooming in further than 0.5 on mag1 */
            if(!triggerMagChange) {
                float zoomLevel = orthoVP.texture.zoomLevel += step;
                if(zoomLevel < VPZOOMMAX) {
                    orthoVP.texture.zoomLevel = VPZOOMMAX;
                }
                else if (zoomLevel > VPZOOMMIN) {
                    orthoVP.texture.zoomLevel = VPZOOMMIN;
                }
            }
        }
    });

   if(triggerMagChange) {
        emit changeDatasetMagSignal(triggerMagChange);
   }
   emit recalcTextureOffsetsSignal();
   emit updateDatasetOptionsWidget();

}

float Viewport3D::zoomStep() const {
    return (0.1* (0.5 - state->skeletonState->zoomLevel));
}

void Viewport3D::zoom(const float step) {
    auto & zoomLvl = state->skeletonState->zoomLevel;
    zoomLvl = (step < 0 && zoomLvl >= SKELZOOMMIN)? std::max(zoomLvl + step, SKELZOOMMIN) : (step > 0 && zoomLvl <= SKELZOOMMAX)? std::min(zoomLvl + step, SKELZOOMMAX) : zoomLvl;
    emit updateDatasetOptionsWidget();
}

void Viewport3D::showHideButtons(bool isShow) {
    ViewportBase::showHideButtons(isShow);
    xyButton.setVisible(isShow);
    xzButton.setVisible(isShow);
    yzButton.setVisible(isShow);
    r90Button.setVisible(isShow);
    r180Button.setVisible(isShow);
    resetButton.setVisible(isShow);
}

void ViewportOrtho::updateOverlayTexture() {
    if (!state->viewer->oc_xy_changed && !state->viewer->oc_xz_changed && !state->viewer->oc_zy_changed) {
        return;
    }
    switch(id) {
    case VP_UPPERLEFT: state->viewer->oc_xy_changed = false; break;
    case VP_LOWERLEFT: state->viewer->oc_xz_changed = false; break;
    case VP_UPPERRIGHT: state->viewer->oc_zy_changed = false; break;
    }

    const int width = state->M * state->cubeEdgeLength;
    const int height = width;
    const auto begin = leftUpperPxInAbsPx_float;
    boost::multi_array_ref<uint8_t, 3> viewportView(reinterpret_cast<uint8_t *>(state->viewerState->overlayData), boost::extents[width][height][4]);
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
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, state->viewerState->overlayData);
    glBindTexture(GL_TEXTURE_2D, 0);
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

void ViewportBase::takeSnapshot(const QString & path, const int size, const bool withAxes, const bool withOverlay, const bool withSkeleton, const bool withScale, const bool withVpPlanes) {
    makeCurrent();
    glPushAttrib(GL_VIEWPORT_BIT); // remember viewport setting
    glViewport(0, 0, size, size);
    QOpenGLFramebufferObject fbo(size, size, QOpenGLFramebufferObject::CombinedDepthStencil);
    const RenderOptions options(withAxes, false, false, withOverlay, withSkeleton, withVpPlanes, false, false);
    fbo.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Qt does not clear it?
    renderViewport(options);
    if(withScale) {
        setFrontFacePerspective();
        renderScaleBar(std::ceil(0.02*size));
    }
    QImage fboImage(fbo.toImage());
    QImage image(fboImage.constBits(), fboImage.width(), fboImage.height(), QImage::Format_RGB32);
    image.save(path);
    glPopAttrib(); // restore viewport setting
    fbo.release();
}

void ViewportOrtho::sendCursorPosition() {
    const auto cursorPos = mapFromGlobal(QCursor::pos());
    emit cursorPositionChanged(getCoordinateFromOrthogonalClick(cursorPos.x(), cursorPos.y(), *this), id);
}
