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

#include "annotation/annotation.h"
#include "dataset.h"
#include "widgets/viewports/viewportarb.h"
#include "widgets/viewports/viewportbase.h"
#include "widgets/viewports/viewportortho.h"
#include "widgets/viewports/viewport3d.h"

#include "functions.h"
#include "gui_wrapper.h"
#include "scriptengine/scripting.h"
#include "segmentation/cubeloader.h"
#include "segmentation/segmentation.h"
#include "segmentation/segmentationsplit.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/preferences/navigationtab.h"
#include "widgets/mainwindow.h"

#include <QApplication>
#include <QMessageBox>

#include <boost/optional.hpp>

#include <cstdlib>
#include <unordered_set>

void merging(const QMouseEvent *event, ViewportOrtho & vp) {
    auto & seg = Segmentation::singleton();
    const auto brushCenter = getCoordinateFromOrthogonalClick(event->pos(), vp);
    const auto subobjectIds = readVoxels(brushCenter, seg.brush.value());
    for (const auto & subobjectPair : subobjectIds) {
        if (seg.selectedObjectsCount() == 1) {
            const auto soid = subobjectPair.first;
            const auto pos = subobjectPair.second;
            auto & subobject = seg.subobjectFromId(soid, pos);
            const auto objectToMergeIdx = seg.smallestImmutableObjectContainingSubobject(subobject);
            // if clicked object is currently selected, an unmerge is requested
            if (seg.isSelected(subobject)) {
                if (event->modifiers().testFlag(Qt::ShiftModifier)) {
                    if (event->modifiers().testFlag(Qt::ControlModifier)) {
                        seg.selectObjectFromSubObject(subobject, pos);
                        seg.unmergeSelectedObjects(pos);
                    } else {
                        if(seg.isSelected(objectToMergeIdx)) { // if no other object to unmerge, just unmerge subobject
                            seg.selectObjectFromSubObject(subobject, pos);
                        }
                        else {
                            seg.selectObject(objectToMergeIdx);
                        }
                        seg.unmergeSelectedObjects(pos);
                    }
                }
            } else { // object is not selected, so user wants to merge
                if (!event->modifiers().testFlag(Qt::ShiftModifier)) {
                    if (event->modifiers().testFlag(Qt::ControlModifier)) {
                        seg.selectObjectFromSubObject(subobject, pos);
                    } else {
                        seg.selectObject(objectToMergeIdx);//select largest object
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

void segmentation_brush_work(const QMouseEvent *event, ViewportOrtho & vp) {
    const Coordinate coord = getCoordinateFromOrthogonalClick(event->pos(), vp);
    auto & seg = Segmentation::singleton();

    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::ObjectMerge)) {
        merging(event, vp);
    } else if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Mode_Paint)) {//paint and erase
        if (!seg.brush.isInverse() && seg.selectedObjectsCount() == 0) {
            seg.createAndSelectObject(coord);
        }
        if (seg.selectedObjectsCount() > 0) {
            uint64_t soid = seg.subobjectIdOfFirstSelectedObject(coord);
            writeVoxels(coord, soid, seg.brush.value());
        }
    }
}


void ViewportOrtho::handleMouseHover(const QMouseEvent *event) {
    auto coord = getCoordinateFromOrthogonalClick(event->pos(), *this);
    emit cursorPositionChanged(coord, viewportType);
    auto subObjectId = readVoxel(coord);
    EmitOnCtorDtor eocd(&SignalRelay::Signal_EventModel_handleMouseHover, state->signalRelay, coord, subObjectId, viewportType, event);
    ViewportBase::handleMouseHover(event);
}

void startNodeSelection(const int x, const int y, const ViewportType vpType, const Qt::KeyboardModifiers modifiers) {
    state->viewerState->nodeSelectionSquare.first.x = x;
    state->viewerState->nodeSelectionSquare.first.y = y;

    // reset second point from a possible previous selection square.
    state->viewerState->nodeSelectionSquare.second = state->viewerState->nodeSelectionSquare.first;
    state->viewerState->nodeSelectSquareData = std::make_pair(vpType, modifiers);
}

void ViewportBase::handleLinkToggle(const QMouseEvent & event) {
    auto * activeNode = state->skeletonState->activeNode;
    auto clickedNode = pickNode(event.x(), event.y(), 10);
    if (clickedNode && activeNode != nullptr) {
        checkedToggleNodeLink(*activeNode, clickedNode.get());
    }
}

void ViewportBase::handleMouseButtonLeft(const QMouseEvent *event) {
    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::NodeSelection)) {
        const bool selection = event->modifiers().testFlag(Qt::ShiftModifier) || event->modifiers().testFlag(Qt::ControlModifier);
        if (selection) {
            startNodeSelection(event->pos().x(), event->pos().y(), viewportType, event->modifiers());
            return;
        }
        //Set Connection between Active Node and Clicked Node
        else if (QApplication::keyboardModifiers() == Qt::ALT) {
            handleLinkToggle(*event);
        }
    }
}

