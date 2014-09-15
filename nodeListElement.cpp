#include "knossos-global.h"
#include "functions.h"
#include "skeletonizer.h"


nodeListElement::nodeListElement() {
}

nodeListElement::nodeListElement(int nodeID, int x, int y, int z, int, float radius, int inVp, int inMag, int time) {
    this->nodeID = nodeID;
    this->radius = radius;

    this->position = Coordinate(x, y, z);
    this->createdInVp = inVp;
    this->createdInMag = inMag;
    this->timestamp = time;
}

QList<segmentListElement *> *nodeListElement::getSegments() {
    QList<segmentListElement *> *segments = new QList<segmentListElement *>();
    segmentListElement *currentSegment = firstSegment;
    while(currentSegment) {
        segments->append(currentSegment);
        currentSegment = currentSegment->next;
    }
    return segments;
}


