#ifndef NODELISTDECORATOR_H
#define NODELISTDECORATOR_H

#include <QObject>
#include "knossos-global.h"

class NodeListDecorator : public QObject
{
    Q_OBJECT
public:
    explicit NodeListDecorator(QObject *parent = 0);
    
signals:
    
public slots:
    /*
    NodeListElement *new_NodeListElement();
    NodeListElement *new_NodeListElement(int nodeID, int x, int y, int z, int parentID = 0, float radius = 1.5, int inVp = 0, int inMag = 1, int time = 0);
    */

    int node_id(NodeListElement *self);
    QList<SegmentListElement *> *segments(NodeListElement *self);
    bool is_branch_node(NodeListElement *self);
    char *comment(NodeListElement *self);
    int time(NodeListElement *self);
    float radius(NodeListElement *self);
    TreeListElement *parent_tree(NodeListElement *self);
    Coordinate coordinate(NodeListElement *self);
    int mag(NodeListElement *self);
    int viewport(NodeListElement *self);
    QString static_NodeListElement_help();
    /*
    void set_node_id(NodeListElement *self, int node_id);
    void set_comment(NodeListElement *self, char *comment);
    void set_time(NodeListElement *self, int time);
    void set_radius(NodeListElement *self, float radius);
    void set_coordinate(NodeListElement *self, int x, int y, int z);
    void set_viewport(NodeListElement *self, int viewport);
    void set_mag(NodeListElement *self, int magnification);


    void set_parent_tree_id(NodeListElement *self, int id);
    void set_coordinate(NodeListElement *self, Coordinate coordinate);
    void set_parent_tree(NodeListElement *self, TreeListElement *parent_tree);
    */

};

#endif // NODELISTDECORATOR_H
