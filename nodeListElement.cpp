#include "knossos-global.h"
#include "functions.h"
#include "skeletonizer.h"

extern stateInfo *state;


nodeListElement::nodeListElement() {

}

nodeListElement::nodeListElement(int nodeID, int x, int y, int z, float radius, int inVp, int inMag, int time, char *comment) {
    this->setNodeID(nodeID);
    this->setRadius(radius);
    this->setCoordinate(x, y, z);
    this->setViewport(inVp);
    this->setMagnification(inMag);
    this->setTime(time);
    this->setComment(comment);
}


void nodeListElement::setNodeID(int id) {
    if(id  < 0) {
        qDebug() << "node id should be positive integer";
    }

    this->nodeID = id;
}

int nodeListElement::getNodeID() {
    return nodeID;
}

void nodeListElement::setComment(char *comment) {
    if(!this->comment) {
        this->comment = new commentListElement();
        this->comment->content = comment;
    }
}

char *nodeListElement::getComment() {
    if(comment)
        return this->comment->content;
    return 0;
}

float nodeListElement::getRadius() {
    return radius;
}

void nodeListElement::setRadius(float radius) {
    if(radius > 0)
        this->radius = radius;
    else {
        qDebug() << "node radius should be positive integer";
    }
}

int nodeListElement::getTime() {
    return this->timestamp;
}

void nodeListElement::setTime(int time) {
    if(time >= 0) {
        this->timestamp = time;
    } else {
        qDebug() << "time should be at least zero";
    }
}

void nodeListElement::setCoordinate(int x, int y, int z) {
    if(checkNodeParameter(0, x, y, z)) {
        this->position.x = x;
        this->position.y = y;
        this->position.z = z;
    } else {
        qDebug() << "one of the coordinates are in negative range";
    }
}

void nodeListElement::setCoordinate(Coordinate coordinate) {
    this->position = coordinate;
}

Coordinate nodeListElement::getCoordinate() {
    return this->position;
}

void nodeListElement::setViewport(int viewport) {
    if(viewport < VIEWPORT_XY) {
        qDebug() << "viewport is whether in negative range";
        return;
    }

    if(viewport > VIEWPORT_ARBITRARY) {
        qDebug() << "invalid viewport id";
        return;
    }

    this->createdInVp = viewport;
}

int nodeListElement::getViewport() {
    return this->createdInVp;
}

void nodeListElement::setMagnification(int magnification) {
    if(magnification < 0) {
        qDebug() << "invalid negative magnification";
        return;
    }

    if(magnification > NUM_MAG_DATASETS) {
        qDebug() << "magnification is out of range";
        return;
    }

    if(magnification % 2 == 0 and magnification <= NUM_MAG_DATASETS) {
        this->createdInMag = magnification;
    }
}

int nodeListElement::getMagnification() {
    return this->createdInMag;
}

/*
void nodeListElement::setParent(int treeID) {

}
*/

void nodeListElement::setParent(treeListElement *parent) {
    this->correspondingTree = parent;
}

treeListElement *nodeListElement::getParent() {
    return this->correspondingTree;
}

int nodeListElement::getParentID() {
    if(this->correspondingTree) {
        return this->correspondingTree->treeID;
    }

    return 0;
}


void nodeListElement::addSegment(segmentListElement *segment) {
    if(segment) {
        Skeletonizer::addSegment(CHANGE_MANUAL, segment->source->getNodeID(), segment->target->getNodeID(), false);
    }
}

QList<segmentListElement *> *nodeListElement::getSegments() {
    QList<segmentListElement *> *segments = new QList<segmentListElement *>();
    segmentListElement *currentSegment = firstSegment;
    while(currentSegment) {
        segments->append(currentSegment);
        currentSegment = currentSegment->next;
    }
}

segmentListElement *nodeListElement::getFirstSegment() {
    return firstSegment;
}
