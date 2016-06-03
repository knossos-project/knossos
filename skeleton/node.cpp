#include "node.h"

#include <coordinate.h>

nodeListElement::nodeListElement(const decltype(nodeID) nodeID, const decltype(radius) radius, const decltype(position) & position, const decltype(createdInMag) inMag
                                 , const decltype(createdInVp) inVP, const decltype(timestamp) ms, const decltype(properties) & properties, decltype(*correspondingTree) & tree)
        : PropertyQuery{properties}, nodeID{nodeID}, radius{radius}, position{position}, createdInMag{inMag}, createdInVp{inVP}, timestamp{ms}, correspondingTree{&tree} {}

bool nodeListElement::operator==(const nodeListElement & other) const {
    return nodeID == other.nodeID;
}

QList<segmentListElement *> *nodeListElement::getSegments() {
    QList<segmentListElement *> * segmentList = new QList<segmentListElement *>();
    for (auto & segment : segments) {
        segmentList->append(&segment);
    }
    return segmentList;
}
