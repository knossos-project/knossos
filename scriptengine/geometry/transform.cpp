#include "transform.h"

Transform::Transform(Coordinate *translate, Coordinate *rotate, Coordinate *scale)
{
    this->translate = translate;
    this->rotate = rotate;
    this->scale = scale;
}
