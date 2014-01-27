#ifndef TESTZOOMANDMULTIRESWIDGET_H
#define TESTZOOMANDMULTIRESWIDGET_H

#include <QObject>
#include <QTest>
#include "viewer.h"

class TestZoomAndMultiresWidget : public QObject
{
    Q_OBJECT
public:
    explicit TestZoomAndMultiresWidget(QObject *parent = 0);
    Viewer *reference;
signals:
    
public slots:
    
private slots:
    void testZoomOrthoViewport();
    void testZoomOrthoViewportByKeyPressed();
    void testZoomOrthoViewportByKeyAndMouseCombination();
    void testZoomSkeletonViewport();
    void testResetZoomFactors();
};

#endif // TESTZOOMANDMULTIRESWIDGET_H
