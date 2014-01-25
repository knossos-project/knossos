#ifndef TESTSKELETONVIEWPORT_H
#define TESTSKELETONVIEWPORT_H

#include <QObject>
#include "viewer.h"
#include <QTest>

class TestSkeletonViewport : public QObject
{
    Q_OBJECT
public:
    explicit TestSkeletonViewport(QObject *parent = 0);
    Viewer *reference;
signals:
    
public slots:
    
private slots:
    void testTranslation();
};

#endif // TESTSKELETONVIEWPORT_H
