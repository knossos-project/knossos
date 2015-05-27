#ifndef CUBELOADER_H
#define CUBELOADER_H

#include "coordinate.h"

#include <cstdint>
#include <unordered_set>

class brush_t;
using CubeCoordSet = std::unordered_set<CoordOfCube>;

void coordCubesMarkChanged(const CubeCoordSet & cubeChangeSet);
uint64_t readVoxel(const Coordinate & pos);
std::unordered_set<uint64_t> readVoxels(const Coordinate & centerPos, const brush_t &);
bool writeVoxel(const Coordinate & pos, const uint64_t value, bool isMarkChanged = true);
void writeVoxels(const Coordinate & centerPos, const uint64_t value, const brush_t &, bool isMarkChanged = true);
CubeCoordSet processRegionByStridedBuf(const Coordinate & globalFirst, const Coordinate &  globalLast, const char *data, const Coordinate & strides, bool isWrite, bool markChanged);

#endif//CUBELOADER_H
