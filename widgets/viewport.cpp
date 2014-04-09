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

#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

#include "eventmodel.h"
#include "functions.h"
#include "knossos-global.h"
#include "renderer.h"
#include "sleeper.h"
#include "viewer.h"
#include "viewport.h"

extern stateInfo *state;

ResizeButton::ResizeButton(Viewport * parent) : QPushButton(parent) {}

void ResizeButton::mouseMoveEvent(QMouseEvent * event) {
    emit vpResize(event);
}

Viewport::Viewport(QWidget *parent, QGLWidget *shared, int viewportType, uint newId) :
        QGLWidget(parent, shared), id(newId), viewportType(viewportType), resizeButtonHold(false) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);

    resizeButton = new ResizeButton(this);
    resizeButton->setCursor(Qt::SizeFDiagCursor);
    resizeButton->setIcon(QIcon(":/images/icons/resize.gif"));
    resizeButton->setMinimumSize(20, 20);
    resizeButton->setMaximumSize(resizeButton->minimumSize());

    QObject::connect(resizeButton, &ResizeButton::vpResize, this, &Viewport::resizeVP);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));

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

    if(viewportType == VIEWPORT_XY)
        this->setToolTip(QString("Viewport %1").arg("XY"));
    else if(viewportType == VIEWPORT_XZ)
        this->setToolTip(QString("Viewport %1").arg("XZ"));
    else if(viewportType == VIEWPORT_YZ)
        this->setToolTip(QString("Viewport %1").arg("YZ"));
    else if(viewportType == VIEWPORT_ARBITRARY)
        this->setToolTip(QString("Viewport %1").arg("Arbitrary"));
    else if(viewportType == VIEWPORT_SKELETON)
        this->setToolTip(QString("Skeleton Viewport"));
}

void Viewport::initializeGL() {
    if(viewportType != VIEWPORT_SKELETON) {
        glGenTextures(1, &state->viewerState->vpConfigs[id].texture.texHandle);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

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
    glDisable(GL_STENCIL);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHT_MODEL_LOCAL_VIEWER);

    if(viewportType == VIEWPORT_SKELETON) {//we want only one output
        qDebug() << reinterpret_cast<const char*>(glGetString(GL_VERSION));
    }
}

bool Viewport::setOrientation(int orientation) {
    if(orientation != VIEWPORT_XY
       && orientation != VIEWPORT_XZ
       && orientation != VIEWPORT_YZ
       && orientation != VIEWPORT_ARBITRARY) {
        return false;
    }
    this->viewportType = orientation;
    state->viewerState->vpConfigs[id].type = orientation;
    return true;
}

void Viewport::initializeOverlayGL() {
    if(viewportType != VIEWPORT_SKELETON) {
        if(state->overlay) {
            glGenTextures(1, &state->viewerState->vpConfigs[id].texture.overlayHandle);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[id].texture.overlayHandle);

            //Set the parameters for the texture.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         state->viewerState->vpConfigs[id].texture.edgeLengthPx,
                         state->viewerState->vpConfigs[id].texture.edgeLengthPx,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         state->viewerState->defaultOverlayData);
        }
    }
}

void Viewport::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat x = (GLfloat)width() / height();

    glFrustum(-x, +x, -1.0, + 1.0, 0.1, 10.0);
    glMatrixMode(GL_MODELVIEW);

    SET_COORDINATE(state->viewerState->vpConfigs[id].upperLeftCorner,
                   geometry().topLeft().x(),
                   geometry().topLeft().y(),
                   0);
    state->viewerState->vpConfigs[id].edgeLength = width();
}

void Viewport::paintGL() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


    if(state->viewerState->viewerReady) {
        if(this->viewportType != VIEWPORT_SKELETON) {
           this->drawViewport(id);
        }  else {
            this->drawSkeletonViewport();
        }
    }
}

void Viewport::showContextMenu(const QPoint &point) {
    if(viewportType == VIEWPORT_SKELETON and QApplication::keyboardModifiers() == Qt::ControlModifier) {
        QMenu menu(this);
        QMenu *subMenu = menu.addMenu("Change view direction");
        subMenu->addAction("xy", this, SLOT(xyButtonClicked()));
        subMenu->addAction("xz", this, SLOT(xzButtonClicked()));
        subMenu->addAction("yz", this, SLOT(xzButtonClicked()));
        subMenu->addAction("r90", this, SLOT(r90ButtonClicked()));
        subMenu->addAction("180", this, SLOT(r180ButtonClicked()));
        subMenu->addAction("reset", this, SLOT(resetButtonClicked()));

        menu.popup(this->mapToGlobal(point));
        menu.exec();
    }
}

