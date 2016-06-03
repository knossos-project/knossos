#ifndef TREE_H
#define TREE_H

#include "node.h"
#include "property_query.h"

#include <QColor>
#include <QHash>
#include <QList>

#include <list>
#include <memory>

class segmentListElement;

class treeListElement : public PropertyQuery {
public:
    int treeID;
    std::list<treeListElement>::iterator iterator;

    std::list<nodeListElement> nodes;

    bool render{true};
    bool isSynapticCleft{false};

    QColor color;
    bool selected{false};
    bool colorSetManually{false};

    QHash<uint64_t, int> subobjectCount;

    treeListElement(const decltype(treeID) id, const decltype(PropertyQuery::properties) & properties);

    QList<nodeListElement *> *getNodes();
    QList<segmentListElement *> getSegments();
};

#endif// TREE_H
