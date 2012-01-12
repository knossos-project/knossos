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


/*
 *      Code for handling coordinates.
 *
 *      Macros are in coordinate.h.
 *
 */

// See comments about includes in hash.c
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <stdio.h>
#include <stdint.h>

#include "knossos-global.h"

Coordinate Px2DcCoord(Coordinate pxCoordinate, struct stateInfo *state) {
    Coordinate dcCoordinate;

    // Rounding should be explicit.
    dcCoordinate.x = pxCoordinate.x / state->cubeEdgeLength;
    dcCoordinate.y = pxCoordinate.y / state->cubeEdgeLength;
    dcCoordinate.z = pxCoordinate.z / state->cubeEdgeLength;

    return dcCoordinate;
}

int32_t transCoordinate(Coordinate *outCoordinate,
                         int32_t x,
                         int32_t y,
                         int32_t z,
                         floatCoordinate scale,
                         Coordinate offset,
                         struct stateInfo *state) {

    /*
     *  Translate a pixel coordinate (x, y, z) relative to a dataset
     *  with scale scale and offset offset to pixel coordinate outCoordinate
     *  relative to the current dataset.
     */

    outCoordinate->x = ((float)x * scale.x
                    + (float)offset.x
                    - (float)state->offset.x)
                    / (float)state->scale.x;
    outCoordinate->y = ((float)y * scale.y
                    + (float)offset.y
                    - (float)state->offset.y)
                    / (float)state->scale.y;
    outCoordinate->z = ((float)z * scale.z
                    + (float)offset.z
                    - (float)state->offset.z)
                    / (float)state->scale.z;

    return TRUE;
}