void ViewportBase::handleMouseButtonMiddle(const QMouseEvent *event) {
    if (event->modifiers().testFlag(Qt::ShiftModifier) && Annotation::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing)) {
        handleLinkToggle(*event);
    }
}

void ViewportOrtho::handleMouseButtonMiddle(const QMouseEvent *event) {
    if (event->modifiers().testFlag(Qt::NoModifier) && Annotation::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing)) {
        if (auto clickedNode = pickNode(event->x(), event->y(), 10)) {
            draggedNode = &clickedNode.get();
        }
    }
    ViewportBase::handleMouseButtonMiddle(event);
}

void ViewportOrtho::handleMouseButtonRight(const QMouseEvent *event) {
    const auto & annotationMode = Annotation::singleton().annotationMode;
    if (annotationMode.testFlag(AnnotationMode::Brush)) {
        Segmentation::singleton().brush.setInverse(event->modifiers().testFlag(Qt::ShiftModifier));
        segmentation_brush_work(event, *this);
        return;
    }
    if (!annotationMode.testFlag(AnnotationMode::NodeEditing)) {
        return;
    }
    Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event->pos(), *this);
    if (Annotation::singleton().outsideMovementArea(clickedCoordinate)) {
        return;
    }
    const quint64 subobjectId = readVoxel(clickedCoordinate);
    const bool background = subobjectId == Segmentation::singleton().getBackgroundId();
    if (annotationMode.testFlag(AnnotationMode::Mode_MergeTracing) && background && !event->modifiers().testFlag(Qt::ControlModifier)) {
        return;
    }

    nodeListElement * oldNode = state->skeletonState->activeNode;
    boost::optional<nodeListElement &> newNode;

    if (annotationMode.testFlag(AnnotationMode::LinkedNodes)) {
        if (oldNode == nullptr || state->skeletonState->activeTree->nodes.empty()) {
            //no node to link with or empty tree
            newNode = Skeletonizer::singleton().UI_addSkeletonNode(clickedCoordinate, viewportType);
        } else if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeTracing)) {
                const auto splitNode = Skeletonizer::singleton().UI_addSkeletonNode(clickedCoordinate, viewportType);
                if (splitNode) {
                    const auto comment = background ? "ecs" : "split";
                    Skeletonizer::singleton().setSubobject(splitNode.get(), subobjectId);
                    Skeletonizer::singleton().setComment(splitNode.get(), comment);
                    Skeletonizer::singleton().setActiveNode(oldNode);
                }
            } else if (auto stumpNode = Skeletonizer::singleton().addSkeletonNodeAndLinkWithActive(clickedCoordinate, viewportType, false)) {
                //Add a "stump", a branch node to which we don't automatically move.
                Skeletonizer::singleton().pushBranchNode(stumpNode.get());
                Skeletonizer::singleton().setActiveNode(oldNode);
            }
        } else {
            const auto lastPos = state->skeletonState->activeNode->position;
            newNode = Skeletonizer::singleton().addSkeletonNodeAndLinkWithActive(clickedCoordinate, viewportType, true);
            if (!newNode) {
                return;
            }
            const auto movement = clickedCoordinate - lastPos;
            //Highlight the viewport with the biggest movement component
            if ((std::abs(movement.x) >= std::abs(movement.y)) && (std::abs(movement.x) >= std::abs(movement.z))) {
                state->viewerState->highlightVp = VIEWPORT_ZY;
            } else if ((std::abs(movement.y) >= std::abs(movement.x)) && (std::abs(movement.y) >= std::abs(movement.z))) {
                state->viewerState->highlightVp = VIEWPORT_XZ;
            } else {
                state->viewerState->highlightVp = VIEWPORT_XY;
            }
            //Determine the directions for the f and d keys based on the signs of the movement components along the three dimensions
            state->viewerState->tracingDirection = movement;

            //Additional move of specified steps along tracing direction
            if (state->viewerState->autoTracingMode == Recentering::AheadOfNode) {
                floatCoordinate walking{movement};
                const auto factor = state->viewerState->autoTracingSteps / walking.length();
                clickedCoordinate += Coordinate(std::lround(movement.x * factor), std::lround(movement.y * factor), std::lround(movement.z * factor));
            }

            clickedCoordinate = clickedCoordinate.capped({0, 0, 0}, Dataset::current().boundary);// Do not allow clicks outside the dataset

            if(state->skeletonState->synapseState == Synapse::State::PostSynapse) {
                //The synapse workflow has been interrupted
                //Reset the synapse
                auto & tempSynapse = state->skeletonState->temporarySynapse;
                if (tempSynapse.getPreSynapse()) { tempSynapse.getPreSynapse()->isSynapticNode = false; }
                Skeletonizer::singleton().delTree(tempSynapse.getCleft()->treeID);
                tempSynapse = Synapse(); //reset temporary class
                state->skeletonState->synapseState = Synapse::State::PreSynapse;
                state->viewer->window->setSynapseState(SynapseState::Off); //reset statusbar entry
            }
        }
    } else { // unlinked
        newNode = Skeletonizer::singleton().UI_addSkeletonNode(clickedCoordinate,viewportType);
    }

    if (newNode) {
        if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeTracing)) {
            Skeletonizer::singleton().setSubobjectSelectAndMergeWithPrevious(newNode.get(), subobjectId, oldNode);
        }
        // Move to the new node position
        if (state->viewerState->autoTracingMode != Recentering::Off) {
            if (viewportType == VIEWPORT_ARBITRARY) {
                state->viewer->setPositionWithRecenteringAndRotation(clickedCoordinate);
            } else {
                state->viewer->setPositionWithRecentering(clickedCoordinate);
            }
        }
        auto & mainWin = *state->viewer->window;
        if (mainWin.segmentState == SegmentState::Off_Once) {
            mainWin.setSegmentState(SegmentState::On);
        }
        if(state->skeletonState->synapseState == Synapse::State::PostSynapse && state->skeletonState->activeTree->nodes.size() == 1) {
            auto & tempSynapse = state->skeletonState->temporarySynapse;
            tempSynapse.setPostSynapse(*state->skeletonState->activeNode);
            if (tempSynapse.getCleft() && tempSynapse.getPreSynapse() && tempSynapse.getPostSynapse()) {
                Skeletonizer::singleton().addFinishedSynapse(*tempSynapse.getCleft(), *tempSynapse.getPreSynapse(), *tempSynapse.getPostSynapse()); //move finished synapse to our synapse vector
            }
            state->skeletonState->synapseState = Synapse::State::PreSynapse;
            tempSynapse = Synapse(); //reset temporary class
            state->viewer->window->toggleSynapseState(); //update statusbar
        }
    }
    ViewportBase::handleMouseButtonRight(event);
}

