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

// The following are functions that do not need to be accessed openly.
//
// See comments in hash.c for some background on ht_hash.

static uint32_t ht_hash(Hashtable *hashtable, Coordinate key);

// These functions are used internally to deal with the doubly linked list in
// which the data is stored.
static uint32_t ht_ll_put(uint32_t place, C2D_Element *destElement, C2D_Element *newElement, C2D_Element *ht_next);
static uint32_t ht_ll_del(C2D_Element *delElement);
static uint32_t ht_ll_rmlist(C2D_Element *delElement);
