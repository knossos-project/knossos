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
