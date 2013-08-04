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

static uint32_t DcoiFromPos(C_Element *Dcoi, Hashtable *currentLoadedHash);
static CubeSlot *slotListGetElement(CubeSlotList *slotList);
int loadCube(loadcube_thread_struct *lts);
static uint32_t cleanUpLoader(struct loaderState *loaderState);

// ALWAYS give the slotList*Element functions an element that
//      1) really is in slotList.
//      2) really exists.
// The functions don't check for that and things will break if it's not the case.
static int32_t slotListDelElement(CubeSlotList *slotList, CubeSlot *element);
static int32_t slotListAddElement(CubeSlotList *slotList, Byte *datacube);
static CubeSlotList *slotListNew();
static int32_t slotListDel(CubeSlotList *delList);
static int32_t initLoader();
static uint32_t removeLoadedCubes(Hashtable *currentLoadedHash, uint32_t prevLoaderMagnification);
static uint32_t loadCubes();
static int32_t addCubicDcSet(int32_t xBase, int32_t yBase, int32_t zBase, int32_t edgeLen, C_Element *target, Hashtable *currentLoadedHash);
