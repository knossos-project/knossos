#include "knossos-global.h"
#include "skeletonizer.h"
#include "functions.h"

TreeListElement::TreeListElement() {

}

TreeListElement::TreeListElement(int treeID, QString comment, Color4F color) {
    this->treeID = treeID;
    this->color = color;
    strcpy(this->comment, comment.toLocal8Bit().data());
}

TreeListElement::TreeListElement(int treeID, QString comment, float r, float g, float b, float a) {
    this->treeID = treeID;
    this->color = Color4F(r, g, b, a);
    strcpy(this->comment, comment.toLocal8Bit().data());
}

QList<nodeListElement *> *TreeListElement::getNodes() {
    QList<nodeListElement *> *nodes = new QList<nodeListElement *>();

    nodeListElement *currentNode = firstNode;
    while(currentNode) {
        nodes->append(currentNode);
        currentNode = currentNode->next;
    }


    return nodes;
}




