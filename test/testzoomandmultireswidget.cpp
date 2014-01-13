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
    double textFieldValue = 1 - zoomWidget->orthogonalDataViewportLabel->text().toDouble() / 100.0;
    QCOMPARE(zoomWidget->orthogonalDataViewportSpinBox->singleStep(), textFieldValue);

}

void TestZoomAndMultiresWidget::testZoomSkeletonViewport() {
    ZoomAndMultiresWidget *zoomWidget = reference->window->widgetContainer->zoomAndMultiresWidget;
    double textFieldValue = 1 - zoomWidget->orthogonalDataViewportLabel->text().toDouble() / 100.0;
    QCOMPARE(zoomWidget->orthogonalDataViewportSpinBox->singleStep(), textFieldValue);
}

void TestZoomAndMultiresWidget::testResetZoomFactors() {
   ZoomAndMultiresWidget *zoomWidget = reference->window->widgetContainer->zoomAndMultiresWidget;
   zoomWidget->zoomDefaultsClicked();

   QCOMPARE(1 - zoomWidget->orthogonalDataViewportSpinBox->value() / 100., VPZOOMMIN);
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

    float diffAfterZoomIn = fabs(state->viewerState->vpConfigs[0].texture.zoomLevel - zoomLevel - 0.1);
    QTest::keyClick(firstViewport, Qt::Key_O, Qt::NoModifier, 50);

    float diffAfterZoomOut = fabs(state->viewerState->vpConfigs[0].texture.zoomLevel - zoomLevel);
    QCOMPARE(diffAfterZoomOut, 0.0);


}


void TestZoomAndMultiresWidget::testZoomOrthoViewportByKeyAndMouseCombination() {
    Viewport *firstViewport = reference->vpUpperLeft;
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    float zoomLevel = state->viewerState->vpConfigs[0].texture.zoomLevel;

}
