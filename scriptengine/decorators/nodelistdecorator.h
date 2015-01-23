#ifndef NODELISTDECORATOR_H
#define NODELISTDECORATOR_H

#include "coordinate.h"
#include "nodecommentdecorator.h"
#include "knossos-global.h"

#include <QObject>

class NodeListDecorator : public QObject
{
    template<std::size_t> friend class Coord;
    Q_OBJECT
public:
    explicit NodeListDecorator(QObject *parent = 0);
    
signals:
    
public slots:

    int node_id(nodeListElement *self);
    QList<segmentListElement *> *segments(nodeListElement *self);
    bool is_branch_node(nodeListElement *self);
    commentListElement *comment(nodeListElement *self);
    int time(nodeListElement *self);
    float radius(nodeListElement *self);
    treeListElement *parent_tree(nodeListElement *self);
    Coordinate coordinate(nodeListElement *self);
    int mag(nodeListElement *self);
    int viewport(nodeListElement *self);
    QString static_Node_help();
};

#endif // NODELISTDECORATOR_H
