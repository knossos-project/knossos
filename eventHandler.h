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

static uint32_t handleUserEvent(SDL_Event event);
static int32_t sdlToAgarEvent(void *obj, SDL_Event sdl_ev, AG_DriverEvent *ag_ev);
static uint32_t handleMouse(SDL_Event event);
static uint32_t handleMouseMotion(SDL_Event event, int32_t VPfound);
static uint32_t handleMouseMotionLeftHold(SDL_Event event, int32_t VPfound);
static uint32_t handleMouseMotionMiddleHold(SDL_Event event, int32_t VPfound);
static uint32_t handleMouseMotionRightHold(SDL_Event event, int32_t VPfound);
static uint32_t handleMouseButtonLeft(SDL_Event event, int32_t VPfound);
static uint32_t handleMouseButtonMiddle(SDL_Event event, int32_t VPfound);
static uint32_t handleMouseButtonRight(SDL_Event event, int32_t VPfound);
static uint32_t handleMouseButtonWheelForward(SDL_Event event, int32_t VPfound);
static uint32_t handleMouseButtonWheelBackward(SDL_Event event, int32_t VPfound);
static uint32_t handleKeyboard(SDL_Event event);
static void moveOrResizeVP(SDL_Event event);
static Coordinate *getCoordinateFromOrthogonalClick(SDL_Event event, int32_t VPfound);
static int32_t checkForViewPortWdgt(AG_Widget *wdgt);
static int mouseOverMoveButton(SDL_Event event);
