#ifndef TESTCOMMENTSWIDGET_H
#define TESTCOMMENTSWIDGET_H

#include <QObject>
#include <QTest>
#include "../knossos-global.h"
#include "viewer.h"

class TestCommentsWidget : public QObject
{
    Q_OBJECT
public:
    explicit TestCommentsWidget(QObject *parent = 0);
    Viewer *reference;
signals:
    void ready();
public slots:
private slots:
    void testEnterComments();
    void testAddNodeComment();
    void testEnableConditionalColoring();
    void testEnableConditionalRadius();
    void testCommentsTable();
    void testCommentsTablePerformance();
    void testDeleteComments(); // this slot should be called at the end.
    void endTestCase();
};
#endif // TESTCOMMENTSWIDGET_H
