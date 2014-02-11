#ifndef SKELETONIZERDECORATOR_H
#define SKELETONIZERDECORATOR_H

#include <QObject>
#include <QList>
#include <QSet>
#include "knossos-global.h"

extern stateInfo *state;

class SkeletonDecorator : public QObject
{
    Q_OBJECT
public:
    explicit SkeletonDecorator(QObject *parent = 0);

signals:

public slots:
    Skeleton *new_Skeleton();
    QSet<treeListElement *> *trees(Skeleton *self);
    //int skeletonTime();
    //int idleTime();
};

#endif // SKELETONIZERDECORATOR_H
