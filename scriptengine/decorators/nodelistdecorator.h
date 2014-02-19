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
    nodeListElement *new_Node();
    nodeListElement *new_Node(int nodeID, float radius, int x, int y, int z, int inVp, int inMag, int time);
    int getNodeID(nodeListElement *self);
    void setNodeID(nodeListElement *self, int nodeID);
    int getTime(nodeListElement *self);
    void setTime(nodeListElement *self, int time);
    float getRadius(nodeListElement *self);
    void setRadius(nodeListElement *self, float radius);

    void setCoordinate(nodeListElement *self, int x, int y, int z);
    void setCoordinate(nodeListElement *self, Coordinate coordinate);
    void getCoordinate(nodeListElement *self);
    void setViewport(nodeListElement *self, int viewport);
    void getViewport(nodeListElement *self);
    void setMagnification(nodeListElement *self, int magnification);
    void getMagnification(nodeListElement *self);

    void setParent(nodeListElement *self, treeListElement *parent);
    treeListElement *getParent(nodeListElement *self);
    int getParentID(nodeListElement *self);
};

#endif // NODELISTDECORATOR_H
