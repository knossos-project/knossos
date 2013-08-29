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
#include "remote.h"
#include "sleeper.h"
#include "viewer.h"
#include "renderer.h"
#include "functions.h"
#include <QDebug>
#include <QTest>
#include <math.h>

extern stateInfo *state;

Remote::Remote(QObject *parent) :
    QThread(parent)
{
    //activeTrajectory = 0;
    maxTrajectories = 16;
    //type = false;
}

void Remote::run() {
    floatCoordinate currToNext; //distance vector
    // remoteSignal is != false as long as the remote is active.
    // Checking for remoteSignal is therefore a way of seeing if the remote
    // is available for doing something.
    //
    // Depending on the contents of remoteState, this thread will either go
    // on to listen to a socket and get its instructions from there or it
    // will follow the trajectory given in a file.

    while(true) {
        //qDebug("remote says hello %i", ++i);
        msleep(50);
        state->protectRemoteSignal->lock();
        while(!state->remoteSignal) {
            state->conditionRemoteSignal->wait(state->protectRemoteSignal);
        }

        state->remoteSignal = false;
        state->protectRemoteSignal->unlock();

        if(state->quitSignal) {
            break;
        }

        updateRemoteState();

        switch(this->type) {

            case REMOTE_TRAJECTORY:
                remoteTrajectory(this->activeTrajectory);
                break;

            case REMOTE_RECENTERING:
                SET_COORDINATE (currToNext, state->viewerState->currentPosition.x - this->recenteringPosition.x,
                state->viewerState->currentPosition.y - this->recenteringPosition.y,
                state->viewerState->currentPosition.z - this->recenteringPosition.z);

                if(euclidicNorm(&currToNext) > JMP_THRESHOLD) {
                    remoteJump(this->recenteringPosition.x,
                                          this->recenteringPosition.y,
                                          this->recenteringPosition.z);
                               break;
                }

                remoteWalk(this->recenteringPosition.x - state->viewerState->currentPosition.x,
                           this->recenteringPosition.y - state->viewerState->currentPosition.y,
                           this->recenteringPosition.z - state->viewerState->currentPosition.z);

                break;

            default:
                LOG("No such remote type (%d)\n", this->type)

        }

        if(state->quitSignal == true) {
            break;
        }
    }

}

bool Remote::newTrajectory(char *trajName, char *trajectory) {

    int i = 0;

    if(*trajName == '\0')
        return false;

    if(state->trajectories == NULL) {
        state->trajectories = (struct trajectory*) malloc(this->maxTrajectories * sizeof(struct trajectory));
        if(state->trajectories == NULL) {
            printf("Out of memory.\n");
            return false;
        }
        memset(state->trajectories, '\0', sizeof(struct trajectory) * this->maxTrajectories);

        state->maxTrajectories = this->maxTrajectories;
    }

    for(i = 0; i < state->maxTrajectories; i++) {
        if(strncmp(state->trajectories[i].name, trajName, 63) == 0) {
            if(state->trajectories[i].source != NULL) {
                free(state->trajectories[i].source);
                state->trajectories[i].source = NULL;
            }
            state->trajectories[i].source = (char*) strdup(trajectory);
            return true;
        }

        if(state->trajectories[i].name[0] == '\0') {
            strncpy(state->trajectories[i].name, trajName, 63);
            state->trajectories[i].source = (char*) strdup(trajectory);
            return true;
        }
    }

    if(i == state->maxTrajectories)
        return false;

    return true;
}

/**
 * @todo lex parser
 */
bool Remote::remoteTrajectory(int trajNumber) {
    /*
    YY_BUFFER_STATE trajBuffer;

    if(state->trajectories != NULL) {
        trajBuffer = yy_scan_string(state->trajectories[trajNumber].source);
        yyparse(state);
        yy_delete_buffer(trajBuffer);
    } */

    return true;
}

bool Remote::updateRemoteState() {

    int i = 0;

    /* @CMP
    if(state->trajectories != NULL) {
        free(state->trajectories);
        state->trajectories = NULL;
    }

    if(tempConfig->trajectories != NULL) {
        state->maxTrajectories = tempConfig->maxTrajectories;
        for(i = 0; i < state->maxTrajectories; i++)
            newTrajectory(tempConfig->trajectories[i].name, tempConfig->trajectories[i].source);
    }
    */

    for(int i = 0; i < state->maxTrajectories; i++)
        newTrajectory(state->trajectories[i].name, state->trajectories[i].source);

    return true;
}

