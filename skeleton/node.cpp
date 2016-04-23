#include "node.h"

#include <coordinate.h>

nodeListElement::nodeListElement(const std::uint64_t nodeID, const float radius, const Coordinate & position, const int inMag, const ViewportType inVP, const uint64_t ms, const QVariantHash &properties, treeListElement & tree)
        : nodeID{nodeID}, radius{radius}, position{position}, createdInMag{inMag}, createdInVp{inVP}, timestamp{ms}, properties{properties}, correspondingTree{&tree} {}

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

QString nodeListElement::getComment() const {
    return properties.value("comment").toString();
}

void nodeListElement::setComment(const QString & comment) {
    properties.insert("comment", comment);
}
