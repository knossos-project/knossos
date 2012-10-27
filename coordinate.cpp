#include "knossos-global.h"

extern stateInfo *state;

Coordinate Coordinate::Px2DcCoord(Coordinate pxCoordinate) {
    Coordinate dcCoordinate;

    // Rounding should be explicit.
    dcCoordinate.x = pxCoordinate.x / state->cubeEdgeLength;
    dcCoordinate.y = pxCoordinate.y / state->cubeEdgeLength;
    dcCoordinate.z = pxCoordinate.z / state->cubeEdgeLength;

    return dcCoordinate;
}

bool Coordinate::transCoordinate(Coordinate *outCoordinate,
                                 int32_t x,
                                 int32_t y,
                                 int32_t z,
                                 floatCoordinate scale, Coordinate offset) {

    /*
     *  Translate a pixel coordinate (x, y, z) relative to a dataset
     *  with scale scale and offset offset to pixel coordinate outCoordinate
     *  relative to the current dataset.
     */

    outCoordinate->x = ((float)x * scale.x
                        + (float)offset.x
                        - (float)state->offset.x)
                        / (float)state->scale.x;
    outCoordinate->y = ((float)y * scale.y
                        + (float)offset.y
                        - (float)state->offset.y)
                        / (float)state->scale.y;
    outCoordinate->z = ((float)z * scale.z
                        + (float)offset.z
                        - (float)state->offset.z)
                        / (float)state->scale.z;

    return true;
}
