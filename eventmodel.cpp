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
#include "session.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "renderer.h"
#include "segmentation/cubeloader.h"
#include "segmentation/segmentation.h"
#include "segmentation/segmentationsplit.h"
#include "viewer.h"
#include "renderer.h"
#include "widgets/navigationwidget.h"
#include "widgets/viewport.h"
#include "widgets/viewportsettings/vpgeneraltabwidget.h"
#include "widgets/viewportsettings/vpsliceplaneviewportwidget.h"

#include <boost/math/constants/constants.hpp>

#include <cstdlib>
#include <unordered_set>

EventModel::EventModel(QObject *parent) : QObject(parent) {}

uint64_t segmentationColorPickingPoint(const int x, const int y, const int viewportId) {
    return readVoxel(getCoordinateFromOrthogonalClick(x, y, viewportId));
}

void segmentation_work(QMouseEvent *event, const int vp) {
    const Coordinate coord = getCoordinateFromOrthogonalClick(event->x(), event->y(), vp);
    auto& seg = Segmentation::singleton();

    if (seg.brush.getTool() == brush_t::tool_t::merge) {
        merging(event, vp);
    } else {//paint and erase
        if (!seg.brush.isInverse() && seg.selectedObjectsCount() == 0) {
            seg.createAndSelectObject(coord);
        }
        uint64_t soid = 0;
        if (seg.selectedObjectsCount()) {
            soid = seg.subobjectIdOfFirstSelectedObject();
            seg.updateLocationForFirstSelectedObject(coord);
        }
        writeVoxels(coord, soid, seg.brush);
        state->viewer->window->notifyUnsavedChanges();
    }

    state->viewer->reslice_notify();
}

void merging(QMouseEvent *event, const int vp) {
    auto & seg = Segmentation::singleton();
    const auto subobjectIds = readVoxels(getCoordinateFromOrthogonalClick(event->x(), event->y(), vp), seg.brush);
    Coordinate clickPos = getCoordinateFromOrthogonalClick(event->x(), event->y(), vp);
    for(const auto subobjectId : subobjectIds) {
        if (seg.selectedObjectsCount() == 1) {
            auto & subobject = seg.subobjectFromId(subobjectId, clickPos);
            const auto objectToMergeId = seg.smallestImmutableObjectContainingSubobject(subobject);
            // if clicked object is currently selected, an unmerge is requested
            if (seg.isSelected(subobject)) {
                if (event->modifiers().testFlag(Qt::ShiftModifier)) {
                    if (event->modifiers().testFlag(Qt::ControlModifier)) {
                        seg.selectObjectFromSubObject(subobject, clickPos);
                        seg.unmergeSelectedObjects(clickPos);
                    } else {
                        if(seg.isSelected(objectToMergeId)) { // if no other object to unmerge, just unmerge subobject
                            seg.selectObjectFromSubObject(subobject, clickPos);
                        }
                        else {
                            seg.selectObject(objectToMergeId);
                        }
                        seg.unmergeSelectedObjects(clickPos);
                    }
                }
            } else { // object is not selected, so user wants to merge
                if (!event->modifiers().testFlag(Qt::ShiftModifier)) {
                    if (event->modifiers().testFlag(Qt::ControlModifier)) {
                        seg.selectObjectFromSubObject(subobject, clickPos);
                    } else {
                        seg.selectObject(objectToMergeId);//select largest object
                    }
                }
                if (seg.selectedObjectsCount() >= 2) {
                    seg.mergeSelectedObjects();
                }
            }
            seg.touchObjects(subobjectId);
        }
    }
}

void EventModel::handleMouseHover(QMouseEvent *event, int VPfound) {
    auto subObjectId = readVoxel(getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound));
    Segmentation::singleton().mouseFocusedObjectId = Segmentation::singleton().tryLargestObjectContainingSubobject(subObjectId);
}

