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

#include "widgets/viewport.h"

#include "functions.h"
#include "knossos.h"
#include "session.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "segmentation/cubeloader.h"
#include "segmentation/segmentation.h"
#include "segmentation/segmentationsplit.h"
#include "viewer.h"
#include "widgets/mainwindow.h"
#include "widgets/navigationwidget.h"

#include <boost/math/constants/constants.hpp>
#include <boost/optional.hpp>

#include <cstdlib>
#include <unordered_set>

void userMove(const floatCoordinate & step, const UserMoveType moveType, const floatCoordinate & vpNormal) {
    if(ViewportOrtho::arbitraryOrientation) {
        state->viewer->userMove_arb(step, moveType, vpNormal);
    }
    else {
        state->viewer->userMove({static_cast<int>(step.x), static_cast<int>(step.y), static_cast<int>(step.z)}, moveType, vpNormal);
    }
}

void merging(const QMouseEvent *event, ViewportOrtho & vp) {
    auto & seg = Segmentation::singleton();
    const auto brushCenter = getCoordinateFromOrthogonalClick(event->x(), event->y(), vp);
    const auto subobjectIds = readVoxels(brushCenter, seg.brush.value());
    for (const auto subobjectPair : subobjectIds) {
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

void segmentation_brush_work(const QMouseEvent *event, ViewportOrtho & vp) {
    const Coordinate coord = getCoordinateFromOrthogonalClick(event->x(), event->y(), vp);
    auto & seg = Segmentation::singleton();

    if (Session::singleton().annotationMode.testFlag(AnnotationMode::ObjectMerge)) {
        merging(event, vp);
    } else if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_Paint)) {//paint and erase
        if (!seg.brush.isInverse() && seg.selectedObjectsCount() == 0) {
            seg.createAndSelectObject(coord);
        }
        uint64_t soid = seg.subobjectIdOfFirstSelectedObject(coord);
        writeVoxels(coord, soid, seg.brush.value());
    }
}


void ViewportOrtho::handleMouseHover(const QMouseEvent *event) {
    auto coord = getCoordinateFromOrthogonalClick(event->x(), event->y(), *this);
    emit cursorPositionChanged(coord, id);
    auto subObjectId = readVoxel(coord);
    Segmentation::singleton().hoverSubObject(subObjectId);
    EmitOnCtorDtor eocd(&SignalRelay::Signal_EventModel_handleMouseHover, state->signalRelay, coord, subObjectId, id, event);
    if(Segmentation::singleton().hoverVersion && state->overlay) {
        Segmentation::singleton().mouseFocusedObjectId = Segmentation::singleton().tryLargestObjectContainingSubobject(subObjectId);
    }
    ViewportBase::handleMouseHover(event);
}

void ViewportBase::handleMouseButtonLeft(const QMouseEvent *event) {
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing)) {
        const bool selection = event->modifiers().testFlag(Qt::ShiftModifier) || event->modifiers().testFlag(Qt::ControlModifier);
        if (selection) {
            startNodeSelection(event->pos().x(), event->pos().y());
            return;
        }
        //Set Connection between Active Node and Clicked Node
        else if (QApplication::keyboardModifiers() == Qt::ALT) {
            auto clickedNode = retrieveVisibleObjectBeneathSquare(event->x(), event->y(), 10);
            auto * activeNode = state->skeletonState->activeNode;
            if (clickedNode && activeNode != nullptr) {
                auto & skel = Skeletonizer::singleton();
                auto * segment = skel.findSegmentBetween(*activeNode, clickedNode.get());
                if (segment) {
                    emit delSegmentSignal(segment);
                } else if ((segment = skel.findSegmentBetween(clickedNode.get(), *activeNode))) {
                    emit delSegmentSignal(segment);
                } else{
                    if(!Session::singleton().annotationMode.testFlag(AnnotationMode::SkeletonCycles) && Skeletonizer::singleton().areConnected(*activeNode, clickedNode.get())) {
                        QMessageBox::information(nullptr, "Cycle detected!", "If you want to allow cycles, please select 'Advanced Tracing' in the dropdown menu in the toolbar.");
                        return;
                    }
                    emit addSegmentSignal(*activeNode, clickedNode.get());
                }
            }
        }
    }
}

void ViewportBase::handleMouseButtonMiddle(const QMouseEvent *event) {
    if (event->modifiers().testFlag(Qt::ShiftModifier) && Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing)) {
        auto & skel = Skeletonizer::singleton();
        auto activeNode = state->skeletonState->activeNode;
        auto clickedNode = retrieveVisibleObjectBeneathSquare(event->x(), event->y(), 10);
        if (clickedNode && activeNode) {
            // Toggle segment between clicked and active node
            auto * segment = skel.findSegmentBetween(*activeNode, clickedNode.get());
            if (segment) {
                emit delSegmentSignal(segment);
            } else if ((segment = skel.findSegmentBetween(clickedNode.get(), *activeNode))) {
                emit delSegmentSignal(segment);
            } else {
                if(!Session::singleton().annotationMode.testFlag(AnnotationMode::SkeletonCycles) && Skeletonizer::singleton().areConnected(*activeNode, clickedNode.get())) {
                    QMessageBox::information(nullptr, "Cycle detected!", "If you want to allow cycles, please select 'Advanced Tracing' in the dropdown menu in the toolbar.");
                    return;
                }
                emit addSegmentSignal(*activeNode, clickedNode.get());
            }
        }
    }
}

void ViewportOrtho::handleMouseButtonMiddle(const QMouseEvent *event) {
    if (event->modifiers().testFlag(Qt::NoModifier) && Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing)) {
        if (auto clickedNode = retrieveVisibleObjectBeneathSquare(event->x(), event->y(), 10)) {
            draggedNode = &clickedNode.get();
        }
    }
    ViewportBase::handleMouseButtonMiddle(event);
}