floatCoordinate ViewportOrtho::handleMovement(const QPoint & pos) {
    const QPointF posDelta(xrel(pos.x()), yrel(pos.y()));
    const QPointF arbitraryMouseSlide = {-posDelta.x() / screenPxXPerMag1Px, -posDelta.y() / screenPxYPerMag1Px};
    const auto move = v1 * arbitraryMouseSlide.x() - v2 * arbitraryMouseSlide.y();
    return move;
}

void ViewportBase::handleMouseMotionLeftHold(const QMouseEvent *event) {
    // pull selection square
    if (state->viewerState->nodeSelectSquareData.first != -1) {
        state->viewerState->nodeSelectionSquare.second.x = event->pos().x();
        state->viewerState->nodeSelectionSquare.second.y = event->pos().y();
    }
}

void Viewport3D::handleMouseMotionLeftHold(const QMouseEvent *event) {
    if (event->modifiers() == Qt::NoModifier) {
        if (Segmentation::singleton().volume_render_toggle) {
            auto & seg = Segmentation::singleton();
            seg.volume_mouse_move_x -= xrel(event->x());
            seg.volume_mouse_move_y -= yrel(event->y());
        } else {
            translateX += -xrel(event->x()) / screenPxXPerDataPx;
            translateY += -yrel(event->y()) / screenPxXPerDataPx;
        }
    }
    ViewportBase::handleMouseMotionLeftHold(event);
}

