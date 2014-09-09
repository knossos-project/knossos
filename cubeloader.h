#ifndef CUBELOADER_H
#define CUBELOADER_H

#include "coordinate.h"
#include "knossos-global.h"
#include "segmentationsplit.h"

#include <boost/multi_array.hpp>

boost::multi_array_ref<uint64_t, 3> getCube(const Coordinate & pos);
uint64_t readVoxel(const Coordinate & pos);
void writeVoxel(const Coordinate & pos, const uint64_t value);
void writeVoxels(const Coordinate & pos, uint64_t value, const brush_t &);

#endif//CUBE_LOADER_H