void ViewportOrtho::handleMouseButtonRight(const QMouseEvent *event) {
    const auto & annotationMode = Session::singleton().annotationMode;
    if (annotationMode.testFlag(AnnotationMode::Brush)) {
        Segmentation::singleton().brush.setInverse(event->modifiers().testFlag(Qt::ShiftModifier));
        segmentation_brush_work(event, *this);
        return;
    }

    if (!mouseEventAtValidDatasetPosition(event)) { //don’t place nodes outside movement area
        return;
    }

    Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event->x(), event->y(), *this);
    const quint64 subobjectId = readVoxel(clickedCoordinate);
    const bool background = subobjectId == Segmentation::singleton().getBackgroundId();
    if (annotationMode.testFlag(AnnotationMode::Mode_MergeTracing) && background && !event->modifiers().testFlag(Qt::ControlModifier)) {
        return;
    }

    nodeListElement * oldNode = state->skeletonState->activeNode;
    boost::optional<nodeListElement &> newNode;

    if (annotationMode.testFlag(AnnotationMode::UnlinkedNodes)) {
        newNode = Skeletonizer::singleton().UI_addSkeletonNode(clickedCoordinate,viewportType);
    } else if (annotationMode.testFlag(AnnotationMode::LinkedNodes)) {
        if (oldNode == nullptr || state->skeletonState->activeTree->firstNode == nullptr) {
            //no node to link with or no empty tree
            newNode = Skeletonizer::singleton().UI_addSkeletonNode(clickedCoordinate, viewportType);
        } else if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeTracing)) {
                const auto splitNode = Skeletonizer::singleton().UI_addSkeletonNode(clickedCoordinate, viewportType);
                if (splitNode) {
                    const auto comment = background ? "ecs" : "split";
                    Skeletonizer::singleton().setSubobject(splitNode.get(), subobjectId);
                    Skeletonizer::singleton().addComment(comment, splitNode.get());
                    Skeletonizer::singleton().setActiveNode(oldNode);
                }
            } else if (auto stumpNode = Skeletonizer::singleton().addSkeletonNodeAndLinkWithActive(clickedCoordinate, viewportType, false)) {
                //Add a "stump", a branch node to which we don't automatically move.
                Skeletonizer::singleton().pushBranchNode(true, true, stumpNode.get());
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
                state->viewerState->highlightVp = VIEWPORT_YZ;
            } else if ((std::abs(movement.y) >= std::abs(movement.x)) && (std::abs(movement.y) >= std::abs(movement.z))) {
                state->viewerState->highlightVp = VIEWPORT_XZ;
            } else {
                state->viewerState->highlightVp = VIEWPORT_XY;
            }
            //Determine the directions for the f and d keys based on the signs of the movement components along the three dimensions
            state->viewerState->vpKeyDirection[VIEWPORT_XY] = (movement.z >= 0) ? 1 : -1;
            state->viewerState->vpKeyDirection[VIEWPORT_XZ] = (movement.y >= 0) ? 1 : -1;
            state->viewerState->vpKeyDirection[VIEWPORT_YZ] = (movement.x >= 0) ? 1 : -1;

            //Auto tracing adjustments – this is out of place here
            state->viewerState->autoTracingDelay = std::min(500, std::max(10, state->viewerState->autoTracingDelay));
            state->viewerState->autoTracingSteps = std::min(50, std::max(1, state->viewerState->autoTracingSteps));

            //Additional move of specified steps along clicked viewport
            if (state->viewerState->autoTracingMode == navigationMode::additionalVPMove) {
                const auto move = state->viewerState->vpKeyDirection[viewportType] * state->viewerState->autoTracingSteps;
                clickedCoordinate += n * move;
            }

            //Additional move of specified steps along tracing direction
            if (state->viewerState->autoTracingMode == navigationMode::additionalTracingDirectionMove) {
                floatCoordinate walking{movement};
                const auto factor = state->viewerState->autoTracingSteps / euclidicNorm(walking);
                clickedCoordinate += {roundFloat(movement.x * factor), roundFloat(movement.y * factor), roundFloat(movement.z * factor)};
            }
            //Additional move of steps equal to distance between last and new node along tracing direction.
            if (state->viewerState->autoTracingMode == navigationMode::additionalMirroredMove) {
                clickedCoordinate += movement;
            }
            clickedCoordinate.capped(0, state->boundary);// Do not allow clicks outside the dataset
        }
    }

    if (newNode) {
        if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeTracing)) {
            Skeletonizer::singleton().setSubobjectSelectAndMergeWithPrevious(newNode.get(), subobjectId, oldNode);
        }
        // Move to the new node position
        if (viewportType == VIEWPORT_ARBITRARY) {
            emit setRecenteringPositionWithRotationSignal(clickedCoordinate, id);
        } else {
            emit setRecenteringPositionSignal(clickedCoordinate);
        }
    }
    if (state->viewerState->autoTracingMode != navigationMode::noRecentering) {
        Knossos::sendRemoteSignal();
    }
    ViewportBase::handleMouseButtonRight(event);
}

boost::optional<Coordinate> handleMovement(const ViewportBase & vp, const QPointF & posDelta, QPointF & userMouseSlide) {
    userMouseSlide -= {posDelta.x() / vp.screenPxXPerDataPx, posDelta.y() / vp.screenPxYPerDataPx};

    const QPoint deviationTrunc(std::trunc(userMouseSlide.x()), std::trunc(userMouseSlide.y()));

    if (!deviationTrunc.isNull()) {
        userMouseSlide -= deviationTrunc;
        floatCoordinate deviation = vp.v1 * deviationTrunc.x() + vp.v2 * deviationTrunc.y();
        return Coordinate(deviation.x, deviation.y, deviation.z);
    }
    return boost::none;
}

void ViewportBase::handleMouseMotionLeftHold(const QMouseEvent *event) {
    // pull selection square
    if (state->viewerState->nodeSelectSquareVpId != -1) {
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
            state->skeletonState->translateX += -xrel(event->x()) * 2.
                * ((float)state->skeletonState->volBoundary
                * (0.5 - state->skeletonState->zoomLevel))
                / ((float)edgeLength);
            state->skeletonState->translateY += -yrel(event->y()) * 2.
                * ((float)state->skeletonState->volBoundary
                * (0.5 - state->skeletonState->zoomLevel))
                / ((float)edgeLength);
        }
    }
    ViewportBase::handleMouseMotionLeftHold(event);
}