void ViewportOrtho::handleMouseMotionLeftHold(const QMouseEvent *event) {
    if (event->modifiers() == Qt::NoModifier) {
        state->viewer->userMove(handleMovement(event->pos()), USERMOVE_HORIZONTAL, n);
    }
    ViewportBase::handleMouseMotionLeftHold(event);
}

void Viewport3D::handleMouseMotionRightHold(const QMouseEvent *event) {
    if (event->modifiers() == Qt::NoModifier && state->skeletonState->rotationcounter == 0) {
        state->skeletonState->rotdx += 90.0 * xrel(event->x()) / width();
        state->skeletonState->rotdy += 90.0 * yrel(event->y()) / height();
    }
    ViewportBase::handleMouseMotionRightHold(event);
}

void ViewportOrtho::handleMouseMotionRightHold(const QMouseEvent *event) {
    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Brush)) {
        const bool notOrigin = event->pos() != mouseDown;//don’t do redundant work
        if (notOrigin) {
            segmentation_brush_work(event, *this);
        }
    }
    ViewportBase::handleMouseMotionRightHold(event);
}

void ViewportOrtho::handleMouseMotionMiddleHold(const QMouseEvent *event) {
    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing) && draggedNode != nullptr) {
        const auto moveAccurate = handleMovement(event->pos());
        arbNodeDragCache += moveAccurate;//accumulate subpixel movements
        Coordinate moveTrunc = arbNodeDragCache;//truncate
        arbNodeDragCache -= moveTrunc;//only keep remaining fraction
        const auto newPos = draggedNode->position - moveTrunc;
        Skeletonizer::singleton().setPosition(*draggedNode, newPos);
    }
    ViewportBase::handleMouseMotionMiddleHold(event);
}

