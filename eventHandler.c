/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2012
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

/*
 *      Code for whatever.
 *
 *      Explain it.
 *
 *      Functions explained in the header file.
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL/SDL.h>
#include <SDL/SDL_Clipboard.h>

#include <agar/core.h>
//#include <agar/gui.h>

#include "knossos-global.h"
#include "eventHandler.h"

extern struct stateInfo *tempConfig;
extern struct stateInfo *state;


uint32_t handleEvent(SDL_Event event) {
    AG_DriverEvent ag_dev;

    /* TDitem find right place for that */
    if(AG_TAILQ_FIRST(&agTimeoutObjQ) != NULL) {
        /* Advance agar's timing wheels. */
        Uint32 time = SDL_GetTicks();
        AG_ProcessTimeout(time);
    }

    switch(event.type) {
        case SDL_MOUSEMOTION:
            if (!handleMouse(event))
                LOG("Error in handleMouse()");
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (!handleMouse(event))
                LOG("Error in handleMouse()");
            break;
        case SDL_MOUSEBUTTONUP:
            if (!handleMouse(event))
                LOG("Error in handleMouse()");
            break;
        case SDL_KEYDOWN:
            if(!handleKeyboard(event))
                LOG("Error in handleKeyboard()");
            break;
        case SDL_VIDEORESIZE:
            /*  */

            break;
        case SDL_QUIT:
            /* We intercept SDL_QUIT to display the "do you really want to
             * quit" dialog if necessary.
             * If we really quit, we generate a SDL_USEREVENT with
             * USEREVENT_REALQUIT. This is necessary to work with the
             * callback function needed for the yes / no prompt. */

            if(state->skeletonState->unsavedChanges) {
                yesNoPrompt(NULL, "There are unsaved changes. Really quit?", quitKnossos, NULL);
            }
            else {
                quitKnossos();
            }

            break;
        case SDL_USEREVENT:
            if (handleUserEvent(event) == FALSE) {
                return FALSE;
            }
    }

    sdlToAgarEvent(agDriverSw, event, &ag_dev);

    /* send events to agar */
    AG_ProcessEvent(NULL, &ag_dev);

    return TRUE;
}

static int32_t sdlToAgarEvent(void *obj, SDL_Event sdl_ev, AG_DriverEvent *ag_ev)
{
    /*
     * This stuff is copied from agar 1.4.0 AG_SDL_GetNextEvent()
     * from gui/drv_sdl_common.c.
     *
     * It's a pretty horrible hack to do it like this, but the
     * agar 1.4 generic event handling interface doesn't suppport some
     * of the things we've been doing with SDL events before with
     * agar 1.3, like pushing SDL_USEREVENT etc. Beside that, a pretty
     * big rewrite of our event handling would be necessary to port
     * to the agar 1.4 style.
     *
     */

    AG_Driver *drv = obj;

    switch (sdl_ev.type) {
    case SDL_MOUSEMOTION:
        AG_MouseMotionUpdate(drv->mouse, sdl_ev.motion.x, sdl_ev.motion.y);

        ag_ev->type = AG_DRIVER_MOUSE_MOTION;
        ag_ev->win = NULL;
        ag_ev->data.motion.x = sdl_ev.motion.x;
        ag_ev->data.motion.y = sdl_ev.motion.y;
        break;
    case SDL_MOUSEBUTTONUP:
        AG_MouseButtonUpdate(drv->mouse, AG_BUTTON_RELEASED,
            sdl_ev.button.button);

        ag_ev->type = AG_DRIVER_MOUSE_BUTTON_UP;
        ag_ev->win = NULL;
        ag_ev->data.button.which = (AG_MouseButton)sdl_ev.button.button;
        ag_ev->data.button.x = sdl_ev.button.x;
        ag_ev->data.button.y = sdl_ev.button.y;
        break;
    case SDL_MOUSEBUTTONDOWN:
        AG_MouseButtonUpdate(drv->mouse, AG_BUTTON_PRESSED,
            sdl_ev.button.button);

        ag_ev->type = AG_DRIVER_MOUSE_BUTTON_DOWN;
        ag_ev->win = NULL;
        ag_ev->data.button.which = (AG_MouseButton)sdl_ev.button.button;
        ag_ev->data.button.x = sdl_ev.button.x;
        ag_ev->data.button.y = sdl_ev.button.y;
        break;
    case SDL_KEYDOWN:
        AG_KeyboardUpdate(drv->kbd, AG_KEY_PRESSED,
            (AG_KeySym)sdl_ev.key.keysym.sym,
            (Uint32)sdl_ev.key.keysym.unicode);

        ag_ev->type = AG_DRIVER_KEY_DOWN;
        ag_ev->win = NULL;
        ag_ev->data.key.ks = (AG_KeySym)sdl_ev.key.keysym.sym;
        ag_ev->data.key.ucs = (Uint32)sdl_ev.key.keysym.unicode;
        break;
    case SDL_KEYUP:
        AG_KeyboardUpdate(drv->kbd, AG_KEY_RELEASED,
            (AG_KeySym)sdl_ev.key.keysym.sym,
            (Uint32)sdl_ev.key.keysym.unicode);

        ag_ev->type = AG_DRIVER_KEY_UP;
        ag_ev->win = NULL;
        ag_ev->data.key.ks = (AG_KeySym)sdl_ev.key.keysym.sym;
        ag_ev->data.key.ucs = (Uint32)sdl_ev.key.keysym.unicode;
        break;
    case SDL_VIDEORESIZE:
        ag_ev->type = AG_DRIVER_VIDEORESIZE;
        ag_ev->win = NULL;
        ag_ev->data.videoresize.x = 0;
        ag_ev->data.videoresize.y = 0;
        ag_ev->data.videoresize.w = (int)sdl_ev.resize.w;
        ag_ev->data.videoresize.h = (int)sdl_ev.resize.h;
//      LOG("resizeevent")
        break;
    case SDL_VIDEOEXPOSE:
        ag_ev->type = AG_DRIVER_EXPOSE;
        ag_ev->win = NULL;
        break;
    case SDL_USEREVENT:
        if(sdl_ev.user.type == USEREVENT_REALQUIT) {
            ag_ev->type = AG_DRIVER_CLOSE;
            ag_ev->win = NULL;
        }
        break;
    case SDL_QUIT:
        /* The standard SDL_QUIT event is intercepted to allow asking
           of quit confirmation. Actual quitting is done by SDL_USEREVENT
           with type USEREVENT_REALQUIT. */
        break;
    default:
        ag_ev->type = AG_DRIVER_UNKNOWN;
        break;
    }

    return TRUE;
}

/* TDitem */
static uint32_t handleUserEvent(SDL_Event event) {
    Coordinate *move = NULL;

    switch(event.user.code) {
        case USEREVENT_JUMP:
            updatePosition(SILENT_COORDINATE_CHANGE);
            break;

        case USEREVENT_MOVE:
            move = (Coordinate *)event.user.data1;
            userMove(move->x, move->y, move->z, TELL_COORDINATE_CHANGE);
            free(move);
            break;

        case USEREVENT_REDRAW:
            drawGUI();
            break;

        case USEREVENT_NOAUTOSAVE:
            state->skeletonState->autoSaveBool = FALSE;
            break;

        case USEREVENT_REALQUIT:
            LOG("Received USEREVENT_REALQUIT. Exiting.");
            sendQuitSignal();
            return FALSE;

        default:
            LOG("Unknown SDL_USEREVENT");
    }
    return TRUE;
}

