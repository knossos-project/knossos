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
#include "widgets/mainwindow.h"
#include "widgets/navigationwidget.h"
#include "widgets/viewport.h"
#include "widgets/viewportsettings/vpgeneraltabwidget.h"
#include "widgets/viewportsettings/vpsliceplaneviewportwidget.h"

#include <boost/math/constants/constants.hpp>
#include <boost/optional.hpp>

#include <cstdlib>
#include <unordered_set>

EventModel::EventModel(QObject *parent) : QObject(parent) {}

uint64_t segmentationColorPickingPoint(const int x, const int y, const int viewportId) {
    return readVoxel(getCoordinateFromOrthogonalClick(x, y, viewportId));
}

void segmentation_work(QMouseEvent *event, const int vp) {
    const Coordinate coord = getCoordinateFromOrthogonalClick(event->x(), event->y(), vp);
    auto& seg = Segmentation::singleton();
    state->skeletonState->branchpointUnresolved = false;

    if (seg.brush.getTool() == brush_t::tool_t::merge) {
        merging(event, vp);
    } else if (seg.brush.getTool() == brush_t::tool_t::add) {//paint and erase
        if (!seg.brush.isInverse() && seg.selectedObjectsCount() == 0) {
            seg.createAndSelectObject(coord);
        }
        uint64_t soid = 0;
        if (seg.selectedObjectsCount() != 0) {
            soid = seg.subobjectIdOfFirstSelectedObject();
            seg.updateLocationForFirstSelectedObject(coord);
        }
        writeVoxels(coord, soid, seg.brush);
    }
}

void merging(QMouseEvent *event, const int vp) {
    auto & seg = Segmentation::singleton();
    const auto brushCenter = getCoordinateFromOrthogonalClick(event->x(), event->y(), vp);
    const auto subobjectIds = readVoxels(brushCenter, seg.brush);
    for(const auto subobjectPair : subobjectIds) {
        if (seg.selectedObjectsCount() == 1) {
            const auto soid = subobjectPair.first;
            const auto pos = subobjectPair.second;
            auto & subobject = seg.subobjectFromId(soid, pos);
            const auto objectToMergeId = seg.smallestImmutableObjectContainingSubobject(subobject);
            // if clicked object is currently selected, an unmerge is requested
            if (seg.isSelected(subobject)) {
                if (event->modifiers().testFlag(Qt::ShiftModifier)) {
                    if (event->modifiers().testFlag(Qt::ControlModifier)) {
                        seg.selectObjectFromSubObject(subobject, pos);
                        seg.unmergeSelectedObjects(pos);
                    } else {
                        if(seg.isSelected(objectToMergeId)) { // if no other object to unmerge, just unmerge subobject
                            seg.selectObjectFromSubObject(subobject, pos);
                        }
                        else {
                            seg.selectObject(objectToMergeId);
                        }
                        seg.unmergeSelectedObjects(pos);
                    }
                }
            } else { // object is not selected, so user wants to merge
                if (!event->modifiers().testFlag(Qt::ShiftModifier)) {
                    if (event->modifiers().testFlag(Qt::ControlModifier)) {
                        seg.selectObjectFromSubObject(subobject, pos);
                    } else {
                        seg.selectObject(objectToMergeId);//select largest object
                    }
                }
                if (seg.selectedObjectsCount() >= 2) {
                    seg.mergeSelectedObjects();
                }
            }
            seg.touchObjects(soid);
        }
    }
}

void EventModel::handleMouseHover(QMouseEvent *event, int VPfound) {
    auto coord = getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound);
    auto subObjectId = readVoxel(coord);
    Segmentation::singleton().hoverSubObject(subObjectId);
    EmitOnCtorDtor eocd(&SignalRelay::Signal_EventModel_handleMouseHover, state->signalRelay, coord, subObjectId, VPfound, event);
    if(Segmentation::singleton().hoverVersion && state->overlay) {
        Segmentation::singleton().mouseFocusedObjectId = Segmentation::singleton().tryLargestObjectContainingSubobject(subObjectId);
    }
}