void Viewport::enterEvent(QEvent *) {
    setFocus();//get the keyboard focus on first mouse move so we don’t need permanent mousetracking
}

void Viewport::mouseMoveEvent(QMouseEvent *event) {
    bool clickEvent = false;

    if(QApplication::mouseButtons() == Qt::LeftButton) {
        Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
        bool ctrl = modifiers.testFlag(Qt::ControlModifier);
        bool alt = modifiers.testFlag(Qt::AltModifier);

        if(ctrl and alt) { // drag viewport around
            moveVP(event);
        } else {// delegate behaviour
            eventDelegate->handleMouseMotionLeftHold(event, id);
            clickEvent = true;
        }
    } else if(QApplication::mouseButtons() == Qt::MidButton) {
        eventDelegate->handleMouseMotionMiddleHold(event, id);
        clickEvent = true;
    } else if(QApplication::mouseButtons() == Qt::RightButton) {
        eventDelegate->handleMouseMotionRightHold(event, id);
        clickEvent = true;
    }

    if(clickEvent) {
        eventDelegate->mouseX = event->x();
        eventDelegate->mouseY = event->y();
    }
}

void Viewport::mousePressEvent(QMouseEvent *event) {
    raise(); //bring this viewport to front

    eventDelegate->mouseX = event->x();
    eventDelegate->mouseY = event->y();

    if(event->button() == Qt::LeftButton) {
        Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
        const auto ctrl = modifiers.testFlag(Qt::ControlModifier);
        const auto alt = modifiers.testFlag(Qt::AltModifier);

        if(ctrl and alt) { // user wants to drag vp
            setCursor(Qt::ClosedHandCursor);
            baseEventX = event->x();
            baseEventY = event->y();
        } else {
            eventDelegate->handleMouseButtonLeft(event, id);
        }
    } else if(event->button() == Qt::MiddleButton) {
        eventDelegate->handleMouseButtonMiddle(event, id);
    } else if(event->button() == Qt::RightButton) {
        eventDelegate->handleMouseButtonRight(event, id);
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent *event) {
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    const auto ctrl = modifiers.testFlag(Qt::ControlModifier);
    const auto alt = modifiers.testFlag(Qt::AltModifier);

    if (ctrl && alt) {
        setCursor(Qt::OpenHandCursor);
    } else if (ctrl) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }

    if(event->button() == Qt::MiddleButton) {
        eventDelegate->handleMouseReleaseMiddle(event, id);
    }
    if(event->button() == Qt::LeftButton) {
        eventDelegate->handleMouseReleaseLeft(event, id);
    }

    for (std::size_t i = 0; i < state->viewerState->numberViewports; i++) {
        state->viewerState->vpConfigs[i].draggedNode = NULL;
        state->viewerState->vpConfigs[i].motionTracking = false;
        state->viewerState->vpConfigs[i].VPmoves = false;
        state->viewerState->vpConfigs[i].VPresizes = false;
        state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
        state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
    }
}

void Viewport::wheelEvent(QWheelEvent *event) {
    if(event->delta() > 0) {
        eventDelegate->handleMouseWheelForward(event, id);
    } else {
        eventDelegate->handleMouseWheelBackward(event, id);
    }
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

    state->viewerKeyRepeat = event->isAutoRepeat();
    if (!event->isAutoRepeat()) {//maybe we need to set it ourselves
        //autorepeat emulation for systems where isAutoRepeat() does not work as expected
        //seperate timer for each key, but only one across all vps
        if (event->key() == Qt::Key_D) {
            static QElapsedTimer timeBase;
            state->viewerKeyRepeat = timeBase.restart() < 100;
        } else if (event->key() == Qt::Key_F) {
            static QElapsedTimer timeBase;
            state->viewerKeyRepeat = timeBase.restart() < 100;
        }
    }
    eventDelegate->handleKeyboard(event, id);
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

    state->viewerKeyRepeat = false;
    if (event->key() == Qt::Key_D) {
        state->keyD = false;
        state->viewerKeyRepeat = false;
    } else if (event->key() == Qt::Key_F) {
        state->keyF = false;
    } else if (event->key() == Qt::Key_Shift) {//decrease movement speed
        state->repeatDirection[0] /= 10;
        state->repeatDirection[1] /= 10;
        state->repeatDirection[2] /= 10;
    }
}

