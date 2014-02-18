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
    treeListElement *new_Tree();
    treeListElement *new_Tree(int treeID, Color color, QString comment);
    treeListElement *new_Tree(int treeID, float r, float g, float b, float a, QString comment);
    int treeID(treeListElement *self);
    void setTreeID(treeListElement *self, int treeID);


    //treeListElement *next(treeListElement *self) { return self->next; }
    /*

    char *comment(treeListElement *self);
    void setComment(treeListElement *self, char *comment);
    */

};

#endif // TREELISTDECORATOR_H
