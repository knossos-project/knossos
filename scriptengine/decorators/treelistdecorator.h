#ifndef TREELISTDECORATOR_H
#define TREELISTDECORATOR_H

#include <QObject>
#include <QList>
#include "knossos-global.h"

class TreeListDecorator : public QObject
{
    Q_OBJECT
public:
    explicit TreeListDecorator(QObject *parent = 0);
    
signals:
    
public slots:
    color4F color(treeListElement *self);
    nodeListElement *first_node(treeListElement *self);
    QList<nodeListElement *> *nodes(treeListElement *self);
    int tree_id(treeListElement *self);
    char *comment(treeListElement *self);
    static QString static_Tree_help();
};

#endif // TREELISTDECORATOR_H
