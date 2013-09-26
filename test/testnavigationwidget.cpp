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

    viewerReference->window->clearSkeletonSlot();
}

// @todo the d/f depending to the added node
void TestNavigationWidget::testJumpFrames() {
    NavigationWidget *navigationWidget = viewerReference->window->widgetContainer->navigationWidget;
    navigationWidget->jumpFramesSpinBox->setValue(20);

    Viewport *firstViewport = viewerReference->vp;
    Viewport *secondViewport = viewerReference->vp2;
    Viewport *thirdViewport = viewerReference->vp3;

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    Coordinate oldCoord;
    oldCoord.x = viewerReference->window->xField->value();
    oldCoord.y = viewerReference->window->yField->value();
    oldCoord.z = viewerReference->window->zField->value();

    // pressing F resulting in a negative translation with the value declared in jumpFrames
    QTest::keyClick(firstViewport, Qt::Key_F,Qt::NoModifier, 500);

    Coordinate newCoord;
    newCoord.x = viewerReference->window->xField->value();
    newCoord.y = viewerReference->window->yField->value();
    newCoord.z = viewerReference->window->zField->value();

    // viewport xy : only z changes
    QCOMPARE(oldCoord.z, newCoord.z - 20);

    // pressing F resulting in a negative translation with the value declared in jumpFrames
    QTest::keyClick(firstViewport, Qt::Key_D,Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->xField->value();
    newCoord.y = viewerReference->window->yField->value();
    newCoord.z = viewerReference->window->zField->value();

    QCOMPARE(oldCoord.z, newCoord.z);

    // viewport yz : only x changes
    pos = secondViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::keyClick(secondViewport, Qt::Key_F, Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->xField->value();
    QCOMPARE(oldCoord.x, newCoord.x - 10);

    QTest::keyClick(secondViewport, Qt::Key_D, Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->xField->value();

    QCOMPARE(oldCoord.x, newCoord.x);

    // viewport xz : only y changes
    pos = thirdViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::keyClick(thirdViewport, Qt::Key_D, Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->yField->value();
    QCOMPARE(oldCoord.y, newCoord.y - 10);

    QTest::keyClick(thirdViewport, Qt::Key_D, Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->yField->value();
    QCOMPARE(oldCoord.y, newCoord.y);

    viewerReference->window->clearSkeletonSlot();
}

/* @todo the d/f depending to the added node  */
void TestNavigationWidget::testWalkFrames() {
    NavigationWidget *navigationWidget = viewerReference->window->widgetContainer->navigationWidget;
    navigationWidget->walkFramesSpinBox->setValue(10);
    Viewport *firstViewport = viewerReference->vp;
    Viewport *secondViewport = viewerReference->vp2;
    Viewport *thirdViewport = viewerReference->vp3;

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    Coordinate oldCoord;
    oldCoord.x = viewerReference->window->xField->value();
    oldCoord.y = viewerReference->window->yField->value();
    oldCoord.z = viewerReference->window->zField->value();

    // pressing R resulting in a negative translation with the value declared in jumpFrames
    QTest::keyClick(firstViewport, Qt::Key_R,Qt::NoModifier, 500);

    Coordinate newCoord;
    newCoord.x = viewerReference->window->xField->value();
    newCoord.y = viewerReference->window->yField->value();
    newCoord.z = viewerReference->window->zField->value();

    // viewport xy : only z changes
    QCOMPARE(oldCoord.z, newCoord.z - 10);

    QTest::keyClick(firstViewport, Qt::Key_E, Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->xField->value();
    newCoord.y = viewerReference->window->yField->value();
    newCoord.z = viewerReference->window->zField->value();

    QCOMPARE(oldCoord.z, newCoord.z);

    // viewport yz : only x changes
    pos = secondViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::keyClick(secondViewport, Qt::Key_R, Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->xField->value();
    QCOMPARE(oldCoord.x, newCoord.x - 10);

    QTest::keyClick(secondViewport, Qt::Key_E, Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->xField->value();

    QCOMPARE(oldCoord.x, newCoord.x);

    // viewport xz : only y changes
    pos = thirdViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::keyClick(thirdViewport, Qt::Key_R, Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->yField->value();
    QCOMPARE(oldCoord.y, newCoord.y - 10);

    QTest::keyClick(thirdViewport, Qt::Key_E, Qt::NoModifier, 500);

    newCoord.x = viewerReference->window->yField->value();
    QCOMPARE(oldCoord.y, newCoord.y);

    viewerReference->window->clearSkeletonSlot();
}
