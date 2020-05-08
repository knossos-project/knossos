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

#include "segmentationsplit.h"

#include "coordinate.h"
#include "cubeloader.h"
#include "dataset.h"
#include "loader.h"
#include "segmentation.h"

#include <unordered_set>
#include <vector>

void brush_subject::setRadius(const radius_t newRadius) {
    const auto scale = Dataset::current().scales[0];
    const auto boundary = Dataset::current().scales[0].componentMul(Dataset::current().boundary);
    radius = std::max(0.5 * std::min({scale.x, scale.y, scale.z}), std::min(1.0 * std::max({boundary.x, boundary.y, boundary.z}), newRadius));
    emit radiusChanged(radius);
}

void subobjectBucketFill(const Coordinate & seed, const uint64_t fillsoid, const brush_t & brush, const Coordinate & areaMin, const Coordinate & areaMax) {
    std::vector<Coordinate> work = {seed};
    CubeCoordSet cubeCoords;

    const auto clickedsoid = readVoxel(seed);
    if (clickedsoid == fillsoid) {
        return;
    }

    const auto voxelSpacing = Dataset::datasets[Segmentation::singleton().layerId].scaleFactor;

    while (!work.empty()) {
        const auto pos = work.back();
        work.pop_back();

        const auto walk = [&work, &cubeCoords, clickedsoid, fillsoid](const Coordinate coord){
            if (readVoxel(coord) == clickedsoid) {
                writeVoxel(coord, fillsoid, false);
                work.emplace_back(coord);
                cubeCoords.insert(Dataset::datasets[Segmentation::singleton().layerId].global2cube(coord));
            }
        };

        const auto posDec = (pos - voxelSpacing).capped(areaMin, areaMax);
        const auto posInc = (pos + voxelSpacing).capped(areaMin, areaMax);

        if (brush.view != brush_t::view_t::xy || brush.mode == brush_t::mode_t::three_dim) {
            walk({pos.x, pos.y, posInc.z});
            walk({pos.x, pos.y, posDec.z});
        }
        if (brush.view != brush_t::view_t::xz || brush.mode == brush_t::mode_t::three_dim) {
            walk({pos.x, posInc.y, pos.z});
            walk({pos.x, posDec.y, pos.z});
        }
        if (brush.view != brush_t::view_t::zy || brush.mode == brush_t::mode_t::three_dim) {
            walk({posInc.x, pos.y, pos.z});
            walk({posDec.x, pos.y, pos.z});
        }
    }

    coordCubesMarkChanged(cubeCoords);
}

std::unordered_set<uint64_t> bucketFill(const Coordinate & seed, const uint64_t objIndexToSplit, const uint64_t newSubObjId, const std::unordered_set<uint64_t> & subObjectsToFill) {
    std::vector<Coordinate> work = {seed};
    std::unordered_set<Coordinate> visitedVoxels;
    std::unordered_set<uint64_t> visitedSubObjects;
    while (!work.empty()) {
        auto pos = work.back();
        work.pop_back();

        auto subobjectId = readVoxel(pos);
        if (subobjectId != Segmentation::singleton().getBackgroundId() && subobjectId != newSubObjId) {
            auto & subobject = Segmentation::singleton().subobjectFromId(subobjectId, seed);
            auto objIndex = Segmentation::singleton().largestObjectContainingSubobject(subobject);
            if (objIndex == objIndexToSplit) {
                if (subObjectsToFill.find(subobjectId) != std::end(subObjectsToFill)) {
                    //only write to cubes which were hit by the splitting plane
                    writeVoxel(pos, newSubObjId);//write to cube
                } else {
                    visitedSubObjects.emplace(subobject.id);//accumulate visited subobjects
                }

                visitedVoxels.emplace(pos.x, pos.y, pos.z);
                auto walk = [&](const Coordinate & newPos){
                    if (visitedVoxels.find(newPos) == std::end(visitedVoxels)) {
                        work.emplace_back(newPos);
                    }
                };
                walk({pos.x + 1, pos.y, pos.z});
                walk({pos.x - 1, pos.y, pos.z});
                walk({pos.x, pos.y + 1, pos.z});
                walk({pos.x, pos.y - 1, pos.z});
                walk({pos.x, pos.y, pos.z + 1});
                walk({pos.x, pos.y, pos.z - 1});
            }
        }
    }
    return visitedSubObjects;
}

