#ifndef TREELISTDECORATOR_H
#define TREELISTDECORATOR_H

#include <QObject>
#include "knossos-global.h"

extern stateInfo *state;

class TreeListDecorator : public QObject
{
    Q_OBJECT
public:
    explicit TreeListDecorator(QObject *parent = 0);
    
signals:
    
public slots:
    treeListElement *new_treeListElement();
    treeListElement *new_treeListElement(int treeID, Color color, QString comment);
    treeListElement *new_treeListElement(int treeID, float r, float g, float b, float a, QString comment);
    int getTreeID(treeListElement *self);
    void setTreeID(treeListElement *self, int treeID);
    nodeListElement *getRoot(treeListElement *self);
    QList<nodeListElement *> *getNodes(treeListElement *self);

    void addNode(treeListElement *self, nodeListElement *node);
    void addNode(treeListElement *self, int nodeID, Coordinate coordinate, QString comment);
    void addNodes(treeListElement *self, QList<nodeListElement *> *nodeList);

    //treeListElement *next(treeListElement *self) { return self->next; }
    /*

    char *comment(treeListElement *self);
    void setComment(treeListElement *self, char *comment);
    */

};

#endif // TREELISTDECORATOR_H
