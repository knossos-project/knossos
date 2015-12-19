#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "coordinate.h"

/* This header contains functions that only use stateInfo independent data types.
 * They can be used every-where.
 */
constexpr bool inRange(const int value, const int min, const int max);
bool insideCurrentSupercube(const Coordinate & coord, const Coordinate & center, const int & cubesPerDimension, const int & cubeSize);
bool currentlyVisible(const Coordinate & coord, const Coordinate & center, const int & cubesPerDimension, const int & cubeSize);

class Rotation {
public:
    floatCoordinate axis;
    float alpha;

    Rotation() : axis(floatCoordinate(0, 0 ,0)), alpha(0) {}
    Rotation(const floatCoordinate & axis, const float alpha) : axis(axis), alpha(alpha) {}
};

int roundFloat(float number);
int sgn(float number);

float radToDeg(float rad);
float degToRad(float deg);

void rotateAndNormalize(floatCoordinate &v, floatCoordinate axis, float angle);
bool checkTreeParameter(int id, float r, float g, float b, float a);
bool checkNodeParameter(int id, int x, int y, int z);
bool chedNodeID(int id);

#endif // FUNCTIONS_H