void ViewportOrtho::handleMouseMotionLeftHold(const QMouseEvent *event) {
    if (event->modifiers() == Qt::NoModifier && state->viewerState->clickReaction == ON_CLICK_DRAG) {
        const QPointF posDelta(xrel(event->x()), yrel(event->y()));
        if (viewportType == VIEWPORT_ARBITRARY) {
            const QPointF arbitraryMouseSlide = {-posDelta.x() / screenPxXPerDataPx, -posDelta.y() / screenPxYPerDataPx};
            const auto move = v1 * arbitraryMouseSlide.x() + v2 * arbitraryMouseSlide.y();
            state->viewer->userMove_arb(move, USERMOVE_HORIZONTAL, n);
        } else if (auto moveIt = handleMovement(*this, posDelta, userMouseSlide)) {
            state->viewer->userMove(*moveIt, USERMOVE_HORIZONTAL, n);
        }
    }
    ViewportBase::handleMouseMotionLeftHold(event);
}

void Viewport3D::handleMouseMotionRightHold(const QMouseEvent *event) {
    if (event->modifiers() == Qt::NoModifier && state->skeletonState->rotationcounter == 0) {
        state->skeletonState->rotdx += xrel(event->x());
        state->skeletonState->rotdy += yrel(event->y());
    }
    ViewportBase::handleMouseMotionRightHold(event);
}


void ViewportOrtho::handleMouseMotionRightHold(const QMouseEvent *event) {
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::Brush)) {
        const bool notOrigin = event->pos() != mouseDown;//don’t do redundant work
        if (notOrigin) {
            segmentation_brush_work(event, *this);
        }
    }
    ViewportBase::handleMouseMotionRightHold(event);
}

void ViewportOrtho::handleMouseMotionMiddleHold(const QMouseEvent *event) {
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing) && draggedNode != nullptr) {
        const QPointF posDelta(xrel(event->x()), yrel(event->y()));
        boost::optional<Coordinate> moveIt;
        if (viewportType == VIEWPORT_ARBITRARY) {
            const QPointF arbitraryMouseSlide = {-posDelta.x() / screenPxXPerDataPx, -posDelta.y() / screenPxYPerDataPx};
            arbNodeDragCache += v1 * arbitraryMouseSlide.x() + v2 * arbitraryMouseSlide.y();//accumulate subpixel movements
            moveIt = arbNodeDragCache;//truncate
            arbNodeDragCache -= *moveIt;//only keep remaining fraction
        } else {
            moveIt = handleMovement(*this, posDelta, userMouseSlide);
        }
        if (moveIt) {
            const auto newPos = draggedNode->position - *moveIt;
            Skeletonizer::singleton().editNode(0, draggedNode, 0., newPos, state->magnification);
        }
    }
    ViewportBase::handleMouseMotionMiddleHold(event);
}

void ViewportBase::handleMouseReleaseLeft(const QMouseEvent *event) {
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing)) {
        QSet<nodeListElement*> selectedNodes;
        int diffX = std::abs(state->viewerState->nodeSelectionSquare.first.x - event->pos().x());
        int diffY = std::abs(state->viewerState->nodeSelectionSquare.first.y - event->pos().y());
        if ((diffX < 5 && diffY < 5) || (event->pos() - mouseDown).manhattanLength() < 5) { // interpreted as click instead of drag
            // mouse released on same spot on which it was pressed down: single node selection
            auto selectedNode = retrieveVisibleObjectBeneathSquare(event->pos().x(), event->pos().y(), 10);
            if (selectedNode) {
                selectedNodes = {&selectedNode.get()};
            }
        } else if (state->viewerState->nodeSelectSquareVpId != -1) {
            selectedNodes = nodeSelection(event->pos().x(), event->pos().y());
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
}

void ViewportOrtho::handleMouseReleaseLeft(const QMouseEvent *event) {
    auto & segmentation = Segmentation::singleton();
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::ObjectSelection) && mouseEventAtValidDatasetPosition(event)) { // in task mode the object should not be switched
        if (event->pos() == mouseDown) {
            const auto clickPos = getCoordinateFromOrthogonalClick(event->x(), event->y(), *this);
            const auto subobjectId = readVoxel(clickPos);
            if (subobjectId != segmentation.getBackgroundId()) {// don’t select the unsegmented area as object
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
    }
    ViewportBase::handleMouseReleaseLeft(event);
}

void ViewportOrtho::handleMouseReleaseRight(const QMouseEvent *event) {
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::Brush)) {
        if (event->pos() != mouseDown) {//merge took already place on mouse down
            segmentation_brush_work(event, *this);
        }
    }
    ViewportBase::handleMouseReleaseRight(event);
}

