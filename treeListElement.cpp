#include "knossos-global.h"
#include "skeletonizer.h"
#include "functions.h"

treeListElement::treeListElement() {

}

treeListElement::treeListElement(int treeID, QString comment, color4F color) {
    this->treeID = treeID;
    this->color = color;
    strcpy(this->comment, comment.toLocal8Bit().data());
}

treeListElement::treeListElement(int treeID, QString comment, float r, float g, float b, float a) {
    this->treeID = treeID;
    this->color = color4F(r, g, b, a);
    strcpy(this->comment, comment.toLocal8Bit().data());
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