void connectedComponent(const Coordinate & seed) {
    auto subobjectId = readVoxel(seed);
    if (subobjectId != Segmentation::singleton().getBackgroundId()) {
        auto & subobject = Segmentation::singleton().subobjectFromId(subobjectId, seed);
        auto splitIndex = Segmentation::singleton().largestObjectContainingSubobject(subobject);
        auto newSubObjId = Segmentation::SubObject::highestId + 1;
        auto newObjectIndex = Segmentation::singleton().createObjectFromSubobjectId(newSubObjId, seed).index;

        std::unordered_set<uint64_t> subObjectsToFill = {subobjectId};

        std::unordered_set<uint64_t> visitedSubObjects = bucketFill({seed.x + 1, seed.y, seed.z}, splitIndex, newSubObjId, subObjectsToFill);

        //merge all traversed but unchanged supervoxel into the new object
        auto & newObject = Segmentation::singleton().objects[newObjectIndex];
        for (auto && id : visitedSubObjects) {
            auto & subobject = Segmentation::singleton().subobjectFromId(id, seed);
            newObject.addExistingSubObject(subobject);
        }
        std::sort(std::begin(newObject.subobjects), std::end(newObject.subobjects));

        //remove the newly created area from the object to split
        Segmentation::singleton().selectObject(splitIndex);
        Segmentation::singleton().selectObject(newObjectIndex);
        Segmentation::singleton().unmergeSelectedObjects(seed);
    }
}

std::unordered_set<uint64_t> verticalSplittingPlane(const Coordinate & pos, const uint64_t objIndexToSplit, const uint64_t newSubObjId) {
    std::vector<Coordinate> work = {pos};
    std::unordered_set<Coordinate> visitedVoxels;
    std::unordered_set<uint64_t> visitedSubObjects;
    while (!work.empty()) {
        auto pos = work.back();
        work.pop_back();

        auto subobjectId = readVoxel(pos);
        if (subobjectId != Segmentation::singleton().getBackgroundId() && subobjectId != newSubObjId) {
            auto & subobject = Segmentation::singleton().subobjectFromId(subobjectId, pos);
            auto objIndex = Segmentation::singleton().largestObjectContainingSubobject(subobject);
            if (objIndex == objIndexToSplit) {
                writeVoxel(pos, newSubObjId);//write to cube
                visitedSubObjects.emplace(subobject.id);//accumulate visited subobjects

                visitedVoxels.emplace(pos.x, pos.y, pos.z);
                auto walk = [&](const Coordinate & newPos){
                    if (visitedVoxels.find(newPos) == std::end(visitedVoxels)) {
                        work.emplace_back(newPos);
                    }
                };
                walk({pos.x, pos.y + 1, pos.z});
                walk({pos.x, pos.y - 1, pos.z});
                walk({pos.x, pos.y, pos.z + 1});
                walk({pos.x, pos.y, pos.z - 1});
            }
        }
    }
    return visitedSubObjects;
}

void verticalSplittingPlane(const Coordinate & seed) {
    auto subobjectId = readVoxel(seed);
    if (subobjectId != Segmentation::singleton().getBackgroundId()) {
        auto & subobject = Segmentation::singleton().subobjectFromId(subobjectId, seed);
        auto splitId = Segmentation::singleton().largestObjectContainingSubobject(subobject);
        auto newSubObjId = Segmentation::SubObject::highestId + 1;
        auto newObjectIndex = Segmentation::singleton().createObjectFromSubobjectId(newSubObjId, seed).index;

        std::unordered_set<uint64_t> subObjectsToFill = verticalSplittingPlane({seed.x, seed.y, seed.z}, splitId, newSubObjId);

        std::unordered_set<uint64_t> visitedSubObjects = bucketFill({seed.x + 1, seed.y, seed.z}, splitId, newSubObjId, subObjectsToFill);

        //add the newly created subobject to all non-splitted objects
        for (auto && id : subObjectsToFill) {
            auto & subobject = Segmentation::singleton().subobjectFromId(id, seed);
            for (auto && objIndex : subobject.objects) {
                if (objIndex != splitId) {
                    auto && object = Segmentation::singleton().objects[objIndex];
                    auto & newSubobject = Segmentation::singleton().subobjectFromId(newSubObjId, seed);
                    object.addExistingSubObject(newSubobject);
                    std::sort(std::begin(object.subobjects), std::end(object.subobjects));
                }
            }
        }

        //merge all traversed but unchanged supervoxel into the new object
        auto & newObject = Segmentation::singleton().objects[newObjectIndex];
        for (auto && id : visitedSubObjects) {
            auto & subobject = Segmentation::singleton().subobjectFromId(id, seed);
            newObject.addExistingSubObject(subobject);
        }
        std::sort(std::begin(newObject.subobjects), std::end(newObject.subobjects));

        //remove the newly created area from the object to split
        Segmentation::singleton().selectObject(splitId);
        Segmentation::singleton().selectObject(newObjectIndex);
        Segmentation::singleton().unmergeSelectedObjects(seed);
    }
}
