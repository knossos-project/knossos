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

#include "viewportbase.h"

#include "dataset.h"
#include "scriptengine/scripting.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>

bool ViewportBase::oglDebug = false;

void ResizeButton::mouseMoveEvent(QMouseEvent * event) {
    emit vpResize(event->globalPos());
}

QViewportFloatWidget::QViewportFloatWidget(QWidget *parent, ViewportBase *vp) : QDialog(parent), vp(vp) {
    const std::array<const char * const, ViewportBase::numberViewports> VP_TITLES{{"XY", "XZ", "ZY", "Arbitrary", "3D"}};
    setWindowTitle(VP_TITLES[vp->viewportType]);
    new QVBoxLayout(this);
}

void QViewportFloatWidget::closeEvent(QCloseEvent *event) {
    vp->setDock(true);
    QWidget::closeEvent(event);
}

void QViewportFloatWidget::moveEvent(QMoveEvent *event) {
    // Entering/leaving fullscreen mode and docking/undocking cause moves and resizes.
    // These should not overwrite the non maximized position and sizes.
    if (vp->isDocked == false && fullscreen == false) {
        nonMaximizedPos = pos();
    }
    QWidget::moveEvent(event);
}

QSize vpSizeFromWindowSize(const QSize & size) {
    const int len = ((size.height() <= size.width()) ? size.height() : size.width()) - 2 * DEFAULT_VP_MARGIN;
    return {len, len};
}

void QViewportFloatWidget::resizeEvent(QResizeEvent *event) {
    if (vp->isDocked == false) {
        vp->resize(vpSizeFromWindowSize(size()));
        if (fullscreen == false) {
            nonMaximizedSize = size();
        }
    }
    QWidget::resizeEvent(event);
}

void QViewportFloatWidget::showEvent(QShowEvent *event) {
    vp->resize(vpSizeFromWindowSize(size()));
    QWidget::showEvent(event);
}

