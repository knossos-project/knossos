/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "coordinate.h"

#include <unordered_map>

using coord2bytep_map_t = std::unordered_map<CoordOfCube, char*>;

bool Coordinate2BytePtr_hash_get_has_key(const coord2bytep_map_t &h, const CoordOfCube &c);
char* Coordinate2BytePtr_hash_get_or_fail(const coord2bytep_map_t &h, const CoordOfCube &c);
void Coordinate2BytePtr_hash_copy_keys_default_value(coord2bytep_map_t &target, const coord2bytep_map_t &source, char *v);
void Coordinate2BytePtr_hash_union_keys_default_value(coord2bytep_map_t &m, const coord2bytep_map_t &a, const coord2bytep_map_t &b, char *v);

#endif//HASHTABLE_H
