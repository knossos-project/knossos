#include "cubeloader.h"

#include "knossos.h"

extern stateInfo * const state;

boost::multi_array_ref<uint64_t, 3> getCube(const Coordinate & pos) {
    const auto posDc = Coordinate::Px2DcCoord(pos, state->cubeEdgeLength);

    state->protectCube2Pointer->lock();
    auto rawcube = Hashtable::ht_get(state->Oc2Pointer[int_log(state->magnification)], posDc);
    state->protectCube2Pointer->unlock();

    if (rawcube == HT_FAILURE) {
        rawcube = loader->bogusOc;
    }
    auto dims = boost::extents[state->cubeEdgeLength][state->cubeEdgeLength][state->cubeEdgeLength];
    boost::multi_array_ref<uint64_t, 3> cube(reinterpret_cast<uint64_t *>(rawcube), dims);
    return cube;
}

uint64_t readVoxel(const Coordinate & pos) {
    const auto inCube = pos.insideCube(state->cubeEdgeLength);
    return getCube(pos)[inCube.z][inCube.y][inCube.x];
}

void writeVoxel(const Coordinate & pos, const uint64_t value) {
    const auto inCube = pos.insideCube(state->cubeEdgeLength);
    getCube(pos)[inCube.z][inCube.y][inCube.x] = value;
    loader->OcModifiedCacheQueue.emplace(pos.cube(state->cubeEdgeLength));
}
