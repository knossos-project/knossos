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
#include "renderer.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

extern stateInfo *state;
extern stateInfo *tempConfig;

/**
  * @note The button group for skeleton viewport does not work as expected
  * another approach is needed
  */
Viewport::Viewport(QWidget *parent, int plane) :
    QGLWidget(parent) {
    delegate = new EventModel();
    this->plane = plane;
    /* per default the widget only receives move event when at least one mouse button is pressed
    to change this behaviour we need to track the mouse position */
    this->setMouseTracking(true);
    setStyleSheet("background:black");
    this->setCursor(Qt::CrossCursor);

    if(plane == VIEWPORT_SKELETON + 10) {
        this->xy = new QPushButton("xy");
        this->xz = new QPushButton("xz");
        this->xy = new QPushButton("yz");
        this->flip = new QPushButton("flip");
        this->reset = new QPushButton("reset");

        QVBoxLayout *mainLayout = new QVBoxLayout();
        QHBoxLayout *hLayout = new QHBoxLayout();

        hLayout->addWidget(xy);
        hLayout->addWidget(xz);
        hLayout->addWidget(yz);
        hLayout->addWidget(flip);
        hLayout->addWidget(reset);

        mainLayout->addLayout(hLayout);
        setLayout(mainLayout);
    }
}

void Viewport::initializeGL() {

    glEnable(GL_TEXTURE_2D);
    /*
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glEnable(GL_POLYGON_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */

    /* display some basic openGL driver statistics
    qDebug("OpenGL v%s on %s from %s\n", glGetString(GL_VERSION),
    glGetString(GL_RENDERER), glGetString(GL_VENDOR));
    */

}

void Viewport::resizeGL(int w, int h) {
    qDebug("resizing");
    glViewport(0, 0, width(), height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat x = (GLfloat)width() / height();
    glFrustum(-x, +x, -1.0, +1.0, 0.0, 15.0);
    glMatrixMode(GL_MODELVIEW);

    SET_COORDINATE(state->viewerState->vpConfigs[plane].upperLeftCorner,
                   geometry().topLeft().x(),
                   geometry().topLeft().y(),
                   0);
    state->viewerState->vpConfigs[plane].edgeLength = width();


}

/**
  * @Todo the case decision in paintGL is crap I know, it is just an adhoc test for rendering.
  */

void Viewport::paintGL() {
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glClearColor(0, 0, 0, 1);

    if(this->plane < VIEWPORT_SKELETON) {
        drawViewport(this->plane);
    } else
        drawSkeletonViewport();



}

//functions to determine position x/y relative to last position lastX, lastY
int Viewport::xrel(int x) {
    return (x - this->lastX);
}
int Viewport::yrel(int y) {
    return (y - this->lastY);
}

void Viewport::mouseMoveEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        handleMouseMotionLeftHold(event, plane);
    }
}

void Viewport::mousePressEvent(QMouseEvent *event) {
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
    handleKeyboard(event);
}

void Viewport::keyReleaseEvent(QKeyEvent *event) {

}

void Viewport::customEvent(QEvent *event) {
    if(event->type() == QEvent::User) {

    }
}

void Viewport::drawViewport(int plane) {
    Renderer::renderOrthogonalVP(plane);
}

void Viewport::drawSkeletonViewport() {
    Renderer::renderSkeletonVP(3);
}


bool Viewport::handleMouseButtonLeft(QMouseEvent *event, int32_t VPfound) {
    return delegate->handleMouseButtonLeft(event, VPfound);
}

bool Viewport::handleMouseButtonMiddle(QMouseEvent *event, int32_t VPfound) {
    return delegate->handleMouseButtonMiddle(event, VPfound);

}

bool Viewport::handleMouseButtonRight(QMouseEvent *event, int32_t VPfound) {
    return delegate->handleMouseButtonRight(event, VPfound);
}

bool Viewport::handleMouseMotion(QMouseEvent *event, int32_t VPfound) {}

bool Viewport::handleMouseMotionLeftHold(QMouseEvent *event, int32_t VPfound) {
    return delegate->handleMouseMotionLeftHold(event, VPfound);

}

bool Viewport::handleMouseMotionMiddleHold(QMouseEvent *event, int32_t VPfound) {
    return delegate->handleMouseMotionMiddleHold(event, VPfound);
}

bool Viewport::handleMouseMotionRightHold(QMouseEvent *event, int32_t VPfound) {
    return delegate->handleMouseMotionRightHold(event, VPfound);
}

bool Viewport::handleMouseWheelForward(QWheelEvent *event, int32_t VPfound) {
    return delegate->handleMouseWheelForward(event, VPfound);
}

bool Viewport::handleMouseWheelBackward(QWheelEvent *event, int32_t VPfound) {
    return delegate->handleMouseWheelBackward(event, VPfound);
}

bool Viewport::handleKeyboard(QKeyEvent *event) {
    return delegate->handleKeyboard(event);
}

Coordinate* Viewport::getCoordinateFromOrthogonalClick(QMouseEvent *event, int32_t VPfound) {

    Coordinate *foundCoordinate;
    foundCoordinate = static_cast<Coordinate*>(malloc(sizeof(Coordinate)));
    int32_t x, y, z;
    x = y = z = 0;

    // These variables store the distance in screen pixels from the left and
    // upper border from the user mouse click to the VP boundaries.
    uint32_t xDistance, yDistance;

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
