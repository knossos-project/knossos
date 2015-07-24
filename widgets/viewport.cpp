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

#include "eventmodel.h"
#include "functions.h"
#include "GuiConstants.h"
#include "profiler.h"
#include "renderer.h"
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

bool Viewport::arbitraryOrientation = false;
bool Viewport::oglDebug = false;
bool Viewport::showNodeComments = false;

ResizeButton::ResizeButton(Viewport * parent) : QPushButton(parent) {}

void ResizeButton::mouseMoveEvent(QMouseEvent * event) {
    emit vpResize(event->globalPos());
}

QViewportFloatWidget::QViewportFloatWidget(QWidget *parent, int id) : QWidget(parent) {
    setWindowFlags(Qt::Window);
    setWindowTitle(VP_TITLES[id]);
    new QVBoxLayout(this);
}

Viewport::Viewport(QWidget *parent, ViewportType viewportType, uint newId) :
        QOpenGLWidget(parent), id(newId), viewportType(viewportType),
        isDocked(true), floatParent(nullptr), resizeButtonHold(false) {
    dockParent = parent;
    setContextMenuPolicy(Qt::CustomContextMenu);
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);

    resizeButton = new ResizeButton(this);
    resizeButton->setCursor(Qt::SizeFDiagCursor);
    resizeButton->setIcon(QIcon(":/resources/icons/resize.gif"));
    resizeButton->setMinimumSize(20, 20);
    resizeButton->setMaximumSize(resizeButton->minimumSize());

    QObject::connect(resizeButton, &ResizeButton::vpResize, this, &Viewport::resizeVP);

    const auto vpLayout = new QVBoxLayout();
    vpLayout->setMargin(0);//attach buttons to vp border

    if(viewportType == VIEWPORT_SKELETON) {
        xyButton = new QPushButton("xy", this);
        xzButton = new QPushButton("xz", this);
        yzButton = new QPushButton("yz", this);
        r90Button = new QPushButton("r90", this);
        r180Button = new QPushButton("r180", this);
        resetButton = new QPushButton("reset", this);

        const auto svpLayout = new QHBoxLayout();
        svpLayout->setSpacing(0);
        svpLayout->setAlignment(Qt::AlignTop | Qt::AlignRight);

        for (auto button : {xyButton, xzButton, yzButton}) {
            button->setMinimumSize(30, 20);
        }
        r90Button->setMinimumSize(35, 20);
        r180Button->setMinimumSize(40, 20);
        resetButton->setMinimumSize(45, 20);

        for (auto button : {xyButton, xzButton, yzButton, r90Button, r180Button, resetButton}) {
            button->setMaximumSize(button->minimumSize());
            button->setCursor(Qt::ArrowCursor);
            svpLayout->addWidget(button);
        }

        vpLayout->addLayout(svpLayout);

        connect(xyButton, SIGNAL(clicked()), this, SLOT(xyButtonClicked()));
        connect(xzButton, SIGNAL(clicked()), this, SLOT(xzButtonClicked()));
        connect(yzButton, SIGNAL(clicked()), this, SLOT(yzButtonClicked()));
        connect(r90Button, SIGNAL(clicked()), this, SLOT(r90ButtonClicked()));
        connect(r180Button, SIGNAL(clicked()), this, SLOT(r180ButtonClicked()));
        connect(resetButton, SIGNAL(clicked()), this, SLOT(resetButtonClicked()));
    }
    vpLayout->addStretch(1);
    vpLayout->addWidget(resizeButton, 0, Qt::AlignBottom | Qt::AlignRight);
    setLayout(vpLayout);

    timeDBase.start();
    timeFBase.start();
}

