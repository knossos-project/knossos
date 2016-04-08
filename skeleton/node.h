#ifndef NODE_H
#define NODE_H

#include "widgets/viewport.h"

#include <QVariantHash>

#include <cstddef>

class segmentListElement;
class treeListElement;

class nodeListElement {
public:
    nodeListElement(const uint64_t nodeID, const float radius, const Coordinate & position, const int inMag, const ViewportType inVP, const uint64_t ms, const QVariantHash & properties, treeListElement & tree);
    bool operator==(const nodeListElement & other) const;
    uint64_t nodeID;
    float radius;
    Coordinate position;
    int createdInMag;
    ViewportType createdInVp;
    uint64_t timestamp;
    QVariantHash properties;
    std::list<nodeListElement>::iterator iterator;
    treeListElement * correspondingTree = nullptr;

    std::list<segmentListElement> segments;
    // circumsphere radius - max. of length of all segments and node radius.
    //Used for frustum culling
    float circRadius = radius;
    bool isBranchNode = false;
    bool selected = false;

    QList<segmentListElement *> *getSegments();
    QString getComment() const;
    void setComment(const QString & comment);
};

class segmentListElement {
public:
    segmentListElement(nodeListElement & source, nodeListElement & target, const bool forward = true) : source{source}, target{target}, forward(forward) {}

    nodeListElement & source;
    nodeListElement & target;
    const bool forward;
    float length = 0;
    //reference to the segment inside the target node
    std::list<segmentListElement>::iterator sisterSegment;
};
#endif//NODE_H
