#include "knossos-global.h"

nodeListElement::nodeListElement() {

}

nodeListElement::nodeListElement(int nodeID, float radius, int x, int y, int z, int inVp, int inMag, int time) {
    this->nodeID = nodeID;
    this->radius = radius;
    this->position.x = x;
    this->position.y = y;
    this->position.z = z;
    this->createdInVp = inVp;
    this->createdInMag = inMag;
    this->timestamp = time;
}


void nodeListElement::setNodeID(int id) {
    this->nodeID = id;
}

int nodeListElement::getNodeID() {
    return nodeID;
}

float nodeListElement::getRadius() {
    return radius;
}

void nodeListElement::setRadius(float radius) {
    if(radius > 0)
        this->radius = radius;
}

int nodeListElement::getTime() {
    return this->timestamp;
}

void nodeListElement::setTime(int time) {
    this->timestamp = time;
}

void nodeListElement::setCoordinate(int x, int y, int z) {
    this->position.x = x;
    this->position.y = y;
    this->position.z = z;
}

void nodeListElement::setCoordinate(Coordinate coordinate) {
    this->position = coordinate;
}

void nodeListElement::getCoordinate() {
    return this->position;
}

void nodeListElement::setViewport(int viewport) {
    if(viewport < VIEWPORT_XY | viewport > VIEWPORT_ARBITRARY) {
        return;
    }
    this->createdInVp = viewport;
}

void nodeListElement::getViewport() {
    return this->createdInVp;
}

void nodeListElement::setMagnification(int magnification) {
    if(magnification % 2 == 0 and magnication <= 8) {
        this->createdInMag = magnification;
    }
}

void nodeListElement::getMagnification() {
    return this->createdInMag;
}

/*
void nodeListElement::setParent(int treeID) {

}
*/

void nodeListElement::setParent(treeListElement *parent) {
    this->correspondingTree = parent;
}

int nodeListElement::getParent() {
    return this->correspondingTree;
}

int nodeListElement::getParentID() {
    if(this->correspondingTree) {
        return this->correspondingTree->treeID;
    }

    return 0;
}
