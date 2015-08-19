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
#include "widgets/navigationwidget.h"
#include "widgets/viewport.h"

#include <QDebug>

#include <cmath>

Remote::Remote(QObject *parent) : QThread(parent) {}

void Remote::run() {
    // remoteSignal is != false as long as the remote is active.
    // Checking for remoteSignal is therefore a way of seeing if the remote
    // is available for doing something.
    //
    // Depending on the contents of remoteState, this thread will either go
    // on to listen to a socket and get its instructions from there or it
    // will follow the trajectory given in a file.
    rotate = false;
    activeVP = 0;
    while(true) {
        state->protectRemoteSignal->lock();
        while(!state->remoteSignal) {
            state->conditionRemoteSignal->wait(state->protectRemoteSignal);
        }

        state->remoteSignal = false;
        state->protectRemoteSignal->unlock();

        if(state->quitSignal) {
            break;
        }
        //distance vector
        floatCoordinate currToNext = recenteringPosition - state->viewerState->currentPosition;
        if(euclidicNorm(currToNext) > jumpThreshold) {
            remoteJump(recenteringPosition.x, recenteringPosition.y, recenteringPosition.z);
        } else {
            remoteWalk(round(currToNext.x), round(currToNext.y), round(currToNext.z));
        }

        if(state->quitSignal == true) {
            break;
        }
    }
}

bool Remote::remoteJump(int x, int y, int z) {
    // is not threadsafe

    emit userMoveSignal(x - state->viewerState->currentPosition.x,
                        y - state->viewerState->currentPosition.y,
                        z - state->viewerState->currentPosition.z,
                        USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);

    return true;
}

void Remote::msleep(unsigned long msec) {
    QThread::msleep(msec);
}

std::deque<floatCoordinate> Remote::getLastNodes() {

    std::deque<floatCoordinate> nodelist;
    nodelist.clear();
    floatCoordinate pos;

    nodeListElement *node = state->skeletonState->activeNode;
    nodeListElement *lastnode=NULL;

    if(node == NULL) {
        return nodelist;
    }
    if(node->firstSegment == NULL || node->getSegments()->size() > 1) { //we are in the middle of our skeleton or we don't have a skeleton
        return nodelist;
    }

    for(int i = 0; i < 10; ++i) {
        qDebug() << node->nodeID;

        if(node->getSegments()->size() == 1) {
            if(node->firstSegment->source == lastnode || node->firstSegment->target == lastnode) break;
            if(node->firstSegment->source == node) {
                lastnode = node;
                node = node->firstSegment->target;
            } else {
                lastnode = node;
                node= node->firstSegment->source;
            }
        } else if(node->getSegments()->size() == 0) {
            break;
        } else {
            if(node->firstSegment->next->source == lastnode || node->firstSegment->next->target == lastnode) {
                //user firstSegment
                if(node->firstSegment->source == node) {
                    lastnode = node;
                    node = node->firstSegment->target;
                } else {
                    lastnode = node;
                    node = node->firstSegment->source;
                }
            } else {
                if(node->firstSegment->next->source == node) {
                    lastnode = node;
                    node = node->firstSegment->next->target;
                } else {
                    lastnode = node;
                    node = node->firstSegment->next->source;
                }
            }
        }

        pos.x = node->position.x;
        pos.y = node->position.y;
        pos.z = node->position.z;

        nodelist.push_back(pos);
    }

    qDebug() << nodelist.size();

    return nodelist;
}

