#include "knossos-global.h"
#include "skeletonizer.h"
#include "functions.h"

treeListElement::treeListElement() {

}

treeListElement::treeListElement(int treeID, Color color, QString comment) {
    this->treeID = treeID;

    color4F col;
    col.r = color.r;
    col.g = color.g;
    col.b = color.b;
    col.a = color.a;

    this->color = col;
    strcpy(this->comment, comment.toLocal8Bit().data());
}

treeListElement::treeListElement(int treeID, float r, float g, float b, float a, QString comment) {
    this->treeID = treeID;
    this->color.r = r;
    this->color.g = g;
    this->color.b = b;
    this->color.a = a;
    strcpy(this->comment, comment.toLocal8Bit().data());
}

int treeListElement::getTreeID() {
    return treeID;
}

void treeListElement::setTreeID(int id) {
    treeID = id;
}

nodeListElement *treeListElement::getRoot() {
    return firstNode;
}

QList<nodeListElement *> *treeListElement::getNodes() {
    QList<nodeListElement *> *nodes = new QList<nodeListElement *>();

    nodeListElement *currentNode = firstNode;
    while(currentNode) {
        nodes->append(currentNode);

        currentNode = currentNode->next;

    }


    return nodes;
}

void treeListElement::addNode(nodeListElement *node) {
    if(!checkNodeParameter(node->nodeID, node->position.x, node->position.y, node->position.z)) {
        return;
    }

    Coordinate coordinate(node->position.x, node->position.y, node->position.z);


    /*if(Skeletonizer::addNode(CHANGE_MANUAL, node->nodeID, node->radius, node->getParent()->getTreeID(), &coordinate, node->createdInVp, node->createdInMag, node->timestamp)) {

    } */
}

void treeListElement::addNode(int nodeID, int x, int y, int z) {

}

void treeListElement::addNodes(QList<nodeListElement *> *nodeList) {

}


