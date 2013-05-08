#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "knossos-global.h"

int roundFloat(float number);
bool normalizeVector(floatCoordinate *v);
float euclidicNorm(floatCoordinate *v);
bool normalizeVector(floatCoordinate *v);
float scalarProduct(floatCoordinate *v1, floatCoordinate *v2);
int32_t sgn(float number);

float radToDeg(float rad);
float degToRad(float deg);
floatCoordinate *crossProduct(floatCoordinate *v1, floatCoordinate *v2);
float vectorAngle(floatCoordinate *v1, floatCoordinate *v2);

#endif // FUNCTIONS_H
