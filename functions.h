#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "knossos-global.h"

/* This header contains functions that only use stateInfo independent data types.
 * They can be used every-where.
 */

int roundFloat(float number);
bool normalizeVector(FloatCoordinate *v);
float euclidicNorm(FloatCoordinate *v);
bool normalizeVector(FloatCoordinate *v);
float scalarProduct(FloatCoordinate *v1, FloatCoordinate *v2);
int sgn(float number);

float radToDeg(float rad);
float degToRad(float deg);
FloatCoordinate *crossProduct(FloatCoordinate *v1, FloatCoordinate *v2);
float vectorAngle(FloatCoordinate *v1, FloatCoordinate *v2);
bool checkTreeParameter(int id, float r, float g, float b, float a);
bool checkNodeParameter(int id, int x, int y, int z);
bool chedNodeID(int id);

#endif // FUNCTIONS_H