void Viewport::drawViewport(int vpID) {
    state->viewer->renderer->renderOrthogonalVP(vpID);
}

void Viewport::drawSkeletonViewport() {
    state->viewer->renderer->renderSkeletonVP(VIEWPORT_SKELETON);
}

void Viewport::zoomOrthogonals(float step){
    int triggerMagChange = false;
    for(uint i = 0; i < state->viewerState->numberViewports; i++) {
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
                    && (state->magnification != state->lowestAvailableMag)) {
                    state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                    triggerMagChange = MAG_DOWN;
                }
                if((state->viewerState->vpConfigs[i].texture.zoomLevel + step > 1.0)
                    && (state->viewerState->vpConfigs[i].texture.zoomLevel <= 1.0)
                    && (state->magnification != state->highestAvailableMag)) {
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
   emit updateZoomAndMultiresWidget();

}

void Viewport::zoomOutSkeletonVP() {
    if (state->skeletonState->zoomLevel >= SKELZOOMMIN) {
        state->skeletonState->zoomLevel -= (0.1* (0.5 - state->skeletonState->zoomLevel));
        if (state->skeletonState->zoomLevel < SKELZOOMMIN) {
            state->skeletonState->zoomLevel = SKELZOOMMIN;
        }
        state->skeletonState->viewChanged = true;
        emit updateZoomAndMultiresWidget();
    }
}
void Viewport::zoomInSkeletonVP() {
    if (state->skeletonState->zoomLevel <= SKELZOOMMAX){
        state->skeletonState->zoomLevel += (0.1 * (0.5 - state->skeletonState->zoomLevel));
        if(state->skeletonState->zoomLevel > SKELZOOMMAX) {
            state->skeletonState->zoomLevel = SKELZOOMMAX;
        }
        state->skeletonState->viewChanged = true;
        emit updateZoomAndMultiresWidget();
    }
}

void Viewport::resizeVP(QMouseEvent *event) {
    raise();//we come from the resize button
    //»If you move the widget as a result of the mouse event, use the global position returned by globalPos() to avoid a shaking motion.«
    const int MIN_VP_SIZE = 50;
    const auto position = mapFromGlobal(event->globalPos());
    const auto horizontalSpace = parentWidget()->width() - x();
    const auto verticalSpace = parentWidget()->height() - y();
    const auto size = std::max(MIN_VP_SIZE, std::min({horizontalSpace, verticalSpace, std::max(position.x(), position.y())}));

    resize(size, size);

    state->viewerState->defaultVPSizeAndPos = false;
}

void Viewport::moveVP(QMouseEvent *event) {
    //»If you move the widget as a result of the mouse event, use the global position returned by globalPos() to avoid a shaking motion.«
    const auto position = mapFromGlobal(event->globalPos());
    const auto horizontalSpace = parentWidget()->width() - width();
    const auto verticalSpace = parentWidget()->height() - height();
    const auto desiredX = x() + position.x() - baseEventX;
    const auto desiredY = y() + position.y() - baseEventY;

    const auto newX = std::max(0, std::min(horizontalSpace, desiredX));
    const auto newY = std::max(0, std::min(verticalSpace, desiredY));

    move(newX, newY);
    state->viewerState->defaultVPSizeAndPos = false;
}

void Viewport::hideButtons() {
    resizeButton->hide();
    if(viewportType == VIEWPORT_SKELETON) {
        xyButton->hide();
        xzButton->hide();
        yzButton->hide();
        r90Button->hide();
        r180Button->hide();
        resetButton->hide();
    }
}

void Viewport::showButtons() {
    resizeButton->show();
    if(viewportType == VIEWPORT_SKELETON) {
        xyButton->show();
        xzButton->show();
        yzButton->show();
        r90Button->show();
        r180Button->show();
        resetButton->show();
    }
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
