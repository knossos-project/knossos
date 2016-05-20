#include "treelistdecorator.h"

#include "scriptengine/proxies/skeletonproxy.h"
#include "skeleton/tree.h"

TreeListDecorator::TreeListDecorator(QObject *parent) :
    QObject(parent)
{
}

int TreeListDecorator::tree_id(treeListElement *self) {
    return self->treeID;
}

QString TreeListDecorator::comment(treeListElement *self) {
    return self->comment;
}

QColor TreeListDecorator::color(treeListElement *self) {
    return self->color;
}

QList<nodeListElement *> *TreeListDecorator::nodes(treeListElement *self) {
    return self->getNodes();
}

nodeListElement *TreeListDecorator::first_node(treeListElement *self) {
    return self->nodes.empty() ? nullptr : &self->nodes.front();
}

QString TreeListDecorator::static_Tree_help() {
    return QString("A read-only class representing a tree of the KNOSSOS skeleton. Access to attributes only via getter."
                   "\n tree_id() : returns the id of the current tree"
                   "\n first_node() : returns the first node object of the current tree"
                   "\n comment() : returns the comment of the current tree"
                   "\n color() : returns the color object of the current tree"
                   "\n nodes() : returns a list of node objects of the current tree");
}
