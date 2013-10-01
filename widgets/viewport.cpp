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

#include "knossos-global.h"
#include "viewport.h"
#include "eventmodel.h"
#include "renderer.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "sleeper.h"
#include "functions.h"
#include <QPainter>

extern stateInfo *state;


Viewport::Viewport(QWidget *parent, int viewportType, int widgetNumber) :
    QGLWidget(parent) {
    delegate = new EventModel();
    /* per default the widget only receives move event when at least one mouse button is pressed
    to change this behaviour we need to track the mouse position */

    this->viewportType = viewportType;
    this->plane = widgetNumber;

    this->setMouseTracking(true);
    this->setCursor(Qt::CrossCursor);
    this->setAutoBufferSwap(true);
    this->setFocusPolicy(Qt::WheelFocus); // this means the widget accepts mouse and keyboard focus. This solves also the problem that viewports had to be clicked before the widget know in which viewport the mouse click occured.

    moveButton = new QPushButton("Move", this);
    moveButton->setGeometry(323, 298, 25, 25);

    resizeButton = new QPushButton("Resize", this);
    resizeButton->setGeometry(322, 322, 25, 25);

    moveButton->show();
    resizeButton->show();

    connect(moveButton, SIGNAL(clicked()), this, SLOT(moveButtonClicked()));
    connect(resizeButton, SIGNAL(clicked()), this, SLOT(resizeButtonClicked()));

    if(widgetNumber == VIEWPORT_SKELETON) {
        xyButton = new QPushButton("xy", this);
        xyButton->setGeometry(113, 2, 35, 20);
        xzButton = new QPushButton("xz", this);
        xzButton->setGeometry(153, 2, 35, 20);
        yzButton = new QPushButton("yz", this);
        yzButton->setGeometry(193, 2, 35, 20);
        r90Button = new QPushButton("r90", this);
        r90Button->setGeometry(233, 2, 35, 20);
        r180Button = new QPushButton("r180", this);
        r180Button->setGeometry(273, 2, 35, 20);
        resetButton = new QPushButton("reset", this);
        resetButton->setGeometry(313, 2, 35, 20);

        connect(xyButton, SIGNAL(clicked()), this, SLOT(xyButtonClicked()));
        connect(xzButton, SIGNAL(clicked()), this, SLOT(xzButtonClicked()));
        connect(yzButton, SIGNAL(clicked()), this, SLOT(yzButtonClicked()));
        connect(r90Button, SIGNAL(clicked()), this, SLOT(r90ButtonClicked()));
        connect(r180Button, SIGNAL(clicked()), this, SLOT(r180ButtonClicked()));
        connect(resetButton, SIGNAL(clicked()), this, SLOT(resetButtonClicked()));
    }
}

void Viewport::initializeGL() {
    if(plane < VIEWPORT_SKELETON) {

        glGenTextures(1, &state->viewerState->vpConfigs[plane].texture.texHandle);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[plane].texture.texHandle);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        // loads an empty texture into video memory - during user movement, this
        // texture is updated via glTexSubImage2D in vpGenerateTexture & vpHandleBacklog
        // We need GL_RGB as texture internal format to color the textures

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     state->viewerState->vpConfigs[plane].texture.edgeLengthPx,
                     state->viewerState->vpConfigs[plane].texture.edgeLengthPx,
                     0,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     state->viewerState->defaultTexData);

        //Handle overlay textures.

    }


    // The following code configures openGL to draw into the current VP
    //set the drawing area in the window to our actually processed view port.
    glViewport(state->viewerState->vpConfigs[plane].upperLeftCorner.x,
               state->viewerState->vpConfigs[plane].upperLeftCorner.y,
               state->viewerState->vpConfigs[plane].edgeLength,
               state->viewerState->vpConfigs[plane].edgeLength);
    //select the projection matrix
    glMatrixMode(GL_PROJECTION);
    //reset it
    glLoadIdentity();
    //define coordinate system for our viewport: left right bottom top near far
    //coordinate values
    glOrtho(0, state->viewerState->vpConfigs[plane].edgeLength,
            state->viewerState->vpConfigs[plane].edgeLength, 0, 25, -25);
    //select the modelview matrix for modification
    glMatrixMode(GL_MODELVIEW);
    //reset it
    glLoadIdentity();
    //glBlendFunc(GL_ONE_MINUS_DST_ALPHA,GL_DST_ALPHA);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);



}

void Viewport::initializeOverlayGL() {
    if(plane < VIEWPORT_SKELETON) {
        if(state->overlay) {
            glGenTextures(1, &state->viewerState->vpConfigs[plane].texture.overlayHandle);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[plane].texture.overlayHandle);

            //Set the parameters for the texture.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         state->viewerState->vpConfigs[plane].texture.edgeLengthPx,
                         state->viewerState->vpConfigs[plane].texture.edgeLengthPx,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         state->viewerState->defaultOverlayData);
        }
    }
}

