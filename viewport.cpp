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
    int32_t clickedNode;
    struct nodeListElement* newActiveNode;
    Coordinate *clickedCoordinate = NULL;

    //new active node selected
    if(event->modifiers() == Qt::ControlModifier) {
        qDebug("control and mouseleft");
        //first assume that user managed to hit the node
        clickedNode = Renderer::retrieveVisibleObjectBeneathSquare(VPfound,
                                                event->x(),
                                                (state->viewerState->screenSizeY - event->y()),
                                                10);
        if(clickedNode) {
            Skeletonizer::setActiveNode(CHANGE_MANUAL, NULL, clickedNode);
            return true;
        }

        if(VPfound == VIEWPORT_SKELETON) {
            return false;
        }

        //in other VPs we traverse nodelist to find nearest node inside the radius
        clickedCoordinate = getCoordinateFromOrthogonalClick(
                            event,
                            VPfound);
        if(clickedCoordinate) {
            newActiveNode = Skeletonizer::findNodeInRadius(*clickedCoordinate);
            if(newActiveNode != NULL) {
                Skeletonizer::setActiveNode(CHANGE_MANUAL, NULL, newActiveNode->nodeID);
                return true;
            }
        }
        return false;
    }

    // check in which type of VP the user clicked and perform appropriate operation
    if(state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON) {
        // Activate motion tracking for this VP
        state->viewerState->vpConfigs[VPfound].motionTracking = 1;
        return true;
    }
    else {
        // Handle the orthogonal viewports

        switch(state->viewerState->workMode) {
        case ON_CLICK_RECENTER:
            clickedCoordinate =
                getCoordinateFromOrthogonalClick(event,
                                                 VPfound);
            if(clickedCoordinate == NULL) {
                return true;
            }
            tempConfig->remoteState->type = REMOTE_RECENTERING;
            SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                           clickedCoordinate->x,
                           clickedCoordinate->y,
                           clickedCoordinate->z);
            Knossos::sendRemoteSignal();
            break;
        case ON_CLICK_DRAG:
            // Activate motion tracking for this VP
            state->viewerState->vpConfigs[VPfound].motionTracking = 1;
            break;
        }
    }
    //Set Connection between Active Node and Clicked Node
    if(event->modifiers() == Qt::AltModifier) {
        qDebug("alt and mouseleft");
        int32_t clickedNode;
        clickedNode = Renderer::retrieveVisibleObjectBeneathSquare(VPfound,
                                                     event->x(),
                                                     (state->viewerState->screenSizeY - event->y()),
                                                     1);
        if(clickedNode) {
            if(state->skeletonState->activeNode) {
                Skeletonizer::addSegment(CHANGE_MANUAL,
                           state->skeletonState->activeNode->nodeID,
                           clickedNode);
            }
        }
    }
    return true;
}

bool Viewport::handleMouseButtonMiddle(QMouseEvent *event, int32_t VPfound) {
    int32_t clickedNode;

    clickedNode = Renderer::retrieveVisibleObjectBeneathSquare(VPfound,
                                                     event->x(),
                                                     (state->viewerState->screenSizeY - event->y()),
                                                     1);
    if(!clickedNode) {
        return true;
    }
    if(event->modifiers() == Qt::ShiftModifier) {
        // Delete segment between clicked and active node
        if(state->skeletonState->activeNode == false) {
            return true;
        }
        if(Skeletonizer::findSegmentByNodeIDs(state->skeletonState->activeNode->nodeID, clickedNode)) {
            Skeletonizer::delSegment(CHANGE_MANUAL,
                                     state->skeletonState->activeNode->nodeID,
                                     clickedNode,
                                     NULL);
        }
        if(Skeletonizer::findSegmentByNodeIDs(clickedNode, state->skeletonState->activeNode->nodeID)) {
            Skeletonizer::delSegment(CHANGE_MANUAL,
                                     clickedNode,
                                     state->skeletonState->activeNode->nodeID,
                                     NULL);
        }
    }
    else if(event->modifiers() == Qt::ControlModifier) {
        if(state->skeletonState->activeNode == false) {
            return true;
        }
        Skeletonizer::addSegment(CHANGE_MANUAL,
                       state->skeletonState->activeNode->nodeID,
                       clickedNode);
    }
    else {
        state->viewerState->vpConfigs[VPfound].draggedNode =
            Skeletonizer::findNodeByNodeID(clickedNode);
        state->viewerState->vpConfigs[VPfound].motionTracking = 1;
    }
    return true;
}

