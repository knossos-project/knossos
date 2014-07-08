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
#include "functions.h"
#include "knossos.h"
#include "skeletonizer.h"
#include "renderer.h"
#include "segmentation.h"
#include "viewer.h"
#include "widgets/widgetcontainer.h"
#include "widgets/viewport.h"

#include <cstdlib>

extern  stateInfo *state;

EventModel::EventModel(QObject *parent) :
    QObject(parent)
{

}

uint64_t segmentationColorPicking(const int x, const int y, const int viewportId) {
    const auto color = state->viewer->renderer->retrieveUniqueColorFromPixel(viewportId, x, y);
    return Segmentation::singleton().subobjectIdFromUniqueColor(color);
}

bool EventModel::handleMouseButtonLeft(QMouseEvent *event, int VPfound) {
    mouseDownX = event->x();
    mouseDownY = event->y();
    uint clickedNode;

    //new active node selected
    if(QApplication::keyboardModifiers() == Qt::ShiftModifier) {
        //first assume that user managed to hit the node
        clickedNode = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->x(), event->y(), 10);

        if(clickedNode) {
            emit setActiveNodeSignal(NULL, clickedNode);
            emit nodeActivatedSignal();
            return true;
        }

        if(VPfound == VIEWPORT_SKELETON) {
            return false;
        }

        return false;
    } else if (QApplication::keyboardModifiers() == Qt::ControlModifier && !Segmentation::singleton().segmentationMode) {
        startNodeSelection(event->pos().x(), event->pos().y(), VPfound);
    } else if(state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON) {
        // always drag in skeleton vp
        state->viewerState->vpConfigs[VPfound].motionTracking = 1;
    } else {
        // check click mode of orthogonal viewports
        if (state->viewerState->clickReaction == ON_CLICK_RECENTER) {
            if(validPosition(event, VPfound) == false) {
                return false;
            }
            Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event, VPfound);
            emit setRecenteringPositionSignal(clickedCoordinate.x, clickedCoordinate.y, clickedCoordinate.z);
            Knossos::sendRemoteSignal();
        } else {// Activate motion tracking for this VP
            state->viewerState->vpConfigs[VPfound].motionTracking = 1;
        }
    }


    //Set Connection between Active Node and Clicked Node
    if(QApplication::keyboardModifiers() == Qt::ALT) {
        int clickedNode = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->x(), event->y(), 10);
        if(clickedNode) {
            if(state->skeletonState->activeNode) {
                if(findSegmentByNodeIDSignal(state->skeletonState->activeNode->nodeID, clickedNode)) {
                    emit delSegmentSignal(state->skeletonState->activeNode->nodeID, clickedNode, NULL);
                } else if(findSegmentByNodeIDSignal(clickedNode, state->skeletonState->activeNode->nodeID)) {
                    emit delSegmentSignal(clickedNode, state->skeletonState->activeNode->nodeID, NULL);
                } else{
                    emit addSegmentSignal(state->skeletonState->activeNode->nodeID, clickedNode);
                }
            }
        }
    }
    return true;
}

bool EventModel::handleMouseButtonMiddle(QMouseEvent *event, int VPfound) {
    int clickedNode = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->x(), event->y(), 10);

    if(clickedNode) {
        Qt::KeyboardModifiers keyMod = QApplication::keyboardModifiers();
        if(keyMod.testFlag(Qt::ShiftModifier)) {
            if(keyMod.testFlag(Qt::ControlModifier)) {
                qDebug() << "shift and control and mouse middle";
                // Pressed SHIFT and CTRL
            } else {
                // Delete segment between clicked and active node
                if(state->skeletonState->activeNode) {
                    if(findSegmentByNodeIDSignal(state->skeletonState->activeNode->nodeID,
                                            clickedNode)) {
                        emit delSegmentSignal(
                                   state->skeletonState->activeNode->nodeID,
                                   clickedNode,
                                   0);
                    } else if(findSegmentByNodeIDSignal(clickedNode,
                                            state->skeletonState->activeNode->nodeID)) {
                        emit delSegmentSignal(
                                   clickedNode,
                                   state->skeletonState->activeNode->nodeID,
                                   0);
                    } else {
                        emit addSegmentSignal(state->skeletonState->activeNode->nodeID, clickedNode);
                    }
                }
            }
        } else {
            // No modifier pressed
            state->viewerState->vpConfigs[VPfound].draggedNode = findNodeByNodeIDSignal(clickedNode);
            state->viewerState->vpConfigs[VPfound].motionTracking = 1;
        }
    }

    return true;
}

