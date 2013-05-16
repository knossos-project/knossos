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

#include "eventmodel.h"
#include "mainwindow.h"
#include "functions.h"
#include "knossos.h"
#include "skeletonizer.h"


extern struct stateInfo *state;
extern struct stateInfo *tempConfig;

EventModel::EventModel(QObject *parent) :
    QObject(parent)
{
}

bool EventModel::handleMouseButtonLeft(QMouseEvent *event, int32_t VPfound)
{

    int32_t clickedNode;
    struct nodeListElement* newActiveNode;
    Coordinate *clickedCoordinate = NULL;

    //new active node selected
    if(QApplication::keyboardModifiers() == Qt::ControlModifier) {
        qDebug("control and mouseleft");
        //first assume that user managed to hit the node
        clickedNode = Renderer::retrieveVisibleObjectBeneathSquare(VPfound,
                                                event->x(),
                                                (state->viewerState->screenSizeY - event->y()),
                                                10);
        if(clickedNode) {
            emit setActiveNodeSignal(CHANGE_MANUAL, NULL, clickedNode);
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
                    emit setActiveNodeSignal(CHANGE_MANUAL, NULL, newActiveNode->nodeID);
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
                if(clickedCoordinate == NULL)
                    return true;

                emit setRemoteStateTypeSignal(REMOTE_RECENTERING);
                emit setRecenteringPositionSignal(clickedCoordinate->x, clickedCoordinate->y, clickedCoordinate->z);
                Knossos::sendRemoteSignal();

                break;

            case ON_CLICK_DRAG:
                // Activate motion tracking for this VP
                state->viewerState->vpConfigs[VPfound].motionTracking = 1;
                break;
            }
    }


    //Set Connection between Active Node and Clicked Node
    if(QApplication::keyboardModifiers() == Qt::AltModifier) {
        qDebug("alt and mouseleft");
        int32_t clickedNode;
        clickedNode = Renderer::retrieveVisibleObjectBeneathSquare(VPfound,
                                                     event->x(),
                                                     (state->viewerState->screenSizeY - event->y()),
                                                     10);
        if(clickedNode) {
            if(state->skeletonState->activeNode) {
                if(Skeletonizer::findSegmentByNodeIDs(state->skeletonState->activeNode->nodeID, clickedNode)) {
                    emit delSegmentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, clickedNode, NULL);
                } else if(Skeletonizer::findSegmentByNodeIDs(clickedNode, state->skeletonState->activeNode->nodeID)) {
                    emit delSegmentSignal(CHANGE_MANUAL, clickedNode, state->skeletonState->activeNode->nodeID, NULL);
                } else{
                    emit addSegmentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, clickedNode);
                }
            }
        }
    }

    return true;
}

bool EventModel::handleMouseButtonMiddle(QMouseEvent *event, int32_t VPfound) {

    int32_t clickedNode;

    clickedNode = Renderer::retrieveVisibleObjectBeneathSquare(VPfound,
                                                     event->x(),
                                                     (state->viewerState->screenSizeY - event->y()),
                                                     1);


    if(clickedNode) {
        Qt::KeyboardModifiers keyMod = QApplication::keyboardModifiers();
        if(keyMod.testFlag(Qt::ShiftModifier)) {
            if(keyMod.testFlag(Qt::ControlModifier)) {
                qDebug("shift and control and mouse middle");
                // Pressed SHIFT and CTRL
            } else {
                qDebug("shift and mouse middle");
                // Pressed SHIFT only.

                // Delete segment between clicked and active node
                if(state->skeletonState->activeNode) {
                    if(Skeletonizer::findSegmentByNodeIDs(state->skeletonState->activeNode->nodeID,
                                            clickedNode)) {
                        emit delSegmentSignal(CHANGE_MANUAL,
                                   state->skeletonState->activeNode->nodeID,
                                   clickedNode,
                                   NULL);
                    } else if(Skeletonizer::findSegmentByNodeIDs(clickedNode,
                                            state->skeletonState->activeNode->nodeID)) {
                        emit delSegmentSignal(CHANGE_MANUAL,
                                   clickedNode,
                                   state->skeletonState->activeNode->nodeID,
                                   NULL);
                    } else {
                        emit addSegmentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, clickedNode);
                    }
                }
            }
        } else {
            // No modifier pressed
            state->viewerState->vpConfigs[VPfound].draggedNode =
                Skeletonizer::findNodeByNodeID(clickedNode);
            state->viewerState->vpConfigs[VPfound].motionTracking = 1;
        }
    }

    return true;
}

