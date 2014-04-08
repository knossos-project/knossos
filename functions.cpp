#include "functions.h"
#include <math.h>

/** this file contains function which are not dependent from any state */

int roundFloat(float number) {
    if(number >= 0) return (int)(number + 0.5);
    else return (int)(number - 0.5);
}

float scalarProduct(FloatCoordinate *v1, FloatCoordinate *v2) {
    return ((v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z));
}

float euclidicNorm(FloatCoordinate *v) {
    return ((float)sqrt((double)scalarProduct(v, v)));
}

bool normalizeVector(FloatCoordinate *v) {
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

FloatCoordinate* crossProduct(FloatCoordinate *v1, FloatCoordinate *v2) {
    FloatCoordinate *result = NULL;
    result = (FloatCoordinate*)malloc(sizeof(FloatCoordinate));
    result->x = v1->y * v2->z - v1->z * v2->y;
    result->y = v1->z * v2->x - v1->x * v2->z;
    result->z = v1->x * v2->y - v1->y * v2->x;
    return result;
}

float vectorAngle(FloatCoordinate *v1, FloatCoordinate *v2) {
    return ((float)acos((double)(scalarProduct(v1, v2)) / (euclidicNorm(v1)*euclidicNorm(v2))));
}

bool checkTreeParameter(int id, float r, float g, float b, float a) {
    if(id < 0 or r < 0 or r > 1 or g < 0 or g > 1 or b < 0 or b > 1 or a < 0 or a > 1) {
        return false;
    }
    return true;
}

bool checkNodeParameter(int id, int x, int y, int z) {
    if(id < 0 or x < 0 or y < 0 or z < 0) {
        return false;
    }
    return true;
}

bool checkNodeParameter(int id) {
    if(id < 0) {
        return false;
    }

    return true;
}

