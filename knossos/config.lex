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
#include <stdlib.h>
#include <string.h>
#include "y.tab.h"
%}

%option bison-bridge
%option always-interactive
%option header-file="lex.yy.h"

%%

 /* String variables for assignments */
experiment\ name        return EXPERIMENT;
data\ path              return DATAPATH;
server\ at				return SERVER;
lock\ comment\ filter   return LOCKCOMMENT;
zfirstorder             return BOERGENS;

 /* Numeric variables for assignments */
remote\ port			return REMOTEPORT;
supercube\ edge			return M_TOK;
M						return M_TOK;
datacube\ edge			return DCEDGE;
scale\ x				return SCALEX;
scale\ y				return SCALEY;
scale\ z				return SCALEZ;
aspect\ ratio\ x		return SCALEX;
aspect\ ratio\ y		return SCALEY;
aspect\ ratio\ z		return SCALEZ;
offset\ x				return OFFSETX;
offset\ y				return OFFSETY;
offset\ z				return OFFSETZ;
boundary\ x				return BOUNDX;
boundary\ y				return BOUNDY;
boundary\ z				return BOUNDZ;
lock\ radius            return LOCKRADIUS;
lock\ comments          return LOCKPOSITIONS;
magnification           return MAGNIFICATION;

 /* Commands for control and trajectory */
def\ trajectory			return TRAJECTORY_TK;
run\ trajectory			return RUNTRAJECTORY_TK;
jump					return JUMPTO_TK;
sleep					return SLEEP_TK;
walk					return WALK_TK;
step					return STEP_TK;
in						return IN_TK;
until\ loader			return TILLLOAD_TK;

 /* Values */
\"[^\"]*\"			    yylval->string=strdup(&(yytext[1])); yylval->string[strlen(yylval->string) - 1] = '\0'; return STRING;
[+-]?[0-9]+        		yylval->integer=atoi(yytext); return _INTEGER;
[+-]?[0-9]+\.[0-9]*		yylval->floating=(float)strtod(yytext, NULL); return _FLOAT;
\{.*\}	    			yylval->string=strdup(&(yytext[1])); yylval->string[strlen(yylval->string) - 1] = '\0'; return BLOCK;

 /* Other stuff */
[ \t\n\r=]*              /* ignore whitespace and = */;
.						return (int) yytext[0];
%%
