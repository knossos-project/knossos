#include "skeletondecorator.h"
#include <QDebug>
#include <QMutableSetIterator>

SkeletonDecorator::SkeletonDecorator(QObject *parent) :
    QObject(parent)
{
}

Skeleton *SkeletonDecorator::new_Skeleton() {
    return new Skeleton();
}

bool SkeletonDecorator::deleteTree(Skeleton *self, int id) {
    QMutableListIterator<treeListElement *> it(*self->trees);
    while(it.hasNext()) {
        treeListElement *tree = it.next();
        if(tree->treeID == id) {
            it.remove();
            return true;
        }
    }

    return false;
}

bool SkeletonDecorator::addTree(Skeleton *self, treeListElement *tree) {
    if(self->trees->contains(tree)) {
        return false;
    }

    self->trees->append(tree);
    return true;
}

QList<treeListElement *> *SkeletonDecorator::trees(Skeleton *self) {
    return self->trees;
}

/*  */
char *SkeletonDecorator::experimentName(Skeleton *self) {

    return state->name;
}

treeListElement *SkeletonDecorator::firstTree(Skeleton *self) {
    if(self->trees) {
        return self->trees->at(0);
    }

    return 0;
}

treeListElement *SkeletonDecorator::findTree(Skeleton *self, int id) {
    for(int i = 0; i < self->trees->size(); i++) {
        if(self->trees->at(i)->treeID == id) {
            return self->trees->at(i);
        }
    }

    return 0;
}

/*
int SkeletonDecorator::skeletonTime() {
    return state->skeletonState->skeletonTime;
}

int SkeletonDecorator::idleTime() {
    return state->skeletonState->idleTime;
}*/

