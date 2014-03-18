#include "shape.h"
#include "transform.h"
#include "knossos-global.h"

Shape::Shape(uint id, Transform *transform, Coordinate *origin)
{
    if(id)
        this->id = id;
    else {

    }

    if(transform)
        this->transform = transform;
    else {
        this->transform = new Transform();
    }


    if(origin)
        this->origin = origin;
    else {
        this->origin = new Coordinate();
    }
}
