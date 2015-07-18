#ifndef NODE_H
#define NODE_H

#include "widgets/viewport.h"

#include <QVariantHash>

#include <cstddef>

class commentListElement;
class segmentListElement;
class treeListElement;

class nodeListElement {
public:
    nodeListElement(const uint64_t nodeID, const float radius, const Coordinate & position, const int inMag, const ViewportType inVP, const uint64_t ms, const QVariantHash & properties, treeListElement & tree);

    uint64_t nodeID;
    float radius;
    Coordinate position;
    int createdInMag;
    ViewportType createdInVp;
    uint64_t timestamp;
    QVariantHash properties;
    treeListElement * correspondingTree = nullptr;

    std::unique_ptr<nodeListElement> next = nullptr;
    nodeListElement * previous = nullptr;
    segmentListElement * firstSegment = nullptr;
    int numSegs;// counts forward AND backward segments!!!
    commentListElement * comment = nullptr;
    // circumsphere radius - max. of length of all segments and node radius.
    //Used for frustum culling
    float circRadius = radius;
    bool isBranchNode = false;
    bool selected = false;

    QList<segmentListElement *> *getSegments();
};

#define SEGMENT_FORWARD true
#define SEGMENT_BACKWARD false

class segmentListElement {
public:
    segmentListElement() {}
    segmentListElement *next;
    segmentListElement *previous;

    //Contains the reference to the segment inside the target node
    segmentListElement *reverseSegment;

    // Use SEGMENT_FORWARD and SEGMENT_BACKWARD.
    bool flag;
    float length;
    //char *comment;

    //Those coordinates are not the same as the coordinates of the source / target nodes
    //when a segment crosses the skeleton DC boundaries. Then these coordinates
    //lie at the skeleton DC borders. This applies only, when we use the segment inside
    //a skeleton DC, not inside of a tree list.
    //Coordinate pos1;
    //Coordinate pos2;

    nodeListElement *source;
    nodeListElement *target;
};

class commentListElement {
public:
    commentListElement *next;
    commentListElement *previous;

    char *content;

    nodeListElement *node;
};

#endif//NODE_H
