#ifndef TESTCOMMENTSWIDGET_H
#define TESTCOMMENTSWIDGET_H

#include <QObject>
#include <QtTest/QTest>
#include "../knossos-global.h"
#include "viewer.h"


class TestCommentsWidget : public QObject
{
    Q_OBJECT
public:
    explicit TestCommentsWidget(QObject *parent = 0);
    Viewer *reference;
signals:
    
public slots:

private slots:
    void testEnterComments();
    void testAddNodeComment();

    void testDeleteComments(); // this slot should called at the end.

};

#endif // TESTCOMMENTSWIDGET_H
