#include "testorthogonalviewport.h"
#include "widgets/widgetcontainer.h"
#include "widgets/viewport.h"
#include "widgets/mainwindow.h"
#include "knossos-global.h"
#include "function.h"

extern stateInfo *state;

TestOrthogonalViewport::TestOrthogonalViewport(QObject *parent) :
    QObject(parent)
{
}

/* how to test the space between left left-border and right and operations between them */
void TestOrthogonalViewport::testAddNodeAccuracy() {
    Viewport *firstViewport = reference->vp;

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    Coordinate coord;
    coord.x = reference->window->xField->value();
    coord.y = reference->window->yField->value();
    coord.z = reference->window->zField->value();

    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);

    Coordinate position = state->skeletonState->activeNode->position;

    qDebug() << coord.x << " " << position.x;
    qDebug() << coord.y << " " << position.y;
    qDebug() << coord.z << " " << position.z;

}
