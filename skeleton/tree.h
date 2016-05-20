#ifndef TREE_H
#define TREE_H

#include "node.h"

#include <QColor>
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

    QColor color;
    bool selected{false};
    bool colorSetManually{false};

    QString comment;

    QHash<uint64_t, int> subobjectCount;

    treeListElement(int id);

    QList<nodeListElement *> *getNodes();
    QList<segmentListElement *> getSegments();
};

#endif// TREE_H
