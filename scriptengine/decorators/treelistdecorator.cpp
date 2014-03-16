#include "treelistdecorator.h"
#include "knossos-global.h"

extern stateInfo *state;


TreeListDecorator::TreeListDecorator(QObject *parent) :
    QObject(parent)
{
}

/*
treeListElement *TreeListDecorator::new_treeListElement() {
    return new treeListElement();
}

treeListElement *TreeListDecorator::new_treeListElement(int tree_id, QString comment, color4F color) {
    return new treeListElement(tree_id, comment, color);
}

treeListElement *TreeListDecorator::new_treeListElement(int tree_id, QString comment, float r, float g, float b, float a) {
    return new treeListElement(tree_id, comment, r, g, b, a);
}
*/

int TreeListDecorator::tree_id(treeListElement *self) {
    return self->treeID;
}

char *TreeListDecorator::comment(treeListElement *self) {
    return self->comment;
}

color4F TreeListDecorator::color(treeListElement *self) {
    return self->color;
}

QList<nodeListElement *> *TreeListDecorator::nodes(treeListElement *self) {
    return self->getNodes();
}

nodeListElement *TreeListDecorator::first_node(treeListElement *self) {
    return self->firstNode;
}

QString TreeListDecorator::static_treeListElement_help() {
    return QString("the tree class of knossos. You gain access to the following methods:"
                   "\n tree_id() : returns the id of the current tree"
                   "\n first_node() : returns the first node object of the current tree"
                   "\n comment() : returns the comment of the current tree"
                   "\n color() : returns the color object of the current tree"
                   "\n nodes() : returns a list of node objects of the current tree");
}

/*
void TreeListDecorator::set_tree_id(treeListElement *self, int tree_id) {
    self->treeID = tree_id;
}

void TreeListDecorator::set_comment(treeListElement *self, char *comment) {
    if(strlen(comment) < 8192) {
        strcpy(self->comment, comment);
    }
}

void TreeListDecorator::set_color(treeListElement *self, color4F color) {
    self->color = color;
}

void TreeListDecorator::set_color(treeListElement *self, float red, float green, float blue, float alpha) {
    self->color = color4F(red, green, blue, alpha);
}
*/
