#ifndef TESTORTHOGONALVIEWPORT_H
#define TESTORTHOGONALVIEWPORT_H

#include <QObject>
#include <QTest>
#include "viewer.h"

class TestOrthogonalViewport : public QObject
{
    Q_OBJECT
public:
    explicit TestOrthogonalViewport(QObject *parent = 0);
    Viewer *reference;
signals:
    
public slots:
    
private slots:
    void testAddNodeAccuracy();
};

#endif // TESTORTHOGONALVIEWPORT_H
