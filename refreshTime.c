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
