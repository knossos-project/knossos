#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "coordinate.h"

/* This header contains functions that only use stateInfo independent data types.
 * They can be used every-where.
 */

class Rotation {
public:
    floatCoordinate axis;
    float alpha;

    Rotation() : axis(floatCoordinate(0, 0 ,0)), alpha(0) {}
    Rotation(const floatCoordinate & axis, const float alpha) : axis(axis), alpha(alpha) {}
};

int roundFloat(float number);
bool normalizeVector(floatCoordinate & v);
float euclidicNorm(const floatCoordinate & v);
float scalarProduct(const floatCoordinate & v1, const floatCoordinate & v2);
int sgn(float number);

float radToDeg(float rad);
float degToRad(float deg);
floatCoordinate crossProduct(const floatCoordinate &v1, const floatCoordinate &v2);
float vectorAngle(const floatCoordinate &v1, const floatCoordinate &v2);
void rotateAndNormalize(floatCoordinate &v, floatCoordinate axis, float angle);
bool checkTreeParameter(int id, float r, float g, float b, float a);
bool checkNodeParameter(int id, int x, int y, int z);
bool chedNodeID(int id);

#endif // FUNCTIONS_H
