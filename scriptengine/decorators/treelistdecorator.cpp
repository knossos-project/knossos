#include "treelistdecorator.h"
#include "knossos-global.h"

TreeListDecorator::TreeListDecorator(QObject *parent) :
    QObject(parent)
{
}

/*
TreeListElement *TreeListDecorator::new_TreeListElement() {
    return new TreeListElement();
}

TreeListElement *TreeListDecorator::new_TreeListElement(int tree_id, QString comment, Color4F color) {
    return new TreeListElement(tree_id, comment, color);
}

TreeListElement *TreeListDecorator::new_TreeListElement(int tree_id, QString comment, float r, float g, float b, float a) {
    return new TreeListElement(tree_id, comment, r, g, b, a);
}
*/

int TreeListDecorator::tree_id(TreeListElement *self) {
    return self->treeID;
}

char *TreeListDecorator::comment(TreeListElement *self) {
    return self->comment;
}

Color4F TreeListDecorator::color(TreeListElement *self) {
    return self->color;
}

QList<NodeListElement *> *TreeListDecorator::nodes(TreeListElement *self) {
    return self->getNodes();
}

NodeListElement *TreeListDecorator::first_node(TreeListElement *self) {
    return self->firstNode;
}

QString TreeListDecorator::static_TreeListElement_help() {
    return QString("A read-only class representing a tree of the KNOSSOS skeleton. Access to attributes only via getter."
                   "\n tree_id() : returns the id of the current tree"
                   "\n first_node() : returns the first node object of the current tree"
                   "\n comment() : returns the comment of the current tree"
                   "\n color() : returns the color object of the current tree"
                   "\n nodes() : returns a list of node objects of the current tree");
}

/*
void TreeListDecorator::set_tree_id(TreeListElement *self, int tree_id) {
    self->treeID = tree_id;
}

void TreeListDecorator::set_comment(TreeListElement *self, char *comment) {
    if(strlen(comment) < 8192) {
        strcpy(self->comment, comment);
    }
}

void TreeListDecorator::set_color(TreeListElement *self, Color4F color) {
    self->color = color;
}

void TreeListDecorator::set_color(TreeListElement *self, float red, float green, float blue, float alpha) {
    self->color = Color4F(red, green, blue, alpha);
}
*/
