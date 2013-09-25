#ifndef TESTNAVIGATIONWIDGET_H
#define TESTNAVIGATIONWIDGET_H

#include <QObject>
#include <QTest>
#include "viewer.h"
#include "remote.h"

class TestNavigationWidget : public QObject
{
    Q_OBJECT
public:
    explicit TestNavigationWidget(QObject *parent = 0);
    Viewer *viewerReference;
    Remote *remoteReference;
signals:
    
public slots:
    
private slots:
    void testMovementSpeed();
};

#endif // TESTNAVIGATIONWIDGET_H
