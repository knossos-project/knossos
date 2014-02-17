#include "skeletonstatedecorator.h"

SkeletonStateDecorator::SkeletonStateDecorator(QObject *parent) :
    QObject(parent)
{
}

uint SkeletonStateDecorator::getSkeletonRevision(skeletonState *self) {
    return self->skeletonRevision;
}

bool SkeletonStateDecorator::hasUnsavedChanges(skeletonState *self) {
    return self->unsavedChanges;
}

int SkeletonStateDecorator::getSkeletonTime(skeletonState *self) {
    return self->skeletonTime;
}

int SkeletonStateDecorator::getSkeletonTimeCorrection(skeletonState *self) {
    return self->skeletonTimeCorrection;
}

treeListElement *SkeletonStateDecorator::firstTree(skeletonState *self) {
    return self->firstTree;
}

treeListElement *SkeletonStateDecorator::activeTree(skeletonState *self) {
    return self->activeTree;
}

nodeListElement *SkeletonStateDecorator::activeNode(skeletonState *self) {
    return self->activeNode;
}
