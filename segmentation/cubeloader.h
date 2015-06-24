#ifndef CUBELOADER_H
#define CUBELOADER_H

#include "coordinate.h"

#include <cstdint>
#include <unordered_set>

std::size_t hash_value(const Coordinate & val);

class brush_t;
using CubeCoordSet = std::unordered_set<CoordOfCube>;
using subobjectRetrievalSet = std::unordered_set<std::pair<uint64_t, Coordinate>, boost::hash<std::pair<uint64_t, Coordinate>>>;

void coordCubesMarkChanged(const CubeCoordSet & cubeChangeSet);
uint64_t readVoxel(const Coordinate & pos);
subobjectRetrievalSet readVoxels(const Coordinate & centerPos, const brush_t &);
bool writeVoxel(const Coordinate & pos, const uint64_t value, bool isMarkChanged = true);
void writeVoxels(const Coordinate & centerPos, const uint64_t value, const brush_t &, bool isMarkChanged = true);
CubeCoordSet processRegionByStridedBuf(const Coordinate & globalFirst, const Coordinate &  globalLast, const char *data, const Coordinate & strides, bool isWrite, bool markChanged);

#endif//CUBELOADER_H