void ViewportBase::handleMouseReleaseLeft(const QMouseEvent *event) {
    auto & skeleton = *state->skeletonState;
    if (mouseDown == event->pos()) { // mouse click
        skeleton.meshLastClickInformation = pickMesh(event->pos());
        if (skeleton.meshLastClickInformation) {
            Skeletonizer::singleton().setActiveTreeByID(skeleton.meshLastClickInformation.get().treeId);
        }
        skeleton.jumpToSkeletonNext = !skeleton.meshLastClickInformation;
    }

    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::NodeSelection)) {
        QSet<nodeListElement*> selectedNodes;
        int diffX = std::abs(state->viewerState->nodeSelectionSquare.first.x - event->pos().x());
        int diffY = std::abs(state->viewerState->nodeSelectionSquare.first.y - event->pos().y());
        const auto boundedX = std::max(0, std::min(width(), event->pos().x()));
        const auto boundedY = std::max(0, std::min(height(), event->pos().y()));
        if ((diffX < 5 && diffY < 5) || (event->pos() - mouseDown).manhattanLength() < 5) { // interpreted as click instead of drag
            // mouse released on same spot on which it was pressed down: single node selection
            auto selectedNode = pickNode(boundedX, boundedY, 10);
            if (selectedNode) {
                selectedNodes = {&selectedNode.get()};
            }
        } else if (state->viewerState->nodeSelectSquareData.first != -1) {
            selectedNodes = nodeSelection(boundedX, boundedY);
        }
        if (state->viewerState->nodeSelectSquareData.first != -1 || !selectedNodes.empty()) {//only select no nodes if we drew a selection rectangle
            if (state->viewerState->nodeSelectSquareData.second == Qt::ControlModifier) {
                Skeletonizer::singleton().toggleNodeSelection(selectedNodes);
            } else {
                Skeletonizer::singleton().selectNodes(selectedNodes);
            }
        }
        state->viewerState->nodeSelectSquareData = std::make_pair(-1, Qt::NoModifier);//disable node selection square
    }
}

void ViewportOrtho::handleMouseReleaseLeft(const QMouseEvent *event) {
    auto & segmentation = Segmentation::singleton();
    const auto clickPos = getCoordinateFromOrthogonalClick(event->pos(), *this);
    if (!Annotation::singleton().outsideMovementArea(clickPos) && Annotation::singleton().annotationMode.testFlag(AnnotationMode::ObjectSelection)) { // in task mode the object should not be switched
        if (event->pos() == mouseDown) {// mouse click
            const auto subobjectId = readVoxel(clickPos);
            if (subobjectId != segmentation.getBackgroundId()) {// don’t select the unsegmented area as object
                auto & subobject = segmentation.subobjectFromId(subobjectId, clickPos);
                auto objIndex = segmentation.largestObjectContainingSubobject(subobject);
                Segmentation::singleton().setObjectLocation(objIndex, clickPos);
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
    }
    state->viewer->userMoveClear();//finish dataset drag

    ViewportBase::handleMouseReleaseLeft(event);
}

void ViewportOrtho::handleMouseReleaseRight(const QMouseEvent *event) {
    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Brush)) {
        if (event->pos() != mouseDown) {//merge took already place on mouse down
            segmentation_brush_work(event, *this);
        }
    }
    ViewportBase::handleMouseReleaseRight(event);
}

void ViewportOrtho::handleMouseReleaseMiddle(const QMouseEvent *event) {
    Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event->pos(), *this);
    if (!Annotation::singleton().outsideMovementArea(clickedCoordinate)) {
        EmitOnCtorDtor eocd(&SignalRelay::Signal_EventModel_handleMouseReleaseMiddle, state->signalRelay, clickedCoordinate, viewportType, event);
        auto & seg = Segmentation::singleton();
        if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Mode_Paint) && seg.selectedObjectsCount() == 1) {
            auto brush_copy = seg.brush.value();
            uint64_t soid = brush_copy.inverse ? seg.getBackgroundId() : seg.subobjectIdOfFirstSelectedObject(clickedCoordinate);
            brush_copy.shape = brush_t::shape_t::angular;
            brush_copy.radius = displayedIsoPx;//set brush to fill visible area

            const auto displayedMag1Px = displayedIsoPx / Dataset::current().scales[0].x;
            auto areaMin = state->viewerState->currentPosition - displayedMag1Px;
            auto areaMax = state->viewerState->currentPosition + displayedMag1Px;

            areaMin = areaMin.capped(Annotation::singleton().movementAreaMin, Annotation::singleton().movementAreaMax);
            areaMax = areaMax.capped(Annotation::singleton().movementAreaMin, Annotation::singleton().movementAreaMax);

            subobjectBucketFill(clickedCoordinate, soid, brush_copy, areaMin, areaMax);
        }
    }
    //finish node drag
    arbNodeDragCache = {};
    draggedNode = nullptr;

    ViewportBase::handleMouseReleaseMiddle(event);
}

