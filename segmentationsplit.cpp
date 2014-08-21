#include "segmentation_split.h"

#include "knossos-global.h"
#include "segmentation.h"

#include <boost/multi_array.hpp>

#include <unordered_set>

extern stateInfo const * state;

void bucketFill(const Coordinate & seed, boost::multi_array_ref<uint64_t, 3> cube, const uint64_t objIdToSplit, const uint64_t newSubObjId, const std::unordered_set<uint64_t> & subObjectsToFill, std::unordered_set<uint64_t> & visitedSubObjects) {
    std::vector<Coordinate> work = {seed};
    std::unordered_set<Coordinate, Coordinate::Hash> visitedVoxels;
    while (!work.empty()) {
        auto pos = work.back();
        work.pop_back();
        auto subobjectId = cube[pos.z][pos.y][pos.x];
        if (subobjectId != 0 && subobjectId != newSubObjId) {
            auto & subobject = Segmentation::singleton().subobjectFromId(subobjectId);
            auto objId = Segmentation::singleton().largestObjectContainingSubobject(subobject);
            if (objId == objIdToSplit) {
                if (subObjectsToFill.find(subobjectId) != std::end(subObjectsToFill)) {
                    //only write to cubes which were hit by the splitting plane
                    cube[pos.z][pos.y][pos.x] = newSubObjId;//write to cube
                } else {
                    visitedSubObjects.emplace(subobject.id);//accumulate visited subobjects
                }

                visitedVoxels.emplace(pos.x, pos.y, pos.z);

                auto walk = [&](const Coordinate & newPos){
                    if (visitedVoxels.find(newPos) == std::end(visitedVoxels)) {
                        if (pos.x >= 0 && pos.y >= 0 && pos.z >= 0 && pos.z < cube.size() && pos.y < cube[0].size() && pos.x < cube[0][0].size()) {
                            work.emplace_back(newPos);
                        }
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
}

void verticalSplittingPlane(const Coordinate & pos, boost::multi_array_ref<uint64_t, 3> cube, const uint64_t objIdToSplit, const uint64_t newSubObjId, std::unordered_set<uint64_t> & visitedSubObjects) {
    auto subobjectId = cube[pos.z][pos.y][pos.x];
    if (subobjectId != 0 && subobjectId != newSubObjId) {
        auto & subobject = Segmentation::singleton().subobjectFromId(subobjectId);
        auto objId = Segmentation::singleton().largestObjectContainingSubobject(subobject);
        if (objId == objIdToSplit) {
            cube[pos.z][pos.y][pos.x] = newSubObjId;//write to cube
            visitedSubObjects.emplace(subobject.id);//accumulate visited subobjects

            verticalSplittingPlane({pos.x, pos.y + 1, pos.z}, cube, objIdToSplit, newSubObjId, visitedSubObjects);
            verticalSplittingPlane({pos.x, pos.y - 1, pos.z}, cube, objIdToSplit, newSubObjId, visitedSubObjects);
            verticalSplittingPlane({pos.x, pos.y, pos.z + 1}, cube, objIdToSplit, newSubObjId, visitedSubObjects);
            verticalSplittingPlane({pos.x, pos.y, pos.z - 1}, cube, objIdToSplit, newSubObjId, visitedSubObjects);
        }
    }
}

void verticalSplittingPlane(const Coordinate & seed) {
    auto seedDc = Coordinate::Px2DcCoord(seed, state->cubeEdgeLength);
    auto pos = Coordinate{seed.x % state->cubeEdgeLength, seed.y % state->cubeEdgeLength, seed.z % state->cubeEdgeLength};
    state->protectCube2Pointer->lock();
    auto rawcube = Hashtable::ht_get(state->Oc2Pointer[int_log(state->magnification)], seedDc);
    state->protectCube2Pointer->unlock();
    if (rawcube != HT_FAILURE) {
        auto dims = boost::extents[state->cubeEdgeLength][state->cubeEdgeLength][state->cubeEdgeLength];
        boost::multi_array_ref<uint64_t, 3> cube(reinterpret_cast<uint64_t *>(rawcube), dims);

        auto subobjectId = cube[pos.z][pos.y][pos.x];
        if (subobjectId != 0) {
            auto & subobject = Segmentation::singleton().subobjectFromId(subobjectId);
            auto splitId = Segmentation::singleton().largestObjectContainingSubobject(subobject);
            auto newSubObjId = Segmentation::SubObject::highestId + 1;
            auto newObjectId = Segmentation::singleton().createObject(newSubObjId).id;

            std::unordered_set<uint64_t> subObjectsToFill;

            verticalSplittingPlane({pos.x, pos.y, pos.z}, cube, splitId, newSubObjId, subObjectsToFill);

            std::unordered_set<uint64_t> visitedSubObjects;

            bucketFill({pos.x + 1, pos.y, pos.z}, cube, splitId, newSubObjId, subObjectsToFill, visitedSubObjects);

            for (auto && id : subObjectsToFill) {
                auto & subobject = Segmentation::singleton().subobjectFromId(id);
                for (auto && objId : subobject.objects) {
                    if (objId != splitId) {
                        auto && object = Segmentation::singleton().objects[objId];
                        auto & newSubobject = Segmentation::singleton().subobjectFromId(newSubObjId);
                        object.addExistingSubObject(newSubobject);
                        std::sort(std::begin(object.subobjects), std::end(object.subobjects));
                    }
                }
            }

            auto & newObject = Segmentation::singleton().objects[newObjectId];
            for (auto && id : visitedSubObjects) {
                auto & subobject = Segmentation::singleton().subobjectFromId(id);
                newObject.addExistingSubObject(subobject);
                std::sort(std::begin(newObject.subobjects), std::end(newObject.subobjects));
            }

            Segmentation::singleton().selectObject(splitId);
            Segmentation::singleton().selectObject(newObjectId);
            Segmentation::singleton().unmergeSelectedObjects();
        }
    }
}