void ViewportOrtho::handleMouseReleaseMiddle(const QMouseEvent *event) {
    if (mouseEventAtValidDatasetPosition(event)) {
        Coordinate clickedCoordinate = getCoordinateFromOrthogonalClick(event->x(), event->y(), *this);
        EmitOnCtorDtor eocd(&SignalRelay::Signal_EventModel_handleMouseReleaseMiddle, state->signalRelay, clickedCoordinate, id, event);
        auto & seg = Segmentation::singleton();
        if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_Paint) && seg.selectedObjectsCount() == 1) {
            auto brush_copy = seg.brush.value();
            uint64_t soid = brush_copy.inverse ? seg.getBackgroundId() : seg.subobjectIdOfFirstSelectedObject(clickedCoordinate);
            brush_copy.shape = brush_t::shape_t::angular;
            brush_copy.radius = displayedlengthInNmX / 2;//set brush to fill visible area
            subobjectBucketFill(clickedCoordinate, state->viewerState->currentPosition, soid, brush_copy);
        }
    }
    ViewportBase::handleMouseReleaseMiddle(event);
}

void ViewportBase::handleWheelEvent(const QWheelEvent *event) {
    const int directionSign = event->delta() > 0 ? -1 : 1;
    auto& seg = Segmentation::singleton();

    if (Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing)
            && event->modifiers() == Qt::SHIFT
            && state->skeletonState->activeNode != nullptr)
    {//change node radius
        float radius = state->skeletonState->activeNode->radius + directionSign * 0.2 * state->skeletonState->activeNode->radius;
        Skeletonizer::singleton().editNode(0, state->skeletonState->activeNode, radius, state->skeletonState->activeNode->position, state->magnification);;
    } else if (Session::singleton().annotationMode.testFlag(AnnotationMode::Brush) && event->modifiers() == Qt::SHIFT) {
        seg.brush.setRadius(seg.brush.getRadius() + event->delta() / 120);
        if(seg.brush.getRadius() < 0) {
            seg.brush.setRadius(0);
        };
    }
}

void Viewport3D::handleWheelEvent(const QWheelEvent *event) {
    if (event->modifiers() == Qt::NoModifier) {
        const int directionSign = event->delta() > 0 ? -1 : 1;
        if(Segmentation::singleton().volume_render_toggle) {
            auto& seg = Segmentation::singleton();
            seg.volume_mouse_zoom *= (directionSign == -1) ? 1.1f : 0.9f;
        } else {
            if (directionSign == -1) {
                zoomIn();
            } else {
                zoomOut();
            }
        }
    }
    ViewportBase::handleWheelEvent(event);
}

void ViewportOrtho::handleWheelEvent(const QWheelEvent *event) {
    const int directionSign = event->delta() > 0 ? -1 : 1;
    if (event->modifiers() == Qt::CTRL) { // Orthogonal VP or outside VP
        zoom(directionSign * zoomStep());
    } else if (event->modifiers() == Qt::NoModifier) {
        const auto multiplier = directionSign * (int)state->viewerState->dropFrames * state->magnification;
        userMove(n * multiplier, USERMOVE_DRILL, n);
    }
    ViewportBase::handleWheelEvent(event);
}

