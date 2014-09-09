#include "point.h"
#include "shape.h"

Point::Point(Transform *transform, Coordinate *origin, uint size, color4F *color) {
    this->transform = transform;
    this->origin = origin;
    this->size = size;
    this->color = color;
}