void Viewport::initializeGL() {
    if (viewportType == VIEWPORT_XY) {//we want only one output
        qDebug() << reinterpret_cast<const char*>(::glGetString(GL_VERSION))
                 << reinterpret_cast<const char*>(::glGetString(GL_VENDOR))
                 << reinterpret_cast<const char*>(::glGetString(GL_RENDERER));
    }
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

    if(viewportType != VIEWPORT_SKELETON) {
        glGenTextures(1, &state->viewerState->vpConfigs[id].texture.texHandle);

        glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[id].texture.texHandle);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        // loads an empty texture into video memory - during user movement, this
        // texture is updated via glTexSubImage2D in vpGenerateTexture
        // We need GL_RGB as texture internal format to color the textures

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     state->viewerState->vpConfigs[id].texture.edgeLengthPx,
                     state->viewerState->vpConfigs[id].texture.edgeLengthPx,
                     0,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     state->viewerState->defaultTexData);

        //Handle overlay textures.
    }

    // The following code configures openGL to draw into the current VP
    //set the drawing area in the window to our actually processed viewport.
    glViewport(state->viewerState->vpConfigs[id].upperLeftCorner.x,
               state->viewerState->vpConfigs[id].upperLeftCorner.y,
               state->viewerState->vpConfigs[id].edgeLength,
               state->viewerState->vpConfigs[id].edgeLength);
    //select the projection matrix
    glMatrixMode(GL_PROJECTION);
    //reset it
    glLoadIdentity();
    //define coordinate system for our viewport: left right bottom top near far
    //coordinate values
    glOrtho(0, state->viewerState->vpConfigs[id].edgeLength,
            state->viewerState->vpConfigs[id].edgeLength, 0, 25, -25);
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

    if (viewportType != VIEWPORT_SKELETON) {
        createOverlayTextures();
    }
}

bool Viewport::setOrientation(ViewportType orientation) {
    if(orientation != VIEWPORT_XY
       && orientation != VIEWPORT_XZ
       && orientation != VIEWPORT_YZ
       && orientation != VIEWPORT_ARBITRARY) {
        return false;
    }
    viewportType = orientation;
    state->viewerState->vpConfigs[id].type = orientation;
    return true;
}

void Viewport::createOverlayTextures() {
    glGenTextures(1, &state->viewerState->vpConfigs[id].texture.overlayHandle);

    glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[id].texture.overlayHandle);

    //Set the parameters for the texture.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    const auto size = state->viewerState->vpConfigs[id].texture.edgeLengthPx;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, state->viewerState->defaultOverlayData);
}

void Viewport::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat x = (GLfloat)width() / height();

    glFrustum(-x, +x, -1.0, + 1.0, 0.1, 10.0);
    glMatrixMode(GL_MODELVIEW);

    state->viewerState->vpConfigs[id].upperLeftCorner = {geometry().topLeft().x(), geometry().topLeft().y(), 0};
    state->viewerState->vpConfigs[id].edgeLength = width();
}

void Viewport::paintGL() {
    if (!isVisible()) {
        return;
    }
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    if(state->viewerState->viewerReady) {
        if (viewportType != VIEWPORT_SKELETON) {
            if (viewportType == VIEWPORT_ARBITRARY) {
                updateOverlayTexture();
            }
            drawViewport(id);
        } else {
            drawSkeletonViewport();
        }
        state->viewer->renderer->renderViewportFrontFace(id);
    }
}

void Viewport::enterEvent(QEvent *) {
    activateWindow();//steal keyboard from other active windows
    setFocus();//get keyboard focus for this widget for viewport specific shortcuts
}

void Viewport::mouseMoveEvent(QMouseEvent *event) {
    const auto mouseBtn = event->buttons();
    const auto penmode = state->viewerState->penmode;

    if((!penmode && mouseBtn == Qt::LeftButton) || (penmode && mouseBtn == Qt::RightButton)) {
        Qt::KeyboardModifiers modifiers = event->modifiers();
        bool ctrl = modifiers.testFlag(Qt::ControlModifier);
        bool alt = modifiers.testFlag(Qt::AltModifier);

        if(ctrl && alt) { // drag viewport around
            moveVP(event->globalPos());
        } else {// delegate behaviour
            eventDelegate->handleMouseMotionLeftHold(event, id);
        }
    } else if(mouseBtn == Qt::MidButton) {
        eventDelegate->handleMouseMotionMiddleHold(event, id);
    } else if( (!penmode && mouseBtn == Qt::RightButton) || (penmode && mouseBtn == Qt::LeftButton)) {
        eventDelegate->handleMouseMotionRightHold(event, id);
    }
    eventDelegate->handleMouseHover(event, id);

    Segmentation::singleton().brush.setView(static_cast<brush_t::view_t>(viewportType));

    eventDelegate->prevMouseMove = event->pos();
}