/**
 * @attention jump event is replaced with userMoveSignal
 */
bool Remote::remoteJump(int x, int y, int z) {
    // is not threadsafe


    emit userMoveSignal(x - state->viewerState->currentPosition.x,
                        y - state->viewerState->currentPosition.y,
                        z - state->viewerState->currentPosition.z,
                        SILENT_COORDINATE_CHANGE);

    return true;
}

void Remote::msleep(unsigned long msec) {
    QThread::msleep(msec);
}

/**
 * @attention moveEvent is replaced  by userMoveSignal
 */
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

    /*
    * BUG: For some reason, events to the viewer seem to become lost under
    * some circumstances, resulting in incorrect paths when calling
    * remoteWalk(). The remoteWalkTo() wrapper takes care of that, but the
    * problem should be addressed in a more general way - I just don't kow
    * how, yet.
    *
    */

    floatCoordinate singleMove;
    floatCoordinate residuals;
    Coordinate doMove;
    Coordinate *sendMove = NULL;
    int totalMoves = 0, i = 0;
    int eventDelay = 0;
    floatCoordinate walkVector;
    float walkLength = 0.;
    uint timePerStep = 0;
    uint recenteringTime = 0;

    walkVector.x = (float) x;
    walkVector.y = (float) y;
    walkVector.z = (float) z;

    //Not locked...

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
    if ((state->viewerState->autoTracingMode != 0) && (state->viewerState->walkOrth == false)){
        recenteringTime = state->viewerState->autoTracingSteps * state->viewerState->autoTracingDelay;
    }

    walkLength = euclidicNorm(&walkVector);

    if(walkLength < 10.) walkLength = 10.;

    timePerStep = recenteringTime / ((uint)walkLength);

    if(timePerStep < 10) timePerStep = 10;

    //emit userMoveSignal(walkVector.x, walkVector.y, walkVector.z, TELL_COORDINATE_CHANGE);

    if(state->viewerState->stepsPerSec > 0)
        eventDelay = 1000 / state->viewerState->stepsPerSec;
    else
        eventDelay = 50;

    if(this->type == REMOTE_RECENTERING)
        eventDelay = timePerStep;

    if(abs(x) >= abs(y) && abs(x) >= abs(z)) {
        totalMoves = abs(x) / state->magnification;
    }
    if(abs(y) >= abs(x) && abs(y) >= abs(z)) {
        totalMoves = abs(y) / state->magnification;

    }
    if(abs(z) >= abs(x) && abs(z) >= abs(y)) {
        totalMoves = abs(z) / state->magnification;
    }

    //qDebug() << totalMoves << "T";

    singleMove.x = (float)x / (float)totalMoves;
    singleMove.y = (float)y / (float)totalMoves;
    singleMove.z = (float)z / (float)totalMoves;

    SET_COORDINATE(residuals, 0, 0, 0);
    for(i = 0; i < totalMoves; i++) {
        SET_COORDINATE(doMove, 0, 0, 0);

        residuals.x += singleMove.x;
        residuals.y += singleMove.y;
        residuals.z += singleMove.z;

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
            sendMove = (Coordinate *) malloc(sizeof(Coordinate));
            if(sendMove == NULL) {
                LOG("Out of memory.\n")
                return false;
            }
            memset(sendMove, '\0', sizeof(Coordinate));

            sendMove->x = doMove.x;
            sendMove->y = doMove.y;
            sendMove->z = doMove.z;

            if(x == 0)
                sendMove->x = 0;
            if(y == 0)
                sendMove->y = 0;
            if(z == 0)
                sendMove->z = 0;

            emit userMoveSignal(sendMove->x, sendMove->y, sendMove->z, TELL_COORDINATE_CHANGE);            
        }

        // This is, of course, not really correct as the time of running
        // the loop body would need to be accounted for. But SDL_Delay()
        // granularity isn't fine enough and it doesn't matter anyway.
        msleep(eventDelay);
    }

    emit idleTimeSignal();
    return true;
}

void Remote::setRemoteStateType(int type) {
    this->type = type;
}

void Remote::setRecenteringPosition(int x, int y, int z) {
    this->recenteringPosition.x = x;
    this->recenteringPosition.y = y;
    this->recenteringPosition.z = z;
}

bool Remote::remoteDelay(int s) {

}