void EventModel::handleMouseButtonRight(QMouseEvent *event, int VPfound) {
    if(Segmentation::singleton().segmentationMode && VPfound != VIEWPORT_SKELETON) {
        if(validPosition(event, VPfound)) {
            Segmentation::singleton().mergeLine.push_back(getCoordinateFromOrthogonalClick(event, VPfound));
        }
        return;
    }
    int newNodeID;
    Coordinate movement, lastPos;

    /* We have to activate motion tracking only for the skeleton VP for a right click */
    if(state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON) {
        state->viewerState->vpConfigs[VPfound].motionTracking = true;
    }
    /* If not, we look up which skeletonizer work mode is
    active and do the appropriate operation */
    if(validPosition(event, VPfound) == false) {
        return;
    }
    Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event, VPfound);

    bool newNode = false;
    bool newTree = state->skeletonState->activeTree == nullptr;//if there was no active tree, a new node will create one
    switch (state->viewer->skeletonizer->getTracingMode()) {
    case Skeletonizer::TracingMode::unlinkedNodes:
        newNode = addSkeletonNodeSignal(&clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
        break;
    case Skeletonizer::TracingMode::skipNextLink:
        newNode = addSkeletonNodeSignal(&clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
        state->viewer->skeletonizer->setTracingMode(Skeletonizer::TracingMode::linkedNodes);//as we only wanted to skip one link
        break;
    case Skeletonizer::TracingMode::linkedNodes:
        if (state->skeletonState->activeNode == nullptr || state->skeletonState->activeTree->firstNode == nullptr) {
            //no node to link with or no empty tree
            newNode = addSkeletonNodeSignal(&clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
            break;
        } else if (event->modifiers().testFlag(Qt::ControlModifier)) {
            //Add a "stump", a branch node to which we don't automatically move.
            emit unselectNodesSignal();
            if((newNodeID = addSkeletonNodeAndLinkWithActiveSignal(&clickedCoordinate,
                                                                state->viewerState->vpConfigs[VPfound].type,
                                                                false))) {
                emit pushBranchNodeSignal(true, true, NULL, newNodeID);
                if(state->skeletonState->activeNode) {
                    if(state->skeletonState->activeNode->selected == false) {
                        state->skeletonState->activeNode->selected = true;
                        state->skeletonState->selectedNodes.push_back(state->skeletonState->activeNode);
                        emit updateTreeviewSignal();
                    }
                }
            }
            break;
        }
        //Add a node and apply tracing modes
        lastPos = state->skeletonState->activeNode->position; //remember last active for movement calculation
        emit unselectNodesSignal();
        newNode = addSkeletonNodeAndLinkWithActiveSignal(&clickedCoordinate,
                                                         state->viewerState->vpConfigs[VPfound].type,
                                                         true);
        if(newNode == false) { //could not add node
            break;
        }

        /* Highlight the viewport with the biggest movement component and set
           behavior of f / d keys accordingly. */
        movement.x = clickedCoordinate.x - lastPos.x;
        movement.y = clickedCoordinate.y - lastPos.y;
        movement.z = clickedCoordinate.z - lastPos.z;
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
        state->viewerState->vpKeyDirection[VP_UPPERLEFT] = (movement.z >= 0)? 1 : -1;
        state->viewerState->vpKeyDirection[VP_LOWERLEFT] = (movement.y >= 0)? 1 : -1;
        state->viewerState->vpKeyDirection[VP_UPPERRIGHT] = (movement.x >= 0)? 1 : -1;

        //Auto tracing adjustments

        if (state->viewerState->autoTracingDelay < 10) { state->viewerState->autoTracingDelay = 10; }
        else if (state->viewerState->autoTracingDelay > 500) { state->viewerState->autoTracingDelay = 500; }
        if (state->viewerState->autoTracingSteps < 1) { state->viewerState->autoTracingSteps = 1; }
        else if (state->viewerState->autoTracingSteps > 50) { state->viewerState->autoTracingSteps = 50; }


        //Additional move of specified steps along clicked viewport
        if (state->viewerState->autoTracingMode == AUTOTRACING_VIEWPORT) {
            switch(state->viewerState->vpConfigs[VPfound].type) {
            case VIEWPORT_XY:
                clickedCoordinate.z += (state->viewerState->vpKeyDirection[VIEWPORT_XY] == 1)?
                                     state->viewerState->autoTracingSteps : -state->viewerState->autoTracingSteps;
                break;
            case VIEWPORT_XZ:
                clickedCoordinate.y += (state->viewerState->vpKeyDirection[VIEWPORT_XZ] == 1)?
                                     state->viewerState->autoTracingSteps : -state->viewerState->autoTracingSteps;
                break;
            case VIEWPORT_YZ:
                clickedCoordinate.x += (state->viewerState->vpKeyDirection[VIEWPORT_YZ] == 1)?
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
                clickedCoordinate.x += roundFloat(walkingVector.x * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
                clickedCoordinate.y += roundFloat(walkingVector.y * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
                clickedCoordinate.z += roundFloat(walkingVector.z * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
            }
            //Additional move of steps equal to distance between last and new node along tracing direction.
            if (state->viewerState->autoTracingMode == AUTOTRACING_MIRROR) {
                clickedCoordinate.x += walkingVector.x;
                clickedCoordinate.y += walkingVector.y;
                clickedCoordinate.z += walkingVector.z;
            }
        }

        /* Do not allow clicks outside the dataset */
        if (clickedCoordinate.x < 0) { clickedCoordinate.x = 0; }
        else if (clickedCoordinate.x > state->boundary.x) { clickedCoordinate.x = state->boundary.x; }
        if (clickedCoordinate.y < 0) { clickedCoordinate.y = 0; }
        else if (clickedCoordinate.y > state->boundary.y) { clickedCoordinate.y = state->boundary.y; }
        if (clickedCoordinate.z < 0) { clickedCoordinate.z = 0; }
        else if (clickedCoordinate.z > state->boundary.z) { clickedCoordinate.z = state->boundary.z; }


        break;
    }
    /* Move to the new node position */
    if (newNode) {
        emit setRecenteringPositionSignal(clickedCoordinate.x, clickedCoordinate.y, clickedCoordinate.z);
        emit nodeAddedSignal();
        if (newTree) {
            emit treeAddedSignal(state->skeletonState->activeTree);
        }
    }
    emit updateViewerStateSignal();
    Knossos::sendRemoteSignal();
}

bool EventModel::handleMouseMotionLeftHold(QMouseEvent *event, int /*VPfound*/) {
    // pull selection square
    if (state->viewerState->nodeSelectSquareVpId != -1) {
        state->viewerState->nodeSelectionSquare.second.x = event->pos().x();
        state->viewerState->nodeSelectionSquare.second.y = event->pos().y();
    }

    uint i;
    for(i = 0; i < state->viewerState->numberViewports; i++) {
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
                    break;
                case VIEWPORT_XY:
                    if(state->viewerState->clickReaction != ON_CLICK_DRAG) break;
                    state->viewerState->vpConfigs[i].userMouseSlideX -=
                            ((float)xrel(event->x())
                        / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
                    state->viewerState->vpConfigs[i].userMouseSlideY -=
                            ((float)yrel(event->y())
                        / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
                    if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {

                        emit userMoveSignal((int)state->viewerState->vpConfigs[i].userMouseSlideX,
                            (int)state->viewerState->vpConfigs[i].userMouseSlideY, 0);
                        state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                        state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                    }
                    break;
                case VIEWPORT_XZ:
                    if(state->viewerState->clickReaction != ON_CLICK_DRAG) break;
                    state->viewerState->vpConfigs[i].userMouseSlideX -=
                            ((float)xrel(event->x()) / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
                    state->viewerState->vpConfigs[i].userMouseSlideY -=
                            ((float)yrel(event->y()) / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
                    if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {

                        emit userMoveSignal((int)state->viewerState->vpConfigs[i].userMouseSlideX, 0,
                            (int)state->viewerState->vpConfigs[i].userMouseSlideY);
                        state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                        state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                    }
                    break;
                case VIEWPORT_YZ:
                    if(state->viewerState->clickReaction != ON_CLICK_DRAG) break;
                    state->viewerState->vpConfigs[i].userMouseSlideX -=
                            ((float)xrel(event->x()) / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
                    state->viewerState->vpConfigs[i].userMouseSlideY -=
                            ((float)yrel(event->y()) / state->viewerState->vpConfigs[i].screenPxYPerDataPx);
                    if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {

                        emit userMoveSignal(0, (int)state->viewerState->vpConfigs[i].userMouseSlideY,
                            (int)state->viewerState->vpConfigs[i].userMouseSlideX);
                        state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
                        state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
                    }
                    break;
                case VIEWPORT_ARBITRARY:
                    if(state->viewerState->clickReaction != ON_CLICK_DRAG) {
                        break;
                    }
                    state->viewerState->vpConfigs[i].userMouseSlideX -=
                            ((float)xrel(event->x()) / state->viewerState->vpConfigs[i].screenPxXPerDataPx);
                    state->viewerState->vpConfigs[i].userMouseSlideY -=
                            ((float)yrel(event->y()) / state->viewerState->vpConfigs[i].screenPxYPerDataPx);

                    if(fabs(state->viewerState->vpConfigs[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->vpConfigs[i].userMouseSlideY) >= 1) {
                        emit userMoveArbSignal(
                                (int)(state->viewerState->vpConfigs[i].v1.x
                                      * state->viewerState->vpConfigs[i].userMouseSlideX
                                      + state->viewerState->vpConfigs[i].v2.x
                                      * state->viewerState->vpConfigs[i].userMouseSlideY),
                                (int)(state->viewerState->vpConfigs[i].v1.y
                                      * state->viewerState->vpConfigs[i].userMouseSlideX
                                      + state->viewerState->vpConfigs[i].v2.y
                                      * state->viewerState->vpConfigs[i].userMouseSlideY),
                                (int)(state->viewerState->vpConfigs[i].v1.z
                                      * state->viewerState->vpConfigs[i].userMouseSlideX
                                      + state->viewerState->vpConfigs[i].v2.z
                                      * state->viewerState->vpConfigs[i].userMouseSlideY));
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

bool EventModel::handleMouseMotionMiddleHold(QMouseEvent *event, int /*VPfound*/) {

    Coordinate newDraggedNodePos;

    for(uint i = 0; i < state->viewerState->numberViewports; i++) {
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

                    emit editNodeSignal(
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
                    emit editNodeSignal(
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
                    emit editNodeSignal(
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

void EventModel::handleMouseMotionRightHold(QMouseEvent *event, int VPfound) {
    auto & segmentation = Segmentation::singleton();
    if(segmentation.segmentationMode && VPfound != VIEWPORT_SKELETON) {
        if(validPosition(event, VPfound)) {
            segmentation.mergeLine.push_back(getCoordinateFromOrthogonalClick(event, VPfound));

            const auto subObjID = segmentationColorPicking(event->x(), event->y(), VPfound);
            if(subObjID != 0) {
                auto & subObj = segmentation.subobjectFromId(subObjID);
                auto & obj = segmentation.smallestImmutableObjectContainingSubobject(subObj);
                segmentation.selectObject(obj);
            }
        }
        return;
    }
    if((state->viewerState->vpConfigs[VIEWPORT_SKELETON].motionTracking == true) && (state->skeletonState->rotationcounter == 0)) {
        state->skeletonState->rotdx += xrel(event->x());
        state->skeletonState->rotdy += yrel(event->y());
        state->skeletonState->viewChanged = true;
    }
}

void EventModel::handleMouseReleaseLeft(QMouseEvent *event, int VPfound) {
    if (Segmentation::singleton().segmentationMode) {
        if(event->x() == mouseDownX && event->y() == mouseDownY) {
            auto & segmentation = Segmentation::singleton();
            const auto subobjectId = segmentationColorPicking(event->x(), event->y(), VPfound);
            if (subobjectId != 0) {// don’t select the unsegmented area as object
                auto & subobject = segmentation.subobjectFromId(subobjectId);
                auto & obj = segmentation.largestObjectContainingSubobject(subobject);
                if (!event->modifiers().testFlag(Qt::ControlModifier)) {
                    segmentation.clearObjectSelection();
                    segmentation.selectObject(obj);
                } else if (segmentation.isSelected(obj)) {// unselect if selected
                    segmentation.unselectObject(obj);
                } else { // select largest object
                    segmentation.selectObject(obj);
                }
                if (segmentation.isSelected(subobject)) {//touch other objects containing this subobject
                    segmentation.touchObjects(subobjectId);
                } else {
                    segmentation.untouchObjects();
                }
            }
        }
        return;
    }

    int diffX = std::abs(state->viewerState->nodeSelectionSquare.first.x - event->pos().x());
    int diffY = std::abs(state->viewerState->nodeSelectionSquare.first.y - event->pos().y());
    if (diffX < 5 && diffY < 5) { // interpreted as click instead of drag
        // mouse released on same spot on which it was pressed down: single node selection
        int nodeID = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->pos().x(), event->pos().y(), 10);
        if (nodeID != 0) {
            nodeListElement *selectedNode = findNodeByNodeIDSignal(nodeID);
            if (selectedNode != nullptr) {
                //check if already in buffer
                auto iter = std::find(std::begin(state->skeletonState->selectedNodes), std::end(state->skeletonState->selectedNodes), selectedNode);
                if (iter == std::end(state->skeletonState->selectedNodes)) {//clicked node was not selected
                    selectedNode->selected = true;
                    state->skeletonState->selectedNodes.emplace_back(selectedNode);
                    if(state->skeletonState->selectedNodes.size() == 1) {
                        emit setActiveNodeSignal(state->skeletonState->selectedNodes[0],
                                                 state->skeletonState->selectedNodes[0]->nodeID);
                    }
                    emit updateTreeviewSignal();
                } else if (state->skeletonState->selectedNodes.size() != 1) {
                    //clicked node was selected → unselect, but one shall not unselect the active node
                    selectedNode->selected = false;
                    state->skeletonState->selectedNodes.erase(iter);
                    // whenever exactly one node is selected, it is the active node
                    if(state->skeletonState->selectedNodes.size() == 1) {
                        emit setActiveNodeSignal(state->skeletonState->selectedNodes[0],
                                                           state->skeletonState->selectedNodes[0]->nodeID);
                    }
                    emit updateTreeviewSignal();
                }
            }
        }
    } else if (state->viewerState->nodeSelectSquareVpId != -1) {
        nodeSelection(event->pos().x(), event->pos().y(), VPfound);
    }
}

void EventModel::handleMouseReleaseRight(QMouseEvent *event, int VPfound) {
    auto & seg = Segmentation::singleton();
    if(seg.segmentationMode && VPfound != VIEWPORT_SKELETON) {
        // merging by clicking
        if(seg.mergeLine.size() == 1) {
            const auto subobjectId = segmentationColorPicking(event->x(), event->y(), VPfound);
            if (subobjectId != 0) {//don’t select the unsegmented area as object
                if (seg.selectedObjectsCount() == 1) {
                    auto & subobject = seg.subobjectFromId(subobjectId);
                    auto & objectToMerge = seg.smallestImmutableObjectContainingSubobject(subobject);
                    //select if not selected and merge
                    if (seg.isSelected(subobject)) {
                        if (event->modifiers().testFlag(Qt::ControlModifier)) {
                            seg.unmergeSelectedObjects(subobject);
                        } else if (event->modifiers().testFlag(Qt::ShiftModifier)) {
                            seg.unmergeSelectedObjects(objectToMerge);
                        }
                    } else {
                        if (event->modifiers().testFlag(Qt::ControlModifier)) {
                            seg.selectObjectFromSubObject(subobject);
                        } else if (!event->modifiers().testFlag(Qt::ShiftModifier)) {
                            seg.selectObject(objectToMerge);//select largest object
                        }
                        if (seg.selectedObjectsCount() >= 2) {
                            seg.mergeSelectedObjects();
                        }
                    }
                    seg.touchObjects(subobjectId);
                }
            }
        }
        else { // merging by line drawing
            seg.mergeSelectedObjects();
        }
        seg.mergeLine.clear();
        return;
    }
}

void EventModel::handleMouseReleaseMiddle(QMouseEvent*, int) {
    // a node was dragged to a new position
    if(state->skeletonState->activeNode) {
        emit nodePositionChangedSignal(state->skeletonState->activeNode);
    }
}

void EventModel::handleMouseWheel(QWheelEvent * const event, int VPfound) {
    const int directionSign = event->delta() > 0 ? -1 : 1;

    if((state->skeletonState->activeNode) and (event->modifiers() == Qt::SHIFT)) {//change node radius
        float radius = state->skeletonState->activeNode->radius + directionSign * 0.2 * state->skeletonState->activeNode->radius;

        emit editNodeSignal(0, state->skeletonState->activeNode, radius
                 , state->skeletonState->activeNode->position.x
                 , state->skeletonState->activeNode->position.y
                 , state->skeletonState->activeNode->position.z
                 , state->magnification);

        emit nodeRadiusChangedSignal(state->skeletonState->activeNode);

        if(state->viewerState->gui->useLastActNodeRadiusAsDefault) {
           state->skeletonState->defaultNodeRadius = radius;
        }
    } else if (VPfound == VIEWPORT_SKELETON) {
        if (directionSign == -1) {
            emit zoomInSkeletonVPSignal();
        } else {
            emit zoomOutSkeletonVPSignal();
        }
    } else if (event->modifiers() == Qt::CTRL) {// Orthogonal VP or outside VP
        emit zoomOrthoSignal(directionSign * 0.1);
    } else {
        const auto multiplier = directionSign * state->viewerState->dropFrames * state->magnification;
        const auto type = state->viewerState->vpConfigs[VPfound].type;
        if (type == VIEWPORT_XY) {
            emit userMoveSignal(0, 0, multiplier);
        } else if (type == VIEWPORT_XZ) {
            emit userMoveSignal(0, multiplier, 0);
        } else if (type == VIEWPORT_YZ) {
            emit userMoveSignal(multiplier, 0, 0);
        } else if (type == VIEWPORT_ARBITRARY) {
            emit userMoveArbSignal(state->viewerState->vpConfigs[VPfound].n.x * multiplier
                , state->viewerState->vpConfigs[VPfound].n.y * multiplier
                , state->viewerState->vpConfigs[VPfound].n.z * multiplier);
        }
    }
}

void EventModel::handleKeyPress(QKeyEvent *event, int VPfound) {
    Qt::KeyboardModifiers keyMod = event->modifiers();
    bool shift   = keyMod.testFlag(Qt::ShiftModifier);
    bool control = keyMod.testFlag(Qt::ControlModifier);
    bool alt     = keyMod.testFlag(Qt::AltModifier);

    // new qt version
    if(event->key() == Qt::Key_Left) {
        if(shift) {
            switch(state->viewerState->vpConfigs[VPfound].type) {
            case VIEWPORT_XY:
                emit userMoveSignal(-10 * state->magnification, 0, 0);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(-10 * state->magnification, 0, 0);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 0, -10 * state->magnification);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(
                         -10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v1.x * state->magnification,
                         -10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v1.y * state->magnification,
                         -10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v1.z * state->magnification);
                break;
            }
        } else {
            switch(state->viewerState->vpConfigs[VPfound].type) {
            case VIEWPORT_XY:
                emit userMoveSignal(-state->viewerState->dropFrames * state->magnification, 0, 0);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(-state->viewerState->dropFrames * state->magnification, 0, 0);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 0, -state->viewerState->dropFrames * state->magnification);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(-state->viewerState->vpConfigs[state->viewerState->activeVP].v1.x
                            * state->viewerState->dropFrames * state->magnification,
                        -state->viewerState->vpConfigs[state->viewerState->activeVP].v1.y
                            * state->viewerState->dropFrames * state->magnification,
                        -state->viewerState->vpConfigs[state->viewerState->activeVP].v1.z
                            * state->viewerState->dropFrames * state->magnification);
                break;
            }
        }
    } else if(event->key() == Qt::Key_Right) {
        if(shift) {
            switch(state->viewerState->vpConfigs[VPfound].type) {
            case VIEWPORT_XY:
                emit userMoveSignal(10 * state->magnification, 0, 0);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(10 * state->magnification, 0, 0);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 0, 10 * state->magnification);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(
                            10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v1.x * state->magnification,
                            10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v1.y * state->magnification,
                            10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v1.z * state->magnification);
                 break;
            }
        } else {
            switch(state->viewerState->vpConfigs[VPfound].type) {
            case VIEWPORT_XY:
                emit userMoveSignal(state->viewerState->dropFrames * state->magnification, 0, 0);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(state->viewerState->dropFrames * state->magnification, 0, 0);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 0, state->viewerState->dropFrames * state->magnification);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(state->viewerState->vpConfigs[state->viewerState->activeVP].v1.x
                            * state->viewerState->dropFrames * state->magnification,
                        state->viewerState->vpConfigs[state->viewerState->activeVP].v1.y
                            * state->viewerState->dropFrames * state->magnification,
                        state->viewerState->vpConfigs[state->viewerState->activeVP].v1.z
                            * state->viewerState->dropFrames * state->magnification);
                break;
            }
        }
    } else if(event->key() == Qt::Key_Down) {
        if(shift) {
            switch(state->viewerState->vpConfigs[VPfound].type) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, -10 * state->magnification, 0);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, 0, -10 * state->magnification);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, -10 * state->magnification, 0);
                    break;
                case VIEWPORT_ARBITRARY:
                    emit userMoveArbSignal(
                        -10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v2.x * state->magnification,
                        -10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v2.y * state->magnification,
                        -10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v2.z * state->magnification);
                     break;
            }
        } else {
            switch(state->viewerState->vpConfigs[VPfound].type) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, -state->viewerState->dropFrames * state->magnification, 0);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, 0, -state->viewerState->dropFrames * state->magnification);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, -state->viewerState->dropFrames * state->magnification, 0);
                    break;
                case VIEWPORT_ARBITRARY:
                    emit userMoveArbSignal(-state->viewerState->vpConfigs[state->viewerState->activeVP].v2.x
                            * state->viewerState->dropFrames * state->magnification,
                        -state->viewerState->vpConfigs[state->viewerState->activeVP].v2.y
                            * state->viewerState->dropFrames * state->magnification,
                        -state->viewerState->vpConfigs[state->viewerState->activeVP].v2.z
                            * state->viewerState->dropFrames * state->magnification);
                    break;
            }
        }
    } else if(event->key() == Qt::Key_Up) {
        if(shift) {
            switch(state->viewerState->vpConfigs[VPfound].type) {
            case VIEWPORT_XY:
                emit userMoveSignal(0, 10 * state->magnification, 0);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(0, 0, 10 * state->magnification);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 10 * state->magnification, 0);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(
                            10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v2.x * state->magnification,
                            10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v2.y * state->magnification,
                            10 * state->viewerState->vpConfigs[state->viewerState->activeVP].v2.z * state->magnification);
                break;
            }
        } else {
            switch(state->viewerState->vpConfigs[VPfound].type) {
            case VIEWPORT_XY:
                emit userMoveSignal(0, state->viewerState->dropFrames * state->magnification, 0);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(0, 0, state->viewerState->dropFrames * state->magnification);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, state->viewerState->dropFrames * state->magnification, 0);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(state->viewerState->vpConfigs[state->viewerState->activeVP].v2.x
                            * state->viewerState->dropFrames * state->magnification,
                        state->viewerState->vpConfigs[state->viewerState->activeVP].v2.y
                            * state->viewerState->dropFrames * state->magnification,
                        state->viewerState->vpConfigs[state->viewerState->activeVP].v2.z
                            * state->viewerState->dropFrames * state->magnification);
                break;
            }
        }
    } else if(event->key() == Qt::Key_R) {
        state->viewerState->walkOrth = 1;
        switch(state->viewerState->vpConfigs[VPfound].type) {
        case VIEWPORT_XY:
            emit setRecenteringPositionSignal(state->viewerState->currentPosition.x,
                                              state->viewerState->currentPosition.y,
                                              state->viewerState->currentPosition.z+ state->viewerState->walkFrames
                                              * state->magnification * state->viewerState->vpKeyDirection[VIEWPORT_XY]);
            emit updateViewerStateSignal();
            Knossos::sendRemoteSignal();
            break;
        case VIEWPORT_XZ:
            emit setRecenteringPositionSignal(state->viewerState->currentPosition.x,
                                              state->viewerState->currentPosition.y + state->viewerState->walkFrames
                                              * state->magnification * state->viewerState->vpKeyDirection[VIEWPORT_XZ],
                                              state->viewerState->currentPosition.z);
            Knossos::sendRemoteSignal();
            break;
        case VIEWPORT_YZ:
            emit setRecenteringPositionSignal(state->viewerState->currentPosition.x + state->viewerState->walkFrames
                                              * state->magnification * state->viewerState->vpKeyDirection[VIEWPORT_YZ],
                           state->viewerState->currentPosition.y,
                           state->viewerState->currentPosition.z);
            Knossos::sendRemoteSignal();
            break;
        case VIEWPORT_ARBITRARY:
            emit setRecenteringPositionSignal(state->viewerState->currentPosition.x + state->viewerState->walkFrames
                            * state->viewerState->vpConfigs[state->viewerState->activeVP].n.x * state->magnification,
                    state->viewerState->currentPosition.y + state->viewerState->walkFrames
                            * state->viewerState->vpConfigs[state->viewerState->activeVP].n.y * state->magnification,
                    state->viewerState->currentPosition.z + state->viewerState->walkFrames
                            * state->viewerState->vpConfigs[state->viewerState->activeVP].n.z * state->magnification);
            Knossos::sendRemoteSignal();
            break;
        }
    } else if(event->key() == Qt::Key_E) {
        state->viewerState->walkOrth = 1;
        switch(state->viewerState->vpConfigs[VPfound].type) {
        case VIEWPORT_XY:
            emit setRecenteringPositionSignal(state->viewerState->currentPosition.x,
                                              state->viewerState->currentPosition.y,
                                              state->viewerState->currentPosition.z - state->viewerState->walkFrames
                                              * state->magnification * state->viewerState->vpKeyDirection[VIEWPORT_XY]);
            Knossos::sendRemoteSignal();
            break;
        case VIEWPORT_XZ:
            emit setRecenteringPositionSignal(state->viewerState->currentPosition.x,
                                              state->viewerState->currentPosition.y - state->viewerState->walkFrames
                                              * state->magnification * state->viewerState->vpKeyDirection[VIEWPORT_XZ],
                                              state->viewerState->currentPosition.z);
            Knossos::sendRemoteSignal();
            break;
        case VIEWPORT_YZ:
            emit setRecenteringPositionSignal(state->viewerState->currentPosition.x - state->viewerState->walkFrames
                                              * state->magnification * state->viewerState->vpKeyDirection[VIEWPORT_YZ],
                                              state->viewerState->currentPosition.y,
                                              state->viewerState->currentPosition.z);
            Knossos::sendRemoteSignal();
            break;
        case VIEWPORT_ARBITRARY:
            emit setRecenteringPositionSignal((int)state->viewerState->currentPosition.x - state->viewerState->walkFrames
                                * roundFloat(state->viewerState->vpConfigs[VPfound].n.x) * state->magnification,
                        (int)state->viewerState->currentPosition.y - state->viewerState->walkFrames
                                * roundFloat(state->viewerState->vpConfigs[VPfound].n.y) * state->magnification,
                        (int)state->viewerState->currentPosition.z - state->viewerState->walkFrames
                                * roundFloat(state->viewerState->vpConfigs[VPfound].n.z) * state->magnification);
            Knossos::sendRemoteSignal();
            break;
        }
    } else if (event->key() == Qt::Key_D || event->key() == Qt::Key_F) {
        state->keyD = event->key() == Qt::Key_D;
        state->keyF = event->key() == Qt::Key_F;
        if (!state->viewerKeyRepeat) {
            const float directionSign = event->key() == Qt::Key_D ? -1 : 1;
            const float shiftMultiplier = shift? 10 : 1;
            const float multiplier = directionSign * state->viewerState->dropFrames * state->magnification * shiftMultiplier;
            switch(state->viewerState->vpConfigs[VPfound].type) {
            case VIEWPORT_XY:
                state->repeatDirection = {{0, 0, multiplier * state->viewerState->vpKeyDirection[VIEWPORT_XY]}};
                emit userMoveSignal(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2]);
                break;
            case VIEWPORT_XZ:
                state->repeatDirection = {{0, multiplier * state->viewerState->vpKeyDirection[VIEWPORT_XZ], 0}};
                emit userMoveSignal(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2]);
                break;
            case VIEWPORT_YZ:
                state->repeatDirection = {{multiplier * state->viewerState->vpKeyDirection[VIEWPORT_YZ], 0, 0}};
                emit userMoveSignal(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2]);
                break;
            case VIEWPORT_ARBITRARY:
                state->repeatDirection = {{
                    multiplier * state->viewerState->vpConfigs[VPfound].n.x
                    , multiplier * state->viewerState->vpConfigs[VPfound].n.y
                    , multiplier * state->viewerState->vpConfigs[VPfound].n.z
                }};
                emit userMoveArbSignal(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2]);
                break;
            }
        }
    } else if (event->key() == Qt::Key_Shift) {
        state->repeatDirection[0] *= 10;
        state->repeatDirection[1] *= 10;
        state->repeatDirection[2] *= 10;
    } else if(event->key() == Qt::Key_K) {
        if(state->viewerState->vpOrientationLocked == false) {
            state->viewerState->alphaCache += 1;
            emit setViewportOrientationSignal(VIEWPORT_ARBITRARY);
        }
        else {
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Information);
            prompt.setWindowTitle("Information");
            prompt.setText("Viewport orientation is still locked. Uncheck 'Lock VP Orientation' first.");
            prompt.exec();
        }
    } else if(event->key() == Qt::Key_L) {
        if(state->viewerState->vpOrientationLocked == false) {
            state->viewerState->betaCache += 1;
            emit setViewportOrientationSignal(VIEWPORT_ARBITRARY);
        }
        else {
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Information);
            prompt.setWindowTitle("Information");
            prompt.setText("Viewport orientation is still locked. Uncheck 'Lock VP Orientation' first.");
            prompt.exec();
        }
    } else if(event->key() == Qt::Key_M) {
        if(state->viewerState->vpOrientationLocked == false) {
            state->viewerState->alphaCache -= 1;
            emit setViewportOrientationSignal(VIEWPORT_ARBITRARY);
        }
        else {
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Information);
            prompt.setWindowTitle("Information");
            prompt.setText("Viewport orientation is still locked. Uncheck 'Lock VP Orientation' first.");
            prompt.exec();
        }
    } else if(event->key() == Qt::Key_Comma) {
        if(state->viewerState->vpOrientationLocked == false) {
            state->viewerState->betaCache -= 1;
            emit setViewportOrientationSignal(VIEWPORT_ARBITRARY);
        }
        else {
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Information);
            prompt.setWindowTitle("Information");
            prompt.setText("Viewport orientation is still locked. Uncheck 'Lock VP Orientation' first.");
            prompt.exec();
        }
    } else if(event->key() == Qt::Key_G) {
        //emit genTestNodesSignal(50000);
        // emit updateTreeviewSignal();

    } else if(event->key() == Qt::Key_3) {
        if(state->viewerState->drawVPCrosshairs) {
           state->viewerState->drawVPCrosshairs = false;
        }
        else {
           state->viewerState->drawVPCrosshairs = true;
        }
        emit updateSlicePlaneWidgetSignal();

    } else if(event->key() == Qt::Key_I) {
        if(VPfound != VIEWPORT_SKELETON) {
            emit zoomOrthoSignal(-0.1);
        }
        else if (state->skeletonState->zoomLevel <= SKELZOOMMAX) {
            emit zoomInSkeletonVPSignal();
        }
    } else if(event->key() == Qt::Key_O) {
        if(VPfound != VIEWPORT_SKELETON) {
            emit zoomOrthoSignal(0.1);
        }
        else if (state->skeletonState->zoomLevel >= SKELZOOMMIN) {
            emit zoomOutSkeletonVPSignal();
        }
    } else if(event->key() == Qt::Key_V) {
       if(control) {
           emit pasteCoordinateSignal();
       }
    } else if(event->key() == Qt::Key_1) { // !
        const auto & vpSettings = state->viewer->window->widgetContainer->viewportSettingsWidget->generalTabWidget;
        const auto hideSkeletonOrtho = vpSettings->hideSkeletonOrthoVPsCheckBox.isChecked();
        vpSettings->hideSkeletonOrthoVPsCheckBox.setChecked(!hideSkeletonOrtho);
        vpSettings->hideSkeletonOrthoVPsCheckBox.clicked(!hideSkeletonOrtho);
        state->skeletonState->skeletonChanged = true;//idk
    } else if(event->key() == Qt::Key_Plus) {
        if(Segmentation::singleton().alpha + 10 > 255) {
            Segmentation::singleton().alpha = 255;
        }
        else {
            Segmentation::singleton().alpha += 10;
        }
        const auto & sliceVPSettings = state->viewer->window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget;
        sliceVPSettings->segmenationOverlaySlider.setValue(Segmentation::singleton().alpha);
    } else if(event->key() == Qt::Key_Minus) {
        if(Segmentation::singleton().alpha - 10 < 0) {
            Segmentation::singleton().alpha = 0;
        }
        else {
            Segmentation::singleton().alpha -= 10;
        }
        const auto & sliceVPSettings = state->viewer->window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget;
        sliceVPSettings->segmenationOverlaySlider.setValue(Segmentation::singleton().alpha);
    } else if(event->key() == Qt::Key_Space) {
        state->overlay = false;
    } else if(event->key() == Qt::Key_Delete) {
        if(state->skeletonState->selectedNodes.size() > 0) {
            bool deleteNodes = true;
            if(state->skeletonState->selectedNodes.size() != 1) {
                QMessageBox prompt;
                prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
                prompt.setIcon(QMessageBox::Question);
                prompt.setWindowTitle("Cofirmation required");
                prompt.setText("Delete selected nodes?");
                QPushButton *confirmButton = prompt.addButton("Yes", QMessageBox::ActionRole);
                prompt.addButton("No", QMessageBox::ActionRole);
                prompt.exec();
                deleteNodes = prompt.clickedButton() == confirmButton;
            }
            if(deleteNodes) {
                emit deleteSelectedNodesSignal();//skeletonizer
                emit updateTreeviewSignal();//annotation->treeview
            }
        }
    }
    else if(event->key() == Qt::Key_Escape) {
        if(state->skeletonState->selectedNodes.size() == 1
           && state->skeletonState->activeNode->selected) {
            // active node must always be selected if nothing else is selected.
            return;
        }
        if(state->skeletonState->selectedNodes.empty() == false) {
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Question);
            prompt.setWindowTitle("Cofirmation required");
            prompt.setText("Unselect current node selection?");
            QPushButton *confirmButton = prompt.addButton("Yes", QMessageBox::ActionRole);
            prompt.addButton("No", QMessageBox::ActionRole);
            prompt.exec();
            if(prompt.clickedButton() == confirmButton) {
                emit unselectNodesSignal();
            }
        }
    }
    else if(event->key() == Qt::Key_F4) {
        if(alt) {
            QApplication::closeAllWindows();
            QApplication::quit();
        }
    }
}

