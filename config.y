/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2011
 *  Max-Planck-Gesellschaft zur Förderung der Wissenschaften e.V.
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


%{

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <SDL/SDL.h>

#include "knossos-global.h"

#define YYPARSE_PARAM param
#define YYLEX_PARAM param
#define YYERROR_VERBOSE

void yyerror(const char *str) {
        printf("Yacc error: %s.\n", str);
}

int yywrap() {
        return 1;
}

extern  stateInfo *tempConfig;

%}

%pure_parser
%token EXPERIMENT DATAPATH SERVER LOCKRADIUS LOCKPOSITIONS LOCKCOMMENT
%token REMOTEPORT M_TOK DCEDGE SCALEX SCALEY SCALEZ OFFSETX OFFSETY
%token OFFSETZ BOUNDX BOUNDY BOUNDZ TRAJECTORY_TK JUMPTO_TK SLEEP_TK WALK_TK
%token STEP_TK IN_TK TILLLOAD_TK RUNTRAJECTORY_TK MAGNIFICATION BOERGENS

%union
{
    int integer;
    float floating;
    char *string;
}

%token <integer> _INTEGER
%token <string> STRING
%token <string> BLOCK
%token <floating> _FLOAT

%%

commands:
        |
        commands command ';'
        |
        commands traj ';'
        ;


command:
        intassn
        |
        wordassn
        |
        floatassn
        |
        remotecommand
        ;

intassn:
        LOCKPOSITIONS _INTEGER
        {
                tempConfig->skeletonState->lockPositions = $2;
        }
        |
        LOCKRADIUS _INTEGER
        {
                tempConfig->skeletonState->lockRadius = $2;
        }
        |
        REMOTEPORT _INTEGER
        {
                tempConfig->clientState->remotePort = $2;
        }
        |
        M_TOK _INTEGER
        {
                tempConfig->M = $2;
        }
        |
        DCEDGE _INTEGER
        {
                tempConfig->cubeEdgeLength = $2;
        }
        |
        BOUNDX _INTEGER
        {
                tempConfig->boundary.x = $2;
        }
        |
        BOUNDY _INTEGER
        {
                tempConfig->boundary.y = $2;
        }
        |
        BOUNDZ _INTEGER
        {
                tempConfig->boundary.z = $2;
        }
        |
        OFFSETX _INTEGER
        {
                tempConfig->offset.x = $2;
        }
        |
        OFFSETY _INTEGER
        {
                tempConfig->offset.y = $2;
        }
        |
        OFFSETZ _INTEGER
        {
                tempConfig->offset.z = $2;
        }
        |
        MAGNIFICATION _INTEGER
        {
                tempConfig->magnification = $2;
        }
        |
        BOERGENS _INTEGER
        {
                tempConfig->boergens = true;
        }
        ;
wordassn:
        EXPERIMENT STRING
        {
                strncpy(tempConfig->name, $2, 1024);
        }
        |
        DATAPATH STRING
        {
                strncpy(tempConfig->path, $2, 1024);
        }
        |
        SERVER STRING
        {
                strncpy(tempConfig->clientState->serverAddress, $2, 1024);
        }
        |
        LOCKCOMMENT STRING
        {
                strncpy(tempConfig->skeletonState->onCommentLock, $2, 1024);
        }
        ;
floatassn:
        SCALEX _FLOAT
        {
                tempConfig->scale.x = $2;
        }
        |
        SCALEY _FLOAT
        {
                tempConfig->scale.y = $2;
        }
        |
        SCALEZ _FLOAT
        {
                tempConfig->scale.z = $2;
        }
        ;
remotecommand:
        jump
        |
        sleep
        |
        walk
        |
        runtraj
        ;
jump:
        JUMPTO_TK '(' _INTEGER ',' _INTEGER ',' _INTEGER ')'
        {
                remoteJump($3, $5, $7);
        }
        ;
sleep:
        SLEEP_TK _INTEGER
        {
                SDL_Delay($2);
        }
        ;
walk:
        WALK_TK '(' _INTEGER ',' _INTEGER ',' _INTEGER ')'
        {
                printf("Trying to walk to (%d, %d, %d)\n", $3, $5, $7);
                remoteWalkTo($3, $5, $7);
        }
        ;
traj:
        TRAJECTORY_TK STRING BLOCK
        {
                newTrajectory($2, $3);
        }
        ;
runtraj:
        RUNTRAJECTORY_TK STRING
        {
                // sendRemoteSignal();
        }
        ;
