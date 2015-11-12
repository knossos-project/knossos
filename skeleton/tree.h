#ifndef TREE_H
#define TREE_H

#include "color4F.h"
#include "node.h"

#include <QHash>
#include <QList>

#include <list>
#include <memory>

class segmentListElement;

class treeListElement {
public:
    std::unique_ptr<treeListElement> next;
    treeListElement * previous = nullptr;
    std::list<nodeListElement> nodes;

    bool render = true;

    int treeID;
    color4F color;
    bool selected;
    bool colorSetManually;

    char comment[8192];

    QHash<uint64_t, int> subobjectCount;

    QList<nodeListElement *> *getNodes();
    QList<segmentListElement *> getSegments();
};

#endif// TREE_H
