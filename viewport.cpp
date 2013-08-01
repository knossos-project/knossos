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

extern stateInfo *state;

/**
  * @note The button group for skeleton viewport does not work as expected
  * another approach is needed
  */
Viewport::Viewport(QWidget *parent, int viewportType, int widgetNumber) :
    QGLWidget(parent) {
    delegate = new EventModel();
    this->setMouseTracking(true); /* per default the widget only receives move event when at least one mouse button is pressed
    to change this behaviour we need to track the mouse position */

    this->viewportType = viewportType;
    this->plane = widgetNumber;

    //this->setMouseTracking(true);
    //this->setCursor(Qt::CrossCursor);
    this->setAutoBufferSwap(true);


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

    glFrustum(-x, +x, -2.0, + 2.0, 0.1, 15.0);
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
    }
    else {
        handleMouseWheelBackward(event, plane);
    }

}

void Viewport::keyPressEvent(QKeyEvent *event) {
    qDebug() << "keypress";
    this->delegate->handleKeyboard(event, this->plane);
}

void Viewport::keyReleaseEvent(QKeyEvent *event) {

}

void Viewport::customEvent(QEvent *event) {
    if(event->type() == QEvent::User) {

    }
}

void Viewport::drawViewport(int plane) {
    ref->renderOrthogonalVP(plane);

}

void Viewport::drawSkeletonViewport() {
    ref->renderSkeletonVP(VIEWPORT_SKELETON);
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

bool Viewport::handleKeyboard(QKeyEvent *event) {
    qDebug() << "key";
    return delegate->handleKeyboard(event, plane);
}

void Viewport::enterEvent(QEvent *event) {
    entered = true;

}

void Viewport::paintEvent(QPaintEvent *event) {

}

void Viewport::leaveEvent(QEvent *event) {
    entered = false;

}

Coordinate* Viewport::getCoordinateFromOrthogonalClick(QMouseEvent *event, int VPfound) {
    Coordinate *foundCoordinate;
    foundCoordinate = static_cast<Coordinate*>(malloc(sizeof(Coordinate)));
    int x, y, z;
    x = y = z = 0;

    // These variables store the distance in screen pixels from the left and
    // upper border from the user mouse click to the VP boundaries.
    uint xDistance, yDistance;

    if((VPfound == -1)
        || (state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON))
            return NULL;

    xDistance = event->x()
        - state->viewerState->vpConfigs[VPfound].upperLeftCorner.x;
    yDistance = event->y()
        - state->viewerState->vpConfigs[VPfound].upperLeftCorner.y;

    switch(state->viewerState->vpConfigs[VPfound].type) {
        case VIEWPORT_XY:
            x = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.x
                + ((int)(((float)xDistance)
                / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx));
            y = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.y
                + ((int)(((float)yDistance)
                / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx));
            z = state->viewerState->currentPosition.z;
            break;
        case VIEWPORT_XZ:
            x = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.x
                + ((int)(((float)xDistance)
                / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx));
            z = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.z
                + ((int)(((float)yDistance)
                / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx));
            y = state->viewerState->currentPosition.y;
            break;
        case VIEWPORT_YZ:
            z = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.z
                + ((int)(((float)xDistance)
                / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx));
            y = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.y
                + ((int)(((float)yDistance)
                / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx));
            x = state->viewerState->currentPosition.x;
            break;
    }
    //check if coordinates are in range
    if((x >= 0) && (x <= state->boundary.x)
        &&(y >= 0) && (y <= state->boundary.y)
        &&(z >= 0) && (z <= state->boundary.z)) {
        SET_COORDINATE((*foundCoordinate), x, y, z);
        return foundCoordinate;
    }
    return NULL;
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