bool EventModel::handleMouseButtonRight(QMouseEvent *event, int32_t VPfound) {

    int32_t newNodeID;
    Coordinate *clickedCoordinate = NULL, movement, lastPos;

    /* We have to activate motion tracking only for the skeleton VP for a right click */
    if(state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON) {
        state->viewerState->vpConfigs[VPfound].motionTracking = true;
    }
    /* If not, we look up which skeletonizer work mode is
    active and do the appropriate operation */
    clickedCoordinate = getCoordinateFromOrthogonalClick(event, VPfound);

    /* could not find any coordinate... */
    if(clickedCoordinate == NULL) {
        return true;
    }

    switch(state->skeletonState->workMode) {
    case SKELETONIZER_ON_CLICK_DROP_NODE:
        emit addSkeletonNodeSignal(clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
        break;
    case SKELETONIZER_ON_CLICK_ADD_NODE:
        emit addSkeletonNodeSignal(clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
        tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE;
        break;
    case SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE:
        if(state->skeletonState->activeNode == NULL) {
            //1. no node to link with
            emit addSkeletonNodeSignal(clickedCoordinate,
                               state->viewerState->vpConfigs[VPfound].type);
            break;
        }


        Qt::KeyboardModifiers keyMod = QApplication::keyboardModifiers();

        if(keyMod.testFlag(Qt::ControlModifier)) {
            //2. Add a "stump", a branch node to which we don't automatically move.
            if((newNodeID = Skeletonizer::addSkeletonNodeAndLinkWithActive(clickedCoordinate,
                                                                state->viewerState->vpConfigs[VPfound].type,
                                                                false))) {
                Skeletonizer::pushBranchNode(CHANGE_MANUAL, true, true, NULL, newNodeID);
            }
            break;
        }
        //3. Add a node and apply tracing modes
        lastPos = state->skeletonState->activeNode->position; //remember last active for movement calculation

        if(Skeletonizer::addSkeletonNodeAndLinkWithActive(clickedCoordinate,
                                               state->viewerState->vpConfigs[VPfound].type,
                                               true) == false) { //could not add node
            break;
        }

        /* Highlight the viewport with the biggest movement component and set
           behavior of f / d keys accordingly. */
        movement.x = clickedCoordinate->x - lastPos.x;
        movement.y = clickedCoordinate->y - lastPos.y;
        movement.z = clickedCoordinate->z - lastPos.z;
        /* Determine which viewport to highlight based on which movement component is biggest. */
        if((abs(movement.x) >= abs(movement.y)) && (abs(movement.x) >= abs(movement.z))) {
            state->viewerState->highlightVp = VIEWPORT_YZ;
        }
        else if((abs(movement.y) >= abs(movement.x)) && (abs(movement.y) >= abs(movement.z))) {
            state->viewerState->highlightVp = VIEWPORT_XZ;
        }
        else {
            state->viewerState->highlightVp = VIEWPORT_XY;
        }
        /* Determine the directions for the f and d keys based on the signs of the movement
           components along the three dimensions. */
        state->viewerState->vpKeyDirection[VIEWPORT_YZ] = (movement.x >= 0)? 1 : -1;
        state->viewerState->vpKeyDirection[VIEWPORT_XZ] = (movement.y >= 0)? 1 : -1;
        state->viewerState->vpKeyDirection[VIEWPORT_XY] = (movement.z >= 0)? 1 : -1;

        //Auto tracing adjustments

        if (state->viewerState->autoTracingDelay < 10) { state->viewerState->autoTracingDelay = 10; }
        else if (state->viewerState->autoTracingDelay > 500) { state->viewerState->autoTracingDelay = 500; }
        if (state->viewerState->autoTracingSteps < 1) { state->viewerState->autoTracingSteps = 1; }
        else if (state->viewerState->autoTracingSteps > 50) { state->viewerState->autoTracingSteps = 50; }


        //Additional move of specified steps along clicked viewport
        if (state->viewerState->autoTracingMode == AUTOTRACING_VIEWPORT) {
            switch(state->viewerState->vpConfigs[VPfound].type) {
            case VIEWPORT_XY:
                clickedCoordinate->z += (state->viewerState->vpKeyDirection[VIEWPORT_XY] == 1)?
                                     state->viewerState->autoTracingSteps : -state->viewerState->autoTracingSteps;
                break;
            case VIEWPORT_XZ:
                clickedCoordinate->y += (state->viewerState->vpKeyDirection[VIEWPORT_XZ] == 1)?
                                     state->viewerState->autoTracingSteps : -state->viewerState->autoTracingSteps;
                break;
            case VIEWPORT_YZ:
                clickedCoordinate->x += (state->viewerState->vpKeyDirection[VIEWPORT_YZ] == 1)?
                                     state->viewerState->autoTracingSteps : -state->viewerState->autoTracingSteps;
                break;
            }
        }

        if ((state->viewerState->autoTracingMode == AUTOTRACING_TRACING)
            || (state->viewerState->autoTracingMode == AUTOTRACING_MIRROR)) {
            floatCoordinate walkingVector;
            walkingVector.x = movement.x;
            walkingVector.y = movement.y;
            walkingVector.z = movement.z;
            //Additional move of specified steps along tracing direction
            if (state->viewerState->autoTracingMode == AUTOTRACING_TRACING) {
                clickedCoordinate->x += roundFloat(walkingVector.x * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
                clickedCoordinate->y += roundFloat(walkingVector.y * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
                clickedCoordinate->z += roundFloat(walkingVector.z * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
            }
            //Additional move of steps equal to distance between last and new node along tracing direction.
            if (state->viewerState->autoTracingMode == AUTOTRACING_MIRROR) {
                clickedCoordinate->x += walkingVector.x;
                clickedCoordinate->y += walkingVector.y;
                clickedCoordinate->z += walkingVector.z;
            }
        }

        /* Do not allow clicks outside the dataset */
        if (clickedCoordinate->x < 0) { clickedCoordinate->x = 0; }
        else if (clickedCoordinate->x > state->boundary.x) { clickedCoordinate->x = state->boundary.x; }
        if (clickedCoordinate->y < 0) { clickedCoordinate->y = 0; }
        else if (clickedCoordinate->y > state->boundary.y) { clickedCoordinate->y = state->boundary.y; }
        if (clickedCoordinate->z < 0) { clickedCoordinate->z = 0; }
        else if (clickedCoordinate->z > state->boundary.z) { clickedCoordinate->z = state->boundary.z; }

        /* Move to the new node position */
        emit setRemoteStateTypeSignal(REMOTE_RECENTERING);
        emit setRecenteringPositionSignal(clickedCoordinate->x, clickedCoordinate->y, clickedCoordinate->z);
        emit updateViewerStateSignal();
        Knossos::sendRemoteSignal();
        break;
    }

    return true;
}

bool EventModel::handleMouseMotionLeftHold(QMouseEvent *event, int32_t VPfound) {

    qDebug() << "mouse motion with left button clicked";

    uint32_t i;



    for(i = 0; i < state->viewerState->numberViewports; i++) {
        /*
        if(state->viewerState->moveVP != -1
                   || state->viewerState->resizeVP != -1) {
               moveOrResizeVP(event);
           } */ /*@todo */

        // motion tracking mode is active for viewport i
        if(state->viewerState->vpConfigs[i].motionTracking == true) {
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
                        /* @todo Remote::checkIdleTime();*/
                    break;
                case VIEWPORT_XY:
                    if(state->viewerState->workMode != ON_CLICK_DRAG) break;
                    state->viewerState->vpConfigs[i].userMouseSlideX -=
                            ((float)xrel(event->x())
                        / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
                    state->viewerState->vpConfigs[i].userMouseSlideY -=
                            ((float)yrel(event->y())
                        / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
                    if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {

                        emit userMoveSignal((int)state->viewerState->vpConfigs[i].userMouseSlideX,
                            (int)state->viewerState->vpConfigs[i].userMouseSlideY, 0,
                            TELL_COORDINATE_CHANGE);
                        state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                        state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                    }
                    break;
                case VIEWPORT_XZ:
                    if(state->viewerState->workMode != ON_CLICK_DRAG) break;
                    state->viewerState->vpConfigs[i].userMouseSlideX -=
                            ((float)xrel(event->x()) / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
                    state->viewerState->vpConfigs[i].userMouseSlideY -=
                            ((float)yrel(event->y()) / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
                    if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {

                        emit userMoveSignal((int)state->viewerState->vpConfigs[i].userMouseSlideX, 0,
                            (int)state->viewerState->vpConfigs[i].userMouseSlideY,
                            TELL_COORDINATE_CHANGE);
                        state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                        state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                    }
                    break;
                case VIEWPORT_YZ:
                    if(state->viewerState->workMode != ON_CLICK_DRAG) break;
                    state->viewerState->vpConfigs[i].userMouseSlideX -=
                            ((float)xrel(event->x()) / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
                    state->viewerState->vpConfigs[i].userMouseSlideY -=
                            ((float)yrel(event->y()) / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
                    if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {

                        emit userMoveSignal(0, (int)state->viewerState->vpConfigs[i].userMouseSlideY,
                            (int)state->viewerState->vpConfigs[i].userMouseSlideX,
                            TELL_COORDINATE_CHANGE);
                        state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                        state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                    }
                    break;
            }
            return true;
        }
    }

    return true;
}

bool EventModel::handleMouseMotionMiddleHold(QMouseEvent *event, int32_t VPfound) {
    int32_t i = 0;
        Coordinate newDraggedNodePos;

        for(i = 0; i < state->viewerState->numberViewports; i++) {
            switch(state->viewerState->vpConfigs[i].type) {
                case VIEWPORT_XY:
                    if(!state->viewerState->vpConfigs[i].draggedNode) break;
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

                        emit editNodeSignal(CHANGE_MANUAL,
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
                    if(!state->viewerState->vpConfigs[i].draggedNode) break;
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
                        emit editNodeSignal(CHANGE_MANUAL,
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
                    if(!state->viewerState->vpConfigs[i].draggedNode) break;
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
                        emit editNodeSignal(CHANGE_MANUAL,
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

bool EventModel::handleMouseMotionRightHold(QMouseEvent *event, int32_t VPfound) {

    if((state->viewerState->vpConfigs[VIEWPORT_SKELETON].motionTracking == true) && (state->skeletonState->rotationcounter == 0)) {
           state->skeletonState->rotdx += xrel(event->x());
           state->skeletonState->rotdy += yrel(event->y());
           state->skeletonState->viewChanged = true;
           /* @todo checkIdleTime(); */


       }


    return true;
}

bool EventModel::handleMouseWheelForward(QWheelEvent *event, int32_t VPfound) {

    qDebug() << "wheel up";

    float radius;

    if(VPfound == -1)
        return true;

    Qt::KeyboardModifiers keyMod = QApplication::keyboardModifiers();


    if((state->skeletonState->activeNode) && (keyMod.testFlag(Qt::ShiftModifier))) {
        qDebug("shift and mouse wheel up and activeNode");
        radius = state->skeletonState->activeNode->radius - 0.2 * state->skeletonState->activeNode->radius;

        emit editNodeSignal(CHANGE_MANUAL,
                 0,
                 state->skeletonState->activeNode,
                 radius,
                 state->skeletonState->activeNode->position.x,
                 state->skeletonState->activeNode->position.y,
                 state->skeletonState->activeNode->position.z,
                 state->magnification);


        if(state->viewerState->gui->useLastActNodeRadiusAsDefault)
            state->skeletonState->defaultNodeRadius = radius;


    } else {
        // Skeleton VP
        if(VPfound == VIEWPORT_SKELETON) {

            if (state->skeletonState->zoomLevel <= SKELZOOMMAX){
                state->skeletonState->zoomLevel += (0.1 * (0.5 - state->skeletonState->zoomLevel));
                state->skeletonState->viewChanged = true;
                state->skeletonState->skeletonChanged = true;
            }
        }
        // Orthogonal VP or outside VP
        else {
            // Zoom when CTRL is pressed
            if(keyMod.testFlag(Qt::ControlModifier)) {
                qDebug("again control and mouse wheel up");

                emit zoomOrthoSignal(-0.1);


            } else { // move otherwiese
                switch(VPfound) {
                    case VIEWPORT_XY:
                        emit userMoveSignal(0, 0, state->viewerState->dropFrames
                            * state->magnification,
                            TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_XZ:
                        emit userMoveSignal(0, state->viewerState->dropFrames
                            * state->magnification, 0,
                            TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_YZ:
                        emit userMoveSignal(state->viewerState->dropFrames
                            * state->magnification, 0, 0,
                            TELL_COORDINATE_CHANGE);
                        break;
                }
            }
        }
    }

    return true;
}

bool EventModel::handleMouseWheelBackward(QWheelEvent *event, int32_t VPfound) {

    qDebug() << "mouseWheelDown";

    float radius;

    if(VPfound == -1)
        return true;

    Qt::KeyboardModifiers keyMod = QApplication::keyboardModifiers();

    if((state->skeletonState->activeNode) && (keyMod.testFlag(Qt::ControlModifier))) {
        qDebug("shift and mouse wheel down");
        radius = state->skeletonState->activeNode->radius + 0.2 * state->skeletonState->activeNode->radius;

        emit editNodeSignal(CHANGE_MANUAL,
                 0,
                 state->skeletonState->activeNode,
                 radius,
                 state->skeletonState->activeNode->position.x,
                 state->skeletonState->activeNode->position.y,
                 state->skeletonState->activeNode->position.z,
                 state->magnification);

        if(state->viewerState->gui->useLastActNodeRadiusAsDefault)
           state->skeletonState->defaultNodeRadius = radius;
    } else {
        // Skeleton VP
        if(VPfound == VIEWPORT_SKELETON) {

            if (state->skeletonState->zoomLevel >= SKELZOOMMIN) {
                state->skeletonState->zoomLevel -= (0.2* (0.5 - state->skeletonState->zoomLevel));
                if (state->skeletonState->zoomLevel < SKELZOOMMIN) state->skeletonState->zoomLevel = SKELZOOMMIN;
                state->skeletonState->viewChanged = true;
                state->skeletonState->skeletonChanged = true;
            }
        }
        // Orthogonal VP or outside VP
        else {
            // Zoom when CTRL is pressed
            if(keyMod.testFlag(Qt::ControlModifier)) {
                qDebug("control and mouse wheel down");

                emit zoomOrthoSignal(0.1);


            }
            // Move otherwise
            else {
                switch(VPfound) {
                    case VIEWPORT_XY:
                        emit userMoveSignal(0, 0, -state->viewerState->dropFrames
                            * state->magnification,
                            TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_XZ:
                        emit userMoveSignal(0, -state->viewerState->dropFrames
                            * state->magnification, 0,
                            TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_YZ:
                        emit userMoveSignal(-state->viewerState->dropFrames
                            * state->magnification, 0, 0,
                            TELL_COORDINATE_CHANGE);
                        break;
                }
            }
        }
    }

    return true;
}

bool EventModel::handleKeyboard(QKeyEvent *event, int32_t VPfound) {

    // This is a workaround for agars behavior of processing / labeling
    //events. Without it, input into a textbox would still trigger global
    //shortcut actions.

    struct treeListElement *prevTree;
    struct treeListElement *nextTree;
    struct nodeListElement *prevNode;
    color4F treeCol;

    Qt::KeyboardModifiers keyMod = QApplication::keyboardModifiers();
    bool shift   = keyMod.testFlag(Qt::ShiftModifier);
    bool control = keyMod.testFlag(Qt::ControlModifier);
    bool alt     = keyMod.testFlag(Qt::AltModifier);

    // new qt version
    if(event->key() == Qt::Key_Left) {
        if(shift) {
            qDebug() << "shift + left key";
            switch(VPfound) {
            qDebug() << state->viewerState->activeVP;
                case VIEWPORT_XY:
                    emit userMoveSignal(-10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(-10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, 0, -10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            qDebug() << "left key";

            switch(VPfound) {
                case VIEWPORT_XY:
                    emit userMoveSignal(-state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(-state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, 0, -state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
    } else if(event->key() == Qt::Key_Right) {

            if(shift) {
                qDebug() << "shift + right key";
                switch(VPfound) {
                    case VIEWPORT_XY:
                        emit userMoveSignal(10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_XZ:
                        emit userMoveSignal(10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_YZ:
                        emit userMoveSignal(0, 0, 10 * state->magnification, TELL_COORDINATE_CHANGE);
                        break;
                }
            } else {
                qDebug() << "right key";
                switch(VPfound) {
                    case VIEWPORT_XY:
                        emit userMoveSignal(state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_XZ:
                        emit userMoveSignal(state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_YZ:
                        emit userMoveSignal(0, 0, state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                        break;
                    }
                }

    } else if(event->key() == Qt::Key_Down) {
        if(shift) {
            qDebug() << "key down and shift";
            switch(VPfound) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, -10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, 0, -10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, -10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            qDebug() << "key down";
            switch(VPfound) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, -state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, 0, -state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, -state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
    } else if(event->key() == Qt::Key_Up) {
        if(shift) {
            qDebug() << "key up and shift";
            switch(VPfound) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, 10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, 0, 10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, 10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            qDebug() << "key up";
            switch(VPfound) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, 0, state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
    } else if(event->key() == Qt::Key_R) {
        qDebug() << "r pressed";
        state->viewerState->walkOrth = 1;
        switch(VPfound) {
            case VIEWPORT_XY:
                emit setRemoteStateTypeSignal(REMOTE_RECENTERING);
                emit setRecenteringPositionSignal(tempConfig->viewerState->currentPosition.x,
                                                  tempConfig->viewerState->currentPosition.y,
                                                  tempConfig->viewerState->currentPosition.z += state->viewerState->walkFrames * state->magnification * state->viewerState->vpKeyDirection[VIEWPORT_XY]);
                Knossos::sendRemoteSignal();
            break;
            case VIEWPORT_XZ:
                emit setRemoteStateTypeSignal(REMOTE_RECENTERING);

                emit setRecenteringPositionSignal(
                               tempConfig->viewerState->currentPosition.x,
                        tempConfig->viewerState->currentPosition.y += state->viewerState->walkFrames * state->magnification  * state->viewerState->vpKeyDirection[VIEWPORT_XZ],
                               tempConfig->viewerState->currentPosition.z);
                               Knossos::sendRemoteSignal();
            break;
            case VIEWPORT_YZ:
                emit setRemoteStateTypeSignal(REMOTE_RECENTERING);

                emit setRecenteringPositionSignal(
                        tempConfig->viewerState->currentPosition.x += state->viewerState->walkFrames * state->magnification * state->viewerState->vpKeyDirection[VIEWPORT_YZ],
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z);
                               Knossos::sendRemoteSignal();
            break;
        }
    } else if(event->key() == Qt::Key_E) {
        qDebug() << "E pressed";
        state->viewerState->walkOrth = 1;
        switch(VPfound) {
            case VIEWPORT_XY:
                 emit setRemoteStateTypeSignal(REMOTE_RECENTERING);

                emit setRecenteringPositionSignal(
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z -= state->viewerState->walkFrames * state->magnification * state->viewerState->vpKeyDirection[VIEWPORT_XY]);
                               Knossos::sendRemoteSignal();
            break;
            case VIEWPORT_XZ:
                 emit setRemoteStateTypeSignal(REMOTE_RECENTERING);

                emit setRecenteringPositionSignal(
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y -= state->viewerState->walkFrames * state->magnification  * state->viewerState->vpKeyDirection[VIEWPORT_XZ],
                               tempConfig->viewerState->currentPosition.z);
                               Knossos::sendRemoteSignal();
            break;
            case VIEWPORT_YZ:
                 emit setRemoteStateTypeSignal(REMOTE_RECENTERING);

                emit setRecenteringPositionSignal(
                               tempConfig->viewerState->currentPosition.x -= state->viewerState->walkFrames * state->magnification  * state->viewerState->vpKeyDirection[VIEWPORT_YZ],
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z);
                               Knossos::sendRemoteSignal();
            break;
        }
    } else if(event->key() == Qt::Key_F) {
        if(shift) {
            qDebug() << "F und shift";
            switch(VPfound) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * 10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * 10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * 10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            qDebug() << "F pressed";
            switch(VPfound) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }

        }
    } else if(event->key() == Qt::Key_D) {
        if(shift) {
            qDebug() << "D und Shift";
            switch(VPfound) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * -10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * -10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * -10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        } else {
            qDebug() << "D pressed";
            switch(VPfound) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * -state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * -state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * -state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
    } else if(event->key() == Qt::Key_G) {
        emit genTestNodesSignal(1000);
    } else if(event->key() == Qt::Key_N) {
        if(shift) {
            qDebug() << "N und shift";
            emit nextCommentlessNodeSignal();
        } else {
            qDebug() << "N pressed";
            emit nextCommentSignal(state->viewerState->gui->commentSearchBuffer);
        }
    } else if(event->key() == Qt::Key_P) {
        if(shift) {
            qDebug() << "P und shift";
            emit previousCommentlessNodeSignal();
        } else {
            qDebug() << "P pressed";
            emit previousCommentSignal(state->viewerState->gui->commentSearchBuffer);
        }
    } else if(event->key() == Qt::Key_3) {
        qDebug() << "3 pressed";
        if(state->viewerState->drawVPCrosshairs) {
           state->viewerState->drawVPCrosshairs = false;
        }
        else {
           state->viewerState->drawVPCrosshairs = true;
        }

    } else if(event->key() == Qt::Key_J) {
        qDebug() << "J pressed";
        Skeletonizer::UI_popBranchNode();
    } else if(event->key() == Qt::Key_B) {
        qDebug() << "B pressed";
        Skeletonizer::pushBranchNode(CHANGE_MANUAL, true, true, state->skeletonState->activeNode, 0);
    } else if(event->key() == Qt::Key_X) {

        if(shift) {
            Skeletonizer::moveToPrevNode();
            return true;
        }

        Skeletonizer::moveToNextNode();
        return true;


    } else if(event->key() == Qt::Key_Z) {

        if(shift) {
            Skeletonizer::moveToPrevTree();
            return true;
        }

        Skeletonizer::moveToNextTree();
        return true;

    } else if(event->key() == Qt::Key_I) {
        qDebug() << "I pressed";
        if (state->viewerState->gui->zoomSkeletonViewport == false){
            emit zoomOrthoSignal(-0.1);

        }
        else if (state->skeletonState->zoomLevel <= SKELZOOMMAX){
            state->skeletonState->zoomLevel += (0.1 * (0.5 - state->skeletonState->zoomLevel));
            state->skeletonState->viewChanged = true;
        }

    } else if(event->key() == Qt::Key_O) {
        qDebug() << "O pressed";
        if (state->viewerState->gui->zoomSkeletonViewport == false){
            emit zoomOrthoSignal(0.1);

        } else if (state->skeletonState->zoomLevel >= SKELZOOMMIN) {
            state->skeletonState->zoomLevel -= (0.2* (0.5 - state->skeletonState->zoomLevel));
            if (state->skeletonState->zoomLevel < SKELZOOMMIN) state->skeletonState->zoomLevel = SKELZOOMMIN;
            state->skeletonState->viewChanged = true;


        }
    } else if(event->key() == Qt::Key_S) {
        if(control) {
            qDebug() << "S und control";
            //saveSkelCallback(NULL); /* @todo */
            return true;
        }

        emit jumpToActiveNodeSignal();


    } else if(event->key() == Qt::Key_A) {
        qDebug() << "A pressed";
        emit workModeAddSignal();
    } else if(event->key() == Qt::Key_W) {
        qDebug() << "W pressed";
        emit workModeLinkSignal();
    } else if(event->key() == Qt::Key_C) {
        qDebug() << "C pressed";
        treeCol.r = -1.;
        Skeletonizer::addTreeListElement(true, CHANGE_MANUAL, 0, treeCol);
        tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
    } else if(event->key() == Qt::Key_V) {
       if(control) {
           qDebug() << "V und shift";
           emit pasteCoordinateSignal();
       }
    } else if(event->key() == Qt::Key_1) {
        qDebug() << "1 pressed";
        if(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE) {
            state->skeletonState->displayMode &= ~DSP_SLICE_VP_HIDE;
            state->viewerState->gui->enableOrthoSkelOverlay = 1;
        }
        else {
            state->skeletonState->displayMode |= DSP_SLICE_VP_HIDE;
            state->viewerState->gui->enableOrthoSkelOverlay = 0;
        }
        state->skeletonState->skeletonChanged = true;
        emit drawGUISignal();
    } else if(event->key() == Qt::Key_Delete) {
        qDebug() << "Delete Key pressed";
        emit deleteActiveNodeSignal();
    } else if(event->key() == Qt::Key_F1) {
        qDebug() << "F1 pressed";
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment1, "", 1) != 0)){
            emit addCommentSignal(CHANGE_MANUAL, state->viewerState->gui->comment1, state->skeletonState->activeNode, 0);
        } else{
            if (strncmp(state->viewerState->gui->comment1, "", 1) != 0) {
                emit editCommentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment1, state->skeletonState->activeNode, 0);
            }
        }
    } else if(event->key() == Qt::Key_F2) {
        qDebug() << "F2 pressed";
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment2, "", 1) != 0)){
            emit addCommentSignal(CHANGE_MANUAL, state->viewerState->gui->comment2, state->skeletonState->activeNode, 0);
        }
        else{
            if (strncmp(state->viewerState->gui->comment2, "", 1) != 0)
                emit editCommentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment2, state->skeletonState->activeNode, 0);
        }
    } else if(event->key() == Qt::Key_F3) {
        qDebug() << "F3 pressed";
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment3, "", 1) != 0)){
            emit addCommentSignal(CHANGE_MANUAL, state->viewerState->gui->comment3, state->skeletonState->activeNode, 0);
        }
        else{
           if(strncmp(state->viewerState->gui->comment3, "", 1) != 0)
                emit editCommentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment3, state->skeletonState->activeNode, 0);
        }
    } else if(event->key() == Qt::Key_F4) {
        if(alt) {
            qDebug() << "F4 und alt";
            if(state->skeletonState->unsavedChanges) {
                //yesNoPrompt(NULL, "There are unsaved changes. Really quit?", quitKnossos, NULL);
            }
            else {
                //Knossos::quitKnossos(); TODO quitKnossos
            }
        } else {
            qDebug() << "F4 pressed";
            if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment4, "", 1) != 0)){
                emit addCommentSignal(CHANGE_MANUAL, state->viewerState->gui->comment4, state->skeletonState->activeNode, 0);
            }
            else{
               if (strncmp(state->viewerState->gui->comment4, "", 1) != 0)
                emit editCommentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment4, state->skeletonState->activeNode, 0);
            }
        }
    } else if(event->key() == Qt::Key_F5) {
        qDebug() << "F5 pressed";
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->gui->comment5, "", 1) != 0)){
            emit addCommentSignal(CHANGE_MANUAL, state->viewerState->gui->comment5, state->skeletonState->activeNode, 0);
        }
        else{
            if (strncmp(state->viewerState->gui->comment5, "", 1) != 0)
            emit editCommentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->gui->comment5, state->skeletonState->activeNode, 0);
        }
    }

    return true;
}

Coordinate *EventModel::getCoordinateFromOrthogonalClick(QMouseEvent *event, int32_t VPfound) {

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
    if((x > 0) && (x <= state->boundary.x)
        &&(y > 0) && (y <= state->boundary.y)
        &&(z > 0) && (z <= state->boundary.z)) {
        SET_COORDINATE((*foundCoordinate), x, y, z);
        return foundCoordinate;
    }
    return NULL;
}

int EventModel::xrel(int x) {
    return (x - this->mouseX);
}

int EventModel::yrel(int y) {
    return (y - this->mouseY);
}
