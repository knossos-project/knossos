#ifndef SKELETONIZERDECORATOR_H
#define SKELETONIZERDECORATOR_H

#include <QObject>
#include <QList>
#include <QSet>

class Skeleton;
class treeListElement;
class SkeletonDecorator : public QObject
{
    Q_OBJECT
public:
    explicit SkeletonDecorator(QObject *parent = 0);

signals:
    void clearSkeletonSignal();
public slots:
    Skeleton *new_Skeleton();
    void addTree(Skeleton *self, treeListElement *tree);
    QList<treeListElement *> *trees(Skeleton *self);
    treeListElement *getFirstTree(Skeleton *self);
    treeListElement *getActiveTree(Skeleton *self);
    void setActiveTree(Skeleton *self, int treeID);    

    /*
    bool deleteTree(Skeleton *self, int id);
    char *experimentName(Skeleton *self);
    treeListElement *firstTree(Skeleton *self);
    treeListElement *findTree(Skeleton *self, int id);
    */


    //int skeletonTime();
    //int idleTime();
};

#endif // SKELETONIZERDECORATOR_H
