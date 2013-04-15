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

/*      BUGS
 *
 *      * Probably race bugs when recentering and remote syncing at the same
 *        time.
 */

/*
 *      Code for the "remote control" of KNOSSOS (recentering and running
 *      trajectories). Maybe this should be changed at some point with
 *      recentering being done in place from viewer.c and this thread only being
 *      used for trajectories.
 *
 *      We need to be careful about race conditions when doing concurrent
 *      movements from multiple threads.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "knossos-global.h"
#include "remote.h"
#include "y.tab.h"
#include "lex.yy.h"


#define MAX_TRY 2
extern struct stateInfo *tempConfig;
extern struct stateInfo *state;

int remote() {
    struct remoteState *remoteState = state->remoteState;
    floatCoordinate currToNext; //distance vector

    // remoteSignal is != FALSE as long as the remote is active.
    // Checking for remoteSignal is therefore a way of seeing if the remote
    // is available for doing something.
    //
    // Depending on the contents of remoteState, this thread will either go
    // on to listen to a socket and get its instructions from there or it
    // will follow the trajectory given in a file.

    while(TRUE) {
        SDL_LockMutex(state->protectRemoteSignal);
        while(state->remoteSignal == FALSE) {
            SDL_CondWait(state->conditionRemoteSignal, state->protectRemoteSignal);
        }

        state->remoteSignal = FALSE;
        SDL_UnlockMutex(state->protectRemoteSignal);

        if(state->quitSignal == TRUE)
            break;

        updateRemoteState();

        switch(remoteState->type) {
            case REMOTE_TRAJECTORY:
                remoteTrajectory(remoteState->activeTrajectory);
                break;

            case REMOTE_RECENTERING:
                //jump if distance is too high
                SET_COORDINATE (currToNext,
                                state->viewerState->currentPosition.x - state->remoteState->recenteringPosition.x,
                                state->viewerState->currentPosition.y - state->remoteState->recenteringPosition.y,
                                state->viewerState->currentPosition.z - state->remoteState->recenteringPosition.z);
                if(euclidicNorm(&currToNext) > JMP_THRESHOLD) {
                    remoteJump(state->remoteState->recenteringPosition.x,
                               state->remoteState->recenteringPosition.y,
                               state->remoteState->recenteringPosition.z);
                    break;
                }
                remoteWalkTo(state->remoteState->recenteringPosition.x, state->remoteState->recenteringPosition.y, state->remoteState->recenteringPosition.z);
                break;

            default:
                printf("No such remote type (%d)\n", remoteState->type);
        }

        if(state->quitSignal == TRUE)
            break;
    }

    cleanUpRemote();

    return TRUE;
}

static int32_t updateRemoteState() {
    struct remoteState *remoteState = state->remoteState;
    int32_t i = 0;

    remoteState->type = tempConfig->remoteState->type;
    remoteState->recenteringPosition = tempConfig->remoteState->recenteringPosition;

    if(state->trajectories != NULL) {
        free(state->trajectories);
        state->trajectories = NULL;
    }

    if(tempConfig->trajectories != NULL) {
        state->maxTrajectories = tempConfig->maxTrajectories;
        for(i = 0; i < state->maxTrajectories; i++)
            newTrajectory(tempConfig->trajectories[i].name, tempConfig->trajectories[i].source);
    }

    remoteState->activeTrajectory = tempConfig->remoteState->activeTrajectory;

    return TRUE;
}

static int32_t cleanUpRemote() {
    free(state->remoteState);
    state->remoteState = NULL;

    return TRUE;
}

int32_t remoteJump(int32_t x, int32_t y, int32_t z) {
    SDL_Event jumpEvent;
    // This function is _not_ thread safe

    tempConfig->viewerState->currentPosition.x = x;
    tempConfig->viewerState->currentPosition.y = y;
    tempConfig->viewerState->currentPosition.z = z;

    jumpEvent.type = SDL_USEREVENT;
    jumpEvent.user.code = USEREVENT_JUMP;

    SDL_PushEvent(&jumpEvent);

    return TRUE;
}

int32_t remoteWalkTo(int32_t x, int32_t y, int32_t z) {
    /* This function is _not_ thread safe
     * Do not get confused!
     * remoteWalkTo walks us TO the coordinates (x, y, z) and remoteWalk
     * walks along the vector (x, y, z) from wherever we start.
     *
     */

    int32_t x_moves = 0, y_moves = 0, z_moves = 0;
    int32_t retval = TRUE;

    /*
     *  TDItem
     *  The commented part below is a workaround that causes the so-called
     *  "jitter bug". I could not reproduce the underlying problem in the agar
     *  version of knossos, so I'll comment it out in the hope that the bug is
     *  gone. Testing is needed here.
     *
     */

    /*
     * The loop is a hack to get around the problem where move events to the
     * viewer are being lost. When that happens, the walk ends somewhere
     * near to the real destination. This is, of course, _wrong_ in
     * principle because it could "bend" the path.
     *
     * Also, the real cause of that bug is very much unclear...
     *
     * This is very shitty and has a serious race condition now that the client
     * runs in a seperate thread.
     *

    while(state->viewerState->currentPosition.x != x || state->viewerState->currentPosition.y != y
            || state->viewerState->currentPosition.z != z) {
        x_moves = x - state->viewerState->currentPosition.x;
        y_moves = y - state->viewerState->currentPosition.y;
        z_moves = z - state->viewerState->currentPosition.z;

        retval = remoteWalk(x_moves, y_moves, z_moves);

        // This is a workaround to cover a bug in the workaround... ;)

        SDL_Delay(100);
    }*/


    x_moves = x - state->viewerState->currentPosition.x;
    y_moves = y - state->viewerState->currentPosition.y;
    z_moves = z - state->viewerState->currentPosition.z;

    retval = remoteWalk(x_moves, y_moves, z_moves);
    return retval;
}

