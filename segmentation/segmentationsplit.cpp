#include <iostream>
#include "segmentationsplit.h"

#include "coordinate.h"
#include "cubeloader.h"
#include "segmentation.h"

#include <unordered_set>

std::unordered_set<uint64_t> bucketFill(const Coordinate & seed, const uint64_t objIndexToSplit, const uint64_t newSubObjId, const std::unordered_set<uint64_t> & subObjectsToFill) {
    std::vector<Coordinate> work = {seed};
    std::unordered_set<Coordinate> visitedVoxels;
    std::unordered_set<uint64_t> visitedSubObjects;
    while (!work.empty()) {
        auto pos = work.back();
        work.pop_back();

        auto subobjectId = readVoxel(pos);
        if (subobjectId != 0 && subobjectId != newSubObjId) {
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
    std::cout << "12" << std::endl;
    auto subobjectId = readVoxel(seed);
    if (subobjectId != 0) {
        auto & subobject = Segmentation::singleton().subobjectFromId(subobjectId, seed);
        auto splitIndex = Segmentation::singleton().largestObjectContainingSubobject(subobject);
        auto newSubObjId = Segmentation::SubObject::highestId + 1;
        auto newObjectIndex = Segmentation::singleton().createObject(newSubObjId, seed).index;

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
        if (subobjectId != 0 && subobjectId != newSubObjId) {
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
    std::cout << "13" << std::endl;
    auto subobjectId = readVoxel(seed);
    if (subobjectId != 0) {
        auto & subobject = Segmentation::singleton().subobjectFromId(subobjectId, seed);
        auto splitId = Segmentation::singleton().largestObjectContainingSubobject(subobject);
        auto newSubObjId = Segmentation::SubObject::highestId + 1;
        auto newObjectIndex = Segmentation::singleton().createObject(newSubObjId, seed).index;

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
