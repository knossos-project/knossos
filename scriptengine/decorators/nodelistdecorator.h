#ifndef NODELISTDECORATOR_H
#define NODELISTDECORATOR_H

#include <QObject>
#include "knossos-global.h"

class NodeListDecorator : public QObject
{
    friend class Coordinate;
    Q_OBJECT
public:
    explicit NodeListDecorator(QObject *parent = 0);
    
signals:
    
public slots:
    /*
    nodeListElement *new_NodeListElement();
    nodeListElement *new_NodeListElement(int nodeID, int x, int y, int z, int parentID = 0, float radius = 1.5, int inVp = 0, int inMag = 1, int time = 0);
    */

    int node_id(nodeListElement *self);
    QList<segmentListElement *> *segments(nodeListElement *self);
    bool is_branch_node(nodeListElement *self);
    char *comment(nodeListElement *self);
    int time(nodeListElement *self);
    float radius(nodeListElement *self);
    treeListElement *parent_tree(nodeListElement *self);
    Coordinate coordinate(nodeListElement *self);
    int mag(nodeListElement *self);
    int viewport(nodeListElement *self);
    QString static_nodeListElement_help();
    /*
    void set_node_id(nodeListElement *self, int node_id);
    void set_comment(nodeListElement *self, char *comment);
    void set_time(nodeListElement *self, int time);
    void set_radius(nodeListElement *self, float radius);
    void set_coordinate(nodeListElement *self, int x, int y, int z);
    void set_viewport(nodeListElement *self, int viewport);
    void set_mag(nodeListElement *self, int magnification);


    void set_parent_tree_id(nodeListElement *self, int id);
    void set_coordinate(nodeListElement *self, Coordinate Coordinate);
    void set_parent_tree(nodeListElement *self, treeListElement *parent_tree);
    */

};

#endif // NODELISTDECORATOR_H
