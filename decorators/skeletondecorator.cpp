#include "skeletondecorator.h"

SkeletonDecorator::SkeletonDecorator(QObject *parent) :
    QObject(parent)
{
}

Skeleton *SkeletonDecorator::new_Skeleton() {
    return new Skeleton();
}

QSet<treeListElement *> *SkeletonDecorator::trees(Skeleton *self) {
    return self->trees;
}

/*
int SkeletonDecorator::skeletonTime() {
    return state->skeletonState->skeletonTime;
}

int SkeletonDecorator::idleTime() {
    return state->skeletonState->idleTime;
}*/

