#include "transformdecorator.h"
#include "../geometry/transform.h"

TransformDecorator::TransformDecorator()
{
}

Transform *TransformDecorator::new_Transform(Coordinate *translate, Coordinate *rotate, Coordinate *scale) {
    return new Transform(translate, rotate, scale);
}
