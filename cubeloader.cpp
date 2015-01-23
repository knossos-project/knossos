#include "cubeloader.h"

#include "coordinate.h"
#include "knossos.h"
#include "knossos-global.h"
#include "segmentation.h"
#include "segmentationsplit.h"

boost::multi_array_ref<uint64_t, 3> getCube(const Coordinate & pos) {
    const auto posDc = Coordinate::Px2DcCoord(pos, state->cubeEdgeLength);

    state->protectCube2Pointer->lock();
    auto rawcube = Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[int_log(state->magnification)], posDc);
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

bool isInsideCircle(int x, int y, int z, int radius) {
    const auto sqdistance = x*x + y*y + z*z;
    if(sqdistance < radius*radius)
        return true;
    else
        return false;
}

void writeVoxels(const Coordinate & pos, uint64_t value, const brush_t & brush) {
    if (brush.getTool() != brush_t::tool_t::merge) {
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
                if (brush.isInverse()) {
                    //if there’re selected objects, we only want to erase these
                    bool shouldErase = Segmentation::singleton().selectedObjectsCount() != 0;
                    if (!shouldErase) {
                        const auto soid = readVoxel({x, y, z});
                        if (Segmentation::singleton().subobjectExists(soid)) {
                            const auto & subobject = Segmentation::singleton().subobjectFromId(soid, pos);
                            shouldErase = Segmentation::singleton().isSelected(subobject);
                        }
                    }
                    if (shouldErase) {
                        writeVoxel({x, y, z}, 0);
                    }
                } else {
                    writeVoxel({x, y, z}, value);
                }
            }
        }
    }
}