void EventModel::handleKeyRelease(QKeyEvent *event) {
    if(event->key() == Qt::Key_Space) {
        state->overlay = true;
    }
}

Coordinate EventModel::getCoordinateFromOrthogonalClick(QMouseEvent *event, int VPfound) {
    int x, y, z;

    // These variables store the distance in screen pixels from the left and
    // upper border from the user mouse click to the VP boundaries.
    int xDistance = event->x();
    int yDistance = event->y();

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
        case VIEWPORT_ARBITRARY:
            x = roundFloat(state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen_float.x
                    + ((float)xDistance / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v1.x
                    + ((float)yDistance / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v2.x
                );

            y = roundFloat(state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen_float.y
                    + ((float)xDistance / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v1.y
                    + ((float)yDistance / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v2.y
                );

            z = roundFloat(state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen_float.z
                    + ((float)xDistance / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v1.z
                    + ((float)yDistance / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v2.z
                );
            break;
    }
    return Coordinate(x, y, z);
}

bool EventModel::validPosition(QMouseEvent *event, int VPfound) {
    if((VPfound == -1) || (state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON)) {
            return false;
    }

    Coordinate pos = getCoordinateFromOrthogonalClick(event, VPfound);
    //check if coordinates are in range
    if((pos.x < 0) || (pos.x > state->boundary.x)
        ||(pos.y < 0) || (pos.y > state->boundary.y)
        ||(pos.z < 0) || (pos.z > state->boundary.z)) {
        return false;
    }
    return true;
}

void EventModel::startNodeSelection(int x, int y, int vpId) {
    state->viewerState->nodeSelectionSquare.first.x = x;
    state->viewerState->nodeSelectionSquare.first.y = y;

    // reset second point from a possible previous selection square.
    CPY_COORDINATE(state->viewerState->nodeSelectionSquare.second,
                   state->viewerState->nodeSelectionSquare.first);
    state->viewerState->nodeSelectSquareVpId = vpId;
}

void EventModel::nodeSelection(int x, int y, int vpId) {
    // node selection square
    state->viewerState->nodeSelectionSquare.second.x = x;
    state->viewerState->nodeSelectionSquare.second.y = y;
    Coordinate first = state->viewerState->nodeSelectionSquare.first;
    Coordinate second = state->viewerState->nodeSelectionSquare.second;
    // create square
    int minX, maxX, minY, maxY;
    minX = std::min(first.x, second.x);
    maxX = std::max(first.x, second.x);
    minY = std::min(first.y, second.y);
    maxY = std::max(first.y, second.y);
    //unselect all nodes before retrieving new selection
    for (auto & elem : state->skeletonState->selectedNodes) {
        elem->selected = false;
    }
    state->skeletonState->selectedNodes = state->viewer->renderer->retrieveAllObjectsBeneathSquare(vpId, minX, minY, maxX - minX, maxY - minY);
    //mark all found nodes as selected
    for (auto & elem : state->skeletonState->selectedNodes) {
        elem->selected = true;
    }
    if(state->skeletonState->selectedNodes.size() == 1) {
        setActiveNodeSignal(state->skeletonState->selectedNodes[0],
                                           state->skeletonState->selectedNodes[0]->nodeID);
    }
    else if (state->skeletonState->selectedNodes.empty() && state->skeletonState->activeNode) {
        // at least one must always be selected
        state->skeletonState->activeNode->selected = true;
        state->skeletonState->selectedNodes.emplace_back(state->skeletonState->activeNode);
    }
    emit updateTreeviewSignal();
    state->viewerState->nodeSelectSquareVpId = -1;
}

int EventModel::xrel(int x) {
    return (x - this->mouseX);
}

int EventModel::yrel(int y) {
    return (y - this->mouseY);
}