void EventModel::handleMouseButtonLeft(QMouseEvent *event, int VPfound) {
    if (Session::singleton().annotationMode == SkeletonizationMode) {
        const bool selection = event->modifiers().testFlag(Qt::ShiftModifier) || event->modifiers().testFlag(Qt::ControlModifier);
        if (selection) {
            startNodeSelection(event->pos().x(), event->pos().y(), VPfound);
        } else if (state->viewerState->vpConfigs[VPfound].type != VIEWPORT_SKELETON) {
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
        if (QApplication::keyboardModifiers() == Qt::ALT) {
            auto clickedNodeId = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->x(), event->y(), 10) != 0;
            auto * activeNode = state->skeletonState->activeNode;
            if (clickedNodeId != 0 && activeNode != nullptr) {
                auto & skel = Skeletonizer::singleton();
                if (skel.findSegmentByNodeIDs(activeNode->nodeID, clickedNodeId)) {
                    emit delSegmentSignal(activeNode->nodeID, clickedNodeId, NULL);
                } else if (skel.findSegmentByNodeIDs(clickedNodeId, activeNode->nodeID)) {
                    emit delSegmentSignal(clickedNodeId, activeNode->nodeID, NULL);
                } else{
                    if(skel.simpleTracing && Skeletonizer::singleton().areConnected(*activeNode, *Skeletonizer::findNodeByNodeID(clickedNodeId))) {
                        QMessageBox::information(nullptr, "Cycle detected!",
                                                 "If you want to allow cycles, please deactivate Simple Tracing under 'Edit Skeleton'.");
                        return;
                    }
                    emit addSegmentSignal(activeNode->nodeID, clickedNodeId);
                }
            }
        }
    }
}

void EventModel::handleMouseButtonMiddle(QMouseEvent *event, int VPfound) {
    if (Session::singleton().annotationMode == SkeletonizationMode) {
        auto clickedNodeId = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->x(), event->y(), 10);

        if (clickedNodeId != 0) {
            auto & skel = Skeletonizer::singleton();
            auto activeNode = state->skeletonState->activeNode;
            Qt::KeyboardModifiers keyMod = event->modifiers();
            if (keyMod.testFlag(Qt::ShiftModifier) && activeNode != nullptr) {
                // Toggle segment between clicked and active node
                if (skel.findSegmentByNodeIDs(activeNode->nodeID, clickedNodeId)) {
                    emit delSegmentSignal(activeNode->nodeID, clickedNodeId, 0);
                } else if (skel.findSegmentByNodeIDs(clickedNodeId, activeNode->nodeID)) {
                    emit delSegmentSignal(clickedNodeId, activeNode->nodeID, 0);
                } else {
                    if(skel.simpleTracing && Skeletonizer::singleton().areConnected(*activeNode, *Skeletonizer::findNodeByNodeID(clickedNodeId))) {
                        QMessageBox::information(nullptr, "Cycle detected!",
                                                 "If you want to allow cycles, please deactivate Simple Tracing under 'Edit Skeleton'.");
                        return;
                    }
                    emit addSegmentSignal(activeNode->nodeID, clickedNodeId);
                }
            } else if (keyMod.testFlag(Qt::NoModifier)) {
                state->viewerState->vpConfigs[VPfound].draggedNode = Skeletonizer::findNodeByNodeID(clickedNodeId);
            }
        }
    }
}

