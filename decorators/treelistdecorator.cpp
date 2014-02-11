#include "treelistdecorator.h"

TreeListDecorator::TreeListDecorator(QObject *parent) :
    QObject(parent)
{
}

treeListElement *TreeListDecorator::new_treeListElement() {
    return new treeListElement();
}

int TreeListDecorator::treeID(treeListElement *self) {
    return self->treeID;
}

char *TreeListDecorator::comment(treeListElement *self) {
    return self->comment;
}

void TreeListDecorator::setComment(treeListElement *self, char *comment) {
    if(strlen(comment) < 8192) {
        strcpy(self->comment, comment);
    }
}

void TreeListDecorator::setTreeID(treeListElement *self, int treeID) {
    self->treeID = treeID;
}

