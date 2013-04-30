#include "functions.h"

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
