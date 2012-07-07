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

//static uint32_t setOGLforVP(uint32_t currentVP, struct stateInfo *state);
//static uint32_t overlayOrthogonalVpPixel(uint32_t currentVP, Coordinate position, color4F color, struct stateInfo *state);
static GLuint renderWholeSkeleton(struct stateInfo *state, Byte callFlag);
static GLuint renderSuperCubeSkeleton(struct stateInfo *state, Byte callFlag);
static GLuint renderActiveTreeSkeleton(struct stateInfo *state, Byte callFlag);
static uint32_t renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius);
static uint32_t renderSphere(Coordinate *pos, float radius);
static uint32_t renderText(Coordinate *pos, char *string);
uint32_t updateDisplayListsSkeleton(struct stateInfo *state);
static uint32_t renderSegPlaneIntersection(struct segmentListElement *segment, struct stateInfo *state);
static uint32_t renderViewportBorders(uint32_t currentVP);