static uint32_t handleMouse(SDL_Event event) {
    /* holds the subscript for the viewPorts array of the found VP.
    -1 means that there's no VP under the mouse pointer */
    int32_t i, VPfound;

    /* this functions returns a pointer to the widget most on top
    at the mouse pointer tip - even when it's a hidden window... AG_Widget:AG_GLView*/
    AG_Widget *foundWdgt = AG_WidgetFindPoint("AG_Widget:*",
        event.button.x, event.button.y);

    /* this is necessary to guarantee intuitive behavior when moving windows */
    if(foundWdgt) {
        VPfound = checkForViewPortWdgt(foundWdgt);
        if(VPfound == -1) {
            foundWdgt = AG_WidgetFindPoint("AG_Widget:AG_Window:*",
                event.button.x, event.button.y);
            if(foundWdgt) {
                if(!AG_WindowIsVisible((AG_Window *)foundWdgt)) {
                    foundWdgt = AG_WidgetFindPoint("AG_Widget:AG_GLView:*",
                        event.button.x, event.button.y);
                    if(foundWdgt) VPfound = checkForViewPortWdgt(foundWdgt);
                }
            }
        }
    }
    else VPfound = -1;

    switch(event.type) {
        case SDL_MOUSEMOTION:
            handleMouseMotion(event, VPfound);
            break;

        case SDL_MOUSEBUTTONUP:
            /* The user release the mouse, so we have to
            stop/reset all mouse tracking features */

            for(i = 0; i < state->viewerState->numberViewPorts; i++) {
                state->viewerState->viewPorts[i].draggedNode = NULL;
                state->viewerState->viewPorts[i].motionTracking = FALSE;
                state->viewerState->viewPorts[i].VPmoves = FALSE;
                state->viewerState->viewPorts[i].VPresizes = FALSE;
                state->viewerState->viewPorts[i].userMouseSlideX = 0.;
                state->viewerState->viewPorts[i].userMouseSlideY = 0.;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            switch(event.button.button) {
                case SDL_BUTTON_LEFT:
                    /* the user did not click inside any VP, so we return */
                    if(VPfound == -1) return TRUE;
                    handleMouseButtonLeft(event, VPfound);
                    break;

                case SDL_BUTTON_MIDDLE:
                    if(VPfound == -1) return TRUE;
                    handleMouseButtonMiddle(event, VPfound);
                    break;

                case SDL_BUTTON_RIGHT:
                    /* the user did not click inside any VP, so we return */
                    if(VPfound == -1) return TRUE;
                    handleMouseButtonRight(event, VPfound);
                    break;

                case SDL_BUTTON_WHEELUP:
                    handleMouseButtonWheelForward(event, VPfound);
                    break;

                case SDL_BUTTON_WHEELDOWN:
                    handleMouseButtonWheelBackward(event, VPfound);
                    break;
            }
            break;
    }

    return TRUE;
}

static int32_t checkForViewPortWdgt(AG_Widget *wdgt) {
    int32_t VPfound;
    if(wdgt == (AG_Widget *)state->viewerState->ag->glViewXy) {
        state->viewerState->activeVP = VIEWPORT_XY;
        //printf("VIEWPORT_XY\n");
        VPfound = VIEWPORT_XY;
    }
    else if(wdgt == (AG_Widget *)state->viewerState->ag->glViewXz) {
        state->viewerState->activeVP = VIEWPORT_XZ;
        //printf("VIEWPORT_XZ\n");
        VPfound = VIEWPORT_XZ;
    }
    else if(wdgt == (AG_Widget *)state->viewerState->ag->glViewYz) {
        state->viewerState->activeVP = VIEWPORT_YZ;
        //printf("VIEWPORT_YZ\n");
        VPfound = VIEWPORT_YZ;
    }
    else if(wdgt == (AG_Widget *)state->viewerState->ag->glViewSkel) {
        /* state->viewerState->activeVP = VIEWPORT_SKELETON; */
        //printf("VIEWPORT_SKELETON\n");
        VPfound = VIEWPORT_SKELETON;
    }
    else {
        VPfound = -1;
    }
    if (VPfound == VIEWPORT_SKELETON) {
        state->viewerState->ag->zoomSkeletonViewport = TRUE;
    }
    if (VPfound != VIEWPORT_SKELETON) {
        state->viewerState->ag->zoomSkeletonViewport = FALSE;
    }
    return VPfound;
}

static uint32_t handleMouseButtonLeft(SDL_Event event, int32_t VPfound) {
    int32_t clickedNode;
    struct nodeListElement* newActiveNode;
    Coordinate *clickedCoordinate = NULL;

    /* these variables store the relative mouse position inside the VP
    maybe agar can give us those values directly?? TDitem */
    /*xDistance = event.button.x - state->viewerState->viewPorts[VPfound].lowerLeftCorner.x;
    yDistance = event.button.y - (((state->viewerState->viewPorts[VPfound].lowerLeftCorner.y
        - state->viewerState->screenSizeY) * -1)
        - state->viewerState->viewPorts[VPfound].edgeLength);
*/

    /* check if the user wants to move or resize the VP */
    /* will be handled by agar TDitem
    if(xDistance < 10 && yDistance < 10) {
        user wants to translate whole VP
        state->viewerState->viewPorts[VPfound].VPmoves = TRUE;
        return TRUE;
    }
    else if(abs(xDistance - state->viewerState->viewPorts[VPfound].edgeLength) < 10
        && abs(yDistance - state->viewerState->viewPorts[VPfound].edgeLength) < 10) {
        user wants to resize whole VP
        state->viewerState->viewPorts[VPfound].VPresizes = TRUE;
        return TRUE;
    }
    */

    //new active node selected
    if(SDL_GetModState() & KMOD_SHIFT) {
        //first assume that user managed to hit the node
        clickedNode = retrieveVisibleObjectBeneathSquare(VPfound,
                                                event.button.x,
                                                (state->viewerState->screenSizeY - event.button.y),
                                                10);
        if(clickedNode) {
            setActiveNode(CHANGE_MANUAL, NULL, clickedNode);
            return TRUE;
        }

        if(VPfound == VIEWPORT_SKELETON) {
            return FALSE;
        }

        //in other VPs we traverse nodelist to find nearest node inside the radius
            clickedCoordinate = getCoordinateFromOrthogonalClick(
                                event,
                                VPfound);
            if(clickedCoordinate) {
                newActiveNode = findNodeInRadius(*clickedCoordinate);
                if(newActiveNode != NULL) {
                    setActiveNode(CHANGE_MANUAL, NULL, newActiveNode->nodeID);
                    return TRUE;
                }
            }
        return FALSE;
    }

    /* check in which type of VP the user clicked and perform appropriate operation */
    if(state->viewerState->viewPorts[VPfound].type == VIEWPORT_SKELETON) {
        /* Activate motion tracking for this VP */
        state->viewerState->viewPorts[VPfound].motionTracking = 1;

        return TRUE;
    }
    else {
        /* Handle the orthogonal viewports */

        switch(state->viewerState->workMode) {
            case ON_CLICK_RECENTER:
                clickedCoordinate =
                    getCoordinateFromOrthogonalClick(event,
                                                     VPfound);
                if(clickedCoordinate == NULL)
                    return TRUE;

                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               clickedCoordinate->x,
                               clickedCoordinate->y,
                               clickedCoordinate->z);
                sendRemoteSignal();
                break;

            case ON_CLICK_DRAG:
                /* Activate motion tracking for this VP */
                state->viewerState->viewPorts[VPfound].motionTracking = 1;
                break;
            }
    }


    //Set or delete Connection between Active Node and Clicked Node
    if(SDL_GetModState() & KMOD_ALT) {
        int32_t clickedNode;
        clickedNode = retrieveVisibleObjectBeneathSquare(VPfound,
                                                     event.button.x,
                                                     (state->viewerState->screenSizeY - event.button.y),
                                                     1);
        if(clickedNode) {
            if(state->skeletonState->activeNode) {
                if(findSegmentByNodeIDs(state->skeletonState->activeNode->nodeID, clickedNode)) {
                    delSegment(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, clickedNode, NULL);
                }
                else if(findSegmentByNodeIDs(clickedNode, state->skeletonState->activeNode->nodeID)) {
                    delSegment(CHANGE_MANUAL, clickedNode, state->skeletonState->activeNode->nodeID, NULL);
                }
                else{
                    addSegment(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, clickedNode);
                }
            }
        }
    }
    return TRUE;
}

