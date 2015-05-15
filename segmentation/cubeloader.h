#ifndef CUBELOADER_H
#define CUBELOADER_H

#include "coordinate.h"
#include "segmentationsplit.h"

#include <boost/multi_array.hpp>

#include <unordered_set>

uint64_t readVoxel(const Coordinate & pos);
std::unordered_set<uint64_t> readVoxels(const Coordinate & centerPos, const brush_t &);
bool writeVoxel(const Coordinate & pos, const uint64_t value);
void writeVoxels(const Coordinate & centerPos, const uint64_t value, const brush_t &);
void processRegionByBuf(const Coordinate & globalFirst, const Coordinate &  globalLast, const char *data, const Coordinate & strides, bool isWrite);
#endif//CUBE_LOADER_H
