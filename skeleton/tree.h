#ifndef TREE_H
#define TREE_H

#include "color4F.h"

#include <QList>

class nodeListElement;
class segmentListElement;

class treeListElement {
public:
    treeListElement();
    treeListElement(int treeID, QString comment, color4F color);
    treeListElement(int treeID, QString comment, float r, float g, float b, float a);

    treeListElement *next;
    treeListElement *previous;
    nodeListElement *firstNode;

    bool render=true;

    int treeID;
    color4F color;
    bool selected;
    int colorSetManually;
    std::size_t size;

    char comment[8192];
    QList<nodeListElement *> *getNodes();
    QList<segmentListElement *> getSegments();
};

#endif// TREE_H
