#include "node.h"

#include <coordinate.h>

nodeListElement::nodeListElement(const uint64_t nodeID, const float radius, const Coordinate & position, const int inMag, const ViewportType inVP, const uint64_t ms, const QVariantHash & properties, treeListElement & tree)
        : nodeID{nodeID}, radius{radius}, position{position}, createdInMag{inMag}, createdInVp{inVP}, timestamp{ms}, properties{properties}, correspondingTree{&tree} {}

bool nodeListElement::operator==(const nodeListElement & other) {
    return nodeID == other.nodeID;
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
