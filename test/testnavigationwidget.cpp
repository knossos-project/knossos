#include <QTime>
#include "testnavigationwidget.h"
#include "widgets/mainwindow.h"
#include "widgets/widgetcontainer.h"
#include "widgets/viewport.h"

extern stateInfo *state;

TestNavigationWidget::TestNavigationWidget(QObject *parent) :
    QObject(parent)
{
}

void TestNavigationWidget::testMovementSpeed() {
    NavigationWidget *navigationWidget = viewerReference->window->widgetContainer->navigationWidget;
    navigationWidget->movementSpeedSpinBox->setValue(20);

    int steps = 1000 / 20.0;

    Viewport *firstViewport = viewerReference->vp;

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 2);
    pos.setY(pos.y() + 2);

    Coordinate coord;
    coord.x = viewerReference->window->xField->value();
    coord.y = viewerReference->window->yField->value();
    coord.z = viewerReference->window->zField->value();

    QTime bench;
    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);
    bench.start();


    int msec = bench.elapsed();
    qDebug() << msec;
}
