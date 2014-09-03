#ifndef SEGMENTATION_SPLIT_H
#define SEGMENTATION_SPLIT_H

#include "coordinate.h"
#include <set>

void verticalSplittingPlane(const Coordinate & seed);
void fillCuboid(const Coordinate& center, const uint64_t id, const int w, const int h, const int d);
void fillCuboidInObjects(const Coordinate& center, const uint64_t id, std::set<uint64_t> in_ids, const int w, const int h, const int d);

#endif//SEGMENTATION_SPLIT_H
