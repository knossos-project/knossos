#include "cubeloader.h"

#include "coordinate.h"
#include "loader.h"
#include "session.h"
#include "segmentation.h"
#include "segmentationsplit.h"
#include "stateInfo.h"

std::pair<bool, char *> getRawCube(const Coordinate & pos) {
    const auto posDc = pos.cube(state->cubeEdgeLength, state->magnification);

    state->protectCube2Pointer->lock();
    auto rawcube = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[int_log(state->magnification)], posDc);
    state->protectCube2Pointer->unlock();

    return std::make_pair(rawcube != nullptr, rawcube);
}

boost::multi_array_ref<uint64_t, 3> getCubeRef(char * const rawcube) {
    const auto dims = boost::extents[state->cubeEdgeLength][state->cubeEdgeLength][state->cubeEdgeLength];
    return boost::multi_array_ref<uint64_t, 3>(reinterpret_cast<uint64_t *>(rawcube), dims);
}

uint64_t readVoxel(const Coordinate & pos) {
    auto cubeIt = getRawCube(pos);
    if (Session::singleton().outsideMovementArea(pos) || !state->overlay || !cubeIt.first) {
        return 0;
    }
    const auto inCube = pos.insideCube(state->cubeEdgeLength, state->magnification);
    return getCubeRef(cubeIt.second)[inCube.z][inCube.y][inCube.x];
}

bool writeVoxel(const Coordinate & pos, const uint64_t value, bool isMarkChanged) {
    auto cubeIt = getRawCube(pos);
    if ((state->magnification != 1) || Session::singleton().outsideMovementArea(pos) || !state->overlay || !cubeIt.first) {//snappy cache only mag1 capable
        return false;
    }
    const auto inCube = pos.insideCube(state->cubeEdgeLength, state->magnification);
    getCubeRef(cubeIt.second)[inCube.z][inCube.y][inCube.x] = value;
    if (isMarkChanged) {
        Loader::Controller::singleton().markOcCubeAsModified(pos.cube(state->cubeEdgeLength, state->magnification), state->magnification);
    }
    return true;
}

bool isInsideCircle(int x, int y, int z, int radius) {
    const auto sqdistance = x*x + y*y + z*z;
    return sqdistance < radius * radius;
}

std::pair<Coordinate, Coordinate> getRegion(const Coordinate & centerPos, const brush_t & brush) {
    auto globalFirst = (centerPos - brush.getRadius()).capped(0, state->boundary);
    auto globalLast = (centerPos + brush.getRadius()).capped(0, state->boundary);

    if (brush.getMode() == brush_t::mode_t::two_dim) {//disable depth
        if (brush.getView() == brush_t::view_t::xy) {
            globalFirst.z = globalLast.z = centerPos.z;
        } else if(brush.getView() == brush_t::view_t::xz) {
            globalFirst.y = globalLast.y = centerPos.y;
        } else if(brush.getView() == brush_t::view_t::yz) {
            globalFirst.x = globalLast.x = centerPos.x;
        }
    }

    return std::make_pair(globalFirst, globalLast);
}

void coordCubesMarkChanged(const CubeCoordSet & cubeChangeSet) {
    for (auto &cubeCoord : cubeChangeSet) {
        Loader::Controller::singleton().markOcCubeAsModified(cubeCoord, state->magnification);
    }
}

auto wholeCubes = [](const Coordinate & globalFirst, const Coordinate & globalLast, const uint64_t value, CubeCoordSet & cubeChangeSet) {
    const auto wholeCubeBegin = (globalFirst + state->cubeEdgeLength - 1).cube(state->cubeEdgeLength, state->magnification);
    const auto wholeCubeEnd = globalLast.cube(state->cubeEdgeLength, state->magnification);

    //fill all whole cubes
    for (int z = wholeCubeBegin.z; z < wholeCubeEnd.z; ++z)
    for (int y = wholeCubeBegin.y; y < wholeCubeEnd.y; ++y)
    for (int x = wholeCubeBegin.x; x < wholeCubeEnd.x; ++x) {
        const auto cubeCoord = CoordOfCube(x, y, z);
        const auto globalCoord = cubeCoord.cube2Global(state->cubeEdgeLength, state->magnification);
        auto rawcube = getRawCube(globalCoord);
        if (rawcube.first) {
            auto cubeRef = getCubeRef(rawcube.second);
            std::fill(cubeRef.data(), cubeRef.data() + cubeRef.num_elements(), value);
            cubeChangeSet.emplace(cubeCoord);
        } else {
            qCritical() << x << y << z << "cube missing for (complete) writeVoxels";
        }
    }
    //returns the skip function for the region traversal
    return [wholeCubeBegin, wholeCubeEnd](int & x, int y, int z){
        if (x == wholeCubeBegin.x && y >= wholeCubeBegin.y && y < wholeCubeEnd.y && z >= wholeCubeBegin.z && z < wholeCubeEnd.z) {
            x = wholeCubeEnd.x;
        }
    };
};