void Viewport::setDock(bool isDock) {
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

void Viewport::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::MouseButton::LeftButton) {
        setDock(!isDocked);
    }
}

void Viewport::mousePressEvent(QMouseEvent *event) {
    raise(); //bring this viewport to front

    eventDelegate->mouseDown = event->pos();

    auto penmode = state->viewerState->penmode;

    if((penmode && event->button() == Qt::RightButton) || (!penmode && event->button() == Qt::LeftButton)) {
        Qt::KeyboardModifiers modifiers = event->modifiers();
        const auto ctrl = modifiers.testFlag(Qt::ControlModifier);
        const auto alt = modifiers.testFlag(Qt::AltModifier);

        if(ctrl && alt) { // user wants to drag vp
            setCursor(Qt::ClosedHandCursor);
            baseEventX = event->x();
            baseEventY = event->y();
        } else {
            eventDelegate->handleMouseButtonLeft(event, id);
        }
    } else if(event->button() == Qt::MiddleButton) {
        eventDelegate->handleMouseButtonMiddle(event, id);
    } else if((penmode && event->button() == Qt::LeftButton) || (!penmode && event->button() == Qt::RightButton)) {
        eventDelegate->handleMouseButtonRight(event, id);
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent *event) {
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
        eventDelegate->handleMouseReleaseLeft(event, id);
    }
    if(event->button() == Qt::RightButton) {
        eventDelegate->handleMouseReleaseRight(event, id);
    }
    if(event->button() == Qt::MiddleButton) {
        eventDelegate->handleMouseReleaseMiddle(event, id);
    }

    eventDelegate->userMouseSlide = {};
    eventDelegate->arbNodeDragCache = {};
    state->viewerState->vpConfigs[id].draggedNode = nullptr;
}

void Viewport::wheelEvent(QWheelEvent *event) {
    eventDelegate->handleMouseWheel(event, id);
}

void Viewport::keyPressEvent(QKeyEvent *event) {
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

    //events
    //↓          #   #   #   #   #   #   #   # ↑  ↓          #  #  #…
    //^ os delay ^       ^---^ os key repeat

    //intended behavior:
    //↓          # # # # # # # # # # # # # # # ↑  ↓          # # # #…
    //^ os delay ^       ^-^ knossos specified interval

    //after a ›#‹ event state->viewerKeyRepeat instructs the viewer to check in each frame if a move should be performed

    //›#‹ events are marked isAutoRepeat correctly on Windows
    //on Mac and Linux only when you move the cursor out of the window (https://bugreports.qt-project.org/browse/QTBUG-21500)
    //to emulate this the time to the previous time the same event occured is measured
    //drawbacks of this emulation are:
    //- you can accidently enable autorepeat – skipping the os delay – although you just pressed 2 times very quickly (falling into the timer threshold)
    //- autorepeat is not activated until the 3rd press event, not the 2nd, because you need a base event for the timer

    if (event->key() == Qt::Key_D || event->key() == Qt::Key_F) {
        state->viewerKeyRepeat = event->isAutoRepeat();
    }
    if (event->key() == Qt::Key_H) {
        if (isDocked) {
            hide();
        }
        else {
            floatParent->hide();
        }
        state->viewerState->defaultVPSizeAndPos = false;
    }
    if (event->key() == Qt::Key_U) {
        if (isDocked) {
            // Currently docked and normal
            // Undock and go fullscreen from docked
            setDock(false);
            floatParent->setWindowState(Qt::WindowFullScreen);
            isFullOrigDocked = true;
        }
        else {
            // Currently undocked
            if (floatParent->isFullScreen()) {
                // Currently fullscreen
                // Go normal and back to original docking state
                floatParent->setWindowState(Qt::WindowNoState);
                if (isFullOrigDocked) {
                    setDock(isFullOrigDocked);
                }
            }
            else {
                // Currently not fullscreen
                // Go fullscreen from undocked
                floatParent->setWindowState(Qt::WindowFullScreen);
                isFullOrigDocked = false;
            }
        }
    }
    if (!event->isAutoRepeat()) {
        //autorepeat emulation for systems where isAutoRepeat() does not work as expected
        //seperate timer for each key, but only one across all vps
        if (event->key() == Qt::Key_D) {
            state->viewerKeyRepeat = timeDBase.restart() < 150;
        } else if (event->key() == Qt::Key_F) {
            state->viewerKeyRepeat = timeFBase.restart() < 150;
        }
    }
    eventDelegate->handleKeyPress(event, id);
}

