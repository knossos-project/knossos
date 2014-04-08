#include "knossos-global.h"
#include "functions.h"
#include "skeletonizer.h"

SegmentListElement::SegmentListElement() {

}

/*
SegmentListElement::SegmentListElement(int sourceID, int targetID) {


}

SegmentListElement::SegmentListElement(NodeListElement *source, NodeListElement *target) {
    this->source = source;
    this->target = target;


    FloatCoordinate coordinate;
    coordinate.x = source->position.x - target->position.x;
    coordinate.y = source->position.y - target->position.y;
    coordinate.z = source->position.z - target->position.z;

    this->length = euclidicNorm(&coordinate);

}
*/

