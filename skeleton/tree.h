#ifndef TREE_H
#define TREE_H

#include "color4F.h"

#include <QHash>
#include <QList>

#include <memory>

class nodeListElement;
class segmentListElement;

class treeListElement {
public:
    std::unique_ptr<treeListElement> next;
    treeListElement * previous = nullptr;
    std::unique_ptr<nodeListElement> firstNode;

    bool render = true;

    int treeID;
    color4F color;
    bool selected;
    bool colorSetManually;
    std::size_t size;

    char comment[8192];

    QHash<uint64_t, int> subobjectCount;

    QList<nodeListElement *> *getNodes();
    QList<segmentListElement *> getSegments();
};

#endif// TREE_H
