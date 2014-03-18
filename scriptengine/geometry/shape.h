#ifndef SHAPE_H
#define SHAPE_H
#include <qobject.h>

class Coordinate;
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
