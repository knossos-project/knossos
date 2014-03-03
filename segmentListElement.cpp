#include "knossos-global.h"
#include "functions.h"
#include "skeletonizer.h"

segmentListElement::segmentListElement() {

}

segmentListElement::segmentListElement(int sourceID, int targetID) {
    this->setSource(sourceID);
    this->setTarget(targetID);
}

segmentListElement::segmentListElement(nodeListElement *source, nodeListElement *target) {
    this->source = source;
    this->target = target;


    floatCoordinate coordinate;
    coordinate.x = source->position.x - target->position.x;
    coordinate.y = source->position.y - target->position.y;
    coordinate.z = source->position.z - target->position.z;

    this->length = euclidicNorm(&coordinate);

}

void segmentListElement::setSource(nodeListElement *source) {
   if(source) {
       this->source = source;
   } else {
       qDebug() << "nodeListElement source is NULL";
   }
}

void segmentListElement::setTarget(nodeListElement *target) {
    if(target) {
        this->target = target;
    } else {
        qDebug() << "nodeListElement target is NULL";
    }
}

int segmentListElement::getSourceID() {
    if(this->source)
        return this->source->getNodeID();
    else {
        qDebug() << "no source is set, returning zero";
    }

    return 0;
}

nodeListElement *segmentListElement::getSource() {
    return this->source;
}

int segmentListElement::getTargetID() {
    if(this->target) {
        return this->target->getNodeID();
    } else {
        qDebug() << "no target is set, returning zero";
    }
}

nodeListElement *segmentListElement::getTarget() {
    return this->target;
}

void segmentListElement::setSource(int sourceID) {
      nodeListElement *node = Skeletonizer::findNodeByNodeID(sourceID);
      if(node) {
        this->setSource(node);
      } else {
          qDebug() << "no source node available";
      }
}


void segmentListElement::setTarget(int targetID) {
    nodeListElement *node = Skeletonizer::findNodeByNodeID(targetID);
    if(node) {
        this->setTarget(node);
    } else {
        qDebug() << "no target node available";
    }
}