void Viewport::keyReleaseEvent(QKeyEvent *event) {
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
        eventDelegate->handleKeyRelease(event);
    }
}

void Viewport::drawViewport(int vpID) {
    state->viewer->renderer->renderOrthogonalVP(vpID, RenderOptions(true, state->viewerState->drawVPCrosshairs, state->overlay && state->viewerState->showOverlay));
}

void Viewport::drawSkeletonViewport() {
    auto& seg = Segmentation::singleton();
    if (seg.volume_render_toggle) {
        if(seg.volume_update_required) {
            seg.volume_update_required = false;
            updateVolumeTexture();
        }
        renderVolumeVP();
    } else {
        state->viewer->renderer->renderSkeletonVP();
    }
}

void Viewport::zoomOrthogonals(float step){
    int triggerMagChange = false;
    for(uint i = 0; i < Viewport::numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != VIEWPORT_SKELETON) {
            /* check if mag is locked */
            if(state->viewerState->datasetMagLock) {
                if(!(state->viewerState->vpConfigs[i].texture.zoomLevel + step < VPZOOMMAX) &&
                   !(state->viewerState->vpConfigs[i].texture.zoomLevel + step > VPZOOMMIN)) {
                    state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                }
            }
            else {
                /* trigger a mag change when possible */
                if((state->viewerState->vpConfigs[i].texture.zoomLevel + step < 0.5)
                    && (state->viewerState->vpConfigs[i].texture.zoomLevel >= 0.5)
                    && (static_cast<uint>(state->magnification) != state->lowestAvailableMag)) {
                    state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                    triggerMagChange = MAG_DOWN;
                }
                if((state->viewerState->vpConfigs[i].texture.zoomLevel + step > 1.0)
                    && (state->viewerState->vpConfigs[i].texture.zoomLevel <= 1.0)
                    && (static_cast<uint>(state->magnification) != state->highestAvailableMag)) {
                    state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                    triggerMagChange = MAG_UP;
                }
                /* performe normal zooming otherwise. This case also covers
                * the special case of zooming in further than 0.5 on mag1 */
                if(!triggerMagChange) {
                    float zoomLevel = state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                    if(zoomLevel < VPZOOMMAX) {
                        state->viewerState->vpConfigs[i].texture.zoomLevel = VPZOOMMAX;
                    }
                    else if (zoomLevel > VPZOOMMIN) {
                        state->viewerState->vpConfigs[i].texture.zoomLevel = VPZOOMMIN;
                    }
                }
            }
        }
    }

   if(triggerMagChange) {
        emit changeDatasetMagSignal(triggerMagChange);
   }
   emit recalcTextureOffsetsSignal();
   emit updateDatasetOptionsWidget();

}

