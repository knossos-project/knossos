#include "nodelistdecorator.h"
#include "knossos-global.h"

NodeListDecorator::NodeListDecorator(QObject *parent) :
    QObject(parent)
{
}

nodeListElement *NodeListDecorator::new_nodeListElement() {
    return new nodeListElement();
}

nodeListElement *NodeListDecorator::new_nodeListElement(int nodeID, int x, int y, int z, float radius, int inVp, int inMag, int time) {
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

void NodeListDecorator::setCoordinate(nodeListElement *self, int x, int y, int z) {
    self->position.x = x;
    self->position.y = y;
    self->position.z = z;
}

void NodeListDecorator::setCoordinate(nodeListElement *self, Coordinate coordinate) {
    self->position = coordinate;
}

Coordinate NodeListDecorator::getCoordinate(nodeListElement *self) {
    return self->position;
}

void NodeListDecorator::setViewport(nodeListElement *self, int viewport) {
    self->createdInVp = viewport;
}

int NodeListDecorator::getViewport(nodeListElement *self) {
    return self->createdInVp;
}

void NodeListDecorator::setMagnification(nodeListElement *self, int magnification) {
    self->createdInMag = magnification;
}

int NodeListDecorator::getMagnification(nodeListElement *self) {
    return self->createdInMag;
}

void NodeListDecorator::setParent(nodeListElement *self, treeListElement *parent) {
    self->setParent(parent);
}

treeListElement *NodeListDecorator::getParent(nodeListElement *self) {
    return self->getParent();
}

int NodeListDecorator::getParentID(nodeListElement *self) {
    return self->getParentID();
}
