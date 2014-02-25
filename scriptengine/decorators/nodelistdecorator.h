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
    nodeListElement *new_nodeListElement();
    nodeListElement *new_nodeListElement(int nodeID, int x, int, int z, float radius = 1.5, int inVp = 1, int inMag = 1, int time = 1);
    int getNodeID(nodeListElement *self);
    void setNodeID(nodeListElement *self, int nodeID);
    int getTime(nodeListElement *self);
    void setTime(nodeListElement *self, int time);
    float getRadius(nodeListElement *self);
    void setRadius(nodeListElement *self, float radius);

    void setCoordinate(nodeListElement *self, int x, int y, int z);
    void setCoordinate(nodeListElement *self, Coordinate coordinate);
    Coordinate getCoordinate(nodeListElement *self);
    void setViewport(nodeListElement *self, int viewport);
    int getViewport(nodeListElement *self);
    void setMagnification(nodeListElement *self, int magnification);
    int getMagnification(nodeListElement *self);

    void setParent(nodeListElement *self, treeListElement *parent);
    treeListElement *getParent(nodeListElement *self);
    int getParentID(nodeListElement *self);
};

#endif // NODELISTDECORATOR_H
