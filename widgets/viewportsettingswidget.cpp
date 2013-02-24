#include "viewportsettingswidget.h"
#include "viewportsettings/vpgeneraltabwidget.h"
#include "viewportsettings/vpsliceplaneviewportwidget.h"
#include "viewportsettings/vpskeletonviewportwidget.h"
#include "mainwindow.h"

ViewportSettingsWidget::ViewportSettingsWidget(QWidget *parent) :
    QDialog(parent)
{

    this->generalTabWidget = new VPGeneralTabWidget();
    this->slicePlaneViewportWidget = new VPSlicePlaneViewportWidget();
    this->skeletonViewportWidget = new VPSkeletonViewportWidget();

    setWindowTitle("Viewport Settings");
    tabs = new QTabWidget(this);
    tabs->addTab(generalTabWidget, "General");
    tabs->addTab(slicePlaneViewportWidget, "Slice Plane Viewports");
    tabs->addTab(skeletonViewportWidget, "Skeleton Viewport");

}

void ViewportSettingsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    MainWindow *parent = (MainWindow *) this->parentWidget();
    parent->uncheckViewportSettingAction();
}
