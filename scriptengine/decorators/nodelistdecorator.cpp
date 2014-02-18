#include "nodelistdecorator.h"

NodeListDecorator::NodeListDecorator(QObject *parent) :
    QObject(parent)
{
}

nodeListElement *NodeListDecorator::new_Node() {
    return new nodeListElement();
}

nodeListElement *NodeListDecorator::new_Node(int nodeID, float radius, int x, int y, int z, int inVp, int inMag, int time) {
    return new nodeListElement(nodeID, radius, x, y, z, inVp, inMag, time);
}

int NodeListDecorator::getNodeID(nodeListElement *self) {
    return self->nodeID;
}

void NodeListDecorator::setNodeID(nodeListElement *self, int nodeID) {
    self->setNodeID(nodeID);
}

int NodeListDecorator::getTime(nodeListElement *self) {
    return self->getTime();
}

void NodeListDecorator::setTime(nodeListElement *self, int time) {
    self->setTime(time);
}

float NodeListDecorator::getRadius(nodeListElement *self) {
    return self->getRadius();
}

void NodeListDecorator::setRadius(nodeListElement *self, float radius) {
    self->setRadius(radius);
}

/*
nodeListElement *NodeListDecorator::new_nodeListElement() {
    return new nodeListElement();
}
*/
