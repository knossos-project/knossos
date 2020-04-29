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

#include "cubeloader.h"

#include "annotation/annotation.h"
#include "loader.h"
#include "segmentation.h"
#include "segmentationsplit.h"
#include "stateInfo.h"

#include <boost/multi_array.hpp>

std::pair<bool, void *> getRawCube(const Coordinate & pos) {
    if (!Segmentation::singleton().enabled) {
        return {false, nullptr};
    }
    const auto posDc = Dataset::current().global2cube(pos);

    state->protectCube2Pointer.lock();
    auto * rawcube = cubeQuery(state->cube2Pointer, Segmentation::singleton().layerId, Dataset::current().magIndex, posDc);
    state->protectCube2Pointer.unlock();

    return std::make_pair(rawcube != nullptr, rawcube);
}

boost::multi_array_ref<uint64_t, 3> getCubeRef(void * const rawcube) {
    const auto cubeEdgeLen = Dataset::current().cubeEdgeLength;
    const auto dims = boost::extents[cubeEdgeLen][cubeEdgeLen][cubeEdgeLen];
    return boost::multi_array_ref<uint64_t, 3>(reinterpret_cast<uint64_t *>(rawcube), dims);
}

uint64_t readVoxel(const Coordinate & pos) {
    auto cubeIt = getRawCube(pos);
    if (Annotation::singleton().outsideMovementArea(pos) || !cubeIt.first) {
        return Segmentation::singleton().getBackgroundId();
    }
    const auto inCube = pos.insideCube(Dataset::current().cubeEdgeLength, Dataset::current().scaleFactor);
    return getCubeRef(cubeIt.second)[inCube.z][inCube.y][inCube.x];
}

bool writeVoxel(const Coordinate & pos, const uint64_t value, bool isMarkChanged) {
    auto cubeIt = getRawCube(pos);
    if (Annotation::singleton().outsideMovementArea(pos) || !cubeIt.first) {
        return false;
    }
    const auto inCube = pos.insideCube(Dataset::current().cubeEdgeLength, Dataset::current().scaleFactor);
    getCubeRef(cubeIt.second)[inCube.z][inCube.y][inCube.x] = value;
    if (isMarkChanged) {
        Loader::Controller::singleton().markOcCubeAsModified(pos.cube(Dataset::current().cubeEdgeLength, Dataset::current().scaleFactor), Dataset::current().magnification);
    }
    return true;
}

bool isInsideSphere(const double xi, const double yi, const double zi, const double radius) {
    const auto x = xi * Dataset::current().scales[0].x;
    const auto y = yi * Dataset::current().scales[0].y;
    const auto z = zi * Dataset::current().scales[0].z;
    const auto sqdistance = x*x + y*y + z*z;
    return sqdistance < radius * radius;
}

std::pair<Coordinate, Coordinate> getRegion(const floatCoordinate & centerPos, const brush_t & brush) { // calcs global AABB of local coordinate system's region
    const auto posArb = centerPos.toLocal(brush.v1, brush.v2, brush.n);
    const auto width = brush.radius / Dataset::current().scales[0].componentMul(brush.v1).length();
    const auto height = brush.radius / Dataset::current().scales[0].componentMul(brush.v2).length();
    const auto depth = (brush.mode == brush_t::mode_t::three_dim) ? brush.radius / Dataset::current().scales[0].componentMul(brush.n).length() : 0;
    std::vector<floatCoordinate> localPoints(8);
    for (std::size_t i = 0; i < localPoints.size(); ++i) { // all possible combinations. Ordering like in a truth table (with - and + instead of false and true). I just didn't want to type it all out…
        localPoints[i].x = (i < 4) ? posArb.x - width : posArb.x + width;
        localPoints[i].y = (i % 4 < 2) ? posArb.y - height : posArb.y + height;
        localPoints[i].z = (i % 2 == 0) ? posArb.z - depth : posArb.z + depth;
    }
    auto min = floatCoordinate(1, 1, 1) * std::numeric_limits<float>::max();
    floatCoordinate max{0, 0, 0};
    for (const auto localCoord : localPoints) {
        const auto worldCoord = localCoord.toWorldFrom(brush.v1, brush.v2, brush.n).capped(Annotation::singleton().movementAreaMin, Annotation::singleton().movementAreaMax);
        min = {std::min(worldCoord.x, min.x), std::min(worldCoord.y, min.y), std::min(worldCoord.z, min.z)};
        max = {std::max(worldCoord.x, max.x), std::max(worldCoord.y, max.y), std::max(worldCoord.z, max.z)};
    }
    return std::make_pair(min, max);
}

