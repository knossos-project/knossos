
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EXPERIMENT = 258,
     DATAPATH = 259,
     SERVER = 260,
     LOCKRADIUS = 261,
     LOCKPOSITIONS = 262,
     LOCKCOMMENT = 263,
     REMOTEPORT = 264,
     M_TOK = 265,
     DCEDGE = 266,
     SCALEX = 267,
     SCALEY = 268,
     SCALEZ = 269,
     OFFSETX = 270,
     OFFSETY = 271,
     OFFSETZ = 272,
     BOUNDX = 273,
     BOUNDY = 274,
     BOUNDZ = 275,
     TRAJECTORY_TK = 276,
     JUMPTO_TK = 277,
     SLEEP_TK = 278,
     WALK_TK = 279,
     STEP_TK = 280,
     IN_TK = 281,
     TILLLOAD_TK = 282,
     RUNTRAJECTORY_TK = 283,
     MAGNIFICATION = 284,
     BOERGENS = 285,
     _INTEGER = 286,
     STRING = 287,
     BLOCK = 288,
     _FLOAT = 289
   };
#endif
/* Tokens.  */
#define EXPERIMENT 258
#define DATAPATH 259
#define SERVER 260
#define LOCKRADIUS 261
#define LOCKPOSITIONS 262
#define LOCKCOMMENT 263
#define REMOTEPORT 264
#define M_TOK 265
#define DCEDGE 266
#define SCALEX 267
#define SCALEY 268
#define SCALEZ 269
#define OFFSETX 270
#define OFFSETY 271
#define OFFSETZ 272
#define BOUNDX 273
#define BOUNDY 274
#define BOUNDZ 275
#define TRAJECTORY_TK 276
#define JUMPTO_TK 277
#define SLEEP_TK 278
#define WALK_TK 279
#define STEP_TK 280
#define IN_TK 281
#define TILLLOAD_TK 282
#define RUNTRAJECTORY_TK 283
#define MAGNIFICATION 284
#define BOERGENS 285
#define _INTEGER 286
#define STRING 287
#define BLOCK 288
#define _FLOAT 289




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 58 "config.y"

    int integer;
    float floating;
    char *string;



/* Line 1676 of yacc.c  */
#line 128 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif
