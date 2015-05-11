#include "cubeloader.h"

#include "coordinate.h"
#include "loader.h"
#include "session.h"
#include "segmentation.h"
#include "segmentationsplit.h"
#include "stateInfo.h"

std::pair<bool, char *> getRawCube(const Coordinate & pos) {
    const auto posDc = pos / state->magnification / state->cubeEdgeLength;

    state->protectCube2Pointer->lock();
    auto rawcube = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[int_log(state->magnification)], posDc);
    state->protectCube2Pointer->unlock();

    return std::make_pair(rawcube != nullptr, rawcube);
}

boost::multi_array_ref<uint64_t, 3> getCube(char * const rawcube) {
    const auto dims = boost::extents[state->cubeEdgeLength][state->cubeEdgeLength][state->cubeEdgeLength];
    return boost::multi_array_ref<uint64_t, 3>(reinterpret_cast<uint64_t *>(rawcube), dims);
}

uint64_t readVoxel(const Coordinate & pos) {
    auto cubeIt = getRawCube(pos);
    if (Session::singleton().outsideMovementArea(pos) || !state->overlay || !cubeIt.first) {
        return 0;
    }
    const auto inCube = (pos / state->magnification).insideCube(state->cubeEdgeLength);
    return getCube(cubeIt.second)[inCube.z][inCube.y][inCube.x];
}

bool writeVoxel(const Coordinate & pos, const uint64_t value) {
    auto cubeIt = getRawCube(pos);
    if ((state->magnification != 1) || Session::singleton().outsideMovementArea(pos) || !state->overlay || !cubeIt.first) {//snappy cache only mag1 capable
        return false;
    }
    const auto inCube = (pos / state->magnification).insideCube(state->cubeEdgeLength);
    getCube(cubeIt.second)[inCube.z][inCube.y][inCube.x] = value;
    Loader::Controller::singleton().markOcCubeAsModified(pos.cube(state->cubeEdgeLength, state->magnification), state->magnification);
    return true;
}

bool isInsideCircle(int x, int y, int z, int radius) {
    const auto sqdistance = x*x + y*y + z*z;
    if(sqdistance < radius*radius)
        return true;
    else
        return false;
}

template<typename Func>
void traverseBrush(const Coordinate & pos, const brush_t & brush, Func func) {
    std::array<int, 3> start = {{
        std::max(0, pos.x - brush.getRadius()),
        std::max(0, pos.y - brush.getRadius()),
        std::max(0, pos.z - brush.getRadius())
    }};
    std::array<int, 3> end = {{
        std::min(state->boundary.x, pos.x + brush.getRadius()),
        std::min(state->boundary.y, pos.y + brush.getRadius()),
        std::min(state->boundary.z, pos.z + brush.getRadius())
    }};
    if (brush.getMode() == brush_t::mode_t::two_dim) {//disable depth
        if (brush.getView() == brush_t::view_t::xy) {
            start[2] = end[2] = pos.z;
        } else if(brush.getView() == brush_t::view_t::xz) {
            start[1] = end[1] = pos.y;
        } else if(brush.getView() == brush_t::view_t::yz) {
            start[0] = end[0] = pos.x;
        }
    }
    for (int x = start[0]; x <= end[0]; ++x)//end is inclusive
    for (int y = start[1]; y <= end[1]; ++y)
    for (int z = start[2]; z <= end[2]; ++z) {
        if ((brush.getShape() == brush_t::shape_t::square) ||
            (brush.getShape() == brush_t::shape_t::circle && isInsideCircle(x - pos.x, y - pos.y, z - pos.z, brush.getRadius()+1))) {
            func({x, y, z});
        }
    }
}

std::unordered_set<uint64_t> readVoxels(const Coordinate & centerPos, const brush_t &brush) {
    std::unordered_set<uint64_t> subobjects;
    traverseBrush(centerPos, brush, [&subobjects](const Coordinate & pos){
        const auto soid = readVoxel(pos);
        if (soid != 0) {// don’t select the unsegmented area as object
            subobjects.emplace(soid);
        }
    });
    return subobjects;
}

void writeVoxels(const Coordinate & centerPos, const uint64_t value, const brush_t & brush) {
    if (brush.getTool() == brush_t::tool_t::add) {
        traverseBrush(centerPos, brush, [&brush, &centerPos, &value](const Coordinate & pos){
            if (brush.isInverse()) {
                //if there’re selected objects, we only want to erase these
                const bool selectedObjects = Segmentation::singleton().selectedObjectsCount() != 0;
                const bool visitingSelected = selectedObjects && Segmentation::singleton().isSubObjectIdSelected(readVoxel(pos));
                if (!selectedObjects || visitingSelected) {
                    writeVoxel(pos, 0);
                }
            } else {
                writeVoxel(pos, value);
            }
        });
    }
}