bool EventModel::handleMouseButtonLeft(QMouseEvent *event, int VPfound) {
    mouseDownX = event->x();
    mouseDownY = event->y();
    auto & skel = Skeletonizer::singleton();
    //new active node selected
    if(QApplication::keyboardModifiers() == Qt::ShiftModifier) {
        //first assume that user managed to hit the node
        auto clickedNode = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->x(), event->y(), 10);

        if(clickedNode) {
            skel.setActiveNode(NULL, clickedNode);
            return true;
        }

        if(VPfound == VIEWPORT_SKELETON) {
            return false;
        }
        return false;
    } else if (QApplication::keyboardModifiers() == Qt::ControlModifier && Session::singleton().annotationMode == SkeletonizationMode) {
        startNodeSelection(event->pos().x(), event->pos().y(), VPfound);
    } else if(state->viewerState->vpConfigs[VPfound].type != VIEWPORT_SKELETON) {
        // check click mode of orthogonal viewports
        if (state->viewerState->clickReaction == ON_CLICK_RECENTER) {
            if(mouseEventAtValidDatasetPosition(event, VPfound)) {
                Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound);
                emit setRecenteringPositionSignal(clickedCoordinate.x, clickedCoordinate.y, clickedCoordinate.z);
                Knossos::sendRemoteSignal();
            }
        }
    }


    //Set Connection between Active Node and Clicked Node
    if(QApplication::keyboardModifiers() == Qt::ALT) {
        auto clickedNode = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->x(), event->y(), 10);
        if(clickedNode) {
            auto activeNode = state->skeletonState->activeNode;
            if(activeNode) {
                if (skel.findSegmentByNodeIDs(activeNode->nodeID, clickedNode)) {
                    emit delSegmentSignal(activeNode->nodeID, clickedNode, NULL);
                } else if (skel.findSegmentByNodeIDs(clickedNode, activeNode->nodeID)) {
                    emit delSegmentSignal(clickedNode, activeNode->nodeID, NULL);
                } else{
                    if(skel.simpleTracing && Skeletonizer::singleton().areConnected(*activeNode, *Skeletonizer::findNodeByNodeID(clickedNode))) {
                        QMessageBox::information(nullptr, "Cycle detected!",
                                                 "If you want to allow cycles, please deactivate Simple Tracing under 'Edit Skeleton'.");
                        return false;
                    }
                    emit addSegmentSignal(activeNode->nodeID, clickedNode);
                }
            }
        }
    }
    return true;
}

bool EventModel::handleMouseButtonMiddle(QMouseEvent *event, int VPfound) {
    auto clickedNode = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->x(), event->y(), 10);

    if(clickedNode) {
        auto & skel = Skeletonizer::singleton();
        auto activeNode = state->skeletonState->activeNode;
        Qt::KeyboardModifiers keyMod = QApplication::keyboardModifiers();
        if(keyMod.testFlag(Qt::ShiftModifier)) {
            // Toggle segment between clicked and active node
            if(activeNode) {
                if(skel.findSegmentByNodeIDs(activeNode->nodeID, clickedNode)) {
                    emit delSegmentSignal(activeNode->nodeID, clickedNode, 0);
                } else if (skel.findSegmentByNodeIDs(clickedNode, activeNode->nodeID)) {
                    emit delSegmentSignal(clickedNode, activeNode->nodeID, 0);
                } else {
                    if(skel.simpleTracing && Skeletonizer::singleton().areConnected(*activeNode, *Skeletonizer::findNodeByNodeID(clickedNode))) {
                        QMessageBox::information(nullptr, "Cycle detected!",
                                                 "If you want to allow cycles, please deactivate Simple Tracing under 'Edit Skeleton'.");
                        return false;
                    }
                    emit addSegmentSignal(activeNode->nodeID, clickedNode);
                }
            }
        } else {
            // No modifier pressed
            state->viewerState->vpConfigs[VPfound].draggedNode = Skeletonizer::findNodeByNodeID(clickedNode);
        }
    }
    return true;
}