int32_t remoteWalk(int32_t x, int32_t y, int32_t z) {
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

    SDL_Event moveEvent;
    floatCoordinate singleMove;
    floatCoordinate residuals;
    Coordinate doMove;
    Coordinate *sendMove = NULL;
    int32_t totalMoves = 0, i = 0;
    int32_t eventDelay = 0;
    floatCoordinate walkVector;
    float walkLength = 0.;
    uint32_t timePerStep = 0;
    uint32_t recenteringTime = 0;

    walkVector.x = (float) x;
    walkVector.y = (float) y;
    walkVector.z = (float) z;

    //Not locked...

    if (tempConfig->viewerState->recenteringTime > 5000){
        tempConfig->viewerState->recenteringTime = 5000;
        updateViewerState();
    }

    if (tempConfig->viewerState->recenteringTimeOrth < 10){
        tempConfig->viewerState->recenteringTimeOrth = 10;
        updateViewerState();
    }

    if (tempConfig->viewerState->recenteringTimeOrth > 5000){
        tempConfig->viewerState->recenteringTimeOrth = 5000;
        updateViewerState();
    }

    if (state->viewerState->walkOrth == FALSE){
        recenteringTime = state->viewerState->recenteringTime;
    }
    else {
        recenteringTime = state->viewerState->recenteringTimeOrth;
        state->viewerState->walkOrth = FALSE;
    }
    if ((state->viewerState->autoTracingMode != 0) && (state->viewerState->walkOrth == FALSE)){
        recenteringTime = state->viewerState->autoTracingSteps * state->viewerState->autoTracingDelay;
    }

    walkLength = euclidicNorm(&walkVector);
    if(walkLength < 10.) { walkLength = 10.; }

    timePerStep = recenteringTime / ((uint32_t)walkLength);
    if(timePerStep < 10) { timePerStep = 10; }

    moveEvent.type = SDL_USEREVENT;
    moveEvent.user.code = USEREVENT_MOVE;

    if(state->viewerState->stepsPerSec > 0) {
        eventDelay = 1000 / state->viewerState->stepsPerSec;
    }
    else {
        eventDelay = 50;
    }
    if(state->remoteState->type == REMOTE_RECENTERING) {
        eventDelay = timePerStep;
    }
    //determine total moves based on main direction
    if(abs(x) >= abs(y) && abs(x) >= abs(z)) {
        totalMoves = abs(x) / state->magnification;
    }
    else if(abs(y) >= abs(x) && abs(y) >= abs(z)) {
        totalMoves = abs(y) / state->magnification;
    }
    else {
        totalMoves = abs(z) / state->magnification;
    }

    singleMove.x = (float)x / (float)totalMoves;
    singleMove.y = (float)y / (float)totalMoves;
    singleMove.z = (float)z / (float)totalMoves;

    SET_COORDINATE(residuals, 0., 0., 0.);
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
            sendMove = malloc(sizeof(Coordinate));
            if(sendMove == NULL) {
                LOG("Out of memory.\n");
                return FALSE;
            }
            memset(sendMove, '\0', sizeof(Coordinate));

            sendMove->x = doMove.x;
            sendMove->y = doMove.y;
            sendMove->z = doMove.z;

            moveEvent.user.data1 = sendMove;

            while(SDL_PushEvent(&moveEvent) == FAIL) {
                printf("get error: %s\n", SDL_GetError());
                SDL_Delay(10);
            }
        }

        // This is, of course, not really correct as the time of running
        // the loop body would need to be accounted for. But SDL_Delay()
        // granularity isn't fine enough and it doesn't matter anyway.
        SDL_Delay(eventDelay);
    }
    checkIdleTime();
    return TRUE;
}

