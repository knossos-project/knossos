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
    this->radius = radius;
}

int nodeListElement::getTime() {
    return this->timestamp;
}

void nodeListElement::setTime(int time) {
    this->timestamp = time;
}
