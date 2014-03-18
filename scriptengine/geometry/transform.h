#ifndef TRANSFORM_H
#define TRANSFORM_H

class Coordinate;
class Transform
{
public:
    Transform(Coordinate *translate = 0, Coordinate *rotate = 0, Coordinate *scale = 0);
    Coordinate *translate;
    Coordinate *rotate;
    Coordinate *scale;

};

#endif // TRANSFORM_H
