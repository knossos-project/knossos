#ifndef NODE_H
#define NODE_H

#include <widgets/viewport.h>

#include <cstddef>

class segmentListElement;
class treeListElement;

class nodeListElement {
public:
    nodeListElement();
    nodeListElement(int nodeID, int x, int y, int z, int parentID, float radius, ViewportType inVp, int inMag, int time);

    nodeListElement *next;
    nodeListElement *previous;

    segmentListElement *firstSegment;

    treeListElement *correspondingTree;

    float radius;

    //can be VIEWPORT_XY, ...
    ViewportType createdInVp;
    int createdInMag;
    int timestamp;

    class commentListElement *comment;

    // counts forward AND backward segments!!!
    int numSegs;

    // circumsphere radius - max. of length of all segments and node radius.
    //Used for frustum culling
    float circRadius;
    uint64_t nodeID;
    Coordinate position;
    bool isBranchNode;
    bool selected;

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
