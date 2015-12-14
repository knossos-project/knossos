#ifndef CUBELOADER_H
#define CUBELOADER_H

#include "coordinate.h"

#include <cstdint>
#include <unordered_set>
#include <unordered_map>

class brush_t;
using CubeCoordSet = std::unordered_set<CoordOfCube>;
using subobjectRetrievalMap = std::unordered_map<uint64_t, Coordinate>;

bool isInsideSphere(const double xi, const double yi, const double zi, const double radius);

void coordCubesMarkChanged(const CubeCoordSet & cubeChangeSet);
uint64_t readVoxel(const Coordinate & pos);
subobjectRetrievalMap readVoxels(const Coordinate & centerPos, const brush_t &);
bool writeVoxel(const Coordinate & pos, const uint64_t value, bool isMarkChanged = true);
void writeVoxels(const Coordinate & centerPos, const uint64_t value, const brush_t &, bool isMarkChanged = true);
CubeCoordSet processRegionByStridedBuf(const Coordinate & globalFirst, const Coordinate &  globalLast, char * data, const Coordinate & strides, bool isWrite, bool markChanged);
void listFill(const Coordinate & centerPos, const brush_t & brush, const uint64_t fillsoid, const std::unordered_set<Coordinate> & voxels);

#endif//CUBELOADER_H
