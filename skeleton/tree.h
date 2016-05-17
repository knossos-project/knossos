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
    int treeID;
    std::list<treeListElement>::iterator iterator;

    std::list<nodeListElement> nodes;

    bool render{true};

    color4F color;
    bool selected{false};
    bool colorSetManually{false};

    QString comment;

    QHash<uint64_t, int> subobjectCount;

    treeListElement(int id);

    QList<nodeListElement *> *getNodes();
    QList<segmentListElement *> getSegments();
};

#endif// TREE_H
