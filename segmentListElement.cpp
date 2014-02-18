#include "knossos-global.h"
#include "functions.h"

segmentListElement::segmentListElement() {

}

segmentListElement::segmentListElement(nodeListElement *source, nodeListElement *target, Byte flag) {
    this->source = source;
    this->target = target;
    this->flag = flag;

    floatCoordinate coordinate;
    coordinate.x = source->position.x - target->position.x;
    coordinate.y = source->position.y - target->position.y;
    coordinate.z = source->position.z - target->position.z;

    this->length = euclidicNorm(&coordinate);

}
