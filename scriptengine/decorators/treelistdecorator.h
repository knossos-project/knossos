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
    /*
    treeListElement *new_Tree();
    treeListElement *new_Tree(int tree_id, QString comment, color4F color);
    treeListElement *new_Tree(int tree_id, QString comment, float r = -1, float g = -1, float b = -1, float a = 1);
    */

    color4F color(treeListElement *self);
    nodeListElement *first_node(treeListElement *self);
    QList<nodeListElement *> *nodes(treeListElement *self);
    int tree_id(treeListElement *self);
    char *comment(treeListElement *self);
    static QString static_Tree_help();


    /*
    void set_tree_id(treeListElement *self, int tree_id);
    void set_comment(treeListElement *self, char *comment);
    void set_color(treeListElement *self, float red, float green, float blue, float alpha);
    void set_color(treeListElement *self, color4F color);
    */
};

#endif // TREELISTDECORATOR_H