static uint32_t handleMouseButtonMiddle(SDL_Event event, int32_t VPfound) {
    int32_t clickedNode;

    clickedNode = retrieveVisibleObjectBeneathSquare(VPfound,
                                                     event.button.x,
                                                     (state->viewerState->screenSizeY - event.button.y),
                                                     1);

    if(clickedNode) {
        if(SDL_GetModState() & KMOD_SHIFT) {
            /* Delete or add segment between clicked and active node */
            if(state->skeletonState->activeNode) {
                if(findSegmentByNodeIDs(state->skeletonState->activeNode->nodeID,
                                        clickedNode)) {
                    delSegment(CHANGE_MANUAL,
                               state->skeletonState->activeNode->nodeID,
                               clickedNode,
                               NULL);
                }
                else if(findSegmentByNodeIDs(clickedNode,
                                             state->skeletonState->activeNode->nodeID)) {
                    delSegment(CHANGE_MANUAL,
                               clickedNode,
                               state->skeletonState->activeNode->nodeID,
                               NULL);
                }
                else{
                    addSegment(CHANGE_MANUAL,
                               state->skeletonState->activeNode->nodeID,
                               clickedNode);
                }
            }
        }
        else {
            /* No modifier pressed */
            state->viewerState->viewPorts[VPfound].draggedNode =
                findNodeByNodeID(clickedNode);
            state->viewerState->viewPorts[VPfound].motionTracking = 1;
        }
    }

    return TRUE;
}

static uint32_t handleMouseButtonRight(SDL_Event event, int32_t VPfound) {
    int32_t newNodeID;
    Coordinate *clickedCoordinate = NULL, movement, lastPos;

    /* We have to activate motion tracking only for the skeleton VP for a right click */
    if(state->viewerState->viewPorts[VPfound].type == VIEWPORT_SKELETON)
        state->viewerState->viewPorts[VPfound].motionTracking = TRUE;

    /* If not, we look up which skeletonizer work mode is
    active and do the appropriate operation */
        clickedCoordinate =
            getCoordinateFromOrthogonalClick(event, VPfound);

        /* could not find any coordinate... */
        if(clickedCoordinate == NULL)
            return TRUE;

        switch(state->skeletonState->workMode) {
            case SKELETONIZER_ON_CLICK_DROP_NODE:
                /* function is inside skeletonizer.c */
                UI_addSkeletonNode(clickedCoordinate,
                                   state->viewerState->viewPorts[VPfound].type);
                break;
            case SKELETONIZER_ON_CLICK_ADD_NODE:
                /* function is inside skeletonizer.c */
                UI_addSkeletonNode(clickedCoordinate,
                                   state->viewerState->viewPorts[VPfound].type);
                tempConfig->skeletonState->workMode =
                    SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE;
                break;
            case SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE:
                /* function is inside skeletonizer.c */
                if(state->skeletonState->activeNode) {
                    if(SDL_GetModState() & KMOD_CTRL) {
                        /* Add a "stump", a branch node to which we don't automatically move. */
                        if((newNodeID = UI_addSkeletonNodeAndLinkWithActive(clickedCoordinate,
                                                                            state->viewerState->viewPorts[VPfound].type,
                                                                            FALSE))) {
                            pushBranchNode(CHANGE_MANUAL,
                                           TRUE,
                                           TRUE,
                                           NULL,
                                           newNodeID);
                        }
                    }
                    else {
                        lastPos.x = state->skeletonState->activeNode->position.x;
                        lastPos.y = state->skeletonState->activeNode->position.y;
                        lastPos.z = state->skeletonState->activeNode->position.z;

                        if(UI_addSkeletonNodeAndLinkWithActive(clickedCoordinate,
                                                               state->viewerState->viewPorts[VPfound].type,
                                                               TRUE)) {


                            /* Highlight the viewport with the biggest movement component and set
                               behavior of f / d keys accordingly. */
                            movement.x = clickedCoordinate->x - lastPos.x;
                            movement.y = clickedCoordinate->y - lastPos.y;
                            movement.z = clickedCoordinate->z - lastPos.z;


                            if (state->viewerState->autoTracingDelay < 10) state->viewerState->autoTracingDelay = 10;
                            if (state->viewerState->autoTracingDelay > 500) state->viewerState->autoTracingDelay = 500;
                                if (state->viewerState->autoTracingSteps < 1) state->viewerState->autoTracingSteps = 1;
                            if (state->viewerState->autoTracingSteps > 50) state->viewerState->autoTracingSteps = 50;

                            //AutoTracingModes 2 and 3
                            if ((state->viewerState->autoTracingMode == AUTOTRACING_TRACING) || (state->viewerState->autoTracingMode == AUTOTRACING_MIRROR))
                            {
                                floatCoordinate walkingVector;

                                walkingVector.x = movement.x;
                                walkingVector.y = movement.y;
                                walkingVector.z = movement.z;

                                if (state->viewerState->autoTracingMode == AUTOTRACING_TRACING){
                                    clickedCoordinate->x += roundFloat(walkingVector.x * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
                                    clickedCoordinate->y += roundFloat(walkingVector.y * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
                                    clickedCoordinate->z += roundFloat(walkingVector.z * state->viewerState->autoTracingSteps / euclidicNorm(&walkingVector));
                                }
                                if (state->viewerState->autoTracingMode == AUTOTRACING_MIRROR){
                                clickedCoordinate->x += walkingVector.x;
                                clickedCoordinate->y += walkingVector.y;
                                clickedCoordinate->z += walkingVector.z;
                                }
                            }

                            /* Determine which viewport to highlight based on which movement component is biggest. */
                            if((abs(movement.x) >= abs(movement.y)) && (abs(movement.x) >= abs(movement.z))) {
                                state->viewerState->highlightVp = VIEWPORT_YZ;
                            }
                            if((abs(movement.y) >= abs(movement.x)) && (abs(movement.y) >= abs(movement.z))) {
                                state->viewerState->highlightVp = VIEWPORT_XZ;
                            }
                            if((abs(movement.z) >= abs(movement.x)) && (abs(movement.z) >= abs(movement.y))) {
                                state->viewerState->highlightVp = VIEWPORT_XY;
                            }

                            /* Determine the directions for the f and d keys based on the signs of the movement
                               components along the three dimensions. */
                            if(movement.x >= 0) {
                                state->viewerState->vpKeyDirection[VIEWPORT_YZ] = 1;
                            }
                            else {
                                state->viewerState->vpKeyDirection[VIEWPORT_YZ] = -1;
                            }
                            if(movement.y >= 0) {
                                state->viewerState->vpKeyDirection[VIEWPORT_XZ] = 1;
                            }
                            else {
                                state->viewerState->vpKeyDirection[VIEWPORT_XZ] = -1;
                            }
                            if(movement.z >= 0) {
                                state->viewerState->vpKeyDirection[VIEWPORT_XY] = 1;
                            }
                            else {
                                state->viewerState->vpKeyDirection[VIEWPORT_XY] = -1;
                            }

                            //AutoTracingMode 1
                            if (state->viewerState->autoTracingMode == AUTOTRACING_VIEWPORT){
                                if (state->viewerState->viewPorts[VPfound].type == VIEWPORT_XY){
                                    if (state->viewerState->vpKeyDirection[VIEWPORT_XY] == 1){
                                        clickedCoordinate->z += state->viewerState->autoTracingSteps;
                                    }
                                    else if (state->viewerState->vpKeyDirection[VIEWPORT_XY] == -1){
                                        clickedCoordinate->z -= state->viewerState->autoTracingSteps;
                                    }
                                }
                                if (state->viewerState->viewPorts[VPfound].type == VIEWPORT_XZ){
                                    if (state->viewerState->vpKeyDirection[VIEWPORT_XZ] == 1){
                                        clickedCoordinate->y += state->viewerState->autoTracingSteps;
                                    }
                                    else if (state->viewerState->vpKeyDirection[VIEWPORT_XZ] == -1){
                                        clickedCoordinate->y -= state->viewerState->autoTracingSteps;
                                    }
                                }
                                else if (state->viewerState->viewPorts[VPfound].type == VIEWPORT_YZ){
                                    if (state->viewerState->vpKeyDirection[VIEWPORT_YZ] == 1){
                                        clickedCoordinate->x += state->viewerState->autoTracingSteps;
                                    }
                                    else if (state->viewerState->vpKeyDirection[VIEWPORT_YZ] == -1){
                                        clickedCoordinate->x -= state->viewerState->autoTracingSteps;
                                    }
                                }
                            }

                            if (clickedCoordinate->x < 0) clickedCoordinate->x = 0;
                            if (clickedCoordinate->y < 0) clickedCoordinate->y = 0;
                            if (clickedCoordinate->z < 0) clickedCoordinate->z = 0;

                            /* Do not allow clicks outside the dataset */
                            if (clickedCoordinate->x
                                > state->boundary.x)
                                clickedCoordinate->x = state->boundary.x;
                            if (clickedCoordinate->y
                                > state->boundary.y)
                                clickedCoordinate->y = state->boundary.y;
                            if (clickedCoordinate->z
                                > state->boundary.z)
                                clickedCoordinate->z = state->boundary.z;

                            /* Move to the new node position */
                            tempConfig->remoteState->type = REMOTE_RECENTERING;
                            SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                                           clickedCoordinate->x,
                                           clickedCoordinate->y,
                                           clickedCoordinate->z);
                            updateViewerState();
                            sendRemoteSignal();
                        }
                    }
                }
                else {
                    UI_addSkeletonNode(clickedCoordinate,
                                       state->viewerState->viewPorts[VPfound].type);
                }

                break;
        }

    return TRUE;
}

static uint32_t handleMouseButtonWheelForward(SDL_Event event, int32_t VPfound) {
    float radius;

    if(VPfound == -1)
        return TRUE;

    if((state->skeletonState->activeNode) && ((SDL_GetModState() & KMOD_SHIFT))) {
        radius = state->skeletonState->activeNode->radius - 0.2 * state->skeletonState->activeNode->radius;

        editNode(CHANGE_MANUAL,
                 0,
                 state->skeletonState->activeNode,
                 radius,
                 state->skeletonState->activeNode->position.x,
                 state->skeletonState->activeNode->position.y,
                 state->skeletonState->activeNode->position.z,
                 state->magnification);

        if(state->viewerState->ag->useLastActNodeRadiusAsDefault)
            state->skeletonState->defaultNodeRadius = radius;

        //drawGUI();
    }
    else {
        /* Skeleton VP */
        if(state->viewerState->viewPorts[VPfound].type == VIEWPORT_SKELETON) {

            if (state->skeletonState->zoomLevel <= SKELZOOMMAX){
                state->skeletonState->zoomLevel += (0.1 * (0.5 - state->skeletonState->zoomLevel));
                state->skeletonState->viewChanged = TRUE;
            }
        }
        /* Orthogonal VP or outside VP */
        else {
            /* Zoom when CTRL is pressed */
            if(SDL_GetModState() & KMOD_CTRL) {
                UI_zoomOrthogonals(-0.1);
            }
            /* Move otherwise */
            else {
                switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                    case VIEWPORT_XY:
                        userMove(0, 0, state->viewerState->dropFrames
                            * state->magnification,
                            TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_XZ:
                        userMove(0, state->viewerState->dropFrames
                            * state->magnification, 0,
                            TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_YZ:
                        userMove(state->viewerState->dropFrames
                            * state->magnification, 0, 0,
                            TELL_COORDINATE_CHANGE);
                        break;
                }
            }
        }
    }

    return TRUE;
}

static uint32_t handleMouseButtonWheelBackward(SDL_Event event, int32_t VPfound) {
    float radius;

    if(VPfound == -1)
        return TRUE;

    if((state->skeletonState->activeNode) && ((SDL_GetModState() & KMOD_SHIFT))) {
        radius = state->skeletonState->activeNode->radius + 0.2 * state->skeletonState->activeNode->radius;

        editNode(CHANGE_MANUAL,
                 0,
                 state->skeletonState->activeNode,
                 radius,
                 state->skeletonState->activeNode->position.x,
                 state->skeletonState->activeNode->position.y,
                 state->skeletonState->activeNode->position.z,
                 state->magnification);

        if(state->viewerState->ag->useLastActNodeRadiusAsDefault)
            state->skeletonState->defaultNodeRadius = radius;

        //drawGUI();
    }
    else {
        /* Skeleton VP */
        if(state->viewerState->viewPorts[VPfound].type == VIEWPORT_SKELETON) {

            if (state->skeletonState->zoomLevel >= SKELZOOMMIN) {
                state->skeletonState->zoomLevel -= (0.2* (0.5 - state->skeletonState->zoomLevel));
                if (state->skeletonState->zoomLevel < SKELZOOMMIN) state->skeletonState->zoomLevel = SKELZOOMMIN;
                state->skeletonState->viewChanged = TRUE;
            }
        }
        /* Orthogonal VP or outside VP */
        else {
            /* Zoom when CTRL is pressed */
            if(SDL_GetModState() & KMOD_CTRL) {
                UI_zoomOrthogonals(0.1);
            }
            /* Move otherwise */
            else {
                switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                    case VIEWPORT_XY:
                        userMove(0, 0, -state->viewerState->dropFrames
                            * state->magnification,
                            TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_XZ:
                        userMove(0, -state->viewerState->dropFrames
                            * state->magnification, 0,
                            TELL_COORDINATE_CHANGE);
                        break;
                    case VIEWPORT_YZ:
                        userMove(-state->viewerState->dropFrames
                            * state->magnification, 0, 0,
                            TELL_COORDINATE_CHANGE);
                        break;
                }
            }
        }
    }

    return TRUE;
}

static uint32_t handleMouseMotion(SDL_Event event, int32_t VPfound) {

    /* mouse is moving with left button hold clicked */
    if(event.motion.state & SDL_BUTTON(1)) {
        handleMouseMotionLeftHold(event, VPfound);
    }
    else if(event.motion.state & SDL_BUTTON(2)) {
        handleMouseMotionMiddleHold(event, VPfound);
    }
    /* mouse is moving with right button hold clicked */
    else if(event.motion.state & SDL_BUTTON(3)) {
        handleMouseMotionRightHold(event, VPfound);
    }

    return TRUE;
}

static uint32_t handleMouseMotionLeftHold(SDL_Event event, int32_t VPfound) {
    int32_t i;

    for(i = 0; i < state->viewerState->numberViewPorts; i++) {
        /* TDitem. Will be handled by Agar. Take care of "lowerLeftCorner" stuff -> used to determine the
        clicked coordinate in data pixels!
        if(state->viewerState->viewPorts[i].VPmoves == TRUE) {
            if(!(state->viewerState->viewPorts[i].lowerLeftCorner.x + event.motion.xrel < 0)) state->viewerState->viewPorts[i].lowerLeftCorner.x += event.motion.xrel;
            if(!(state->viewerState->viewPorts[i].lowerLeftCorner.y + state->viewerState->viewPorts[i].edgeLength - event.motion.yrel > (state->viewerState->screenSizeY - 15))) state->viewerState->viewPorts[i].lowerLeftCorner.y += -event.motion.yrel;
            return TRUE;
        }
        else if(state->viewerState->viewPorts[i].VPresizes == TRUE) {
            if(!(state->viewerState->viewPorts[i].lowerLeftCorner.y + state->viewerState->viewPorts[i].edgeLength + (event.motion.xrel + event.motion.yrel) > (state->viewerState->screenSizeY - 15)) && (!(state->viewerState->viewPorts[i].edgeLength + event.motion.xrel + event.motion.yrel < 50))) state->viewerState->viewPorts[i].edgeLength += (event.motion.xrel + event.motion.yrel);
            return TRUE;
        }
        */

        /* motion tracking mode is active for viewPort i */
        if(state->viewerState->viewPorts[i].motionTracking == TRUE) {
            switch(state->viewerState->viewPorts[i].type) {
                /* the user wants to drag the skeleton inside the VP */
                case VIEWPORT_SKELETON:
                    state->skeletonState->translateX += -event.motion.xrel * 2.
                        * ((float)state->skeletonState->volBoundary
                        * (0.5 - state->skeletonState->zoomLevel))
                        / ((float)state->viewerState->viewPorts[i].edgeLength);
                    state->skeletonState->translateY += -event.motion.yrel * 2.
                        * ((float)state->skeletonState->volBoundary
                        * (0.5 - state->skeletonState->zoomLevel))
                        / ((float)state->viewerState->viewPorts[i].edgeLength);
                        checkIdleTime();
                    break;
                case VIEWPORT_XY:
                    if(state->viewerState->workMode != ON_CLICK_DRAG) break;
                    state->viewerState->viewPorts[i].userMouseSlideX -=
                        ((float)event.motion.xrel
                        / state->viewerState->viewPorts[i].screenPxXPerDataPx);
                    state->viewerState->viewPorts[i].userMouseSlideY -=
                        ((float)event.motion.yrel
                        / state->viewerState->viewPorts[i].screenPxYPerDataPx);
                    if(fabs(state->viewerState->viewPorts[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->viewPorts[i].userMouseSlideY) >= 1) {

                        userMove((int)state->viewerState->viewPorts[i].userMouseSlideX,
                            (int)state->viewerState->viewPorts[i].userMouseSlideY, 0,
                            TELL_COORDINATE_CHANGE);
                        state->viewerState->viewPorts[i].userMouseSlideX = 0.;
                        state->viewerState->viewPorts[i].userMouseSlideY = 0.;
                    }
                    break;
                case VIEWPORT_XZ:
                    if(state->viewerState->workMode != ON_CLICK_DRAG) break;
                    state->viewerState->viewPorts[i].userMouseSlideX -=
                        ((float)event.motion.xrel / state->viewerState->viewPorts[i].screenPxXPerDataPx);
                    state->viewerState->viewPorts[i].userMouseSlideY -=
                        ((float)event.motion.yrel / state->viewerState->viewPorts[i].screenPxYPerDataPx);
                    if(fabs(state->viewerState->viewPorts[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->viewPorts[i].userMouseSlideY) >= 1) {

                        userMove((int)state->viewerState->viewPorts[i].userMouseSlideX, 0,
                            (int)state->viewerState->viewPorts[i].userMouseSlideY,
                            TELL_COORDINATE_CHANGE);
                        state->viewerState->viewPorts[i].userMouseSlideX = 0.;
                        state->viewerState->viewPorts[i].userMouseSlideY = 0.;
                    }
                    break;
                case VIEWPORT_YZ:
                    if(state->viewerState->workMode != ON_CLICK_DRAG) break;
                    state->viewerState->viewPorts[i].userMouseSlideX -=
                        ((float)event.motion.xrel / state->viewerState->viewPorts[i].screenPxXPerDataPx);
                    state->viewerState->viewPorts[i].userMouseSlideY -=
                        ((float)event.motion.yrel / state->viewerState->viewPorts[i].screenPxYPerDataPx);
                    if(fabs(state->viewerState->viewPorts[i].userMouseSlideX) >= 1
                        || fabs(state->viewerState->viewPorts[i].userMouseSlideY) >= 1) {

                        userMove(0, (int)state->viewerState->viewPorts[i].userMouseSlideY,
                            (int)state->viewerState->viewPorts[i].userMouseSlideX,
                            TELL_COORDINATE_CHANGE);
                        state->viewerState->viewPorts[i].userMouseSlideX = 0.;
                        state->viewerState->viewPorts[i].userMouseSlideY = 0.;
                    }
                    break;
            }
            return TRUE;
        }
    }

    return TRUE;
}

static uint32_t handleMouseMotionRightHold(SDL_Event event, int32_t VPfound) {
    if(state->viewerState->viewPorts[VIEWPORT_SKELETON].motionTracking == TRUE) {
        if(fabs(event.motion.xrel)  >= fabs(event.motion.yrel))
            state->skeletonState->rotateZ += event.motion.xrel;
        else state->skeletonState->rotateX += event.motion.yrel;
            state->skeletonState->viewChanged = TRUE;
        checkIdleTime();
        return TRUE;
    }

    return TRUE;
}

static uint32_t handleMouseMotionMiddleHold(SDL_Event event, int32_t VPfound) {
    int32_t i = 0;
    Coordinate newDraggedNodePos;

    for(i = 0; i < state->viewerState->numberViewPorts; i++) {
        switch(state->viewerState->viewPorts[i].type) {
            case VIEWPORT_XY:
                if(!state->viewerState->viewPorts[i].draggedNode) break;
                state->viewerState->viewPorts[i].userMouseSlideX -=
                    ((float)event.motion.xrel
                    / state->viewerState->viewPorts[i].screenPxXPerDataPx);

                state->viewerState->viewPorts[i].userMouseSlideY -=
                    ((float)event.motion.yrel
                    / state->viewerState->viewPorts[i].screenPxYPerDataPx);
                if(fabs(state->viewerState->viewPorts[i].userMouseSlideX) >= 1
                    || fabs(state->viewerState->viewPorts[i].userMouseSlideY) >= 1) {

                    SET_COORDINATE(newDraggedNodePos,
                        state->viewerState->viewPorts[i].userMouseSlideX,
                        state->viewerState->viewPorts[i].userMouseSlideY, 0);
                    state->viewerState->viewPorts[i].userMouseSlideX = 0.;
                    state->viewerState->viewPorts[i].userMouseSlideY = 0.;
                    newDraggedNodePos.x =
                        state->viewerState->viewPorts[i].draggedNode->position.x
                        - newDraggedNodePos.x;
                    newDraggedNodePos.y =
                        state->viewerState->viewPorts[i].draggedNode->position.y
                        - newDraggedNodePos.y;
                    newDraggedNodePos.z =
                        state->viewerState->viewPorts[i].draggedNode->position.z
                            - newDraggedNodePos.z;

                    editNode(CHANGE_MANUAL,
                             0,
                             state->viewerState->viewPorts[i].draggedNode,
                             0.,
                             newDraggedNodePos.x,
                             newDraggedNodePos.y,
                             newDraggedNodePos.z,
                             state->magnification);
                }
                break;
            case VIEWPORT_XZ:
                if(!state->viewerState->viewPorts[i].draggedNode) break;
                state->viewerState->viewPorts[i].userMouseSlideX -=
                    ((float)event.motion.xrel
                    / state->viewerState->viewPorts[i].screenPxXPerDataPx);
                state->viewerState->viewPorts[i].userMouseSlideY -=
                    ((float)event.motion.yrel
                    / state->viewerState->viewPorts[i].screenPxYPerDataPx);
                if(fabs(state->viewerState->viewPorts[i].userMouseSlideX) >= 1
                    || fabs(state->viewerState->viewPorts[i].userMouseSlideY) >= 1) {

                    SET_COORDINATE(newDraggedNodePos,
                        state->viewerState->viewPorts[i].userMouseSlideX, 0,
                        state->viewerState->viewPorts[i].userMouseSlideY);
                    state->viewerState->viewPorts[i].userMouseSlideX = 0.;
                    state->viewerState->viewPorts[i].userMouseSlideY = 0.;
                    newDraggedNodePos.x =
                        state->viewerState->viewPorts[i].draggedNode->position.x
                        - newDraggedNodePos.x;
                    newDraggedNodePos.y =
                        state->viewerState->viewPorts[i].draggedNode->position.y
                        - newDraggedNodePos.y;
                    newDraggedNodePos.z =
                        state->viewerState->viewPorts[i].draggedNode->position.z
                        - newDraggedNodePos.z;
                    editNode(CHANGE_MANUAL,
                             0,
                             state->viewerState->viewPorts[i].draggedNode,
                             0.,
                             newDraggedNodePos.x,
                             newDraggedNodePos.y,
                             newDraggedNodePos.z,
                             state->magnification);
                }
                break;
            case VIEWPORT_YZ:
                if(!state->viewerState->viewPorts[i].draggedNode) break;
                state->viewerState->viewPorts[i].userMouseSlideX -=
                    ((float)event.motion.xrel
                    / state->viewerState->viewPorts[i].screenPxXPerDataPx);
                state->viewerState->viewPorts[i].userMouseSlideY -=
                    ((float)event.motion.yrel
                    / state->viewerState->viewPorts[i].screenPxYPerDataPx);
                if(fabs(state->viewerState->viewPorts[i].userMouseSlideX) >= 1
                    || fabs(state->viewerState->viewPorts[i].userMouseSlideY) >= 1) {
                    SET_COORDINATE(newDraggedNodePos,
                        0, state->viewerState->viewPorts[i].userMouseSlideY,
                        state->viewerState->viewPorts[i].userMouseSlideX);

                    state->viewerState->viewPorts[i].userMouseSlideX = 0.;
                    state->viewerState->viewPorts[i].userMouseSlideY = 0.;
                    newDraggedNodePos.x =
                        state->viewerState->viewPorts[i].draggedNode->position.x
                        - newDraggedNodePos.x;
                    newDraggedNodePos.y =
                        state->viewerState->viewPorts[i].draggedNode->position.y
                        - newDraggedNodePos.y;
                    newDraggedNodePos.z =
                        state->viewerState->viewPorts[i].draggedNode->position.z
                        - newDraggedNodePos.z;
                    editNode(CHANGE_MANUAL,
                             0,
                             state->viewerState->viewPorts[i].draggedNode,
                             0.,
                             newDraggedNodePos.x,
                             newDraggedNodePos.y,
                             newDraggedNodePos.z,
                             state->magnification);
                }
                break;
        }
    }

    return TRUE;
}


static uint32_t handleKeyboard(SDL_Event event) {
    /* This is a workaround for agars behavior of processing / labeling
    events. Without it, input into a textbox would still trigger global
    shortcut actions.*/
    if(state->viewerState->ag->agInputWdgtFocused) {
        return TRUE;
    }

    struct treeListElement *prevTree;
    struct treeListElement *nextTree;
    struct nodeListElement *prevNode;
    struct nodeListElement *nextNode;
    color4F treeCol;

    switch(event.key.keysym.sym) {

    //cursor key ... pressed. behaviour of an enduring key press can be
    //specified further with
    //int SDL_EnableKeyRepeat  (int delay, int interval);
    case SDLK_LEFT:
        if(SDL_GetModState() & KMOD_SHIFT) {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(-10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(-10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(0, 0, -10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        else {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(-state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(-state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(0, 0, -state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
            }
        }

        break;
    case SDLK_RIGHT:
        if(SDL_GetModState() & KMOD_SHIFT) {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(0, 0, 10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        else {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(0, 0, state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        break;
    case SDLK_DOWN:
        if(SDL_GetModState() & KMOD_SHIFT) {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(0, -10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(0, 0, -10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(0, -10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        else {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(0, -state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(0, 0, -state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(0, -state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        break;
    case SDLK_UP:
        if(SDL_GetModState() & KMOD_SHIFT) {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(0, 10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(0, 0, 10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(0, 10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        else {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(0, state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(0, 0, state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(0, state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        break;
    case SDLK_r:
        state->viewerState->walkOrth = 1;
        switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
            case VIEWPORT_XY:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z += 10  * state->magnification);
                               sendRemoteSignal();
            break;
            case VIEWPORT_XZ:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y += 10 * state->magnification,
                               tempConfig->viewerState->currentPosition.z);
                               sendRemoteSignal();
            break;
            case VIEWPORT_YZ:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x += 10 * state->magnification,
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z);
                               sendRemoteSignal();
            break;
        }
        break;
    case SDLK_e:
        state->viewerState->walkOrth = 1;
        switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
            case VIEWPORT_XY:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z -= 10 * state->magnification);
                               sendRemoteSignal();
            break;
            case VIEWPORT_XZ:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x,
                               tempConfig->viewerState->currentPosition.y -= 10 * state->magnification,
                               tempConfig->viewerState->currentPosition.z);
                               sendRemoteSignal();
            break;
            case VIEWPORT_YZ:
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               tempConfig->viewerState->currentPosition.x -= 10 * state->magnification,
                               tempConfig->viewerState->currentPosition.y,
                               tempConfig->viewerState->currentPosition.z);
                               sendRemoteSignal();
            break;
        }
    break;
    case SDLK_f:
        if(SDL_GetModState() & KMOD_SHIFT) {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * 10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * 10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * 10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        else {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        break;
    case SDLK_d:
        if(SDL_GetModState() & KMOD_SHIFT) {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * -10 * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * -10 * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * -10 * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        else {
            switch(state->viewerState->viewPorts[state->viewerState->activeVP].type) {
                case VIEWPORT_XY:
                    userMove(0, 0, state->viewerState->vpKeyDirection[VIEWPORT_XY] * -state->viewerState->dropFrames * state->magnification, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_XZ:
                    userMove(0, state->viewerState->vpKeyDirection[VIEWPORT_XZ] * -state->viewerState->dropFrames * state->magnification, 0, TELL_COORDINATE_CHANGE);
                    break;
                case VIEWPORT_YZ:
                    userMove(state->viewerState->vpKeyDirection[VIEWPORT_YZ] * -state->viewerState->dropFrames * state->magnification, 0, 0, TELL_COORDINATE_CHANGE);
                    break;
            }
        }
        break;
    case SDLK_g:
        //For testing issues
        //genTestNodes(500000);
        break;
    case SDLK_n:
        if(SDL_GetModState() & KMOD_SHIFT) {
            nextCommentlessNode();
        }
        else {
            nextComment(state->viewerState->ag->commentSearchBuffer);
        }
        break;
    case SDLK_p:
        if(SDL_GetModState() & KMOD_SHIFT) {
            previousCommentlessNode();
        }
        else {
            previousComment(state->viewerState->ag->commentSearchBuffer);
        }
        break;
    case SDLK_3:
        if(state->viewerState->drawVPCrosshairs) {
           state->viewerState->drawVPCrosshairs = FALSE;
        }
        else {
           state->viewerState->drawVPCrosshairs = TRUE;
        }
        break;
    case SDLK_j:
        UI_popBranchNode();
        break;
    case SDLK_b:
        pushBranchNode(CHANGE_MANUAL, TRUE, TRUE, state->skeletonState->activeNode, 0);
        break;
    case SDLK_x:
        if(state->skeletonState->activeNode == NULL) {
                break;
        }
        //Shift + x = previous node by ID
        if(SDL_GetModState() & KMOD_SHIFT) {
            prevNode = getNodeWithPrevID(state->skeletonState->activeNode);
            if(prevNode) {
                setActiveNode(CHANGE_MANUAL, prevNode, prevNode->nodeID);
                tempConfig->remoteState->type = REMOTE_RECENTERING;
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               prevNode->position.x,
                               prevNode->position.y,
                               prevNode->position.z);
                sendRemoteSignal();
            }
            else {
                LOG("Reached first node.");
            }
            break;
        }
        // x = next node by ID
        nextNode = getNodeWithNextID(state->skeletonState->activeNode);
        if(nextNode) {
            setActiveNode(CHANGE_MANUAL, nextNode, nextNode->nodeID);
            tempConfig->remoteState->type = REMOTE_RECENTERING;
            SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               nextNode->position.x,
                               nextNode->position.y,
                               nextNode->position.z);
            sendRemoteSignal();
        }
        else {
            LOG("Reached last node.");
        }
        break;
    case SDLK_z: //change active tree
        if(state->skeletonState->activeTree == NULL) {
            break;
        }
        //get tree with previous ID
        if(SDL_GetModState() & KMOD_SHIFT) {
            prevTree = getTreeWithPrevID(state->skeletonState->activeTree);
            if(prevTree) {
                if(setActiveTreeByID(prevTree->treeID)) {
                    setActiveNode(CHANGE_MANUAL, prevTree->firstNode, prevTree->firstNode->nodeID);
                    SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               prevTree->firstNode->position.x,
                               prevTree->firstNode->position.y,
                               prevTree->firstNode->position.z);
                    sendRemoteSignal();
                }
            }
            else {
                LOG("Reached first tree.");
            }
            break;
        }
        //get tree with next ID
        nextTree = getTreeWithNextID(state->skeletonState->activeTree);
        if(nextTree) {
            if(setActiveTreeByID(nextTree->treeID)) {
                setActiveNode(CHANGE_MANUAL, nextTree->firstNode, nextTree->firstNode->nodeID);
                SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                               nextTree->firstNode->position.x,
                               nextTree->firstNode->position.y,
                               nextTree->firstNode->position.z);
                sendRemoteSignal();
            }
        }
        else {
            LOG("Reached last tree.");
        }
        break;

    case SDLK_i:
        if (state->viewerState->ag->zoomSkeletonViewport == FALSE){
            UI_zoomOrthogonals(-0.1);
        }
        else if (state->skeletonState->zoomLevel <= SKELZOOMMAX){
            state->skeletonState->zoomLevel += (0.1 * (0.5 - state->skeletonState->zoomLevel));
            state->skeletonState->viewChanged = TRUE;
        }
        break;
    case SDLK_o:
        if (state->viewerState->ag->zoomSkeletonViewport == FALSE){
            UI_zoomOrthogonals(0.1);
        }
        else if (state->skeletonState->zoomLevel >= SKELZOOMMIN) {
            state->skeletonState->zoomLevel -= (0.2* (0.5 - state->skeletonState->zoomLevel));
            if (state->skeletonState->zoomLevel < SKELZOOMMIN) state->skeletonState->zoomLevel = SKELZOOMMIN;
            state->skeletonState->viewChanged = TRUE;
        }

        break;
    case SDLK_s:
        if(SDL_GetModState() & KMOD_CTRL) {
            saveSkelCallback(NULL);
            break;
        }
        if(state->skeletonState->activeNode) {
            tempConfig->viewerState->currentPosition.x =
                state->skeletonState->activeNode->position.x;
            tempConfig->viewerState->currentPosition.y =
                state->skeletonState->activeNode->position.y;
            tempConfig->viewerState->currentPosition.z =
                state->skeletonState->activeNode->position.z;
            updatePosition(TELL_COORDINATE_CHANGE);
        }
        break;
    case SDLK_a:
        UI_workModeAdd();
        break;
    case SDLK_w:
        UI_workModeLink();
        break;
    case SDLK_c:
        treeCol.r = -1.;
        addTreeListElement(TRUE, CHANGE_MANUAL, 0, treeCol);
        tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
        break;
    case SDLK_v:
        if(SDL_GetModState() & KMOD_CTRL) {
            UI_pasteClipboardCoordinates();
        }
        break;
    /* toggle skeleton overlay on/off */
    case SDLK_1:
        if(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE) {
            state->skeletonState->displayMode &= ~DSP_SLICE_VP_HIDE;
            state->viewerState->ag->enableOrthoSkelOverlay = 1;
        }
        else {
            state->skeletonState->displayMode |= DSP_SLICE_VP_HIDE;
            state->viewerState->ag->enableOrthoSkelOverlay = 0;
        }
        state->skeletonState->skeletonChanged = TRUE;
        drawGUI();
        break;

/*
    case SDLK_2:
        state->skeletonState->skeletonDisplayMode = 1;
        state->skeletonState->skeletonChanged = TRUE;
        drawGUI();
        break;
    case SDLK_3:
        state->skeletonState->skeletonDisplayMode = 2;
        state->skeletonState->skeletonChanged = TRUE;
        drawGUI();
        break;
    case SDLK_4:
        state->skeletonState->skeletonDisplayMode = 3;
        state->skeletonState->skeletonChanged = TRUE;
        drawGUI();
        break;
    case SDLK_5:
        state->skeletonState->skeletonDisplayMode = 4;
        state->skeletonState->skeletonChanged = TRUE;
        drawGUI();
        break;
    case SDLK_6:
        state->skeletonState->skeletonDisplayMode = 5;
        state->skeletonState->skeletonChanged = TRUE;
        drawGUI();
        break;
    case SDLK_7:
        state->skeletonState->skeletonDisplayMode = 6;
        state->skeletonState->skeletonChanged = TRUE;
        drawGUI();
        break;
*/
    case SDLK_DELETE:
        delActiveNode();
        break;

    case SDLK_F1:
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->ag->comment1, "", 1) != 0)){
            addComment(CHANGE_MANUAL, state->viewerState->ag->comment1, state->skeletonState->activeNode, 0);
        }
        else{
            editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->ag->comment1, state->skeletonState->activeNode, 0);
        }
        break;

    case SDLK_F2:
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->ag->comment2, "", 1) != 0)){
            addComment(CHANGE_MANUAL, state->viewerState->ag->comment2, state->skeletonState->activeNode, 0);
        }
        else{
            editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->ag->comment2, state->skeletonState->activeNode, 0);
        }
        break;

    case SDLK_F3:
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->ag->comment3, "", 1) != 0)){
            addComment(CHANGE_MANUAL, state->viewerState->ag->comment3, state->skeletonState->activeNode, 0);
        }
        else{
            editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->ag->comment3, state->skeletonState->activeNode, 0);
        }
        break;

    case SDLK_F4:
        if(SDL_GetModState() & KMOD_ALT) {
            if(state->skeletonState->unsavedChanges) {
                yesNoPrompt(NULL, "There are unsaved changes. Really quit?", quitKnossos, NULL);
            }
            else {
                quitKnossos();
            }
            break;
        }

        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->ag->comment4, "", 1) != 0)){
            addComment(CHANGE_MANUAL, state->viewerState->ag->comment4, state->skeletonState->activeNode, 0);
        }
        else{
            editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->ag->comment4, state->skeletonState->activeNode, 0);
        }
        break;

     case SDLK_F5:
        if((!state->skeletonState->activeNode->comment) && (strncmp(state->viewerState->ag->comment5, "", 1) != 0)){
            addComment(CHANGE_MANUAL, state->viewerState->ag->comment5, state->skeletonState->activeNode, 0);
        }
        else{
            editComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, state->viewerState->ag->comment5, state->skeletonState->activeNode, 0);
        }
        break;
    default:
        ;
        //printf("Unknown keypress: %d\n", event.key.keysym.sym);
    }
    return TRUE;
}

static Coordinate *getCoordinateFromOrthogonalClick(SDL_Event event, int32_t VPfound) {
    Coordinate *foundCoordinate;
    foundCoordinate = malloc(sizeof(Coordinate));
    uint32_t x, y, z;
    x = y = z = 0;

    /* These variables store the distance in screen pixels from the left and
    upper border from the user mouse click to the VP boundaries. */
    uint32_t xDistance, yDistance;

    if((VPfound == -1)
        || (state->viewerState->viewPorts[VPfound].type == VIEWPORT_SKELETON))
            return NULL;

    xDistance = event.button.x
        - state->viewerState->viewPorts[VPfound].upperLeftCorner.x;
    yDistance = event.button.y
        - state->viewerState->viewPorts[VPfound].upperLeftCorner.y;

    switch(state->viewerState->viewPorts[VPfound].type) {
        case VIEWPORT_XY:
            x = state->viewerState->viewPorts[VPfound].leftUpperDataPxOnScreen.x
                + ((int)(((float)xDistance)
                / state->viewerState->viewPorts[VPfound].screenPxXPerDataPx));
            y = state->viewerState->viewPorts[VPfound].leftUpperDataPxOnScreen.y
                + ((int)(((float)yDistance)
                / state->viewerState->viewPorts[VPfound].screenPxYPerDataPx));
            z = state->viewerState->currentPosition.z;
            break;
        case VIEWPORT_XZ:
            x = state->viewerState->viewPorts[VPfound].leftUpperDataPxOnScreen.x
                + ((int)(((float)xDistance)
                / state->viewerState->viewPorts[VPfound].screenPxXPerDataPx));
            z = state->viewerState->viewPorts[VPfound].leftUpperDataPxOnScreen.z
                + ((int)(((float)yDistance)
                / state->viewerState->viewPorts[VPfound].screenPxYPerDataPx));
            y = state->viewerState->currentPosition.y;
            break;
        case VIEWPORT_YZ:
            z = state->viewerState->viewPorts[VPfound].leftUpperDataPxOnScreen.z
                + ((int)(((float)xDistance)
                / state->viewerState->viewPorts[VPfound].screenPxXPerDataPx));
            y = state->viewerState->viewPorts[VPfound].leftUpperDataPxOnScreen.y
                + ((int)(((float)yDistance)
                / state->viewerState->viewPorts[VPfound].screenPxYPerDataPx));
            x = state->viewerState->currentPosition.x;
            break;
    }
    if(!((x < 0)
        || (x > state->boundary.x)
        || (y < 0)
        || (y > state->boundary.y)
        || (z < 0)
        || (z > state->boundary.z))) {


        SET_COORDINATE((*foundCoordinate),
                       x,
                       y,
                       z);


        return foundCoordinate;
    }

    return NULL;
}

uint32_t bindInput(int32_t mouse,
                  int32_t modkey,
                  int32_t key,
                  int32_t action,
                  struct stateInfo *state) {

    /*
     * This function will add a mapping (mouse, modkey, key) -> action to the
     * input mapping list. If the (mouse, modkey, key) combination is already
     * in the list, it will be replaced. If it isn't, it will be added to the
     * end of the list.
     *
     */

    struct inputmap *currentmapping = NULL;
    struct inputmap *prevmapping = NULL;
    struct inputmap *newmapping = NULL;

    currentmapping = state->viewerState->inputmap;

    newmapping = malloc(sizeof(struct inputmap));
    if(newmapping == NULL) {
        printf("Out of memory");
        _Exit(FALSE);
    }
    memset(newmapping, '\0', sizeof(struct inputmap));

    newmapping->mouse = mouse;
    newmapping->modkey = modkey;
    newmapping->key = key;
    newmapping->action = action;
    newmapping->next = NULL;

    if(state->viewerState->inputmap == NULL)
        state->viewerState->inputmap = newmapping;
    else {
        while(currentmapping != NULL) {
            if(currentmapping->mouse == mouse &&
               currentmapping->modkey == modkey &&
               currentmapping->key) {
                if(prevmapping != NULL)
                    prevmapping->next = newmapping;
                else
                    state->viewerState->inputmap = newmapping;
                newmapping->next = currentmapping->next;

                free(currentmapping);

                return TRUE;
            }

            prevmapping = currentmapping;
            currentmapping = currentmapping->next;
        }
        prevmapping->next = newmapping;
    }

    return TRUE;
}

uint32_t inputToAction (int32_t mouse,
                       int32_t modkey,
                       int32_t key,
                       struct stateInfo *state) {
    /*
     * Given input parameters (mouse, modkey, key) this will return the
     * associated action (an integer, refer to input.h for symbolic names) that
     * will have to be handled downstream.
     *
     */

    struct inputmap *currentmapping = NULL;

    currentmapping = state->viewerState->inputmap;

    if(state->viewerState->inputmap == NULL)
        return ACTION_NONE;

    while(currentmapping != NULL) {
        if(currentmapping->mouse == mouse &&
           currentmapping->modkey == modkey &&
           currentmapping->key == key)
            return currentmapping->action;

        currentmapping = currentmapping->next;
    }

    return ACTION_NONE;
}
