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
};

#endif // NODELISTDECORATOR_H
