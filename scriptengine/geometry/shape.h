#ifndef SHAPE_H
#define SHAPE_H

#include "coordinate.h"

#include <qobject.h>

class Transform;
class Shape
{
public:
    Shape(uint id = 0, Transform *transform = 0, Coordinate *origin = 0);
    Transform *transform;
    Coordinate *origin;
    uint id;


};

#endif // SHAPE_H
