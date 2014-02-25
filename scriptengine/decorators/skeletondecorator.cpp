#include "skeletondecorator.h"
#include "treelistdecorator.h"
#include <QDebug>
#include <QMutableSetIterator>
#include "knossos-global.h"

extern stateInfo *state;

SkeletonDecorator::SkeletonDecorator(QObject *parent) :
    QObject(parent)
{
}

Skeleton *SkeletonDecorator::new_Skeleton() {
    return new Skeleton();
}

treeListElement *SkeletonDecorator::getFirstTree(Skeleton *self) {
    return self->firstTree;
}

treeListElement *SkeletonDecorator::getActiveTree(Skeleton *self) {
    return self->getFirstTree();
}

void SkeletonDecorator::setActiveTree(Skeleton *self, int treeID) {
    self->setActiveTree(treeID);
}


void SkeletonDecorator::addTree(Skeleton *self, treeListElement *tree) {
    if(self->trees()->contains(tree)) {
        return;
    }

    //self->trees()->append(tree);
}


QList<treeListElement *> *SkeletonDecorator::trees(Skeleton *self) {
    return self->trees();
}

/*
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
*/
/*
int SkeletonDecorator::skeletonTime() {
    return state->skeletonState->skeletonTime;
}

int SkeletonDecorator::idleTime() {
    return state->skeletonState->idleTime;
}*/