void Viewport::zoomOutSkeletonVP() {
    if (state->skeletonState->zoomLevel >= SKELZOOMMIN) {
        state->skeletonState->zoomLevel -= (0.1* (0.5 - state->skeletonState->zoomLevel));
        if (state->skeletonState->zoomLevel < SKELZOOMMIN) {
            state->skeletonState->zoomLevel = SKELZOOMMIN;
        }
        emit updateDatasetOptionsWidget();
    }
}
void Viewport::zoomInSkeletonVP() {
    if (state->skeletonState->zoomLevel <= SKELZOOMMAX){
        state->skeletonState->zoomLevel += (0.1 * (0.5 - state->skeletonState->zoomLevel));
        if(state->skeletonState->zoomLevel > SKELZOOMMAX) {
            state->skeletonState->zoomLevel = SKELZOOMMAX;
        }
        emit updateDatasetOptionsWidget();
    }
}

void Viewport::sizeAdapt() {
    sizeAdapt({size().width(), size().height()});
}

void Viewport::sizeAdapt(const QPoint & desiredSize) {
    const auto MIN_VP_SIZE = 50;
    const auto horizontalSpace = parentWidget()->width() - x();
    const auto verticalSpace = parentWidget()->height() - y();
    const auto size = std::max(MIN_VP_SIZE, std::min({horizontalSpace, verticalSpace, std::max(desiredSize.x(), desiredSize.y())}));

    resize({size, size});
}

void Viewport::resizeVP(const QPoint & globalPos) {
    if (!isDocked) {
        // Floating viewports are resized indirectly by container window
        return;
    }
    raise();//we come from the resize button
    //»If you move the widget as a result of the mouse event, use the global position returned by globalPos() to avoid a shaking motion.«
    const auto desiredSize = mapFromGlobal(globalPos);

    sizeAdapt(desiredSize);

    state->viewerState->defaultVPSizeAndPos = false;
}

void Viewport::posAdapt() {
    posAdapt(pos());
}

void Viewport::posAdapt(const QPoint & desiredPos) {
    const auto horizontalSpace = parentWidget()->width() - width();
    const auto verticalSpace = parentWidget()->height() - height();
    const auto newX = std::max(0, std::min(horizontalSpace, desiredPos.x()));
    const auto newY = std::max(0, std::min(verticalSpace, desiredPos.y()));

    move(newX, newY);
}

void Viewport::moveVP(const QPoint & globalPos) {
    if (!isDocked) {
        // Moving viewports is relevant only when docked
        return;
    }
    //»If you move the widget as a result of the mouse event, use the global position returned by globalPos() to avoid a shaking motion.«
    const auto position = mapFromGlobal(globalPos);
    const auto desiredX = x() + position.x() - eventDelegate->mouseDown.x();
    const auto desiredY = y() + position.y() - eventDelegate->mouseDown.y();

    posAdapt({desiredX, desiredY});

    state->viewerState->defaultVPSizeAndPos = false;
}

void Viewport::showHideButtons(bool isShow) {
    resizeButton->setVisible(isShow);
    if(viewportType == VIEWPORT_SKELETON) {
        xyButton->setVisible(isShow);
        xzButton->setVisible(isShow);
        yzButton->setVisible(isShow);
        r90Button->setVisible(isShow);
        r180Button->setVisible(isShow);
        resetButton->setVisible(isShow);
    }
}