template<typename Func, typename Skip>
CubeCoordSet processRegion(const Coordinate & globalFirst, const Coordinate &  globalLast, Func func, Skip skip) {
    const auto cubeBegin = globalFirst.cube(state->cubeEdgeLength, state->magnification);
    const auto cubeEnd = globalLast.cube(state->cubeEdgeLength, state->magnification) + 1;
    CubeCoordSet cubeCoords;

    //traverse all remaining cubes
    for (int z = cubeBegin.z; z < cubeEnd.z; ++z)
    for (int y = cubeBegin.y; y < cubeEnd.y; ++y)
    for (int x = cubeBegin.x; x < cubeEnd.x; ++x) {
        skip(x, y, z);//skip cubes which got processed before
        const auto cubeCoord = CoordOfCube(x, y, z);
        const auto globalCubeBegin = cubeCoord.cube2Global(state->cubeEdgeLength, state->magnification);
        auto rawcube = getRawCube(globalCubeBegin);
        if (rawcube.first) {
            auto cubeRef = getCubeRef(rawcube.second);
            const auto globalCubeEnd = globalCubeBegin + state->cubeEdgeLength * state->magnification - 1;
            const auto localStart = globalFirst.capped(globalCubeBegin, globalCubeEnd).insideCube(state->cubeEdgeLength, state->magnification);
            const auto localEnd = globalLast.capped(globalCubeBegin, globalCubeEnd).insideCube(state->cubeEdgeLength, state->magnification);

            for (int z = localStart.z; z <= localEnd.z; ++z)
            for (int y = localStart.y; y <= localEnd.y; ++y)
            for (int x = localStart.x; x <= localEnd.x; ++x) {
                const Coordinate globalCoord{globalCubeBegin.x + x * state->magnification, globalCubeBegin.y + y * state->magnification, globalCubeBegin.z + z * state->magnification};
                func(cubeRef[z][y][x], globalCoord);
            }
            cubeCoords.emplace(cubeCoord);
        } else {
            qCritical() << x << y << z << "cube missing for (partial) writeVoxels";
        }
    }
    return cubeCoords;
}

template<typename Func>//wrapper without Skip
CubeCoordSet processRegion(const Coordinate & globalFirst, const Coordinate &  globalLast, Func func) {
    return processRegion(globalFirst, globalLast, func, [](int &, int, int){});
}

std::unordered_set<uint64_t> readVoxels(const Coordinate & centerPos, const brush_t &brush) {
    std::unordered_set<uint64_t> subobjects;
    const auto region = getRegion(centerPos, brush);
    processRegion(region.first, region.second, [&subobjects](uint64_t & voxel, Coordinate){
        if (voxel != 0) {//don’t select the unsegmented area as object
            subobjects.emplace(voxel);
        }
    });
    return subobjects;
}

void writeVoxels(const Coordinate & centerPos, const uint64_t value, const brush_t & brush, bool isMarkChanged) {
    //all the different invocations here are listed explicitly so the compiler can inline the fuck out of it
    //the brush differentiations were moved outside the core lambda which is called for every voxel
    CubeCoordSet cubeChangeSet;
    CubeCoordSet cubeChangeSetWholeCube;
    if (brush.getTool() == brush_t::tool_t::add) {
        const auto region = getRegion(centerPos, brush);
        if (brush.getShape() == brush_t::shape_t::square) {
            if (!brush.isInverse() || Segmentation::singleton().selectedObjectsCount() == 0) {
                //for rectangular brushes no further range checks are needed
                if (brush.getMode() == brush_t::mode_t::three_dim && brush.getShape() == brush_t::shape_t::square) {
                    //rarest special case: processes completely exclosed cubes first
                    cubeChangeSet = processRegion(region.first, region.second, [&brush, centerPos, value](uint64_t & voxel, Coordinate){
                        voxel = value;
                    }, wholeCubes(region.first, region.second, value, cubeChangeSetWholeCube));
                } else {
                    cubeChangeSet = processRegion(region.first, region.second, [&brush, centerPos, value](uint64_t & voxel, Coordinate){
                        voxel = value;
                    });
                }
            } else {//inverse but selected
                cubeChangeSet = processRegion(region.first, region.second, [&brush, centerPos, value](uint64_t & voxel, Coordinate){
                    if (Segmentation::singleton().isSubObjectIdSelected(voxel)) {//if there’re selected objects, we only want to erase these
                        voxel = 0;
                    }
                });
            }
        } else if (!brush.isInverse() || Segmentation::singleton().selectedObjectsCount() == 0) {
            //voxel need to check if they are inside the circle
            cubeChangeSet = processRegion(region.first, region.second, [&brush, centerPos, value](uint64_t & voxel, Coordinate globalPos){
                if (isInsideCircle(globalPos.x - centerPos.x, globalPos.y - centerPos.y, globalPos.z - centerPos.z, brush.getRadius()+1)) {
                    voxel = value;
                }
            });
        } else {//circle, inverse and selected
            cubeChangeSet = processRegion(region.first, region.second, [&brush, centerPos, value](uint64_t & voxel, Coordinate globalPos){
                if (isInsideCircle(globalPos.x - centerPos.x, globalPos.y - centerPos.y, globalPos.z - centerPos.z, brush.getRadius()+1)
                        && Segmentation::singleton().isSubObjectIdSelected(voxel)) {
                    voxel = 0;
                }
            });
        }
    }
    if (isMarkChanged) {
        for (auto &elem : cubeChangeSetWholeCube) {
            cubeChangeSet.emplace(elem);
        }
        coordCubesMarkChanged(cubeChangeSet);
    }
}

CubeCoordSet processRegionByStridedBuf(const Coordinate & globalFirst, const Coordinate &  globalLast, const char *data, const Coordinate & strides, bool isWrite, bool markChanged) {
    CubeCoordSet cubeChangeSet;
    if (isWrite) {
        cubeChangeSet = processRegion(globalFirst, globalLast,
                [globalFirst,data,strides](uint64_t & voxel, Coordinate globalPos){
                voxel = *(uint64_t*)(&data[((globalPos - globalFirst)*strides).sum()]);
            });
        if (markChanged) {
            coordCubesMarkChanged(cubeChangeSet);
        }
    }
    else {
        cubeChangeSet = processRegion(globalFirst, globalLast,
                [globalFirst,data,strides](uint64_t & voxel, Coordinate globalPos){
                *(uint64_t*)(&data[((globalPos - globalFirst)*strides).sum()]) = voxel;
            });
    }
    return cubeChangeSet;
}
