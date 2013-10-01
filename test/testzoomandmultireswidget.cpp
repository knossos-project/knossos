#include "testzoomandmultireswidget.h"
#include "widgets/mainwindow.h"
#include "widgets/widgetcontainer.h"
#include "widgets/zoomandmultireswidget.h"
#include "widgets/viewport.h"

extern stateInfo *state;

TestZoomAndMultiresWidget::TestZoomAndMultiresWidget(QObject *parent) :
    QObject(parent)
{
}

void TestZoomAndMultiresWidget::testZoomOrthoViewport() {
    ZoomAndMultiresWidget *zoomWidget = reference->window->widgetContainer->zoomAndMultiresWidget;
    zoomWidget->orthogonalDataViewportSlider->setValue(80);

    QVERIFY(80 == zoomWidget->orthogonalDataViewportSpinBox->value());

}

void TestZoomAndMultiresWidget::testZoomSkeletonViewport() {

}

void TestZoomAndMultiresWidget::testResetZoomFactors() {
   ZoomAndMultiresWidget *zoomWidget = reference->window->widgetContainer->zoomAndMultiresWidget;
   zoomWidget->zoomDefaultsClicked();

   QCOMPARE(zoomWidget->orthogonalDataViewportSpinBox->value(), 1.0);
   QCOMPARE(zoomWidget->orthogonalDataViewportSlider->value(), 1);
   QCOMPARE(zoomWidget->skeletonViewportSpinBox->value(), 0.0);
   QCOMPARE(zoomWidget->skeletonViewportSlider->value(), 0);

   for(int i = 0; i < 3; i++)
   QCOMPARE(state->viewerState->vpConfigs[i].texture.zoomLevel, 1.0);
   QCOMPARE(state->skeletonState->zoomLevel, 0.0);

}

void TestZoomAndMultiresWidget::testZoomOrthoViewportByKeyPressed() {
    Viewport *firstViewport = reference->vp;
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    float zoomLevel = state->viewerState->vpConfigs[0].texture.zoomLevel;

    QTest::keyClick(firstViewport, Qt::Key_I);

    QCOMPARE(state->viewerState->vpConfigs[0].texture.zoomLevel, zoomLevel - 0.1);
    QTest::keyClick(firstViewport, Qt::Key_O);
    QCOMPARE(state->viewerState->vpConfigs[0].texture.zoomLevel, zoomLevel);


}

/* @todo the key combination ctrl + mouse wheel up/down seems not to work at the moment */
void TestZoomAndMultiresWidget::testZoomOrthoViewportByKeyAndMouseCombination() {
    Viewport *firstViewport = reference->vp;
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    float zoomLevel = state->viewerState->vpConfigs[0].texture.zoomLevel;

}