void coordCubesMarkChanged(const CubeCoordSet & cubeChangeSet) {
    for (auto &cubeCoord : cubeChangeSet) {
        Loader::Controller::singleton().markOcCubeAsModified(cubeCoord, Dataset::current().magnification);
    }
}

auto wholeCubes = [](const Coordinate & globalFirst, const Coordinate & globalLast, const uint64_t value, CubeCoordSet & cubeChangeSet) {
    const auto wholeCubeBegin = Dataset::current().global2cube(globalFirst + Dataset::current().cubeEdgeLength - 1);
    const auto wholeCubeEnd = Dataset::current().global2cube(globalLast);

    //fill all whole cubes
    for (int z = wholeCubeBegin.z; z < wholeCubeEnd.z; ++z)
    for (int y = wholeCubeBegin.y; y < wholeCubeEnd.y; ++y)
    for (int x = wholeCubeBegin.x; x < wholeCubeEnd.x; ++x) {
        const auto cubeCoord = CoordOfCube(x, y, z);
        const auto globalCoord = Dataset::current().cube2global(cubeCoord);
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
    const auto & cubeEdgeLen = Dataset::current().cubeEdgeLength;
    const auto cubeBegin = Dataset::current().global2cube(globalFirst);
    const auto cubeEnd = Dataset::current().global2cube(globalLast) + 1;
    CubeCoordSet cubeCoords;

    //traverse all remaining cubes
    for (int z = cubeBegin.z; z < cubeEnd.z; ++z)
    for (int y = cubeBegin.y; y < cubeEnd.y; ++y)
    for (int x = cubeBegin.x; x < cubeEnd.x; ++x) {
        skip(x, y, z);//skip cubes which got processed before
        const auto cubeCoord = CoordOfCube(x, y, z);
        const auto globalCubeBegin = Dataset::current().cube2global(cubeCoord);
        auto rawcube = getRawCube(globalCubeBegin);
        if (rawcube.first) {
            auto cubeRef = getCubeRef(rawcube.second);
            const auto globalCubeEnd = globalCubeBegin + Dataset::current().scaleFactor * cubeEdgeLen - 1;
            const auto localStart = globalFirst.capped(globalCubeBegin, globalCubeEnd).insideCube(cubeEdgeLen, Dataset::current().scaleFactor);
            const auto localEnd = globalLast.capped(globalCubeBegin, globalCubeEnd).insideCube(cubeEdgeLen, Dataset::current().scaleFactor);

            for (int z = localStart.z; z <= localEnd.z; ++z)
            for (int y = localStart.y; y <= localEnd.y; ++y)
            for (int x = localStart.x; x <= localEnd.x; ++x) {
                const Coordinate globalFromVoxelCoord{globalCubeBegin + Dataset::current().scaleFactor.componentMul(Coordinate{x, y, z})};
                const auto adjustedGlobalCoord = globalFromVoxelCoord.capped(globalFirst, globalLast);// fit to region boundaries that don’t exactly match mag2+ voxel coords
                func(cubeRef[z][y][x], adjustedGlobalCoord);
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

subobjectRetrievalMap readVoxels(const Coordinate & centerPos, const brush_t &brush) {
    subobjectRetrievalMap subobjects;
    const auto region = getRegion(centerPos, brush);
    processRegion(region.first, region.second, [&subobjects](uint64_t & voxel, Coordinate position){
        if (voxel != 0) {//don’t select the unsegmented area as object
            subobjects.emplace(std::piecewise_construct, std::make_tuple(voxel), std::make_tuple(position));
        }
    });
    return subobjects;
}

void writeVoxels(const Coordinate & centerPos, const uint64_t value, const brush_t & brush, bool isMarkChanged) {
    //all the different invocations here are listed explicitly so the compiler can inline the fuck out of it
    //the brush differentiations were moved outside the core lambda which is called for every voxel
    CubeCoordSet cubeChangeSet;
    CubeCoordSet cubeChangeSetWholeCube;
    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Mode_Paint)) {
        const auto region = getRegion(centerPos, brush);
        if (brush.shape == brush_t::shape_t::angular) {
            if (!brush.inverse || Segmentation::singleton().selectedObjectsCount() == 0) {
                //for rectangular brushes no further range checks are needed
                if (brush.mode == brush_t::mode_t::three_dim && brush.shape == brush_t::shape_t::angular) {
                    //rarest special case: processes completely exclosed cubes first
                    cubeChangeSet = processRegion(region.first, region.second, [value](uint64_t & voxel, Coordinate){
                        voxel = value;
                    }, wholeCubes(region.first, region.second, value, cubeChangeSetWholeCube));
                } else {
                    cubeChangeSet = processRegion(region.first, region.second, [value](uint64_t & voxel, Coordinate){
                        voxel = value;
                    });
                }
            } else {//inverse but selected
                cubeChangeSet = processRegion(region.first, region.second, [](uint64_t & voxel, Coordinate){
                    if (Segmentation::singleton().isSubObjectIdSelected(voxel)) {//if there’re selected objects, we only want to erase these
                        voxel = 0;
                    }
                });
            }
        } else if (!brush.inverse || Segmentation::singleton().selectedObjectsCount() == 0) {
            //voxel need to check if they are inside the circle
            cubeChangeSet = processRegion(region.first, region.second, [&brush, centerPos, value](uint64_t & voxel, Coordinate globalPos){
                if (isInsideSphere(globalPos.x - centerPos.x, globalPos.y - centerPos.y, globalPos.z - centerPos.z, brush.radius)) {
                    voxel = value;
                }
            });
        } else {//circle, inverse and selected
            cubeChangeSet = processRegion(region.first, region.second, [&brush, centerPos](uint64_t & voxel, Coordinate globalPos){
                if (isInsideSphere(globalPos.x - centerPos.x, globalPos.y - centerPos.y, globalPos.z - centerPos.z, brush.radius)
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

CubeCoordSet processRegionByStridedBuf(const Coordinate & globalFirst, const Coordinate &  globalLast, char * data, const Coordinate & strides, bool isWrite, bool markChanged) {
    CubeCoordSet cubeChangeSet;
    if (isWrite) {
        cubeChangeSet = processRegion(globalFirst, globalLast,
                [globalFirst,data,strides](uint64_t & voxel, Coordinate globalPos){
                voxel = reinterpret_cast<const uint64_t &>(data[(globalPos - globalFirst).componentMul(strides).sum()]);
            });
        if (markChanged) {
            coordCubesMarkChanged(cubeChangeSet);
        }
    }
    else {
        cubeChangeSet = processRegion(globalFirst, globalLast,
                [globalFirst,data,strides](uint64_t & voxel, Coordinate globalPos){
                reinterpret_cast<uint64_t &>(data[(globalPos - globalFirst).componentMul(strides).sum()]) = voxel;
            });
    }
    return cubeChangeSet;
}

void listFill(const Coordinate & centerPos, const brush_t & brush, const uint64_t fillsoid, const std::unordered_set<Coordinate> & voxels) {
    const auto region = getRegion(centerPos, brush);
    auto cubeChangeSet = processRegion(region.first, region.second, [fillsoid, &voxels](uint64_t & voxel, Coordinate globalPos){
        if (voxels.find(globalPos) != std::end(voxels)) {
            voxel = fillsoid;
        }
    });
    coordCubesMarkChanged(cubeChangeSet);
}