void ViewportBase::keyPressEvent(QKeyEvent *event) {
    Qt::KeyboardModifiers modifiers = event->modifiers();
    const auto shift = modifiers.testFlag(Qt::ShiftModifier);
    const auto ctrl = modifiers.testFlag(Qt::ControlModifier);
    const auto alt = modifiers.testFlag(Qt::AltModifier);

    if (ctrl && alt) {
        setCursor(Qt::OpenHandCursor);
    } else if (ctrl) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }

    //events
    //↓          #   #   #   #   #   #   #   # ↑  ↓          #  #  #…
    //^ os delay ^       ^---^ os key repeat

    //intended behavior:
    //↓          # # # # # # # # # # # # # # # ↑  ↓          # # # #…
    //^ os delay ^       ^-^ knossos specified interval

    //after a ›#‹ event state->viewerKeyRepeat instructs the viewer to check in each frame if a move should be performed

    //›#‹ events are marked isAutoRepeat correctly on Windows
    //on Mac and Linux only when you move the cursor out of the window (https://bugreports.qt-project.org/browse/QTBUG-21500)
    //to emulate this the time to the previous time the same event occured is measured
    //drawbacks of this emulation are:
    //- you can accidently enable autorepeat – skipping the os delay – although you just pressed 2 times very quickly (falling into the timer threshold)
    //- autorepeat is not activated until the 3rd press event, not the 2nd, because you need a base event for the timer

    if (event->key() == Qt::Key_D || event->key() == Qt::Key_F) {
        state->viewerKeyRepeat = event->isAutoRepeat();
    }
    if (event->key() == Qt::Key_H) {
        if (isDocked) {
            hide();
        }
        else {
            floatParent->hide();
        }
        state->viewerState->defaultVPSizeAndPos = false;
    }
    if (event->key() == Qt::Key_U) {
        if (isDocked) {
            // Currently docked and normal
            // Undock and go fullscreen from docked
            setDock(false);
            floatParent->setWindowState(Qt::WindowFullScreen);
            isFullOrigDocked = true;
        }
        else {
            // Currently undocked
            if (floatParent->isFullScreen()) {
                // Currently fullscreen
                // Go normal and back to original docking state
                floatParent->setWindowState(Qt::WindowNoState);
                if (isFullOrigDocked) {
                    setDock(isFullOrigDocked);
                }
            }
            else {
                // Currently not fullscreen
                // Go fullscreen from undocked
                floatParent->setWindowState(Qt::WindowFullScreen);
                isFullOrigDocked = false;
            }
        }
    }
    if (!event->isAutoRepeat()) {
        //autorepeat emulation for systems where isAutoRepeat() does not work as expected
        //seperate timer for each key, but only one across all vps
        if (event->key() == Qt::Key_D) {
            state->viewerKeyRepeat = timeDBase.restart() < 150;
        } else if (event->key() == Qt::Key_F) {
            state->viewerKeyRepeat = timeFBase.restart() < 150;
        }
    }

    if(event->key() == Qt::Key_Left) {
        if(shift) {
            userMove(v1 * (-10) * state->magnification, USERMOVE_HORIZONTAL, n);
        } else {
            userMove(v1 * -static_cast<int>(state->viewerState->dropFrames) * state->magnification, USERMOVE_HORIZONTAL, n);
        }
    } else if(event->key() == Qt::Key_Right) {
        if(shift) {
            userMove(v1 * 10 * state->magnification, USERMOVE_HORIZONTAL, n);
        } else {
            userMove(v1 * static_cast<int>(state->viewerState->dropFrames) * state->magnification, USERMOVE_HORIZONTAL, n);
        }
    } else if(event->key() == Qt::Key_Down) {
        if(shift) {
            userMove(v2 * (-10) * state->magnification, USERMOVE_HORIZONTAL, n);
        } else {
            userMove(v2 * -static_cast<int>(state->viewerState->dropFrames) * state->magnification, USERMOVE_HORIZONTAL, n);
        }
    } else if(event->key() == Qt::Key_Up) {
        if(shift) {
            userMove(v2 * 10 * state->magnification, USERMOVE_HORIZONTAL, n);
        } else {
            userMove(v2 * static_cast<int>(state->viewerState->dropFrames) * state->magnification, USERMOVE_HORIZONTAL, n);
        }
    } else if(event->key() == Qt::Key_R) {
        state->viewerState->walkOrth = true;
        emit setRecenteringPositionSignal(state->viewerState->currentPosition + n * state->viewerState->walkFrames * state->magnification);
        Knossos::sendRemoteSignal();
    } else if(event->key() == Qt::Key_E) {
        state->viewerState->walkOrth = true;
        emit setRecenteringPositionSignal(state->viewerState->currentPosition - n * state->viewerState->walkFrames * state->magnification);
        Knossos::sendRemoteSignal();
    } else if (event->key() == Qt::Key_D || event->key() == Qt::Key_F) {
        state->keyD = event->key() == Qt::Key_D;
        state->keyF = event->key() == Qt::Key_F;
        if (!state->viewerKeyRepeat) {
            const float directionSign = event->key() == Qt::Key_D ? -1 : 1;
            const float shiftMultiplier = shift? 10 : 1;
            const float multiplier = directionSign * state->viewerState->dropFrames * state->magnification * shiftMultiplier;
            state->repeatDirection = {{ multiplier * n.x, multiplier * n.y, multiplier * n.z }};
            userMove(Coordinate(state->repeatDirection[0], state->repeatDirection[1], state->repeatDirection[2]), USERMOVE_HORIZONTAL, n);
        }
    } else if (event->key() == Qt::Key_Shift) {
        state->repeatDirection[0] *= 10;
        state->repeatDirection[1] *= 10;
        state->repeatDirection[2] *= 10;
        //enable erase mode on shift down
        Segmentation::singleton().brush.setInverse(true);
    } else if(event->key() == Qt::Key_K || event->key() == Qt::Key_L || event->key() == Qt::Key_M || event->key() == Qt::Key_Comma) {
        if(ViewportOrtho::arbitraryOrientation == false) {
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Information);
            prompt.setWindowTitle("Information");
            prompt.setText("Viewport orientation is still locked. Check 'Arbitrary Viewport Orientation' under 'Appearance Settings → Viewports' first.");
            prompt.exec();
        }
        else {
            switch(event->key()) {
            case Qt::Key_K:
                emit rotationSignal({0., 0., 1.}, boost::math::constants::pi<float>()/180);
                break;
            case Qt::Key_L:
                emit rotationSignal({0., 1., 0.}, boost::math::constants::pi<float>()/180);
                break;
            case Qt::Key_M:
                emit rotationSignal({0., 0., 1.}, -boost::math::constants::pi<float>()/180);
                break;
            case Qt::Key_Comma:
                emit rotationSignal({0., 1., 0.}, -boost::math::constants::pi<float>()/180);
                break;
            }
        }
    } else if(event->key() == Qt::Key_G) {
        //emit genTestNodesSignal(50000);
        // emit updateTreeviewSignal();
    } else if (event->key() == Qt::Key_0) {
        if (ctrl) {
            emit zoomReset();
        }
    } else if(event->key() == Qt::Key_3) {
        if(state->viewerState->drawVPCrosshairs) {
           state->viewerState->drawVPCrosshairs = false;
        }
        else {
           state->viewerState->drawVPCrosshairs = true;
        }
        auto & vpSettings = state->viewer->window->widgetContainer.appearanceWidget.viewportTab;
        vpSettings.drawIntersectionsCrossHairCheckBox.setChecked(state->viewerState->drawVPCrosshairs);

    } else if(event->key() == Qt::Key_I) {
        zoom(zoomStep());
    } else if(event->key() == Qt::Key_O) {
        zoom(-zoomStep());
    } else if(event->key() == Qt::Key_V) {
        if(ctrl) {
            emit pasteCoordinateSignal();
        }
    } else if(event->key() == Qt::Key_1) { // !
        auto & skelSettings = state->viewer->window->widgetContainer.appearanceWidget.treesTab;
        const auto showkeletonOrtho = skelSettings.skeletonInOrthoVPsCheck.isChecked();
        skelSettings.skeletonInOrthoVPsCheck.setChecked(!showkeletonOrtho);
        skelSettings.skeletonInOrthoVPsCheck.clicked(!showkeletonOrtho);
    } else if(event->key() == Qt::Key_Plus) {
        if(ctrl) {
            Segmentation::singleton().brush.setRadius(Segmentation::singleton().brush.getRadius() + 1);
        } else {
            if(Segmentation::singleton().alpha + 10 > 255) {
                Segmentation::singleton().alpha = 255;
            }
            else {
                Segmentation::singleton().alpha += 10;
            }
            auto & segSettings = state->viewer->window->widgetContainer.appearanceWidget.datasetAndSegmentationTab;
            segSettings.segmentationOverlaySlider.setValue(Segmentation::singleton().alpha);
        }
    } else if(event->key() == Qt::Key_Minus) {
        if(ctrl) {
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
            auto & segSettings = state->viewer->window->widgetContainer.appearanceWidget.datasetAndSegmentationTab;
            segSettings.segmentationOverlaySlider.setValue(Segmentation::singleton().alpha);
        }
    } else if(event->key() == Qt::Key_Space) {
        state->viewerState->showOverlay = false;
        state->viewer->oc_reslice_notify_visible();
    } else if(event->key() == Qt::Key_Delete) {
        if(ctrl) {
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
                Skeletonizer::singleton().setActiveNode(state->skeletonState->activeNode);
            }
        }
    }
    else if(event->key() == Qt::Key_F4) {
        if(alt) {
            QApplication::closeAllWindows();
        }
    }
}

