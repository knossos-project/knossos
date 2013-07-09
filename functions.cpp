#include "functions.h"
#include <math.h>

int roundFloat(float number) {
    if(number >= 0) return (int)(number + 0.5);
    else return (int)(number - 0.5);
}

float scalarProduct(floatCoordinate *v1, floatCoordinate *v2) {
    return ((v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z));
}

float euclidicNorm(floatCoordinate *v) {
    return ((float)sqrt((double)scalarProduct(v, v)));
}

bool normalizeVector(floatCoordinate *v) {
    float norm = euclidicNorm(v);
    v->x /= norm;
    v->y /= norm;
    v->z /= norm;
    return true;
}

int sgn(float number) {
    if(number > 0.) return 1;
    else if(number == 0.) return 0;
    else return -1;
}


//Some math helper functions
float radToDeg(float rad) {
    return ((180. * rad) / PI);
}

float degToRad(float deg) {
    return ((deg / 180.) * PI);
}

floatCoordinate* crossProduct(floatCoordinate *v1, floatCoordinate *v2) {
    floatCoordinate *result = NULL;
    result = (floatCoordinate*)malloc(sizeof(floatCoordinate));
    result->x = v1->y * v2->z - v1->z * v2->y;
    result->y = v1->z * v2->x - v1->x * v2->z;
    result->z = v1->x * v2->y - v1->y * v2->x;
    return result;
}

float vectorAngle(floatCoordinate *v1, floatCoordinate *v2) {
    return ((float)acos((double)(scalarProduct(v1, v2)) / (euclidicNorm(v1)*euclidicNorm(v2))));
}