bool Remote::remoteWalk(int x, int y, int z) {

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
    auto rotation = Rotation(); // initially no rotation
    if(Viewport::arbitraryOrientation) {
        std::deque<floatCoordinate> lastRecenterings;
        lastRecenterings = getLastNodes();
        if(rotate && lastRecenterings.empty() == false) {
            // calculate avg of last x recentering positions
            floatCoordinate avg;
            for(const auto coord : lastRecenterings) {
                avg += coord;
            }
            avg /= lastRecenterings.size();

            floatCoordinate delta = {recenteringPosition.x - avg.x, recenteringPosition.y - avg.y, recenteringPosition.z - avg.z};
            normalizeVector(delta);
            float scalar = scalarProduct(state->viewerState->vpConfigs[activeVP].n, delta);
            rotation.alpha = acosf(std::min(1.f, std::max(-1.f, scalar)));
            rotation.axis = crossProduct(state->viewerState->vpConfigs[activeVP].n, delta);
            normalizeVector(rotation.axis);
        }
    }

    floatCoordinate walkVector = Coordinate{x, y, z};

    uint recenteringTime = 0;
    if (state->viewerState->recenteringTime > 5000){
        state->viewerState->recenteringTime = 5000;
        emit updateViewerStateSignal();
    }
    if (state->viewerState->recenteringTimeOrth < 10){
        state->viewerState->recenteringTimeOrth = 10;
        emit updateViewerStateSignal();
    }
    if (state->viewerState->recenteringTimeOrth > 5000){
        state->viewerState->recenteringTimeOrth = 5000;
        emit updateViewerStateSignal();
    }
    if (state->viewerState->walkOrth == false){
        recenteringTime = state->viewerState->recenteringTime;
    }
    else {
        recenteringTime = state->viewerState->recenteringTimeOrth;
        state->viewerState->walkOrth = false;
    }
    if ((state->viewerState->autoTracingMode != navigationMode::recenter) && (state->viewerState->walkOrth == false)){
        recenteringTime = state->viewerState->autoTracingSteps * state->viewerState->autoTracingDelay;
    }

    float walkLength = std::max(10.f, euclidicNorm(walkVector));
    uint timePerStep = std::max(10u, recenteringTime / ((uint)walkLength));
    float totalMoves = std::max(std::max(abs(x), abs(y)), abs(z)) / state->magnification;
    floatCoordinate singleMove = walkVector / totalMoves;
    floatCoordinate residuals;
    float anglesPerStep = 0;
    if(Viewport::arbitraryOrientation) {
        anglesPerStep = rotation.alpha/totalMoves;
    }
    for(int i = 0; i < totalMoves; i++) {
        if(Viewport::arbitraryOrientation) {
            emit rotationSignal(rotation.axis.x, rotation.axis.y, rotation.axis.z, anglesPerStep);
        }
        Coordinate doMove;
        residuals += singleMove;

        if(residuals.x >= state->magnification) {
            doMove.x = state->magnification;
            residuals.x -= state->magnification;
        }
        else if(residuals.x <= -state->magnification) {
            doMove.x = -state->magnification;
            residuals.x += state->magnification;
        }

        if(residuals.y >= state->magnification) {
            doMove.y = state->magnification;
            residuals.y -= state->magnification;
        }
        else if(residuals.y <= -state->magnification) {
            doMove.y = -state->magnification;
            residuals.y += state->magnification;
        }

        if(residuals.z >= state->magnification) {
            doMove.z = state->magnification;
            residuals.z -= state->magnification;
        }
        else if(residuals.z <= -state->magnification) {
            doMove.z = -state->magnification;
            residuals.z += state->magnification;
        }

        if(doMove.x != 0 || doMove.z != 0 || doMove.y != 0) {
            emit userMoveSignal(doMove.x, doMove.y, doMove.z,
                                USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
        }
        // This is, of course, not really correct as the time of running
        // the loop body would need to be accounted for. But SDL_Delay()
        // granularity isn't fine enough and it doesn't matter anyway.
        msleep(timePerStep);
    }
    emit userMoveSignal(round(residuals.x), round(residuals.y), round(residuals.z),
                        USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
    return true;
}

void Remote::setRecenteringPosition(float x, float y, float z) {
    recenteringPosition.x = x;
    recenteringPosition.y = y;
    recenteringPosition.z = z;
    rotate = false;
}

void Remote::setRecenteringPositionWithRotation(float x, float y, float z, uint vp) {
    recenteringPosition.x = x;
    recenteringPosition.y = y;
    recenteringPosition.z = z;
    rotate = true;
    activeVP = vp;
}

