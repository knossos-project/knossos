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
    treeListElement *new_treeListElement();
    treeListElement *new_treeListElement(int tree_id, QString comment, Color4F color);
    treeListElement *new_treeListElement(int tree_id, QString comment, float r = -1, float g = -1, float b = -1, float a = 1);
    */

    Color4F color(TreeListElement *self);
    nodeListElement *first_node(TreeListElement *self);
    QList<nodeListElement *> *nodes(TreeListElement *self);
    int tree_id(TreeListElement *self);
    char *comment(TreeListElement *self);
    static QString static_treeListElement_help();


    /*
    void set_tree_id(treeListElement *self, int tree_id);
    void set_comment(treeListElement *self, char *comment);
    void set_color(treeListElement *self, float red, float green, float blue, float alpha);
    void set_color(treeListElement *self, Color4F color);
    */
};

#endif // TREELISTDECORATOR_H
