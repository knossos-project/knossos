/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2012
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

static uint32_t DcoiFromPos(Hashtable *Dcoi, struct stateInfo *state);
static CubeSlot *slotListGetElement(CubeSlotList *slotList);
static uint32_t loadCube(Coordinate coordinate, Byte *freeDcSlot, Byte *freeOcSlot, struct stateInfo *state);
static uint32_t cleanUpLoader(struct loaderState *loaderState);

// ALWAYS give the slotList*Element functions an element that
//      1) really is in slotList.
//      2) really exists.
// The functions don't check for that and things will break if it's not the case.
static int32_t slotListDelElement(CubeSlotList *slotList, CubeSlot *element);
static int32_t slotListAddElement(CubeSlotList *slotList, Byte *datacube);
static CubeSlotList *slotListNew();
static int32_t slotListDel(CubeSlotList *delList);
static int32_t initLoader(struct stateInfo *state);
static uint32_t removeLoadedCubes(struct stateInfo *state);
static uint32_t loadCubes(struct stateInfo *state);
static int32_t addCubicDcSet(int32_t x, int32_t y, int32_t z, int32_t edgeLen, Hashtable *target);