void ViewportBase::handleWheelEvent(const QWheelEvent *event) {
    if (QApplication::activeWindow() != nullptr) {//only if active widget belongs to application
        activateWindow();//steal keyboard focus
    }
    setFocus();//get keyboard focus for this widget for viewport specific shortcuts

    const int directionSign = event->delta() > 0 ? -1 : 1;
    auto& seg = Segmentation::singleton();

    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing)
            && event->modifiers() == Qt::SHIFT
            && state->skeletonState->activeNode != nullptr)
    {//change node radius
        float radius = state->skeletonState->activeNode->radius + directionSign * 0.2 * state->skeletonState->activeNode->radius;
        Skeletonizer::singleton().setRadius(*state->skeletonState->activeNode, radius);
    } else if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Brush) && event->modifiers() == Qt::SHIFT) {
        auto curRadius = seg.brush.getRadius();
        // brush radius delta factor (float), as a function of current radius
        seg.brush.setRadius(curRadius + 0.1 * curRadius * (event->delta() / 120));
    }
}

void ViewportBase::applyZoom(const QWheelEvent *event, float direction) {
    QPoint scrollPixels = event->pixelDelta();
    QPoint scrollAngle = event->angleDelta() / 8;
    float scrollAmount = 0.0f;

    if (!scrollPixels.isNull()) {
        scrollAmount = scrollPixels.y() / 15.0f;
    } else if (!scrollAngle.isNull()) {
        scrollAmount = scrollAngle.y() / 15.0f;
    } else { // fallback legacy mode zoom
        if(event->phase() == Qt::NoScrollPhase) { // macOS fix
            if (event->delta() > 0) {
                zoomIn();
            } else {
                zoomOut();
            }
        }
    }

    if(scrollAmount != 0.0f) {
        zoom(std::pow(2, scrollAmount * zoomSpeed * direction));
        scrollAmount = 0.0f;
    }
}

void Viewport3D::handleWheelEvent(const QWheelEvent *event) {
    if (event->modifiers() == Qt::NoModifier) {
        if(Segmentation::singleton().volume_render_toggle) {
            auto& seg = Segmentation::singleton();
            seg.volume_mouse_zoom *= (event->delta() > 0) ? 1.1f : 0.9f;
        } else {
            const QPointF mouseRel{(event->x() - 0.5 * width()), (event->y() - 0.5 * height())};
            const auto oldZoom = zoomFactor;
            applyZoom(event);
            const auto oldFactor = state->skeletonState->volBoundary() / oldZoom;
            const auto newFactor = state->skeletonState->volBoundary() / zoomFactor;
            translateX += mouseRel.x() * (oldFactor - newFactor) / width();
            translateY += mouseRel.y() * (oldFactor - newFactor) / height();
        }
    }
    ViewportBase::handleWheelEvent(event);
}

void ViewportOrtho::handleWheelEvent(const QWheelEvent *event) {
    if (event->modifiers() == Qt::CTRL) { // Orthogonal VP or outside VP
        applyZoom(event, -1.0f);
    } else if (event->modifiers() == Qt::NoModifier) {
        const float directionSign = event->delta() > 0 ? -1 : 1;
        const auto multiplier = directionSign * state->viewerState->dropFrames;
        state->viewer->userMove(Dataset::current().scaleFactor.componentMul(n) * multiplier, USERMOVE_DRILL, n);
    }
    ViewportBase::handleWheelEvent(event);
}

void ViewportBase::handleKeyPress(const QKeyEvent *event) {
    const auto ctrl = event->modifiers().testFlag(Qt::ControlModifier);
    const auto alt = event->modifiers().testFlag(Qt::AltModifier);
    const auto shift = event->modifiers().testFlag(Qt::ShiftModifier);
    if (event->key() == Qt::Key_F11) {
        fullscreenAction.trigger();
    } else if (ctrl && shift && event->key() == Qt::Key_C) {
        if(state->skeletonState->activeNode && state->skeletonState->activeNode->isSynapticNode) {
            state->skeletonState->activeNode->correspondingSynapse->toggleDirection();
        }
    } else if (event->key() == Qt::Key_Shift) {
        if (!event->isAutoRepeat() && state->viewerState->keyRepeat) {// if ctrl was pressed initially don’t apply it again
            state->viewerState->repeatDirection *= 10;// increase movement speed
        }
        Segmentation::singleton().brush.setInverse(true);// enable erase mode on shift down
    } else if(event->key() == Qt::Key_I || event->key() == Qt::Key_O) {
        const float angle = ctrl ? -1: 1;
        switch(event->key()) {
        case Qt::Key_I:
            state->viewer->addRotation(QQuaternion::fromAxisAndAngle(state->viewer->viewportArb->n, angle));
            break;
        case Qt::Key_O:
            state->viewer->addRotation(QQuaternion::fromAxisAndAngle(state->viewer->viewportArb->v2, angle));
            break;
        }
    } else if(event->key() == Qt::Key_V) {
        if(ctrl) {
            emit pasteCoordinateSignal();
        }
    } else if(event->key() == Qt::Key_Space) {
        state->viewerState->showOnlyRawData = true;
        state->viewer->reslice_notify();
        state->viewer->mainWindow.forEachVPDo([] (ViewportBase & vp) {
            vp.showHideButtons(false);
        });
    } else if(event->key() == Qt::Key_Delete) {
        if (ctrl) {
            if (state->skeletonState->activeTree != nullptr) {
                Skeletonizer::singleton().delTree(state->skeletonState->activeTree->treeID);
            }
        } else if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing) && !state->skeletonState->selectedNodes.empty()) {
            bool deleteNodes = true;
            if (state->skeletonState->selectedNodes.size() > 1) {
                QMessageBox prompt{QApplication::activeWindow()};
                prompt.setIcon(QMessageBox::Question);
                prompt.setText(tr("Delete selected nodes?"));
                QPushButton *confirmButton = prompt.addButton(tr("Delete"), QMessageBox::AcceptRole);
                prompt.addButton(tr("Cancel"), QMessageBox::RejectRole);
                prompt.exec();
                deleteNodes = prompt.clickedButton() == confirmButton;
            }
            if (deleteNodes) {
                Skeletonizer::singleton().deleteSelectedNodes();
            }
        }
    } else if(event->key() == Qt::Key_Escape) {
        if (state->skeletonState->selectedNodes.size() > 1) {// active node is not allowed to be deselected
            QMessageBox prompt{QApplication::activeWindow()};
            prompt.setIcon(QMessageBox::Question);
            prompt.setText(tr("Clear current node selection?"));
            QPushButton *confirmButton = prompt.addButton(tr("Clear Selection"), QMessageBox::AcceptRole);
            prompt.addButton(tr("Cancel"), QMessageBox::RejectRole);
            prompt.exec();
            if (prompt.clickedButton() == confirmButton) {
                Skeletonizer::singleton().selectNodes({});
            }
        }
    } else if(event->key() == Qt::Key_F4) {
        if(alt) {
            QApplication::closeAllWindows();
        }
    }
}

