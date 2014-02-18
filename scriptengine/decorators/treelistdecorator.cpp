#include "treelistdecorator.h"

TreeListDecorator::TreeListDecorator(QObject *parent) :
    QObject(parent)
{
}

treeListElement *TreeListDecorator::new_Tree() {
    return new treeListElement();
}

treeListElement *TreeListDecorator::new_Tree(int treeID, Color color, QString comment) {
    return new treeListElement(treeID, color, comment);
}

treeListElement *TreeListDecorator::new_Tree(int treeID, float r, float g, float b, float a, QString comment) {
    return new treelistElement(treeID, r, g, b, a, comment);
}

int TreeListDecorator::treeID(treeListElement *self) {
    return self->treeID;
}

void TreeListDecorator::setTreeID(treeListElement *self, int treeID) {
    self->treeID = treeID;
}

/*
char *TreeListDecorator::comment(treeListElement *self) {
    return self->comment;
}
*/

/*
void TreeListDecorator::setComment(treeListElement *self, char *comment) {
    if(strlen(comment) < 8192) {
        strcpy(self->comment, comment);
    }
}



*/
