#ifndef TESTSKELETONLOADANDSAVE_H
#define TESTSKELETONLOADANDSAVE_H

#include <QObject>
#include "viewer.h"

class TestSkeletonLoadAndSave : public QObject
{
    Q_OBJECT
public:
    explicit TestSkeletonLoadAndSave(QObject *parent = 0);
    Viewer *reference;
signals:
    
public slots:

private slots:
    void testLoadSkeleton();
    void testSaveSkeleton();
    
};

#endif // TESTSKELETONLOADANDSAVE_H
