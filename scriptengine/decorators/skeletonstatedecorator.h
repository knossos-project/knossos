#ifndef SKELETONSTATEDECORATOR_H
#define SKELETONSTATEDECORATOR_H

#include <QObject>
#include "knossos-global.h"

extern stateInfo *state;

class SkeletonStateDecorator : public QObject
{
    Q_OBJECT
public:
    explicit SkeletonStateDecorator(QObject *parent = 0);
    
signals:
/*
public slots:
    uint getSkeletonRevision(skeletonState *self);
    bool hasUnsavedChanges(skeletonState *self);
    int getSkeletonTime(skeletonState *self);
    int getSkeletonTimeCorrection(skeletonState *self);

    treeListElement *firstTree(skeletonState *self);
    treeListElement *activeTree(skeletonState *self);
    nodeListElement *activeNode(skeletonState *self);
*/

};

#endif // SKELETONSTATEDECORATOR_H
