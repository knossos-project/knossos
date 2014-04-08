#include "knossos-global.h"
#include "functions.h"
#include "skeletonizer.h"


NodeListElement::NodeListElement() {

}

NodeListElement::NodeListElement(int nodeID, int x, int y, int z, int parentID, float radius, int inVp, int inMag, int time) {
    this->nodeID = nodeID;
    this->radius = radius;

    this->position = Coordinate(x, y, z);
    this->createdInVp = inVp;
    this->createdInMag = inMag;
    this->timestamp = time;

}

QList<SegmentListElement *> *NodeListElement::getSegments() {
    QList<SegmentListElement *> *segments = new QList<SegmentListElement *>();
    SegmentListElement *currentSegment = firstSegment;
    while(currentSegment) {
        segments->append(currentSegment);
        currentSegment = currentSegment->next;
    }
    return segments;
}