int32_t newTrajectory(char *trajName, char *trajectory) {
    int32_t i = 0;

    if(*trajName == '\0')
        return FALSE;

    if(state->trajectories == NULL) {
        state->trajectories = malloc(sizeof(struct trajectory) * state->remoteState->maxTrajectories);
        if(state->trajectories == NULL) {
            printf("Out of memory.\n");
            return FALSE;
        }
        memset(state->trajectories, '\0', sizeof(struct trajectory) * state->remoteState->maxTrajectories);

        state->maxTrajectories = state->remoteState->maxTrajectories;
    }

    for(i = 0; i < state->maxTrajectories; i++) {
        if(strncmp(state->trajectories[i].name, trajName, 63) == 0) {
            if(state->trajectories[i].source != NULL) {
                free(state->trajectories[i].source);
                state->trajectories[i].source = NULL;
            }
            state->trajectories[i].source = (char*) strdup(trajectory);
            return TRUE;
        }

        if(state->trajectories[i].name[0] == '\0') {
            strncpy(state->trajectories[i].name, trajName, 63);
            state->trajectories[i].source = (char*) strdup(trajectory);
            return TRUE;
        }
    }

    if(i == state->maxTrajectories)
        return FALSE;

    return TRUE;
}

static int32_t remoteTrajectory(int32_t trajNumber) {
    YY_BUFFER_STATE trajBuffer;

    if(state->trajectories != NULL) {
        trajBuffer = yy_scan_string(state->trajectories[trajNumber].source);
        yyparse(state);
        yy_delete_buffer(trajBuffer);
    }

    return TRUE;
}

void checkIdleTime() {
    int time =SDL_GetTicks();
    state->skeletonState->idleTimeLast = state->skeletonState->idleTimeNow;
    state->skeletonState->idleTimeNow = time;
    if (state->skeletonState->idleTimeNow - state->skeletonState->idleTimeLast > 900000) { //tolerance of 15 minutes
        state->skeletonState->idleTime += state->skeletonState->idleTimeNow - state->skeletonState->idleTimeLast;
        state->skeletonState->idleTimeSession += state->skeletonState->idleTimeNow - state->skeletonState->idleTimeLast;
    }
    int hoursIdleTime = (int)(floor(state->skeletonState->idleTimeSession*0.001)/3600.0);
    int minutesIdleTime = (int)(floor(state->skeletonState->idleTimeSession*0.001)/60.0 - hoursIdleTime * 60);
    int secondsIdleTime = (int)(floor(state->skeletonState->idleTimeSession*0.001) - hoursIdleTime * 3600 - minutesIdleTime * 60);
    AG_LabelText(state->viewerState->ag->idleTime, "Idle Time: %02i:%02i:%02i", hoursIdleTime, minutesIdleTime, secondsIdleTime);

    int hoursTracingTime = (int)((floor(time*0.001) - floor(state->skeletonState->idleTimeSession*0.001))/3600.0);
    int minutesTracingTime = (int)((floor(time*0.001) - floor(state->skeletonState->idleTimeSession*0.001))/60.0 - hoursTracingTime * 60);
    int secondsTracingTime = (int)((floor(time*0.001) - floor(state->skeletonState->idleTimeSession*0.001)) - hoursTracingTime * 3600 - minutesTracingTime * 60);
    AG_LabelText(state->viewerState->ag->tracingTime, "Tracing Time: %02i:%02i:%02i", hoursTracingTime, minutesTracingTime, secondsTracingTime);
}
