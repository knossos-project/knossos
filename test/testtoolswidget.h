#ifndef TESTTOOLSWIDGET_H
#define TESTTOOLSWIDGET_H

#include <QObject>
#include <QTest>
#include <QFile>
#include "viewer.h"

class TestToolsWidget : public QObject
{
    Q_OBJECT
public:
    explicit TestToolsWidget(QObject *parent = 0);
    Viewer *reference;
signals:
    
public slots:

private slots:   
    void testAddTreesPerMouseClick();
    void testAddTreesPerKeyPress();
    void testAddNodesPerMouseClick();
    void testDeleteActiveTreeCaseZero();
    void testDeleteActiveTreeCaseNotZero();
    void testMergeTrees();
    void testSplitConnectedComponents();
    void testRestoreColors();

    // TODO Comments Locking

    
};

#endif // TESTTOOLSWIDGET_H