void EventModel::handleMouseButtonRight(QMouseEvent *event, int VPfound) {
    if(mouseEventAtValidDatasetPosition(event, VPfound) == false) {
        return;
    }
    if (Session::singleton().annotationMode == SegmentationMode && VPfound != VIEWPORT_SKELETON) {
        Segmentation::singleton().brush.setInverse(event->modifiers().testFlag(Qt::ShiftModifier));
        if (event->x() != rightMouseDownX && event->y() != rightMouseDownY) {
             rightMouseDownX = event->x();
             rightMouseDownY = event->y();
             segmentation_work(event, VPfound);
        }
        return;
    }
    Coordinate movement, lastPos;
    Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound);

    bool newNode = false;
    bool newTree = state->skeletonState->activeTree == nullptr;//if there was no active tree, a new node will create one
    switch (state->viewer->skeletonizer->getTracingMode()) {
    case Skeletonizer::TracingMode::unlinkedNodes:
        newNode = Skeletonizer::singleton().UI_addSkeletonNode(&clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
        break;
    case Skeletonizer::TracingMode::skipNextLink:
        newNode = Skeletonizer::singleton().UI_addSkeletonNode(&clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
        state->viewer->skeletonizer->setTracingMode(Skeletonizer::TracingMode::linkedNodes);//as we only wanted to skip one link
        break;
    case Skeletonizer::TracingMode::linkedNodes:
        if (state->skeletonState->activeNode == nullptr || state->skeletonState->activeTree->firstNode == nullptr) {
            //no node to link with or no empty tree
            newNode = Skeletonizer::singleton().UI_addSkeletonNode(&clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
            break;
        } else if (event->modifiers().testFlag(Qt::ControlModifier)) {
            //Add a "stump", a branch node to which we don't automatically move.
            uint newNodeID;
            if((newNodeID = Skeletonizer::singleton().addSkeletonNodeAndLinkWithActive(&clickedCoordinate,
                                                                state->viewerState->vpConfigs[VPfound].type,
                                                                false))) {
                Skeletonizer::singleton().pushBranchNode(true, true, NULL, newNodeID);
                Skeletonizer::singleton().setActiveNode(nullptr, newNodeID);
            }
            break;
        }
        //Add a node and apply tracing modes
        lastPos = state->skeletonState->activeNode->position; //remember last active for movement calculation
        newNode = Skeletonizer::singleton().addSkeletonNodeAndLinkWithActive(&clickedCoordinate,
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
        if (state->viewerState->autoTracingMode == navigationMode::additionalVPMove) {
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

        if ((state->viewerState->autoTracingMode == navigationMode::additionalTracingDirectionMove)
            || (state->viewerState->autoTracingMode == navigationMode::additionalMirroredMove)) {
            floatCoordinate walkingVector;
            walkingVector.x = movement.x;
            walkingVector.y = movement.y;
            walkingVector.z = movement.z;
            //Additional move of specified steps along tracing direction
            if (state->viewerState->autoTracingMode == navigationMode::additionalTracingDirectionMove) {
                clickedCoordinate.x += roundFloat(walkingVector.x * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
                clickedCoordinate.y += roundFloat(walkingVector.y * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
                clickedCoordinate.z += roundFloat(walkingVector.z * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
            }
            //Additional move of steps equal to distance between last and new node along tracing direction.
            if (state->viewerState->autoTracingMode == navigationMode::additionalMirroredMove) {
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
        if (state->viewerState->vpConfigs[VPfound].type == VIEWPORT_ARBITRARY) {
            emit setRecenteringPositionWithRotationSignal(clickedCoordinate.x, clickedCoordinate.y, clickedCoordinate.z, VPfound);
        } else {
            emit setRecenteringPositionSignal(clickedCoordinate.x, clickedCoordinate.y, clickedCoordinate.z);
        }
    }
    emit updateViewerStateSignal();
    if (state->viewerState->autoTracingMode != navigationMode::noRecentering) {
        Knossos::sendRemoteSignal();
    }
}

bool EventModel::handleMouseMotionLeftHold(QMouseEvent *event, int VPfound) {
    // pull selection square
    if (state->viewerState->nodeSelectSquareVpId != -1) {
        state->viewerState->nodeSelectionSquare.second.x = event->pos().x();
        state->viewerState->nodeSelectionSquare.second.y = event->pos().y();
        return true;
    }

    switch(state->viewerState->vpConfigs[VPfound].type) {
    // the user wants to drag the skeleton inside the VP
    case VIEWPORT_SKELETON:
    state->skeletonState->translateX += -xrel(event->x()) * 2.
            * ((float)state->skeletonState->volBoundary
            * (0.5 - state->skeletonState->zoomLevel))
            / ((float)state->viewerState->vpConfigs[VPfound].edgeLength);
    state->skeletonState->translateY += -yrel(event->y()) * 2.
            * ((float)state->skeletonState->volBoundary
            * (0.5 - state->skeletonState->zoomLevel))
            / ((float)state->viewerState->vpConfigs[VPfound].edgeLength);
        break;
    case VIEWPORT_XY:
        if(state->viewerState->clickReaction != ON_CLICK_DRAG) break;
        state->viewerState->vpConfigs[VPfound].userMouseSlideX -=
                ((float)xrel(event->x())
            / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx);
        state->viewerState->vpConfigs[VPfound].userMouseSlideY -=
                ((float)yrel(event->y())
            / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx);
        if(fabs(state->viewerState->vpConfigs[VPfound].userMouseSlideX) >= 1
            || fabs(state->viewerState->vpConfigs[VPfound].userMouseSlideY) >= 1) {

            emit userMoveSignal((int)state->viewerState->vpConfigs[VPfound].userMouseSlideX,
                (int)state->viewerState->vpConfigs[VPfound].userMouseSlideY, 0,
                                USERMOVE_HORIZONTAL, state->viewerState->vpConfigs[VPfound].type);
            state->viewerState->vpConfigs[VPfound].userMouseSlideX = 0.;
            state->viewerState->vpConfigs[VPfound].userMouseSlideY = 0.;
        }
        break;
    case VIEWPORT_XZ:
        if(state->viewerState->clickReaction != ON_CLICK_DRAG) break;
        state->viewerState->vpConfigs[VPfound].userMouseSlideX -=
                ((float)xrel(event->x()) / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx);
        state->viewerState->vpConfigs[VPfound].userMouseSlideY -=
                ((float)yrel(event->y()) / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx);
        if(fabs(state->viewerState->vpConfigs[VPfound].userMouseSlideX) >= 1
            || fabs(state->viewerState->vpConfigs[VPfound].userMouseSlideY) >= 1) {

            emit userMoveSignal((int)state->viewerState->vpConfigs[VPfound].userMouseSlideX, 0,
                (int)state->viewerState->vpConfigs[VPfound].userMouseSlideY,
                                USERMOVE_HORIZONTAL, state->viewerState->vpConfigs[VPfound].type);
            state->viewerState->vpConfigs[VPfound].userMouseSlideX = 0.;
            state->viewerState->vpConfigs[VPfound].userMouseSlideY = 0.;
        }
        break;
    case VIEWPORT_YZ:
        if(state->viewerState->clickReaction != ON_CLICK_DRAG) break;
        state->viewerState->vpConfigs[VPfound].userMouseSlideX -=
                ((float)xrel(event->x()) / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx);
        state->viewerState->vpConfigs[VPfound].userMouseSlideY -=
                ((float)yrel(event->y()) / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx);
        if(fabs(state->viewerState->vpConfigs[VPfound].userMouseSlideX) >= 1
            || fabs(state->viewerState->vpConfigs[VPfound].userMouseSlideY) >= 1) {

            emit userMoveSignal(0, (int)state->viewerState->vpConfigs[VPfound].userMouseSlideY,
                (int)state->viewerState->vpConfigs[VPfound].userMouseSlideX,
                                USERMOVE_HORIZONTAL, state->viewerState->vpConfigs[VPfound].type);
            state->viewerState->vpConfigs[VPfound].userMouseSlideX = 0.;
            state->viewerState->vpConfigs[VPfound].userMouseSlideY = 0.;
        }
        break;
    case VIEWPORT_ARBITRARY:
        if(state->viewerState->clickReaction != ON_CLICK_DRAG) {
            break;
        }
        state->viewerState->vpConfigs[VPfound].userMouseSlideX -=
                ((float)xrel(event->x()) / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx);
        state->viewerState->vpConfigs[VPfound].userMouseSlideY -=
                ((float)yrel(event->y()) / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx);

        if(fabs(state->viewerState->vpConfigs[VPfound].userMouseSlideX) >= 1
            || fabs(state->viewerState->vpConfigs[VPfound].userMouseSlideY) >= 1) {
            emit userMoveArbSignal(
                    (int)(state->viewerState->vpConfigs[VPfound].v1.x
                          * state->viewerState->vpConfigs[VPfound].userMouseSlideX
                          + state->viewerState->vpConfigs[VPfound].v2.x
                          * state->viewerState->vpConfigs[VPfound].userMouseSlideY),
                    (int)(state->viewerState->vpConfigs[VPfound].v1.y
                          * state->viewerState->vpConfigs[VPfound].userMouseSlideX
                          + state->viewerState->vpConfigs[VPfound].v2.y
                          * state->viewerState->vpConfigs[VPfound].userMouseSlideY),
                    (int)(state->viewerState->vpConfigs[VPfound].v1.z
                          * state->viewerState->vpConfigs[VPfound].userMouseSlideX
                          + state->viewerState->vpConfigs[VPfound].v2.z
                          * state->viewerState->vpConfigs[VPfound].userMouseSlideY));
            state->viewerState->vpConfigs[VPfound].userMouseSlideX = 0.;
            state->viewerState->vpConfigs[VPfound].userMouseSlideY = 0.;
        }
        break;
    }

    return true;
}

bool EventModel::handleMouseMotionMiddleHold(QMouseEvent *event, int /*VPfound*/) {

    Coordinate newDraggedNodePos;

    for(uint i = 0; i < Viewport::numberViewports; i++) {
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

                    Skeletonizer::singleton().editNode(
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
                    Skeletonizer::singleton().editNode(
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
                    Skeletonizer::singleton().editNode(
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
    if (Session::singleton().annotationMode == SegmentationMode && VPfound != VIEWPORT_SKELETON) {
        const bool notOrigin = event->x() != rightMouseDownX && event->y() != rightMouseDownY;
        if (mouseEventAtValidDatasetPosition(event, VPfound) && notOrigin) {
            segmentation_work(event, VPfound);
        }
        return;
    }
    if (VPfound == VIEWPORT_SKELETON && state->skeletonState->rotationcounter == 0) {
        state->skeletonState->rotdx += xrel(event->x());
        state->skeletonState->rotdy += yrel(event->y());
    }
}

void EventModel::handleMouseReleaseLeft(QMouseEvent *event, int VPfound) {
    auto & segmentation = Segmentation::singleton();
    if (Session::singleton().annotationMode == SegmentationMode && segmentation.job.active == false && mouseEventAtValidDatasetPosition(event, VPfound)) { // in task mode the object should not be switched
        if(event->x() == mouseDownX && event->y() == mouseDownY) {
            const auto clickPos = getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound);
            const auto subobjectId = readVoxel(getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound));
            if (subobjectId != 0) {// don’t select the unsegmented area as object
                auto & subobject = segmentation.subobjectFromId(subobjectId, clickPos);
                auto objIndex = segmentation.largestObjectContainingSubobject(subobject);
                if (!event->modifiers().testFlag(Qt::ControlModifier)) {
                    segmentation.clearObjectSelection();
                    segmentation.selectObject(objIndex);
                } else if (segmentation.isSelected(objIndex)) {// unselect if selected
                    segmentation.unselectObject(objIndex);
                } else { // select largest object
                    segmentation.selectObject(objIndex);
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
        auto nodeID = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->pos().x(), event->pos().y(), 10);
        if (nodeID != 0) {
            nodeListElement *selectedNode = Skeletonizer::findNodeByNodeID(nodeID);
            if (selectedNode != nullptr) {
                Skeletonizer::singleton().toggleNodeSelection({selectedNode});
            }
        }
    } else if (state->viewerState->nodeSelectSquareVpId != -1) {
        nodeSelection(event->pos().x(), event->pos().y(), VPfound);
    }
}

void EventModel::handleMouseReleaseRight(QMouseEvent *event, int VPfound) {
    if (Session::singleton().annotationMode == SegmentationMode && mouseEventAtValidDatasetPosition(event, VPfound)) {
        if (event->x() != rightMouseDownX && event->y() != rightMouseDownY) {//merge took already place on mouse down
            segmentation_work(event, VPfound);
        }
        rightMouseDownX = rightMouseDownY = -1;
        return;
    }
}

void EventModel::handleMouseReleaseMiddle(QMouseEvent * event, int VPfound) {
    if (mouseEventAtValidDatasetPosition(event, VPfound) && Session::singleton().annotationMode == SegmentationMode && Segmentation::singleton().selectedObjectsCount() == 1) {
        Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound);
        connectedComponent(clickedCoordinate);
    }
}

void EventModel::handleMouseWheel(QWheelEvent * const event, int VPfound) {
    const int directionSign = event->delta() > 0 ? -1 : 1;
    auto& seg = Segmentation::singleton();

    if (Session::singleton().annotationMode == SkeletonizationMode
            && event->modifiers() == Qt::SHIFT
            && state->skeletonState->activeNode != nullptr)
    {//change node radius
        float radius = state->skeletonState->activeNode->radius + directionSign * 0.2 * state->skeletonState->activeNode->radius;

        Skeletonizer::singleton().editNode(0, state->skeletonState->activeNode, radius
                 , state->skeletonState->activeNode->position.x
                 , state->skeletonState->activeNode->position.y
                 , state->skeletonState->activeNode->position.z
                 , state->magnification);
    } else if (Session::singleton().annotationMode == SegmentationMode && event->modifiers() == Qt::SHIFT) {
        seg.brush.setRadius(seg.brush.getRadius() + event->delta() / 120);
        if(seg.brush.getRadius() < 0) {
            seg.brush.setRadius(0);
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
        const auto multiplier = directionSign * (int)state->viewerState->dropFrames * state->magnification;
        const auto type = state->viewerState->vpConfigs[VPfound].type;
        if (type == VIEWPORT_XY) {
            emit userMoveSignal(0, 0, multiplier, USERMOVE_DRILL, type);
        } else if (type == VIEWPORT_XZ) {
            emit userMoveSignal(0, multiplier, 0, USERMOVE_DRILL, type);
        } else if (type == VIEWPORT_YZ) {
            emit userMoveSignal(multiplier, 0, 0, USERMOVE_DRILL, type);
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

    const auto type = state->viewerState->vpConfigs[VPfound].type;

    // new qt version
    if(event->key() == Qt::Key_Left) {
        if(shift) {
            switch(type) {
            case VIEWPORT_XY:
                emit userMoveSignal(-10 * state->magnification, 0, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(-10 * state->magnification, 0, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 0, -10 * state->magnification,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(
                         -10 * state->viewerState->vpConfigs[VPfound].v1.x * state->magnification,
                         -10 * state->viewerState->vpConfigs[VPfound].v1.y * state->magnification,
                         -10 * state->viewerState->vpConfigs[VPfound].v1.z * state->magnification);
                break;
            }
        } else {
            switch(type) {
            case VIEWPORT_XY:
                emit userMoveSignal(-state->viewerState->dropFrames * state->magnification, 0, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(-state->viewerState->dropFrames * state->magnification, 0, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 0, -state->viewerState->dropFrames * state->magnification,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(-state->viewerState->vpConfigs[VPfound].v1.x
                            * state->viewerState->dropFrames * state->magnification,
                        -state->viewerState->vpConfigs[VPfound].v1.y
                            * state->viewerState->dropFrames * state->magnification,
                        -state->viewerState->vpConfigs[VPfound].v1.z
                            * state->viewerState->dropFrames * state->magnification);
                break;
            }
        }
    } else if(event->key() == Qt::Key_Right) {
        if(shift) {
            switch(type) {
            case VIEWPORT_XY:
                emit userMoveSignal(10 * state->magnification, 0, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(10 * state->magnification, 0, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 0, 10 * state->magnification,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(
                            10 * state->viewerState->vpConfigs[VPfound].v1.x * state->magnification,
                            10 * state->viewerState->vpConfigs[VPfound].v1.y * state->magnification,
                            10 * state->viewerState->vpConfigs[VPfound].v1.z * state->magnification);
                 break;
            }
        } else {
            switch(type) {
            case VIEWPORT_XY:
                emit userMoveSignal(state->viewerState->dropFrames * state->magnification, 0, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(state->viewerState->dropFrames * state->magnification, 0, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 0, state->viewerState->dropFrames * state->magnification,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(state->viewerState->vpConfigs[VPfound].v1.x
                            * state->viewerState->dropFrames * state->magnification,
                        state->viewerState->vpConfigs[VPfound].v1.y
                            * state->viewerState->dropFrames * state->magnification,
                        state->viewerState->vpConfigs[VPfound].v1.z
                            * state->viewerState->dropFrames * state->magnification);
                break;
            }
        }
    } else if(event->key() == Qt::Key_Down) {
        if(shift) {
            switch(type) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, -10 * state->magnification, 0,
                                        USERMOVE_HORIZONTAL, type);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, 0, -10 * state->magnification,
                                        USERMOVE_HORIZONTAL, type);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, -10 * state->magnification, 0,
                                        USERMOVE_HORIZONTAL, type);
                    break;
                case VIEWPORT_ARBITRARY:
                    emit userMoveArbSignal(
                        -10 * state->viewerState->vpConfigs[VPfound].v2.x * state->magnification,
                        -10 * state->viewerState->vpConfigs[VPfound].v2.y * state->magnification,
                        -10 * state->viewerState->vpConfigs[VPfound].v2.z * state->magnification);
                     break;
            }
        } else {
            switch(type) {
                case VIEWPORT_XY:
                    emit userMoveSignal(0, -state->viewerState->dropFrames * state->magnification, 0,
                                        USERMOVE_HORIZONTAL, type);
                    break;
                case VIEWPORT_XZ:
                    emit userMoveSignal(0, 0, -state->viewerState->dropFrames * state->magnification,
                                        USERMOVE_HORIZONTAL, type);
                    break;
                case VIEWPORT_YZ:
                    emit userMoveSignal(0, -state->viewerState->dropFrames * state->magnification, 0,
                                        USERMOVE_HORIZONTAL, type);
                    break;
                case VIEWPORT_ARBITRARY:
                    emit userMoveArbSignal(-state->viewerState->vpConfigs[VPfound].v2.x
                            * state->viewerState->dropFrames * state->magnification,
                        -state->viewerState->vpConfigs[VPfound].v2.y
                            * state->viewerState->dropFrames * state->magnification,
                        -state->viewerState->vpConfigs[VPfound].v2.z
                            * state->viewerState->dropFrames * state->magnification);
                    break;
            }
        }
    } else if(event->key() == Qt::Key_Up) {
        if(shift) {
            switch(type) {
            case VIEWPORT_XY:
                emit userMoveSignal(0, 10 * state->magnification, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(0, 0, 10 * state->magnification,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, 10 * state->magnification, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(
                            10 * state->viewerState->vpConfigs[VPfound].v2.x * state->magnification,
                            10 * state->viewerState->vpConfigs[VPfound].v2.y * state->magnification,
                            10 * state->viewerState->vpConfigs[VPfound].v2.z * state->magnification);
                break;
            }
        } else {
            switch(type) {
            case VIEWPORT_XY:
                emit userMoveSignal(0, state->viewerState->dropFrames * state->magnification, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_XZ:
                emit userMoveSignal(0, 0, state->viewerState->dropFrames * state->magnification,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_YZ:
                emit userMoveSignal(0, state->viewerState->dropFrames * state->magnification, 0,
                                    USERMOVE_HORIZONTAL, type);
                break;
            case VIEWPORT_ARBITRARY:
                emit userMoveArbSignal(state->viewerState->vpConfigs[VPfound].v2.x
                            * state->viewerState->dropFrames * state->magnification,
                        state->viewerState->vpConfigs[VPfound].v2.y
                            * state->viewerState->dropFrames * state->magnification,
                        state->viewerState->vpConfigs[VPfound].v2.z
                            * state->viewerState->dropFrames * state->magnification);
                break;
            }
        }
    } else if(event->key() == Qt::Key_R) {
        state->viewerState->walkOrth = 1;
        switch(type) {
        case VIEWPORT_XY:
            emit setRecenteringPositionSignal(state->viewerState->currentPosition.x,
                                              state->viewerState->currentPosition.y,
                                              state->viewerState->currentPosition.z + state->viewerState->walkFrames
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
                            * state->viewerState->vpConfigs[VPfound].n.x * state->magnification,
                    state->viewerState->currentPosition.y + state->viewerState->walkFrames
                            * state->viewerState->vpConfigs[VPfound].n.y * state->magnification,
                    state->viewerState->currentPosition.z + state->viewerState->walkFrames
                            * state->viewerState->vpConfigs[VPfound].n.z * state->magnification);
            Knossos::sendRemoteSignal();
            break;
        }
    } else if(event->key() == Qt::Key_E) {
        state->viewerState->walkOrth = 1;
        switch(type) {
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
            emit setRecenteringPositionSignal(state->viewerState->currentPosition.x - state->viewerState->walkFrames
                                * state->viewerState->vpConfigs[VPfound].n.x * state->magnification,
                        state->viewerState->currentPosition.y - state->viewerState->walkFrames
                                * state->viewerState->vpConfigs[VPfound].n.y * state->magnification,
                        state->viewerState->currentPosition.z - state->viewerState->walkFrames
                                * state->viewerState->vpConfigs[VPfound].n.z * state->magnification);
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
            switch(type) {
            case VIEWPORT_XY:
                state->repeatDirection = {{0, 0, multiplier * state->viewerState->vpKeyDirection[VIEWPORT_XY]}};
                emit userMoveSignal(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2],
                        USERMOVE_DRILL, type);
                break;
            case VIEWPORT_XZ:
                state->repeatDirection = {{0, multiplier * state->viewerState->vpKeyDirection[VIEWPORT_XZ], 0}};
                emit userMoveSignal(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2],
                        USERMOVE_DRILL, type);
                break;
            case VIEWPORT_YZ:
                state->repeatDirection = {{multiplier * state->viewerState->vpKeyDirection[VIEWPORT_YZ], 0, 0}};
                emit userMoveSignal(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2],
                        USERMOVE_DRILL, type);
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
        //enable erase mode on shift down
        Segmentation::singleton().brush.setInverse(true);
    } else if(event->key() == Qt::Key_K || event->key() == Qt::Key_L || event->key() == Qt::Key_M || event->key() == Qt::Key_Comma) {
        if(Viewport::arbitraryOrientation == false) {
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Information);
            prompt.setWindowTitle("Information");
            prompt.setText("Viewport orientation is still locked. Check 'Arbitrary Viewport Orientation' under 'Viewport Settings -> Slice Plane Viewports' first.");
            prompt.exec();
        }
        else {
            switch(event->key()) {
            case Qt::Key_K:
                emit rotationSignal(0., 0., 1., boost::math::constants::pi<float>()/180);
                break;
            case Qt::Key_L:
                emit rotationSignal(0., 1., 0., boost::math::constants::pi<float>()/180);
                break;
            case Qt::Key_M:
                emit rotationSignal(0., 0., 1., -boost::math::constants::pi<float>()/180);
                break;
            case Qt::Key_Comma:
                emit rotationSignal(0., 1., 0., -boost::math::constants::pi<float>()/180);
                break;
            }
        }
    } else if(event->key() == Qt::Key_G) {
        //emit genTestNodesSignal(50000);
        // emit updateTreeviewSignal();
    } else if (event->key() == Qt::Key_0) {
        if (control) {
            emit zoomReset();
        }
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
    } else if(event->key() == Qt::Key_Plus) {
        if(control) {
            Segmentation::singleton().brush.setRadius(Segmentation::singleton().brush.getRadius() + 1);
        } else {
            if(Segmentation::singleton().alpha + 10 > 255) {
                Segmentation::singleton().alpha = 255;
            }
            else {
                Segmentation::singleton().alpha += 10;
            }
            const auto & sliceVPSettings = state->viewer->window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget;
            sliceVPSettings->segmenationOverlaySlider.setValue(Segmentation::singleton().alpha);
        }
    } else if(event->key() == Qt::Key_Minus) {
        if(control) {
            if(Segmentation::singleton().brush.getRadius() > 0) {
                Segmentation::singleton().brush.setRadius(Segmentation::singleton().brush.getRadius() - 1);
            }
        } else {
            if(Segmentation::singleton().alpha - 10 < 0) {
                Segmentation::singleton().alpha = 0;
            }
            else {
                Segmentation::singleton().alpha -= 10;
            }
            const auto & sliceVPSettings = state->viewer->window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget;
            sliceVPSettings->segmenationOverlaySlider.setValue(Segmentation::singleton().alpha);
        }
    } else if(event->key() == Qt::Key_Space) {
        state->overlay = false;
    } else if(event->key() == Qt::Key_Delete) {
        if(control) {
            if(state->skeletonState->activeTree) {
                Skeletonizer::singleton().delTree(state->skeletonState->activeTree->treeID);
            }
        }
        else if(state->skeletonState->selectedNodes.size() > 0) {
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
                Skeletonizer::singleton().deleteSelectedNodes();
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
                Skeletonizer::singleton().setActiveNode(state->skeletonState->activeNode, 0);
            }
        }
    }
    else if(event->key() == Qt::Key_F4) {
        if(alt) {
            QApplication::closeAllWindows();
        }
    }
}

void EventModel::handleKeyRelease(QKeyEvent *event) {
    if(event->key() == Qt::Key_Space) {
        state->overlay = true;
    }
    if (event->key() == Qt::Key_5) {
        static uint originalCompressionRatio;
        if (state->compressionRatio != 0) {
            originalCompressionRatio = state->compressionRatio;
            state->compressionRatio = 0;
        } else {
            state->compressionRatio = originalCompressionRatio;
        }
        state->viewer->changeDatasetMag(DATA_SET);
        emit compressionRatioToggled();
    }
}

Coordinate getCoordinateFromOrthogonalClick(const int x_dist, const int y_dist, int VPfound) {
    int x = 0, y = 0, z = 0;

    switch(state->viewerState->vpConfigs[VPfound].type) {
        case VIEWPORT_XY:
            x = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.x
                + ((int)(((float)x_dist)
                / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx));
            y = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.y
                + ((int)(((float)y_dist)
                / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx));
            z = state->viewerState->currentPosition.z;
            break;
        case VIEWPORT_XZ:
            x = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.x
                + ((int)(((float)x_dist)
                / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx));
            z = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.z
                + ((int)(((float)y_dist)
                / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx));
            y = state->viewerState->currentPosition.y;
            break;
        case VIEWPORT_YZ:
            z = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.z
                + ((int)(((float)x_dist)
                / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx));
            y = state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen.y
                + ((int)(((float)y_dist)
                / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx));
            x = state->viewerState->currentPosition.x;
            break;
        case VIEWPORT_ARBITRARY:
            x = roundFloat(state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen_float.x
                    + ((float)x_dist / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v1.x
                    + ((float)y_dist / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v2.x
                );

            y = roundFloat(state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen_float.y
                    + ((float)x_dist / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v1.y
                    + ((float)y_dist / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v2.y
                );

            z = roundFloat(state->viewerState->vpConfigs[VPfound].leftUpperDataPxOnScreen_float.z
                    + ((float)x_dist / state->viewerState->vpConfigs[VPfound].screenPxXPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v1.z
                    + ((float)y_dist / state->viewerState->vpConfigs[VPfound].screenPxYPerDataPx)
                    * state->viewerState->vpConfigs[VPfound].v2.z
                );
            break;
    }
    return Coordinate(x, y, z);
}

bool EventModel::mouseEventAtValidDatasetPosition(QMouseEvent *event, int VPfound) {
    if(VPfound == -1 || state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON ||
       event->x() < 0 || event->x() > (int)state->viewerState->vpConfigs[VPfound].edgeLength ||
       event->y() < 0 || event->y() > (int)state->viewerState->vpConfigs[VPfound].edgeLength) {
            return false;
    }

    Coordinate pos = getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound);
    const auto min = Session::singleton().movementAreaMin;
    const auto max = Session::singleton().movementAreaMax;
    //check if coordinates are in range
    if((pos.x < min.x) || (pos.x > max.x)
        ||(pos.y < min.y) || (pos.y > max.y)
        ||(pos.z < min.z) || (pos.z > max.z)) {
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
    const auto width = std::abs(maxX - minX);
    const auto height = std::abs(maxY - minY);
    const auto centerX = minX + width / 2;
    const auto centerY = minY + height / 2;
    auto nodes = state->viewer->renderer->retrieveAllObjectsBeneathSquare(vpId, centerX, centerY, width, height);
    Skeletonizer::singleton().selectNodes(nodes);
    state->viewerState->nodeSelectSquareVpId = -1;
}

int EventModel::xrel(int x) {
    return (x - this->mouseX);
}

int EventModel::yrel(int y) {
    return (y - this->mouseY);
}

Coordinate EventModel::getMouseCoordinate(int VPfound) {
    return getCoordinateFromOrthogonalClick(mousePosX, mousePosY, VPfound);
}
