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
#include "remote.h"

#include "skeleton/node.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

#include <QQuaternion>
#include <QVector3D>

#include <algorithm>
#include <cmath>

const qint64 Remote::ms{10};
const float Remote::goodEnough{0.01};

Remote::Remote() {
    timer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&timer, &QTimer::timeout, [this](){remoteWalk();});
}

void Remote::process(const Coordinate & pos) {
    //distance vector
    floatCoordinate deltaPos = pos - state->viewerState->currentPosition;
    int jumpThreshold = 0.5 * state->cubeEdgeLength * state->M * state->magnification;//approximately inside sc
    if (deltaPos.length() > jumpThreshold) {
        state->viewer->setPosition(pos);
    } else if (pos != state->viewerState->currentPosition) {
        recenteringOffset = pos - state->viewerState->currentPosition;
        elapsed.restart();
        timer.start(ms);
        state->viewer->userMoveClear();
        remoteWalk();
    }
}

std::deque<floatCoordinate> Remote::getLastNodes() {
    std::deque<floatCoordinate> nodelist;
    nodeListElement const * node = state->skeletonState->activeNode;

    //we are in the middle of our skeleton or we don't have a skeleton
    if (node == nullptr || node->segments.size() != 1) {
        return nodelist;
    }

    nodeListElement const * previousNode = nullptr;
    for (int i = 0; i < 10; ++i) {
        if (node->segments.size() == 1) {
            if (previousNode != nullptr && (node->segments.front().source == *previousNode || node->segments.front().target == *previousNode)) {
                break;
            }
            previousNode = node;
            if (node->segments.front().source == *node) {
                node = &node->segments.front().target;
            } else {
                node = &node->segments.front().source;
            }
        } else if (node->segments.empty()) {
            break;
        } else {
            auto nextIt = std::next(std::begin(node->segments));//check next segment
            if (previousNode != nullptr && (nextIt->source == *previousNode || nextIt->target == *previousNode)) {
                previousNode = node;
                if (node->segments.front().source == *node) {
                    node = &node->segments.front().target;
                } else {
                    node = &node->segments.front().source;
                }
            } else {//use first segment if second segment was reverse segment to previous node
                previousNode = node;
                if (nextIt->source == *node) {
                    node = &nextIt->target;
                } else {
                    node = &nextIt->source;
                }
            }
        }
        nodelist.push_back(node->position);
    }

    return nodelist;
}

void Remote::remoteWalk() {
    /*
    * This function breaks the big walk distance into many small movements
    * where the maximum length of the movement along any single axis is
    * equal to the magnification, i.e. in mag4 it is 4.
    * As we cannot move by fractions, this function keeps track of
    * residuals that add up to make a movement of an integer along an
    * axis every once in a while.
    * An alternative would be to store the currentPosition as a float or
    * double but that has its own problems. We might do it in the future,
    * though.
    * Possible improvement to this function: Make the length of a complete
    * singleMove to match mag, not the length of the movement on one axis.
    *
    */
    QQuaternion quaternion; // initially no rotation
    if (rotate) {
        std::deque<floatCoordinate> lastRecenterings = getLastNodes();
        if (!lastRecenterings.empty()) {
            floatCoordinate avg;
            for(const auto coord : lastRecenterings) {
                avg += coord;
            }
            avg /= lastRecenterings.size();

            const auto target = state->viewerState->currentPosition + recenteringOffset;
            floatCoordinate delta = target - avg;
            const auto normal = state->viewer->window->viewportOrtho(activeVP)->n;
            quaternion = QQuaternion::rotationTo({normal.x, normal.y, normal.z}, {delta.x, delta.y, delta.z});
        }
    }

    const auto seconds = recenteringOffset.length() / (state->viewerState->movementSpeed * state->magnification);
    const auto totalMoves = std::max(1.0f, seconds / (std::max(ms, elapsed.elapsed()) / 1000.0f));
    const floatCoordinate singleMove = recenteringOffset / totalMoves;

    float angle = 0;
    QVector3D axis;
    if (rotate) {
        quaternion.getAxisAndAngle(&axis, &angle);
        quaternion = QQuaternion::fromAxisAndAngle(axis, angle / totalMoves);
        state->viewer->addRotation(quaternion);
    }

    state->viewer->userMove(singleMove);
    recenteringOffset -= singleMove;
    if (std::abs(recenteringOffset.x) < goodEnough && std::abs(recenteringOffset.y) < goodEnough && std::abs(recenteringOffset.z) < goodEnough && angle < goodEnough) {
        state->viewer->userMoveRound();
        timer.stop();
    } else {
        elapsed.restart();
    }
}
