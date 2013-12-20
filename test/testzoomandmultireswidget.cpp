#include "testzoomandmultireswidget.h"
#include "widgets/mainwindow.h"
#include "widgets/widgetcontainer.h"
#include "widgets/zoomandmultireswidget.h"
#include "widgets/viewport.h"
#include <cfloat>

extern stateInfo *state;

TestZoomAndMultiresWidget::TestZoomAndMultiresWidget(QObject *parent) :
    QObject(parent)
{
}

void TestZoomAndMultiresWidget::testZoomOrthoViewport() {
    ZoomAndMultiresWidget *zoomWidget = reference->window->widgetContainer->zoomAndMultiresWidget;
    int random = rand() % 100;
    QVERIFY(random *zoomWidget->orthogonalDataViewportSpinBox->singleStep() == zoomWidget->orthogonalDataViewportSpinBox->value());

}

void TestZoomAndMultiresWidget::testZoomSkeletonViewport() {

}

void TestZoomAndMultiresWidget::testResetZoomFactors() {
   ZoomAndMultiresWidget *zoomWidget = reference->window->widgetContainer->zoomAndMultiresWidget;
   zoomWidget->zoomDefaultsClicked();

   QCOMPARE(zoomWidget->orthogonalDataViewportSpinBox->value(), VPZOOMMIN);
   QCOMPARE(zoomWidget->skeletonViewportSpinBox->value(), 100.);

   for(int i = 0; i < 3; i++)
   QCOMPARE(state->viewerState->vpConfigs[i].texture.zoomLevel, 1.0);
   QCOMPARE(state->skeletonState->zoomLevel, 0.0);

}

void TestZoomAndMultiresWidget::testZoomOrthoViewportByKeyPressed() {
    Viewport *firstViewport = reference->vpUpperLeft;
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    float zoomLevel = state->viewerState->vpConfigs[0].texture.zoomLevel;
    QTest::keyClick(firstViewport, Qt::Key_I, Qt::NoModifier, 50);
    QVERIFY(fabs(state->viewerState->vpConfigs[0].texture.zoomLevel - zoomLevel - 0.1) <= 0.1/*FLT_EPSILON*/); // machine epsilon

    QTest::keyClick(firstViewport, Qt::Key_O, 0, 50);
    float value = fabs(state->viewerState->vpConfigs[0].texture.zoomLevel - zoomLevel);
    qDebug() << value;
    //QVERIFY(fabs(state->viewerState->vpConfigs[0].texture.zoomLevel - zoomLevel) <= 0.1/*FLT_EPSILON*/);
}


void TestZoomAndMultiresWidget::testZoomOrthoViewportByKeyAndMouseCombination() {
    Viewport *firstViewport = reference->vpUpperLeft;
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    float zoomLevel = state->viewerState->vpConfigs[0].texture.zoomLevel;

}