void Viewport::resizeGL(int w, int h) {

    LOG("resizing")
    glViewport(0, 0, width(), height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat x = (GLfloat)width() / height();

    glFrustum(-x, +x, -1.0, + 1.0, 0.1, 15.0);
    glMatrixMode(GL_MODELVIEW);

    SET_COORDINATE(state->viewerState->vpConfigs[plane].upperLeftCorner,
                   geometry().topLeft().x(),
                   geometry().topLeft().y(),
                   0);
    state->viewerState->vpConfigs[plane].edgeLength = width();


}

void Viewport::paintGL() {

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


    if(state->viewerState->viewerReady) {
        if(this->plane < VIEWPORT_SKELETON) {
           this->drawViewport(plane);
        }  else {
            this->drawSkeletonViewport();
        }
    }

}

//functions to determine position x/y relative to last position lastX, lastY
int Viewport::xrel(int x) {
    return (x - this->lastX);
}
int Viewport::yrel(int y) {
    return (y - this->lastY);
}


void Viewport::mouseMoveEvent(QMouseEvent *event) {

    bool clickEvent = false;

    if(QApplication::mouseButtons() == Qt::LeftButton) {
        handleMouseMotionLeftHold(event, plane);
        clickEvent = true;
    } else if(QApplication::mouseButtons() == Qt::MidButton) {
        handleMouseMotionMiddleHold(event, plane);
        clickEvent = true;
    } else if(QApplication::mouseButtons() == Qt::RightButton) {
        handleMouseMotionRightHold(event, plane);
        clickEvent = true;
    }


    if(clickEvent) {
        delegate->mouseX = event->x();
        delegate->mouseY = event->y();
    }
}

void Viewport::mousePressEvent(QMouseEvent *event) {

    delegate->mouseX = event->x();
    delegate->mouseY = event->y();

    if(event->button() == Qt::LeftButton) {
        handleMouseButtonLeft(event, plane);
    }
    else if(event->button() == Qt::MidButton) {
        handleMouseButtonMiddle(event, plane);
    }
    else if(event->button() == Qt::RightButton) {
        handleMouseButtonRight(event, plane);
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent *event) {
    if(state->viewerState->moveVP != -1)
        state->viewerState->moveVP = -1;
    if(state->viewerState->resizeVP != -1)
        state->viewerState->resizeVP = -1;

    for(int i = 0; i < state->viewerState->numberViewports; i++) {
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
        handleMouseWheelForward(event, plane);
    } else {
        handleMouseWheelBackward(event, plane);
    }

}

void Viewport::keyPressEvent(QKeyEvent *event) {
    qDebug() << "keypress";
    this->delegate->handleKeyboard(event, focus);
}

void Viewport::keyReleaseEvent(QKeyEvent *event) {

}

void Viewport::drawViewport(int plane) {
    reference->renderOrthogonalVP(plane);

}

void Viewport::drawSkeletonViewport() {
    reference->renderSkeletonVP(VIEWPORT_SKELETON);
}

bool Viewport::handleMouseButtonLeft(QMouseEvent *event, int VPfound) {
    return delegate->handleMouseButtonLeft(event, VPfound);
}

bool Viewport::handleMouseButtonMiddle(QMouseEvent *event, int VPfound) {
    return delegate->handleMouseButtonMiddle(event, VPfound);

}

bool Viewport::handleMouseButtonRight(QMouseEvent *event, int VPfound) {
    return delegate->handleMouseButtonRight(event, VPfound);
}


bool Viewport::handleMouseMotionLeftHold(QMouseEvent *event, int VPfound) {
    return delegate->handleMouseMotionLeftHold(event, VPfound);
}

bool Viewport::handleMouseMotionMiddleHold(QMouseEvent *event, int VPfound) {
    return delegate->handleMouseMotionMiddleHold(event, VPfound);
}

bool Viewport::handleMouseMotionRightHold(QMouseEvent *event, int VPfound) {
    return delegate->handleMouseMotionRightHold(event, VPfound);
}

bool Viewport::handleMouseWheelForward(QWheelEvent *event, int VPfound) {
    return delegate->handleMouseWheelForward(event, VPfound);
}

bool Viewport::handleMouseWheelBackward(QWheelEvent *event, int VPfound) {
    return delegate->handleMouseWheelBackward(event, VPfound);
}

void Viewport::enterEvent(QEvent *event) {
    entered = true;
    focus = this->plane;
    this->setCursor(Qt::CrossCursor);
}

void Viewport::paintEvent(QPaintEvent *event) {

}

void Viewport::leaveEvent(QEvent *event) {
    entered = false;    

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
                    if(!(state->viewerState->vpConfigs[i].texture.zoomLevel + step < 0.09999) &&
                       !(state->viewerState->vpConfigs[i].texture.zoomLevel + step > 1.0000)) {
                        state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                    }
                }
            }
        }
    }

   if(triggerMagChange)
        emit changeDatasetMagSignal(triggerMagChange);

   emit recalcTextureOffsetsSignal();

}

void Viewport::moveButtonClicked() {

}

void Viewport::resizeButtonClicked() {

}

void Viewport::xyButtonClicked() {

}

void Viewport::xzButtonClicked() {

}

void Viewport::yzButtonClicked() {

}

void Viewport::r90ButtonClicked() {

}

void Viewport::r180ButtonClicked() {

}

void Viewport::resetButtonClicked() {

}
