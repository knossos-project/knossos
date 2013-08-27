#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "knossos-global.h"

/* This header contains functions that only use stateInfo independent data types.
 * They can be used every-where.
 */

int roundFloat(float number);
bool normalizeVector(floatCoordinate *v);
float euclidicNorm(floatCoordinate *v);
bool normalizeVector(floatCoordinate *v);
float scalarProduct(floatCoordinate *v1, floatCoordinate *v2);
int sgn(float number);

float radToDeg(float rad);
float degToRad(float deg);
floatCoordinate *crossProduct(floatCoordinate *v1, floatCoordinate *v2);
float vectorAngle(floatCoordinate *v1, floatCoordinate *v2);

#endif // FUNCTIONS_H
