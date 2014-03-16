#include "knossos-global.h"
#include "functions.h"
#include "skeletonizer.h"

segmentListElement::segmentListElement() {

}

/*
segmentListElement::segmentListElement(int sourceID, int targetID) {


}

segmentListElement::segmentListElement(nodeListElement *source, nodeListElement *target) {
    this->source = source;
    this->target = target;


    floatCoordinate coordinate;
    coordinate.x = source->position.x - target->position.x;
    coordinate.y = source->position.y - target->position.y;
    coordinate.z = source->position.z - target->position.z;

    this->length = euclidicNorm(&coordinate);

}
*/