ViewportBase::ViewportBase(QWidget *parent, ViewportType viewportType) :
    QOpenGLWidget(parent), dockParent(parent), resizeButton(this), viewportType(viewportType), floatParent(parent, this), edgeLength(width()) {
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);

    QObject::connect(&snapshotAction, &QAction::triggered, [this]() { emit snapshotTriggered(this->viewportType); });
    QObject::connect(&floatingWindowAction, &QAction::triggered, [this]() { setDock(!isDocked); });
    QObject::connect(&fullscreenAction, &QAction::triggered, [this]() {
        if (isDocked) {
            // Currently docked and normal
            // Undock and go fullscreen from docked
            floatParent.setWindowState(Qt::WindowFullScreen);
            setDock(false);
            floatParent.fullscreen = true;
            isFullOrigDocked = true;
        }
        else {
            // Currently undocked
            if (floatParent.isFullScreen()) {
                // Currently fullscreen
                // Go normal and back to original docking state
                floatParent.setWindowState(Qt::WindowNoState);
                floatParent.fullscreen = false;
                if (isFullOrigDocked) { // was docked before it went fullscreen
                    setDock(true);
                }
            }
            else {
                // Currently not fullscreen
                // Go fullscreen from undocked
                floatParent.setWindowState(Qt::WindowFullScreen);
                floatParent.fullscreen = true;
                isFullOrigDocked = false;
            }
        }
    });
    QObject::connect(&hideAction, &QAction::triggered, this, &ViewportBase::hideVP);
    menuButton.setText("…");
    menuButton.setCursor(Qt::ArrowCursor);
    menuButton.setMinimumSize(35, 20);
    menuButton.setMaximumSize(menuButton.minimumSize());
    menuButton.setMenu(&menu);
    zoomInAction.setShortcuts(QKeySequence::ZoomIn);
    zoomOutAction.setShortcuts(QKeySequence::ZoomOut);
    addAction(&zoomInAction);
    addAction(&zoomOutAction);
    zoomInAction.setShortcutContext(Qt::WidgetShortcut);
    zoomOutAction.setShortcutContext(Qt::WidgetShortcut);
    menuButton.menu()->addAction(&snapshotAction);
    menuButton.menu()->addAction(&floatingWindowAction);
    menuButton.menu()->addAction(&fullscreenAction);
    menuButton.menu()->addAction(&zoomInAction);
    menuButton.menu()->addAction(&zoomOutAction);
    zoomEndSeparator = menuButton.menu()->addSeparator();
    menuButton.menu()->addAction(&hideAction);
    menuButton.setPopupMode(QToolButton::InstantPopup);
    resizeButton.setCursor(Qt::SizeFDiagCursor);
    resizeButton.setIcon(QIcon(":/resources/icons/resize.gif"));
    resizeButton.setMinimumSize(20, 20);
    resizeButton.setMaximumSize(resizeButton.minimumSize());
    QObject::connect(&zoomInAction, &QAction::triggered, this, &ViewportBase::zoomIn);
    QObject::connect(&zoomOutAction, &QAction::triggered, this, &ViewportBase::zoomOut);

    QObject::connect(&resizeButton, &ResizeButton::vpResize, [this](const QPoint & globalPos) {
        raise();//we come from the resize button
        //»If you move the widget as a result of the mouse event, use the global position returned by globalPos() to avoid a shaking motion.«
        const auto desiredSize = mapFromGlobal(globalPos);
        sizeAdapt(desiredSize);
        state->viewer->setDefaultVPSizeAndPos(false);
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
    isDocked = isDock;
    if (isDock) {
        setParent(dockParent);
        floatParent.hide();
        move(dockPos);
        resize(dockSize);
    } else {
        state->viewer->setDefaultVPSizeAndPos(false);
        if (floatParent.nonMaximizedPos == boost::none) {
            floatParent.nonMaximizedSize = QSize( size().width() + 2 * DEFAULT_VP_MARGIN, size().height() + 2 * DEFAULT_VP_MARGIN );
            floatParent.nonMaximizedPos = mapToGlobal(QPoint(0, 0));
        }
        setParent(&floatParent);
        if (floatParent.fullscreen) {
            floatParent.setWindowState(Qt::WindowFullScreen);
        } else {
            floatParent.move(floatParent.nonMaximizedPos.get());
            floatParent.resize(floatParent.nonMaximizedSize.get());
        }
        move(QPoint(DEFAULT_VP_MARGIN, DEFAULT_VP_MARGIN));
        floatParent.show();
    }
    floatingWindowAction.setVisible(isDock);
    resizeButton.setVisible(isDock && state->viewerState->showVpDecorations);
    show();
    setFocus();
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
    state->viewer->setDefaultVPSizeAndPos(false);
}

void ViewportBase::hideVP() {
    if (isDocked) {
        hide();
    } else {
        floatParent.hide();
        setParent(dockParent);
        hide();
    }
    state->viewer->setDefaultVPSizeAndPos(false);
}

void ViewportBase::moveEvent(QMoveEvent *event) {
    if (isDocked) {
        dockPos = pos();
    }
    QOpenGLWidget::moveEvent(event);
}

void ViewportBase::resizeEvent(QResizeEvent *event) {
    if (isDocked) {
        dockSize = size();
    }
    QOpenGLWidget::resizeEvent(event);
}


void ViewportBase::initializeGL() {
    static bool printed = false;
    const auto glversion  = reinterpret_cast<const char*>(::glGetString(GL_VERSION));
    const auto glvendor   = reinterpret_cast<const char*>(::glGetString(GL_VENDOR));
    const auto glrenderer = reinterpret_cast<const char*>(::glGetString(GL_RENDERER));
    if (!printed) {
        qDebug() << QString("%1, %2, %3").arg(glversion).arg(glvendor).arg(glrenderer);
        printed = true;
    }
    if (!initializeOpenGLFunctions()) {
        QMessageBox msgBox{QApplication::activeWindow()}; //use property based api
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Insufficient OpenGL version.\nKNOSSOS requires at least OpenGL version 1.4"));
        msgBox.setInformativeText(tr("Please update drivers, or graphics hardware."));
        msgBox.setDetailedText(QString("GL_VERSION:\t%1\nGL_VENDOR:\t%2\nGL_RENDERER:\t%3").arg(glversion).arg(glvendor).arg(glrenderer));
        msgBox.exec();
        throw std::runtime_error("initializeOpenGLFunctions failed");
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
    if (!meshShader.isLinked()) { // only setup shaders once, not on every vp dock/undock
        meshShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/mesh/meshshader.vert");
        meshShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/mesh/meshshader.frag");
        meshShader.link();
    }
    if (!meshTreeColorShader.isLinked()) {
        meshTreeColorShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/mesh/meshtreecolorshader.vert");
        meshTreeColorShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/mesh/meshtreecolorshader.frag");
        meshTreeColorShader.link();
    }
    if (!meshIdShader.isLinked()) {
        auto enabled = meshIdShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/mesh/meshidshader.vert");
        enabled = enabled && meshIdShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/mesh/meshidshader.frag");
        state->viewerState->MeshPickingEnabled = enabled && meshIdShader.link();
    }
    for (auto * shader : {&meshShader, &meshTreeColorShader, &meshIdShader}) {
        if (!shader->log().isEmpty()) {
            qDebug() << shader->log();
        }
    }
}

void ViewportBase::resizeGL(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat x = (GLfloat)width / height;

    glFrustum(-x, +x, -1.0, + 1.0, 0.1, 10.0);
    glMatrixMode(GL_MODELVIEW);

    edgeLength = width;
    state->viewer->recalcTextureOffsets();
}

void ViewportBase::focusOutEvent(QFocusEvent * event) {
    if (QApplication::focusWidget() != state->viewer->window->viewportXY.get()
        && QApplication::focusWidget() != state->viewer->window->viewportXZ.get()
        && QApplication::focusWidget() != state->viewer->window->viewportZY.get()
        && QApplication::focusWidget() != state->viewer->window->viewport3D.get()) {
        // simulate space key release, lest the event get lost in another widget.
        QKeyEvent keyEvent(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier);
        keyReleaseEvent(&keyEvent);
    }
    QWidget::focusOutEvent(event);
}

void ViewportBase::enterEvent(QEvent * event) {
    hasCursor = true;
    QOpenGLWidget::enterEvent(event);
}

void ViewportBase::leaveEvent(QEvent * event) {
    hasCursor = false;
    emit cursorPositionChanged(Coordinate(), VIEWPORT_UNDEFINED);
    QOpenGLWidget::leaveEvent(event);
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

void ViewportBase::tabletEvent(QTabletEvent *event) {
    const auto notRelease = event->type() != QEvent::TabletRelease;
    stylusDetected = notRelease;
    stylusDetected &= event->pointerType() == QTabletEvent::Pen || event->pointerType() == QTabletEvent::Eraser;
    if (event->pointerType() == QTabletEvent::Eraser && notRelease) {
        Segmentation::singleton().brush.setInverse(true);
    } else if (event->pointerType() == QTabletEvent::Eraser && event->type() == QEvent::TabletRelease) {
        Segmentation::singleton().brush.setInverse(false);
    }
    QWidget::tabletEvent(event);
}

void ViewportBase::mouseMoveEvent(QMouseEvent *event) {
    const auto mouseBtn = event->buttons();
    const auto penmode = state->viewerState->penmode || stylusDetected;

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

    const bool keyD = event->key() == Qt::Key_D;
    const bool keyF = event->key() == Qt::Key_F;
    const bool keyLeft = event->key() == Qt::Key_Left;
    const bool keyRight = event->key() == Qt::Key_Right;
    const bool keyUp = event->key() == Qt::Key_Up;
    const bool keyDown = event->key() == Qt::Key_Down;
    const auto singleVoxelKey = keyD || keyF || keyLeft || keyRight || keyUp || keyDown;
    if (event->key() == Qt::Key_Shift) {
        state->viewerState->repeatDirection /= 10;// decrease movement speed
        Segmentation::singleton().brush.setInverse(false);
    } else if (!event->isAutoRepeat() && singleVoxelKey) {
        state->viewer->userMoveRound();
        state->viewerState->keyRepeat = false;
        state->viewerState->notFirstKeyRepeat = false;
    }
    handleKeyRelease(event);
}

void ViewportBase::takeSnapshotVpSize(SnapshotOptions o) {
    o.size = width();
    takeSnapshot(o);
}

void ViewportBase::takeSnapshot(const SnapshotOptions & o) {
    if (isHidden()) {
        QMessageBox prompt{QApplication::activeWindow()};
        prompt.setIcon(QMessageBox::Question);
        prompt.setText(tr("Please enable the viewport for taking a snapshot of it."));
        const auto *accept = prompt.addButton(tr("Activate && take snapshot"), QMessageBox::AcceptRole);
        prompt.addButton(tr("Cancel"), QMessageBox::RejectRole);
        prompt.exec();
        if (prompt.clickedButton() == accept) {
            if (viewportType == VIEWPORT_ARBITRARY) {
                state->viewer->setEnableArbVP(true);
            }
            setVisible(true);
        } else {
            return;
        }
    }
    makeCurrent();
    glEnable(GL_MULTISAMPLE);
    glPushAttrib(GL_VIEWPORT_BIT); // remember viewport setting
    glViewport(0, 0, o.size, o.size);
    QOpenGLFramebufferObjectFormat format;
    format.setSamples(state->viewerState->sampleBuffers);
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    QImage fboImage;
    { // fbo scope
    auto fbo = std::make_shared<QOpenGLFramebufferObject>(o.size, o.size, format);
    snapshotFbo = fbo;
    const auto options = RenderOptions::snapshotRenderOptions(o.withAxes, o.withBox, o.withOverlay, o.withMesh, o.withSkeleton, o.withVpPlanes);
    fbo->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Qt does not clear it?
    bool tmpLinesAndPoints = state->viewerState->onlyLinesAndPoints;
    state->viewerState->onlyLinesAndPoints = false;
    renderViewport(options);
    state->viewerState->onlyLinesAndPoints = tmpLinesAndPoints;
    if(o.withScale) {
        setFrontFacePerspective();
        renderScaleBar();
    }
    glPopAttrib(); // restore viewport setting
    fbo->release();
    fboImage = fbo->toImage();
    }
    // Need to specify image format with no premultiplied alpha.
    // Otherwise the image is automatically unpremultiplied on save even though it was never premultiplied in the first place. See https://doc.qt.io/qt-5/qopenglframebufferobject.html#toImage
    QImage image(fboImage.constBits(), fboImage.width(), fboImage.height(), QImage::Format_RGB32);
    image = image.copy();// prevent implicit sharing from breaking when fboImage gets out-of-scope
    if (o.path) {
        qDebug() << tr("snapshot ") + (!image.save(o.path.get()) ? "un" : "") + tr("successful.");
    } else {
        emit snapshotFinished(image);
    }
}
