#include "knossos-global.h"
#include "skeletonizer.h"
#include "functions.h"

treeListElement::treeListElement() {

}

treeListElement::treeListElement(int treeID, Color color, QString comment) {
    this->setTreeID(treeID);
    this->setColor(color);
    strcpy(this->comment, comment.toLocal8Bit().data());
}

treeListElement::treeListElement(int treeID, float r, float g, float b, float a, QString comment) {
    this->setTreeID(treeID);
    this->setColor(Color(r, g, b, a));
    strcpy(this->comment, comment.toLocal8Bit().data());
}

int treeListElement::getTreeID() {
    return treeID;
}

void treeListElement::setTreeID(int id) {
    if(id < 0 ) {
        qDebug() << "tree id should be positive integer";
        return;
    }

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
        qDebug() << "no negative integer for arguments allowed";
        return;
    }

    Coordinate coordinate(node->position.x, node->position.y, node->position.z);

    if(Skeletonizer::addNode(CHANGE_MANUAL, node->nodeID, node->radius, node->getParent()->getTreeID(), &coordinate, node->createdInVp, node->createdInMag, node->timestamp, false, false)) {

    }
}

void treeListElement::addNode(int nodeID, int x, int y, int z) {
    if(!checkNodeParameter(nodeID, x, y, z)) {
        qDebug() << "no negative integer for arguments allowed";
        return;
    }
}

void treeListElement::addNodes(QList<nodeListElement *> *nodeList) {

}

void treeListElement::setColor(Color color) {
    if(color.r < 0 or color.g < 0 or color.b < 0 or color.a < 0) {
        qDebug() << "one of the color components is in negative range";
    }

    this->color.r = color.r;
    this->color.g = color.g;
    this->color.b = color.b;
    this->color.a = color.a;


}

Color treeListElement::getColor() {
    Color color(this->color.r, this->color.g, this->color.b, this->color.a);
    return color;

}
