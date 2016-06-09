#ifndef NODE_H
#define NODE_H

#include "property_query.h"
#include "widgets/viewport.h"

#include <QVariantHash>

#include <cstddef>

class segmentListElement;
class treeListElement;

class nodeListElement : public PropertyQuery {
public:
    std::uint64_t nodeID;
    float radius;
    Coordinate position;
    int createdInMag;
    ViewportType createdInVp;
    uint64_t timestamp;
    std::list<nodeListElement>::iterator iterator;
    treeListElement * correspondingTree = nullptr;

    std::list<segmentListElement> segments;
    // circumsphere radius - max. of length of all segments and node radius.
    //Used for frustum culling
    float circRadius{radius};
    bool isBranchNode{false};
    bool isSynapticNode{false}; //pre- or postSynapse
    bool selected{false};

    nodeListElement(const decltype(nodeID) nodeID, const decltype(radius) radius, const decltype(position) & position, const decltype(createdInMag) inMag
                    , const decltype(createdInVp) inVP, const decltype(timestamp) ms, const decltype(properties) & properties, decltype(*correspondingTree) & tree);
    bool operator==(const nodeListElement & other) const;

    QList<segmentListElement *> *getSegments();
};

class segmentListElement {
public:
    segmentListElement(nodeListElement & source, nodeListElement & target, const bool forward = true) : source{source}, target{target}, forward(forward) {}

    nodeListElement & source;
    nodeListElement & target;
    const bool forward;
    float length{0.0};
    //reference to the segment inside the target node
    std::list<segmentListElement>::iterator sisterSegment;
};
#endif//NODE_H
