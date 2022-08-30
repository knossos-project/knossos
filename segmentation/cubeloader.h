/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include "coordinate.h"

#include <cstdint>
#include <optional>
#include <unordered_set>
#include <unordered_map>

class brush_t;
using CubeCoordSet = std::unordered_set<CoordOfCube>;
using subobjectRetrievalMap = std::unordered_map<uint64_t, Coordinate>;

bool isInsideSphere(const double xi, const double yi, const double zi, const double radius);

void coordCubesMarkChanged(const CubeCoordSet & cubeChangeSet);
std::optional<std::uint64_t> readLayerVoxel(const Coordinate & pos, const std::size_t layerIdx);
std::uint64_t readVoxel(const Coordinate & pos);
subobjectRetrievalMap readVoxels(const Coordinate & centerPos, const brush_t &);
void collectFromMovementArea();
void assignNewIdInMovementArea(const std::uint64_t newId);
bool writeVoxel(const Coordinate & pos, const uint64_t value, bool isMarkChanged = true);
void writeVoxels(const Coordinate & centerPos, const uint64_t value, const brush_t &, bool isMarkChanged = true);
void coordCubesMarkChanged(const CubeCoordSet & cubeChangeSet);
CubeCoordSet processRegionByStridedBuf(const Coordinate & globalFirst, const Coordinate &  globalLast, char * data, const Coordinate & strides, bool isWrite, bool markChanged);
void listFill(const Coordinate & centerPos, const brush_t & brush, const uint64_t fillsoid, const std::unordered_set<Coordinate> & voxels);