void EventModel::handleMouseButtonRight(QMouseEvent *event, int VPfound) {
    if (!mouseEventAtValidDatasetPosition(event, VPfound)) {
        return;
    }
    Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound);
    if (Session::singleton().annotationMode == SegmentationMode && VPfound != VIEWPORT_SKELETON) {
        Segmentation::singleton().brush.setInverse(event->modifiers().testFlag(Qt::ShiftModifier));
        segmentation_work(event, VPfound);
        return;
    }
    Coordinate movement, lastPos;

    const quint64 subobjectId = readVoxel(clickedCoordinate);
    if (subobjectId == 0) {
        return;
    }

    uint64_t newNodeId = 0;
    switch (state->viewer->skeletonizer->getTracingMode()) {
    case Skeletonizer::TracingMode::unlinkedNodes:
        newNodeId = Skeletonizer::singleton().UI_addSkeletonNode(clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
        break;
    case Skeletonizer::TracingMode::skipNextLink:
        newNodeId = Skeletonizer::singleton().UI_addSkeletonNode(clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
        state->viewer->skeletonizer->setTracingMode(Skeletonizer::TracingMode::linkedNodes);//as we only wanted to skip one link
        break;
    case Skeletonizer::TracingMode::linkedNodes:
        if (state->skeletonState->activeNode == nullptr || state->skeletonState->activeTree->firstNode == nullptr) {
            //no node to link with or no empty tree
            newNodeId = Skeletonizer::singleton().UI_addSkeletonNode(clickedCoordinate, state->viewerState->vpConfigs[VPfound].type);
            break;
        } else if (event->modifiers().testFlag(Qt::ControlModifier)) {
            //Add a "stump", a branch node to which we don't automatically move.
            if((newNodeId = Skeletonizer::singleton().addSkeletonNodeAndLinkWithActive(clickedCoordinate,
                                                                state->viewerState->vpConfigs[VPfound].type,
                                                                false))) {
                Skeletonizer::singleton().pushBranchNode(true, true, NULL, newNodeId);
                Skeletonizer::singleton().setActiveNode(nullptr, newNodeId);
            }
            break;
        }
        //Add a node and apply tracing modes
        lastPos = state->skeletonState->activeNode->position; //remember last active for movement calculation
        newNodeId = Skeletonizer::singleton().addSkeletonNodeAndLinkWithActive(clickedCoordinate,
                                                         state->viewerState->vpConfigs[VPfound].type,
                                                         true);
        if(newNodeId == false) { //could not add node
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

    Skeletonizer::singleton().setSubobject(newNodeId, subobjectId);

    /* Move to the new node position */
    if (newNodeId) {
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

Coordinate deviationVP(const vpConfig & config, const QPoint deviation){
    switch (config.type) {
    case VIEWPORT_XY: return {deviation.x(), deviation.y(), 0};
    case VIEWPORT_XZ: return {deviation.x(), 0, deviation.y()};
    case VIEWPORT_YZ: return {0, deviation.y(), deviation.x()};
    }
}

boost::optional<Coordinate> handleMovement(const vpConfig & config, const QPointF & posDelta, QPointF & userMouseSlide) {
    userMouseSlide -= {posDelta.x() / config.screenPxXPerDataPx, posDelta.y() / config.screenPxYPerDataPx};

    const QPoint deviationTrunc(std::trunc(userMouseSlide.x()), std::trunc(userMouseSlide.y()));

    if (!deviationTrunc.isNull()) {
        userMouseSlide -= deviationTrunc;
        return deviationVP(config, deviationTrunc);
    }
    return boost::none;
}

void EventModel::handleMouseMotionLeftHold(QMouseEvent *event, int VPfound) {
    // pull selection square
    if (state->viewerState->nodeSelectSquareVpId != -1) {
        state->viewerState->nodeSelectionSquare.second.x = event->pos().x();
        state->viewerState->nodeSelectionSquare.second.y = event->pos().y();
        return;
    }

    if (state->viewerState->vpConfigs[VPfound].type == VIEWPORT_SKELETON) {
        if(Segmentation::singleton().volume_render_toggle) {
            auto & seg = Segmentation::singleton();
            seg.volume_mouse_move_x -= xrel(event->x());
            seg.volume_mouse_move_y -= yrel(event->y());
        } else {
            state->skeletonState->translateX += -xrel(event->x()) * 2.
                * ((float)state->skeletonState->volBoundary
                * (0.5 - state->skeletonState->zoomLevel))
                / ((float)state->viewerState->vpConfigs[VPfound].edgeLength);
            state->skeletonState->translateY += -yrel(event->y()) * 2.
                * ((float)state->skeletonState->volBoundary
                * (0.5 - state->skeletonState->zoomLevel))
                / ((float)state->viewerState->vpConfigs[VPfound].edgeLength);
        }
    } else if(state->viewerState->clickReaction == ON_CLICK_DRAG) {
        const auto & config = state->viewerState->vpConfigs[VPfound];
        const QPointF posDelta(xrel(event->x()), yrel(event->y()));
        if (config.type == VIEWPORT_ARBITRARY) {
            const QPointF arbitraryMouseSlide = {-posDelta.x() / config.screenPxXPerDataPx, -posDelta.y() / config.screenPxYPerDataPx};
            const auto v1 = config.v1 * arbitraryMouseSlide.x();
            const auto v2 = config.v2 * arbitraryMouseSlide.y();
            const auto move = v1 + v2;
            state->viewer->userMove_arb(move.x, move.y, move.z);//subpixel movements are accumulated within userMove_arb
        } else if (auto moveIt = handleMovement(config, posDelta, userMouseSlide)) {
            state->viewer->userMove(moveIt->x, moveIt->y, moveIt->z, USERMOVE_HORIZONTAL, config.type);
        }
    }
}

void EventModel::handleMouseMotionMiddleHold(QMouseEvent *event, int VPfound) {
    if (Session::singleton().annotationMode == SkeletonizationMode) {
        const auto & config = state->viewerState->vpConfigs[VPfound];
        if (config.draggedNode != nullptr) {
            const QPointF posDelta(xrel(event->x()), yrel(event->y()));
            boost::optional<Coordinate> moveIt;
            if (config.type == VIEWPORT_ARBITRARY) {
                const QPointF arbitraryMouseSlide = {-posDelta.x() / config.screenPxXPerDataPx, -posDelta.y() / config.screenPxYPerDataPx};
                const auto v1 = config.v1 * arbitraryMouseSlide.x();
                const auto v2 = config.v2 * arbitraryMouseSlide.y();
                arbNodeDragCache += v1 + v2;//accumulate subpixel movements
                moveIt = arbNodeDragCache;//truncate
                arbNodeDragCache -= *moveIt;//only keep remaining fraction
            } else {
                moveIt = handleMovement(config, posDelta, userMouseSlide);
            }
            if (moveIt) {
                const auto newPos = config.draggedNode->position - *moveIt;
                Skeletonizer::singleton().editNode(0, config.draggedNode, 0., newPos, state->magnification);
            }
        }
    }
}

void EventModel::handleMouseMotionRightHold(QMouseEvent *event, int VPfound) {
    if (Session::singleton().annotationMode == SegmentationMode && VPfound != VIEWPORT_SKELETON) {
        const bool notOrigin = event->pos() != mouseDown;//don’t do redundant work
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
        if (event->pos() == mouseDown) {
            const auto clickPos = getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound);
            const auto subobjectId = readVoxel(clickPos);
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

    std::vector<nodeListElement*> selectedNodes;
    int diffX = std::abs(state->viewerState->nodeSelectionSquare.first.x - event->pos().x());
    int diffY = std::abs(state->viewerState->nodeSelectionSquare.first.y - event->pos().y());
    if ((diffX < 5 && diffY < 5) || (event->pos() - mouseDown).manhattanLength() < 5) { // interpreted as click instead of drag
        // mouse released on same spot on which it was pressed down: single node selection
        auto nodeID = state->viewer->renderer->retrieveVisibleObjectBeneathSquare(VPfound, event->pos().x(), event->pos().y(), 10);
        nodeListElement *selectedNode = Skeletonizer::findNodeByNodeID(nodeID);
        if (nodeID != 0 && selectedNode != nullptr) {
            selectedNodes = {selectedNode};
        }
    } else if (state->viewerState->nodeSelectSquareVpId != -1) {
        selectedNodes = nodeSelection(event->pos().x(), event->pos().y(), VPfound);
    }
    if (state->viewerState->nodeSelectSquareVpId != -1 || !selectedNodes.empty()) {//only select no nodes if we drew a selection rectangle
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            Skeletonizer::singleton().toggleNodeSelection(selectedNodes);
        } else {
            Skeletonizer::singleton().selectNodes(selectedNodes);
        }
    }
    state->viewerState->nodeSelectSquareVpId = -1;//disable node selection square
}

void EventModel::handleMouseReleaseRight(QMouseEvent *event, int VPfound) {
    if (Session::singleton().annotationMode == SegmentationMode && mouseEventAtValidDatasetPosition(event, VPfound)) {
        if (event->pos() != mouseDown) {//merge took already place on mouse down
            segmentation_work(event, VPfound);
        }
    }
}

void EventModel::handleMouseReleaseMiddle(QMouseEvent * event, int VPfound) {
    if (mouseEventAtValidDatasetPosition(event, VPfound)) {
        Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event->x(), event->y(), VPfound);
        EmitOnCtorDtor eocd(&SignalRelay::Signal_EventModel_handleMouseReleaseMiddle, state->signalRelay, clickedCoordinate, VPfound, event);
        if (Session::singleton().annotationMode == SegmentationMode && Segmentation::singleton().selectedObjectsCount() == 1) {
            connectedComponent(clickedCoordinate);
        }
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
        Skeletonizer::singleton().editNode(0, state->skeletonState->activeNode, radius, state->skeletonState->activeNode->position, state->magnification);
    } else if (Session::singleton().annotationMode == SegmentationMode && event->modifiers() == Qt::SHIFT) {
        seg.brush.setRadius(seg.brush.getRadius() + event->delta() / 120);
        if(seg.brush.getRadius() < 0) {
            seg.brush.setRadius(0);
        }
    } else if (VPfound == VIEWPORT_SKELETON) {
        if(Segmentation::singleton().volume_render_toggle) {
            auto& seg = Segmentation::singleton();
            seg.volume_mouse_zoom *= (directionSign == -1) ? 1.1f : 0.9f;
        } else {
            if (directionSign == -1) {
                emit zoomInSkeletonVPSignal();
            } else {
                emit zoomOutSkeletonVPSignal();
            }
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
        state->viewerState->showOverlay = false;
        state->viewer->oc_reslice_notify_visible();
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
        state->viewerState->showOverlay = true;
        state->viewer->oc_reslice_notify_visible();
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
    state->viewerState->nodeSelectionSquare.second = state->viewerState->nodeSelectionSquare.first;
    state->viewerState->nodeSelectSquareVpId = vpId;
}

std::vector<nodeListElement*> EventModel::nodeSelection(int x, int y, int vpId) {
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
    return state->viewer->renderer->retrieveAllObjectsBeneathSquare(vpId, centerX, centerY, width, height);
}

int EventModel::xrel(const int x) {
    return x - prevMouseMove.x();
}

int EventModel::yrel(const int y) {
    return y - prevMouseMove.y();
}

Coordinate EventModel::getMouseCoordinate(int VPfound) {
    return getCoordinateFromOrthogonalClick(prevMouseMove.x(), prevMouseMove.y(), VPfound);
}
