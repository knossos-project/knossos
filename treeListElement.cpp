
#include "knossos-global.h"

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