void ViewportOrtho::handleKeyPress(const QKeyEvent *event) {
    //events
    //↓          #   #   #   #   #   #   #   # ↑  ↓          #  #  #…
    //^ os delay ^       ^---^ os key repeat

    //intended behavior:
    //↓          # # # # # # # # # # # # # # # ↑  ↓          # # # #…
    //^ os delay ^       ^-^ knossos specified interval

    //after a ›#‹ event state->viewerKeyRepeat instructs the viewer to check in each frame if a move should be performed
    const bool keyD = event->key() == Qt::Key_D;
    const bool keyF = event->key() == Qt::Key_F;
    const bool keyLeft = event->key() == Qt::Key_Left;
    const bool keyRight = event->key() == Qt::Key_Right;
    const bool keyUp = event->key() == Qt::Key_Up;
    const bool keyDown = event->key() == Qt::Key_Down;
    const auto singleVoxelKey = keyD || keyF || keyLeft || keyRight || keyUp || keyDown;
    const bool keyE = event->key() == Qt::Key_E;
    if (!event->isAutoRepeat()) {
        const int shiftMultiplier = event->modifiers().testFlag(Qt::ShiftModifier) ? 10 : 1;
        const auto direction = (n * -1).dot(state->viewerState->tracingDirection) >= 0 ? 1 : -1;// reverse n into the frame
        const float directionSign = (keyLeft || keyUp) ? -1 : (keyRight || keyDown) ? 1 : direction * (keyD || keyE ? -1 : 1);
        if (singleVoxelKey) {
            const auto vector = (keyLeft || keyRight) ? v1 : (keyUp || keyDown) ? (v2 * -1) : (n * -1); // transform v2 and n from 1. to 8. octant
            state->viewerState->repeatDirection = Dataset::current().scaleFactor.componentMul(vector) * directionSign * shiftMultiplier * state->viewerState->dropFrames;
            state->viewer->userMove(state->viewerState->repeatDirection, USERMOVE_HORIZONTAL, n);
        } else if(event->key() == Qt::Key_R || keyE) {
            state->viewer->setPositionWithRecentering(state->viewerState->currentPosition - Dataset::current().scaleFactor.componentMul(n) * directionSign * shiftMultiplier * state->viewerState->walkFrames);
        }
    } else if (singleVoxelKey) {
        state->viewerState->keyRepeat = true;
    }
    ViewportBase::handleKeyPress(event);
}

void ViewportBase::handleKeyRelease(const QKeyEvent *event) {
    if(event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        state->viewerState->showOnlyRawData = false;
        state->viewer->updateCurrentPosition();
        state->viewer->reslice_notify();
        state->viewer->mainWindow.forEachVPDo([] (ViewportBase & vp) {
            vp.showHideButtons(state->viewerState->showVpDecorations);
        });
    }
}

void Viewport3D::handleKeyPress(const QKeyEvent *event) {
    if (event->key() == Qt::Key_W && !event->isAutoRepeat()) {// real key press
        wiggletimer.start();
        wiggleButton.setChecked(true);
    }
    ViewportBase::handleKeyPress(event);
}

void Viewport3D::resetWiggle() {
    wiggletimer.stop();
    state->skeletonState->rotdx -= wiggle;
    state->skeletonState->rotdy -= wiggle;
    wiggleDirection = true;
    wiggle = 0;
    wiggleButton.setChecked(false);
}

void Viewport3D::handleKeyRelease(const QKeyEvent *event) {
    if (event->key() == Qt::Key_W && !event->isAutoRepeat()) {// real key release
        resetWiggle();
    }
    ViewportBase::handleKeyRelease(event);
}

void Viewport3D::focusOutEvent(QFocusEvent * event) {
    resetWiggle();
    ViewportBase::focusOutEvent(event);
}

Coordinate getCoordinateFromOrthogonalClick(const QPointF pos, ViewportOrtho & vp) {
    const auto leftUpper = floatCoordinate{state->viewerState->currentPosition} - (vp.v1 * vp.edgeLength / vp.screenPxXPerMag1Px - vp.v2 * vp.edgeLength / vp.screenPxYPerMag1Px) * 0.5;
    return leftUpper + vp.v1 * (pos.x() / vp.screenPxXPerMag1Px - 0.5) - vp.v2 * (pos.y() / vp.screenPxYPerMag1Px - 0.5);
}

QSet<nodeListElement*> ViewportBase::nodeSelection(int x, int y) {
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
    const auto selectedNodes = pickNodes(centerX, centerY, width, height);
    QSet<nodeListElement*> selectedSet;
    for (const auto & elem : selectedNodes) {
        selectedSet.insert(elem);
    }
    return selectedSet;
}

Coordinate ViewportOrtho::getMouseCoordinate() {
    return getCoordinateFromOrthogonalClick(prevMouseMove, *this);
}
