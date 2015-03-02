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

    Rotation() {
        SET_COORDINATE(axis, 0, 0, 0);
        alpha = 0;
    }

    Rotation(float x, float y, float z, float alpha) : alpha(alpha){
        SET_COORDINATE(axis, x, y, z);
    }

    void setRotation(float x, float y, float z, float alpha) {
        SET_COORDINATE(axis, x, y, z);
        this->alpha = alpha;
    }
};

int roundFloat(float number);
bool normalizeVector(floatCoordinate *v);
float euclidicNorm(floatCoordinate *v);
bool normalizeVector(floatCoordinate *v);
float scalarProduct(floatCoordinate *v1, floatCoordinate *v2);
int sgn(float number);

float radToDeg(float rad);
float degToRad(float deg);
floatCoordinate crossProduct(floatCoordinate *v1, floatCoordinate *v2);
float vectorAngle(floatCoordinate *v1, floatCoordinate *v2);
void rotateAndNormalize(floatCoordinate &v, floatCoordinate axis, float angle);
bool checkTreeParameter(int id, float r, float g, float b, float a);
bool checkNodeParameter(int id, int x, int y, int z);
bool chedNodeID(int id);

#endif // FUNCTIONS_H