bool Viewport::handleMouseButtonRight(QMouseEvent *event, int32_t VPfound) {
    int32_t newNodeID;
    Coordinate *clickedCoordinate = NULL, movement, lastPos;

    // We have to activate motion tracking only for the skeleton VP for a right click
    if(state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON) {
        state->viewerState->vpConfigs[VPfound].motionTracking = true;
    }
    // If not, we look up which skeletonizer work mode is
    // active and do the appropriate operation
    clickedCoordinate = getCoordinateFromOrthogonalClick(event, VPfound);

    // could not find any coordinate...
    if(clickedCoordinate == NULL) {
            return true;
    }
    switch(state->skeletonState->workMode) {
    case SKELETONIZER_ON_CLICK_DROP_NODE:
        // function is inside skeletonizer.c
        Skeletonizer::UI_addSkeletonNode(clickedCoordinate,
                                         state->viewerState->vpConfigs[VPfound].type);
        break;
    case SKELETONIZER_ON_CLICK_ADD_NODE:
        // function is inside skeletonizer.c
        Skeletonizer::UI_addSkeletonNode(clickedCoordinate,
                                         state->viewerState->vpConfigs[VPfound].type);
        tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE;
        break;
    case SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE:
        // function is inside skeletonizer.c
        if(state->skeletonState->activeNode == NULL) {
            Skeletonizer::UI_addSkeletonNode(clickedCoordinate,
                               state->viewerState->vpConfigs[VPfound].type);
        }
        if(event->modifiers() == Qt::ControlModifier) {
            // Add a "stump", a branch node to which we don't automatically move.
            if((newNodeID = Skeletonizer::UI_addSkeletonNodeAndLinkWithActive(clickedCoordinate,
                                                                            state->viewerState->vpConfigs[VPfound].type,
                                                                            false))) {
            Skeletonizer::pushBranchNode(CHANGE_MANUAL, true, true, NULL, newNodeID);
            }
        }
        else {
            lastPos.x = state->skeletonState->activeNode->position.x;
            lastPos.y = state->skeletonState->activeNode->position.y;
            lastPos.z = state->skeletonState->activeNode->position.z;

            if(Skeletonizer::UI_addSkeletonNodeAndLinkWithActive(clickedCoordinate,
                                                                 state->viewerState->vpConfigs[VPfound].type,
                                                                 true)
               == false) {
                break;
            }
            // Highlight the viewport with the biggest movement component and set
            //  behavior of f / d keys accordingly.
            movement.x = clickedCoordinate->x - lastPos.x;
            movement.y = clickedCoordinate->y - lastPos.y;
            movement.z = clickedCoordinate->z - lastPos.z;

            if (state->viewerState->autoTracingDelay < 10) { state->viewerState->autoTracingDelay = 10; }
            else if (state->viewerState->autoTracingDelay > 500) { state->viewerState->autoTracingDelay = 500; }
            if (state->viewerState->autoTracingSteps < 1) { state->viewerState->autoTracingSteps = 1; }
            else if (state->viewerState->autoTracingSteps > 50) { state->viewerState->autoTracingSteps = 50; }

            //AutoTracingModes 2 and 3
            if ((state->viewerState->autoTracingMode == AUTOTRACING_TRACING)
                || (state->viewerState->autoTracingMode == AUTOTRACING_MIRROR)) {
                floatCoordinate walkingVector;
                walkingVector.x = movement.x;
                walkingVector.y = movement.y;
                walkingVector.z = movement.z;

                if (state->viewerState->autoTracingMode == AUTOTRACING_TRACING){
                    clickedCoordinate->x += Renderer::roundFloat(walkingVector.x * state->viewerState->autoTracingSteps / Renderer::euclidicNorm(&walkingVector));
                    clickedCoordinate->y += Renderer::roundFloat(walkingVector.y * state->viewerState->autoTracingSteps / Renderer::euclidicNorm(&walkingVector));
                    clickedCoordinate->z += Renderer::roundFloat(walkingVector.z * state->viewerState->autoTracingSteps / Renderer::euclidicNorm(&walkingVector));
                }
                if (state->viewerState->autoTracingMode == AUTOTRACING_MIRROR){
                    clickedCoordinate->x += walkingVector.x;
                    clickedCoordinate->y += walkingVector.y;
                    clickedCoordinate->z += walkingVector.z;
                }
            }

            // Determine which viewport to highlight based on which movement component is biggest.
            if((abs(movement.x) >= abs(movement.y)) && (abs(movement.x) >= abs(movement.z))) {
                state->viewerState->highlightVp = VIEWPORT_YZ;
            }
            if((abs(movement.y) >= abs(movement.x)) && (abs(movement.y) >= abs(movement.z))) {
                state->viewerState->highlightVp = VIEWPORT_XZ;
            }
            if((abs(movement.z) >= abs(movement.x)) && (abs(movement.z) >= abs(movement.y))) {
                state->viewerState->highlightVp = VIEWPORT_XY;
            }

            // Determine the directions for the f and d keys based on the signs of the movement
            //   components along the three dimensions.
            if(movement.x >= 0) {
                state->viewerState->vpKeyDirection[VIEWPORT_YZ] = 1;
            }
            else {
                state->viewerState->vpKeyDirection[VIEWPORT_YZ] = -1;
            }
            if(movement.y >= 0) {
                state->viewerState->vpKeyDirection[VIEWPORT_XZ] = 1;
            }
            else {
                state->viewerState->vpKeyDirection[VIEWPORT_XZ] = -1;
            }
            if(movement.z >= 0) {
                state->viewerState->vpKeyDirection[VIEWPORT_XY] = 1;
            }
            else {
                state->viewerState->vpKeyDirection[VIEWPORT_XY] = -1;
            }

            //AutoTracingMode 1
            if (state->viewerState->autoTracingMode == AUTOTRACING_VIEWPORT){
                switch(state->viewerState->vpConfigs[VPfound].type) {
                case VIEWPORT_XY:
                    if (state->viewerState->vpKeyDirection[VIEWPORT_XY] == 1){
                        clickedCoordinate->z += state->viewerState->autoTracingSteps;
                    }
                    else if (state->viewerState->vpKeyDirection[VIEWPORT_XY] == -1){
                                        clickedCoordinate->z -= state->viewerState->autoTracingSteps;
                    }
                    break;
                case VIEWPORT_XZ:
                    if (state->viewerState->vpKeyDirection[VIEWPORT_XZ] == 1){
                        clickedCoordinate->y += state->viewerState->autoTracingSteps;
                    }
                    else if (state->viewerState->vpKeyDirection[VIEWPORT_XZ] == -1){
                        clickedCoordinate->y -= state->viewerState->autoTracingSteps;
                    }
                    break;
                case VIEWPORT_YZ:
                    if (state->viewerState->vpKeyDirection[VIEWPORT_YZ] == 1){
                        clickedCoordinate->x += state->viewerState->autoTracingSteps;
                        }
                    else if (state->viewerState->vpKeyDirection[VIEWPORT_YZ] == -1){
                        clickedCoordinate->x -= state->viewerState->autoTracingSteps;
                    }
                    break;
                }
            }

            if (clickedCoordinate->x < 0) { clickedCoordinate->x = 0; }
            if (clickedCoordinate->y < 0) { clickedCoordinate->y = 0; }
            if (clickedCoordinate->z < 0) { clickedCoordinate->z = 0; }

            // Do not allow clicks outside the dataset/
            if (clickedCoordinate->x > state->boundary.x) {
                clickedCoordinate->x = state->boundary.x;
            }
            if (clickedCoordinate->y > state->boundary.y) {
                clickedCoordinate->y = state->boundary.y;
            }
            if (clickedCoordinate->z > state->boundary.z) {
                clickedCoordinate->z = state->boundary.z;
            }
            // Move to the new node position
            tempConfig->remoteState->type = REMOTE_RECENTERING;
            SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                           clickedCoordinate->x,
                           clickedCoordinate->y,
                           clickedCoordinate->z);
            Viewer::updateViewerState();
            Knossos::sendRemoteSignal();
        }
    }
    return true;
}

