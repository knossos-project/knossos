
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

extern stateInfo *state;

Remote::Remote(QObject *parent) :
    QObject(parent)
{
}

static bool cleanUpRemote() {
    free(state->remoteState);
    state->remoteState = NULL;

    return true;
}

void Remote::start() {
    struct remoteState *remoteState = state->remoteState;

    // remoteSignal is != false as long as the remote is active.
    // Checking for remoteSignal is therefore a way of seeing if the remote
    // is available for doing something.
    //
    // Depending on the contents of remoteState, this thread will either go
    // on to listen to a socket and get its instructions from there or it
    // will follow the trajectory given in a file.
    int i = 0;
    while(true) {
        //qDebug("remote says hello %i", ++i);
        Sleeper::msleep(50);
        /*SDL_LockMutex(state->protectRemoteSignal);
        while(state->remoteSignal == false) {
            SDL_CondWait(state->conditionRemoteSignal, state->protectRemoteSignal);
        }

        state->remoteSignal = false;
        SDL_UnlockMutex(state->protectRemoteSignal);

        if(state->quitSignal == true)
            break;

        updateRemoteState();

        switch(remoteState->type) {
            case REMOTE_TRAJECTORY:
                remoteTrajectory(remoteState->activeTrajectory);
                break;

            case REMOTE_RECENTERING:
                //remoteRecentering();
                remoteWalkTo(state->remoteState->recenteringPosition.x, state->remoteState->recenteringPosition.y, state->remoteState->recenteringPosition.z);
                break;

            default:
                printf("No such remote type (%d)\n", remoteState->type);
        }

        if(state->quitSignal == true) {
            break;
        }
        */
    }

    cleanUpRemote();

    QThread::currentThread()->quit();
    emit finished();
}

void Remote::checkIdleTime() { }

int32_t Remote::remoteJump(int32_t x, int32_t y, int32_t z) { return 0;}
