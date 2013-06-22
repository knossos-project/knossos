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

#include <math.h>

#include "knossos-global.h"
extern struct stateInfo *state;

int refreshTime() {
    while(TRUE) {
        SDL_Delay(1000);
        refreshViewports();
        if(state->quitSignal == TRUE){
            break;
        }
    }
    return TRUE;
}

void refreshTimeLabel() {
    int time = SDL_GetTicks();
    int hoursRunningTime = (int)(time*0.001/3600.0);
    int minutesRunningTime = (int)(time*0.001/60.0 - hoursRunningTime * 60);
    int secondsRunningTime = (int)(time*0.001 - hoursRunningTime * 3600 - minutesRunningTime * 60);
    AG_LabelText(state->viewerState->ag->runningTime,
                 "Running Time: %02i:%02i:%02i",
                 hoursRunningTime, minutesRunningTime, secondsRunningTime);
}


void checkIdleTime() {
    int time = SDL_GetTicks();
    state->skeletonState->idleTimeLast = state->skeletonState->idleTimeNow;
    state->skeletonState->idleTimeNow = time;
    if (state->skeletonState->idleTimeNow - state->skeletonState->idleTimeLast > 600000) {
        //tolerance of 10 minutes
        state->skeletonState->idleTime +=
            state->skeletonState->idleTimeNow - state->skeletonState->idleTimeLast;
        state->skeletonState->idleTimeSession +=
            state->skeletonState->idleTimeNow - state->skeletonState->idleTimeLast;
    }

    state->skeletonState->unsavedChanges = TRUE;

    int hoursIdleTime = (int)(floor(state->skeletonState->idleTimeSession*0.001)/3600.0);
    int minutesIdleTime = (int)(floor(state->skeletonState->idleTimeSession*0.001)/60.0 - hoursIdleTime * 60);
    int secondsIdleTime = (int)(floor(state->skeletonState->idleTimeSession*0.001) - hoursIdleTime * 3600 - minutesIdleTime * 60);
    AG_LabelText(state->viewerState->ag->idleTime, "Idle Time: %02i:%02i:%02i", hoursIdleTime, minutesIdleTime, secondsIdleTime);

    int hoursTracingTime = (int)((floor(time*0.001) - floor(state->skeletonState->idleTimeSession*0.001))/3600.0);
    int minutesTracingTime = (int)((floor(time*0.001) - floor(state->skeletonState->idleTimeSession*0.001))/60.0 - hoursTracingTime * 60);
    int secondsTracingTime = (int)((floor(time*0.001) - floor(state->skeletonState->idleTimeSession*0.001)) - hoursTracingTime * 3600 - minutesTracingTime * 60);
    AG_LabelText(state->viewerState->ag->tracingTime, "Tracing Time: %02i:%02i:%02i", hoursTracingTime, minutesTracingTime, secondsTracingTime);
}