bool Viewport::handleMouseMotion(QMouseEvent *event, int32_t VPfound) {}

bool Viewport::handleMouseMotionLeftHold(QMouseEvent *event, int32_t VPfound) {
    uint32_t i;
    for(i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].motionTracking == false) {
            continue;
        }
        // motion tracking mode is active for viewport i
        switch(state->viewerState->vpConfigs[i].type) {
        // the user wants to drag the skeleton inside the VP
        case VIEWPORT_SKELETON:
            state->skeletonState->translateX += -xrel(event->x()) * 2.
                * ((float)state->skeletonState->volBoundary
                * (0.5 - state->skeletonState->zoomLevel))
                / ((float)state->viewerState->vpConfigs[i].edgeLength);
            state->skeletonState->translateY += -yrel(event->y()) * 2.
                * ((float)state->skeletonState->volBoundary
                * (0.5 - state->skeletonState->zoomLevel))
                / ((float)state->viewerState->vpConfigs[i].edgeLength);
            Remote::checkIdleTime();
            break;
        case VIEWPORT_XY:
            if(state->viewerState->workMode != ON_CLICK_DRAG) {
                break;
            }
            state->viewerState->vpConfigs[i].userMouseSlideX -=
                ((float)xrel(event->x())
                / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
            state->viewerState->vpConfigs[i].userMouseSlideY -=
                ((float)yrel(event->y())
                / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
            if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
               || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {
                Viewer::userMove((int)state->viewerState->vpConfigs[i].userMouseSlideX,
                                 (int)state->viewerState->vpConfigs[i].userMouseSlideY, 0,
                                 TELL_COORDINATE_CHANGE);
                state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                }
            break;
        case VIEWPORT_XZ:
            if(state->viewerState->workMode != ON_CLICK_DRAG) {
                break;
            }
            state->viewerState->vpConfigs[i].userMouseSlideX -=
                ((float)xrel(event->x()) / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
            state->viewerState->vpConfigs[i].userMouseSlideY -=
                ((float)yrel(event->y()) / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
            if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
               || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {
                Viewer::userMove((int)state->viewerState->vpConfigs[i].userMouseSlideX, 0,
                                 (int)state->viewerState->vpConfigs[i].userMouseSlideY,
                                 TELL_COORDINATE_CHANGE);
                state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
            }
            break;
        case VIEWPORT_YZ:
            if(state->viewerState->workMode != ON_CLICK_DRAG) {
                break;
            }
            state->viewerState->vpConfigs[i].userMouseSlideX -=
                ((float)xrel(event->x()) / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
            state->viewerState->vpConfigs[i].userMouseSlideY -=
                ((float)yrel(event->y()) / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
            if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
               || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {
                Viewer::userMove(0, (int)state->viewerState->vpConfigs[i].userMouseSlideY,
                                 (int)state->viewerState->vpConfigs[i].userMouseSlideX,
                                 TELL_COORDINATE_CHANGE);
                state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
            }
            break;
        }
        return true;
    }
    return true;
}

bool Viewport::handleMouseMotionMiddleHold(QMouseEvent *event, int32_t VPfound) {
    uint32_t i = 0;
    Coordinate newDraggedNodePos;

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        switch(state->viewerState->vpConfigs[i].type) {
        case VIEWPORT_XY:
            if(!state->viewerState->vpConfigs[i].draggedNode) {
                break;
            }
            state->viewerState->vpConfigs[i].userMouseSlideX -=
                    ((float)xrel(event->x())
                    / state->viewerState->vpConfigs[i].screenPxXPerDataPx);

            state->viewerState->vpConfigs[i].userMouseSlideY -=
                    ((float)yrel(event->y())
                    / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
            if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
               || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {
                SET_COORDINATE(newDraggedNodePos,
                               state->viewerState->vpConfigs[i].userMouseSlideX,
                               state->viewerState->vpConfigs[i].userMouseSlideY, 0);
                state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                newDraggedNodePos.x =
                        state->viewerState->vpConfigs[i].draggedNode->position.x
                        - newDraggedNodePos.x;
                newDraggedNodePos.y =
                        state->viewerState->vpConfigs[i].draggedNode->position.y
                        - newDraggedNodePos.y;
                newDraggedNodePos.z =
                        state->viewerState->vpConfigs[i].draggedNode->position.z
                        - newDraggedNodePos.z;

                Skeletonizer::editNode(CHANGE_MANUAL,
                                       0,
                                       state->viewerState->vpConfigs[i].draggedNode,
                                       0.,
                                       newDraggedNodePos.x,
                                       newDraggedNodePos.y,
                                       newDraggedNodePos.z,
                                       state->magnification);
            }
            break;
        case VIEWPORT_XZ:
            if(!state->viewerState->vpConfigs[i].draggedNode) { break;
            }
            state->viewerState->vpConfigs[i].userMouseSlideX -=
                        ((float)xrel(event->x())
                        / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
            state->viewerState->vpConfigs[i].userMouseSlideY -=
                        ((float)yrel(event->y())
                        / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
            if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
               || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {
                SET_COORDINATE(newDraggedNodePos,
                               state->viewerState->vpConfigs[i].userMouseSlideX, 0,
                               state->viewerState->vpConfigs[i].userMouseSlideY);
                state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                newDraggedNodePos.x =
                        state->viewerState->vpConfigs[i].draggedNode->position.x
                        - newDraggedNodePos.x;
                newDraggedNodePos.y =
                        state->viewerState->vpConfigs[i].draggedNode->position.y
                        - newDraggedNodePos.y;
                newDraggedNodePos.z =
                        state->viewerState->vpConfigs[i].draggedNode->position.z
                        - newDraggedNodePos.z;
                Skeletonizer::editNode(CHANGE_MANUAL,
                                       0,
                                       state->viewerState->vpConfigs[i].draggedNode,
                                       0.,
                                       newDraggedNodePos.x,
                                       newDraggedNodePos.y,
                                       newDraggedNodePos.z,
                                       state->magnification);
            }
            break;
        case VIEWPORT_YZ:
            if(!state->viewerState->vpConfigs[i].draggedNode) {
                break;
            }
            state->viewerState->vpConfigs[i].userMouseSlideX -=
                        ((float)xrel(event->x())
                        / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
            state->viewerState->vpConfigs[i].userMouseSlideY -=
                        ((float)yrel(event->y())
                        / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
            if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
               || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {
                SET_COORDINATE(newDraggedNodePos,
                               0, state->viewerState->vpConfigs[i].userMouseSlideY,
                               state->viewerState->vpConfigs[i].userMouseSlideX);

                state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                newDraggedNodePos.x =
                        state->viewerState->vpConfigs[i].draggedNode->position.x
                        - newDraggedNodePos.x;
                newDraggedNodePos.y =
                        state->viewerState->vpConfigs[i].draggedNode->position.y
                        - newDraggedNodePos.y;
                newDraggedNodePos.z =
                        state->viewerState->vpConfigs[i].draggedNode->position.z
                        - newDraggedNodePos.z;
                Skeletonizer::editNode(CHANGE_MANUAL,
                                       0,
                                       state->viewerState->vpConfigs[i].draggedNode,
                                       0.,
                                       newDraggedNodePos.x,
                                       newDraggedNodePos.y,
                                       newDraggedNodePos.z,
                                       state->magnification);
            }
            break;
        }
    }
    return true;
}

bool Viewport::handleMouseMotionRightHold(QMouseEvent *event, int32_t VPfound) {
    if((state->viewerState->vpConfigs[VIEWPORT_SKELETON].motionTracking == true)
        && (state->skeletonState->rotationcounter == 0)) {
        state->skeletonState->rotdx += xrel(event->x());
        state->skeletonState->rotdy += yrel(event->y());
        state->skeletonState->viewChanged = true;
        Remote::checkIdleTime();
        return true;
    }
    return true;
}

bool Viewport::handleMouseWheelForward(QWheelEvent *event, int32_t VPfound) {
    float radius;

    if(VPfound == -1) {
        return true;
    }
    if((state->skeletonState->activeNode) && ((event->modifiers() == Qt::ShiftModifier))) {
        radius = state->skeletonState->activeNode->radius - 0.2 * state->skeletonState->activeNode->radius;

        Skeletonizer::editNode(CHANGE_MANUAL,
                 0,
                 state->skeletonState->activeNode,
                 radius,
                 state->skeletonState->activeNode->position.x,
                 state->skeletonState->activeNode->position.y,
                 state->skeletonState->activeNode->position.z,
                 state->magnification);

        // !! AG
        //if(state->viewerState->gui->useLastActNodeRadiusAsDefault)
        //    state->skeletonState->defaultNodeRadius = radius;

        //drawGUI();
    }
    else {
        // Skeleton VP
        if(state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON) {
            if (state->skeletonState->zoomLevel <= SKELZOOMMAX){
                state->skeletonState->zoomLevel += (0.1 * (0.5 - state->skeletonState->zoomLevel));
                state->skeletonState->viewChanged = true;
            }
        }
        // Orthogonal VP or outside VP
        else {
            // Zoom when CTRL is pressed
            if(event->modifiers() == Qt::ControlModifier) {
                //MainWindow::UI_zoomOrthogonals(-0.1); TODO UI_zoomOrtho
            }
            // Move otherwise
            else {
                switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, 0, state->viewerState->dropFrames
                                     * state->magnification,
                                     TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, state->viewerState->dropFrames
                                     * state->magnification, 0,
                                     TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(state->viewerState->dropFrames
                                     * state->magnification, 0, 0,
                                     TELL_COORDINATE_CHANGE);
                    break;
                }
            }
        }
    }
    return true;
}

bool Viewport::handleMouseWheelBackward(QWheelEvent *event, int32_t VPfound) {
    float radius;

    if(VPfound == -1) {
        return true;
    }
    if((state->skeletonState->activeNode) && ((event->modifiers() == Qt::ShiftModifier))) {
        radius = state->skeletonState->activeNode->radius + 0.2 * state->skeletonState->activeNode->radius;
        Skeletonizer::editNode(CHANGE_MANUAL,
                 0,
                 state->skeletonState->activeNode,
                 radius,
                 state->skeletonState->activeNode->position.x,
                 state->skeletonState->activeNode->position.y,
                 state->skeletonState->activeNode->position.z,
                 state->magnification);

        //
        //  !! AG
        //if(state->viewerState->gui->useLastActNodeRadiusAsDefault)
       //     state->skeletonState->defaultNodeRadius = radius;


        //drawGUI();
    }
    else {
        // Skeleton VP
        if(state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON) {
            if (state->skeletonState->zoomLevel >= SKELZOOMMIN) {
                state->skeletonState->zoomLevel -= (0.2* (0.5 - state->skeletonState->zoomLevel));
                if (state->skeletonState->zoomLevel < SKELZOOMMIN) { state->skeletonState->zoomLevel = SKELZOOMMIN;}
                state->skeletonState->viewChanged = true;
            }
        }
        // Orthogonal VP or outside VP
        else {
            // Zoom when CTRL is pressed
            if(event->modifiers() == Qt::ControlModifier) {
                qDebug("control and mouse wheel down");
                //UI_zoomOrthogonals(0.1); TODO UI_zoomOrtho
            }
            // Move otherwise
            else {
                switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, 0, -state->viewerState->dropFrames
                                     * state->magnification,
                                     TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, -state->viewerState->dropFrames
                                     * state->magnification, 0,
                                     TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(-state->viewerState->dropFrames
                                     * state->magnification, 0, 0,
                                     TELL_COORDINATE_CHANGE);
                    break;
                }
            }
        }
    }
    return true;
}

bool Viewport::handleKeyboard(QKeyEvent *event) {

    // This is a workaround for agars behavior of processing / labeling
    //events. Without it, input into a textbox would still trigger global
    //shortcut actions.

    struct treeListElement *prevTree;
    struct treeListElement *nextTree;
    struct nodeListElement *prevNode;
    color4F treeCol;

    // new qt version
    if(event->key() == Qt::Key_Left) {
        if(event->key() == Qt::Key_Shift) {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(-10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(-10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(0, 0, -10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(-state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(-state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(0, 0, -state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
    } else if(event->key() == Qt::RightArrow) {
            if(event->key() == Qt::Key_Shift) {
                switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                    case VIEWPORT_XY:
                        Viewer::userMove(10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_XZ:
                        Viewer::userMove(10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_YZ:
                        Viewer::userMove(0, 0, 10 * state->magnification, TELL_COORDINATE_CHANGE);
                        break;
                }
            } else {
                switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                    case VIEWPORT_XY:
                        Viewer::userMove(state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_XZ:
                        Viewer::userMove(state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_YZ:
                        Viewer::userMove(0, 0, state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                        break;
                    }
                }

    } else if(event->key() == Qt::Key_Down) {
        if(event->key() == Qt::Key_Shift) {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, -10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, 0, -10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(0, -10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, -state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, 0, -state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(0, -state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
    } else if(event->key() == Qt::Key_Up) {
        if(event->key() == Qt::Key_Shift) {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, 10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, 0, 10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(0, 10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, 0, state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(0, state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
    } else if(event->key() == Qt::Key_R) {
        state->viewerState->walkOrth = 1;
        switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
            case VIEWPORT_XY:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z += 10  * state->magnification);
                               Knossos::sendRemoteSignal();
            break;
            case VIEWPORT_XZ:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y += 10 * state->magnification,
                               tempConfig->viewerState->currentPosition.z);
                               Knossos::sendRemoteSignal();
            break;
            case VIEWPORT_YZ:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x += 10 * state->magnification,
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z);
                               Knossos::sendRemoteSignal();
            break;
        }
    } else if(event->key() == Qt::Key_E) {
        state->viewerState->walkOrth = 1;
        switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
            case VIEWPORT_XY:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z -= 10 * state->magnification);
                               Knossos::sendRemoteSignal();
            break;
            case VIEWPORT_XZ:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y -= 10 * state->magnification,
                               tempConfig->viewerState->currentPosition.z);
                               Knossos::sendRemoteSignal();
            break;
            case VIEWPORT_YZ:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x -= 10 * state->magnification,
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z);
                               Knossos::sendRemoteSignal();
            break;
        }
    } else if(event->key() == Qt::Key_F) {
        if(event->key() == Qt::Key_Shift) {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * 10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * 10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * 10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }

        }
    } else if(event->key() == Qt::Key_D) {
        if(event->key() == Qt::Key_Shift) {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * -10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * -10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * -10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            switch(state->viewerState->vpConfigs[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    Viewer::userMove(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * -state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    Viewer::userMove(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * -state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    Viewer::userMove(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * -state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
    } else if(event->key() == Qt::Key_G) {

    } else if(event->key() == Qt::Key_N) {
        if(event->key() == Qt::Key_Shift) {
            Skeletonizer::nextCommentlessNode();
        } else {
            Skeletonizer::nextComment(state->viewerState->gui->commentSearchBuffer);
        }
    } else if(event->key() == Qt::Key_P) {
        if(event->key() == Qt::Key_Shift) {
            Skeletonizer::previousCommentlessNode();
        } else {
            Skeletonizer::previousComment(state->viewerState->gui->commentSearchBuffer);
        }
    } else if(event->key() == Qt::Key_3) {
        if(state->viewerState->drawVPCrosshairs) {
           state->viewerState->drawVPCrosshairs = false;
        }
        else {
           state->viewerState->drawVPCrosshairs = true;
        }
    } else if(event->key() == Qt::Key_J) {
        Skeletonizer::UI_popBranchNode();
    } else if(event->key() == Qt::Key_B) {
        Skeletonizer::pushBranchNode(CHANGE_MANUAL, true, true, state->skeletonState->activeNode, 0);
    } else if(event->key() == Qt::Key_X) {
        if(event->key() == Qt::Key_Shift) {
            prevNode = Skeletonizer::getNodeWithPrevID(state->skeletonState->activeNode);
            if(prevNode) {
                Skeletonizer::setActiveNode(CHANGE_MANUAL, prevNode, prevNode->nodeID);
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               prevNode->position.x,
                               prevNode->position.y,
                               prevNode->position.z);
                Knossos::sendRemoteSignal();
            }
            else {
                LOG("Reached first node.");
            }
        }
    } else if(event->key() == Qt::Key_Z) {
        if(state->skeletonState->activeTree == NULL) {
            return true;
        }

        //get tree with previous ID
        if(event->key() == Qt::Key_Shift) {
            prevTree = Skeletonizer::getTreeWithPrevID(state->skeletonState->activeTree);
            if(prevTree) {
                if(Skeletonizer::setActiveTreeByID(prevTree->treeID)) {
                    Skeletonizer::setActiveNode(CHANGE_MANUAL, prevTree->firstNode, prevTree->firstNode->nodeID);
                    SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               prevTree->firstNode->position.x,
                               prevTree->firstNode->position.y,
                               prevTree->firstNode->position.z);
                    Knossos::sendRemoteSignal();
                }
            } else {
                LOG("Reached first tree.");
            }

        } else {
            //get tree with next ID
            nextTree = Skeletonizer::getTreeWithNextID(state->skeletonState->activeTree);
            if(nextTree) {
                if(Skeletonizer::setActiveTreeByID(nextTree->treeID)) {
                    Skeletonizer::setActiveNode(CHANGE_MANUAL, nextTree->firstNode, nextTree->firstNode->nodeID);
                    SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                                   nextTree->firstNode->position.x,
                                   nextTree->firstNode->position.y,
                                   nextTree->firstNode->position.z);
                    Knossos::sendRemoteSignal();
                }
            }
            else {
                LOG("Reached last tree.");
            }
        }

    } else if(event->key() == Qt::Key_I) {
        if (state->viewerState->gui->zoomSkeletonViewport == false){
           // UI_zoomOrthogonals(-0.1); TODO UI_zoomOrtho
        }
        else if (state->skeletonState->zoomLevel <= SKELZOOMMAX){
            state->skeletonState->zoomLevel += (0.1 * (0.5 - state->skeletonState->zoomLevel));
            state->skeletonState->viewChanged = true;
        }
    } else if(event->key() == Qt::Key_O) {
        if (state->viewerState->gui->zoomSkeletonViewport == false){
            //UI_zoomOrthogonals(0.1); TODO UI_zoomOrtho
        }
        else if (state->skeletonState->zoomLevel >= SKELZOOMMIN) {
            state->skeletonState->zoomLevel -= (0.2* (0.5 - state->skeletonState->zoomLevel));
            if (state->skeletonState->zoomLevel < SKELZOOMMIN) state->skeletonState->zoomLevel = SKELZOOMMIN;
            state->skeletonState->viewChanged = true;
        }
    } else if(event->key() == Qt::Key_S) {
        if(event->key() == Qt::Key_Control) {
            //saveSkelCallback(NULL);
            return true;
        }
        if(state->skeletonState->activeNode) {
            tempConfig->viewerState->currentPosition.x =
                state->skeletonState->activeNode->position.x;
            tempConfig->viewerState->currentPosition.y =
                state->skeletonState->activeNode->position.y;
            tempConfig->viewerState->currentPosition.z =
                state->skeletonState->activeNode->position.z;
            Viewer::updatePosition(TELL_COORDINATE_CHANGE);
        }
    } else if(event->key() == Qt::Key_A) {
        //UI_workModeAdd(); TODO UI_workModeAdd
    } else if(event->key() == Qt::Key_W) {
        // UI_workModeLink(); TODO UI_workModeLink
    } else if(event->key() == Qt::Key_C) {
        treeCol.r = -1.;
        Skeletonizer::addTreeListElement(true, CHANGE_MANUAL, 0, treeCol);
        tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
    } else if(event->key() == Qt::Key_V) {
       if(event->key() == Qt::Key_Shift) {
            //MainWindow::pasteClipboardCoordinates();
       }
    } else if(event->key() == Qt::Key_1) {
        if(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE) {
            state->skeletonState->displayMode &= ~DSP_SLICE_VP_HIDE;
            state->viewerState->gui->enableOrthoSkelOverlay = 1;
        }
        else {
            state->skeletonState->displayMode |= DSP_SLICE_VP_HIDE;
            state->viewerState->gui->enableOrthoSkelOverlay = 0;
        }
        state->skeletonState->skeletonChanged = true;
        Renderer::drawGUI();
    } else if(event->key() == Qt::Key_Delete) {
        Skeletonizer::delActiveNode();
    } else if(event->key() == Qt::Key_F1) {
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment1, "", 1) != 0)){
            Skeletonizer::addComment(CHANGE_MANUAL, state->viewerState->gui->comment1, state->skeletonState->activeNode, 0);
        }
        else{
            Skeletonizer::editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment1, state->skeletonState->activeNode, 0);
        }
    } else if(event->key() == Qt::Key_F2) {
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment2, "", 1) != 0)){
            Skeletonizer::addComment(CHANGE_MANUAL, state->viewerState->gui->comment2, state->skeletonState->activeNode, 0);
        }
        else{
            Skeletonizer::editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment2, state->skeletonState->activeNode, 0);
        }
    } else if(event->key() == Qt::Key_F3) {
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment3, "", 1) != 0)){
            Skeletonizer::addComment(CHANGE_MANUAL, state->viewerState->gui->comment3, state->skeletonState->activeNode, 0);
        }
        else{
            Skeletonizer::editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment3, state->skeletonState->activeNode, 0);
        }
    } else if(event->key() == Qt::Key_F4) {
        if(event->key() == Qt::Key_Alt) {
            if(state->skeletonState->unsavedChanges) {
                //yesNoPrompt(NULL, "There are unsaved changes. Really quit?", quitKnossos, NULL);
            }
            else {
                //Knossos::quitKnossos(); TODO quitKnossos
            }
        } else {
            if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment4, "", 1) != 0)){
                Skeletonizer::addComment(CHANGE_MANUAL, state->viewerState->gui->comment4, state->skeletonState->activeNode, 0);
            }
            else{
                Skeletonizer::editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment4, state->skeletonState->activeNode, 0);
            }
        }
    } else if(event->key() == Qt::Key_F5) {
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment5, "", 1) != 0)){
            Skeletonizer::addComment(CHANGE_MANUAL, state->viewerState->gui->comment5, state->skeletonState->activeNode, 0);
        }
        else{
            Skeletonizer::editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment5, state->skeletonState->activeNode, 0);
        }
    }
    return true;
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