void ViewportBase::handleKeyRelease(const QKeyEvent *event) {
    if(event->key() == Qt::Key_Space) {
        state->viewerState->showOverlay = true;
        state->viewer->oc_reslice_notify_visible();
    }
    else if (event->key() == Qt::Key_5) {
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

Coordinate getCoordinateFromOrthogonalClick(const int x_dist, const int y_dist, ViewportOrtho & vp) {
    const auto & currentPos = state->viewerState->currentPosition;
    const auto leftUpper = currentPos - (vp.v1 * vp.edgeLength / 2 / vp.screenPxXPerDataPx) - (vp.v2 * vp.edgeLength / 2 / vp.screenPxYPerDataPx);
    return {roundFloat(leftUpper.x + (x_dist / vp.screenPxXPerDataPx) * vp.v1.x + (y_dist / vp.screenPxYPerDataPx) * vp.v2.x),
            roundFloat(leftUpper.y + (x_dist / vp.screenPxXPerDataPx) * vp.v1.y + (y_dist / vp.screenPxYPerDataPx) * vp.v2.y),
            roundFloat(leftUpper.z + (x_dist / vp.screenPxXPerDataPx) * vp.v1.z + (y_dist / vp.screenPxYPerDataPx) * vp.v2.z)};
}

bool ViewportOrtho::mouseEventAtValidDatasetPosition(const QMouseEvent *event) {
    if(event->x() < 0 || event->x() > (int)edgeLength ||
       event->y() < 0 || event->y() > (int)edgeLength) {
            return false;
    }

    Coordinate pos = getCoordinateFromOrthogonalClick(event->x(), event->y(), *this);
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

void ViewportBase::startNodeSelection(int x, int y) {
    state->viewerState->nodeSelectionSquare.first.x = x;
    state->viewerState->nodeSelectionSquare.first.y = y;

    // reset second point from a possible previous selection square.
    state->viewerState->nodeSelectionSquare.second = state->viewerState->nodeSelectionSquare.first;
    state->viewerState->nodeSelectSquareVpId = id;
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
    return retrieveAllObjectsBeneathSquare(centerX, centerY, width, height);
}

Coordinate ViewportOrtho::getMouseCoordinate() {
    return getCoordinateFromOrthogonalClick(prevMouseMove.x(), prevMouseMove.y(), *this);
}