void Viewport::updateOverlayTexture() {
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
    const auto & config = state->viewerState->vpConfigs[id];
    const auto begin = config.leftUpperPxInAbsPx_float;
    boost::multi_array_ref<uint8_t, 3> viewportView(reinterpret_cast<uint8_t *>(state->viewerState->overlayData), boost::extents[width][height][4]);
    for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
        const auto dataPos = static_cast<Coordinate>(begin + config.v1 * state->magnification * x + config.v2 * state->magnification * y);
        if (dataPos.x < 0 || dataPos.y < 0 || dataPos.z < 0) {
            viewportView[y][x][0] = viewportView[y][x][1] = viewportView[y][x][2] = viewportView[y][x][3] = 0;
        } else {
            const auto soid = readVoxel(dataPos);
            const auto color = Segmentation::singleton().colorObjectFromId(soid);
            viewportView[y][x][0] = std::get<0>(color);
            viewportView[y][x][1] = std::get<1>(color);
            viewportView[y][x][2] = std::get<2>(color);
            viewportView[y][x][3] = std::get<3>(color);
        }
    }
    glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[id].texture.overlayHandle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, state->viewerState->overlayData);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Viewport::updateVolumeTexture() {
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
    std::unordered_map<uint64_t, std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>> selectedIdColors;
    std::tuple<uint64_t, std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>> lastIdColor;

    state->protectCube2Pointer->lock();

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
                auto idColor = seg.colorObjectFromId(subobjectId);
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

    state->protectCube2Pointer->unlock();

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

void Viewport::xyButtonClicked() {
    if(state->skeletonState->rotationcounter == 0) {
        state->skeletonState->definedSkeletonVpView = SKELVP_XY_VIEW;
    }
}

void Viewport::xzButtonClicked() {
    if(state->skeletonState->rotationcounter == 0) {
        state->skeletonState->definedSkeletonVpView = SKELVP_XZ_VIEW;
    }
}

void Viewport::yzButtonClicked() {
    if(state->skeletonState->rotationcounter == 0) {
        state->skeletonState->definedSkeletonVpView = SKELVP_YZ_VIEW;
    }
}

void Viewport::r90ButtonClicked() {
    if(state->skeletonState->rotationcounter == 0) {
        state->skeletonState->definedSkeletonVpView = SKELVP_R90;
    }
}

void Viewport::r180ButtonClicked() {
    if(state->skeletonState->rotationcounter == 0) {
        state->skeletonState->definedSkeletonVpView = SKELVP_R180;
    }
}

void Viewport::resetButtonClicked() {
    if(state->skeletonState->rotationcounter == 0) {
        state->skeletonState->definedSkeletonVpView = SKELVP_RESET;
    }
}

void Viewport::takeSnapshot(const QString & path, const int size, const bool withOverlay, const bool withSkeleton, const bool withScale, const bool withVpPlanes) {
    glPushAttrib(GL_VIEWPORT_BIT); // remember viewport setting
    glViewport(0, 0, size, size);
    QOpenGLFramebufferObject fbo(size, size, QOpenGLFramebufferObject::Depth);
    const RenderOptions options(false, false, withOverlay, withSkeleton, withVpPlanes, false, false);
    fbo.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Qt does not clear it?
    if(viewportType == VIEWPORT_SKELETON) {
        auto& seg = Segmentation::singleton();
        if (seg.volume_render_toggle) {
            if(seg.volume_update_required) {
                seg.volume_update_required = false;
                updateVolumeTexture();
            }
            renderVolumeVP();
        }
        else {
            state->viewer->renderer->renderSkeletonVP(options);
        }
    }
    else {
        state->viewer->renderer->renderOrthogonalVP(id, options);
    }
    if(withScale) {
        state->viewer->renderer->setFrontFacePerspective(id);
        state->viewer->renderer->renderScaleBar(id, std::ceil(0.006*size), 0);
    }

    QImage fboImage(fbo.toImage());
    QImage image(fboImage.constBits(), fboImage.width(), fboImage.height(), QImage::Format_RGB32);
    if(withScale && id != VIEWPORT_SKELETON) {
        QString sizeLabel = QString::number(state->viewerState->vpConfigs[id].displayedlengthInNmY/3*0.001) + " µm";
        int edge_len = state->viewerState->vpConfigs[id].edgeLength/3;
        float scale = size/edge_len;
        int min_x = edge_len*0.05*scale, x = min_x + edge_len/6*scale, y = size - min_x - std::ceil(0.006*size) - 3;

        QPainter painter;
        painter.begin(&image);
        painter.setFont(QFont(painter.font().family(), std::ceil(0.02*size)));
        QFontMetrics metrics(painter.font());
        painter.drawText(x - metrics.width(sizeLabel)/2, y, sizeLabel);
        painter.end();
    }
    image.save(path);
    glPopAttrib(); // restore viewport setting
    fbo.bindDefault();
    fbo.release();
}
