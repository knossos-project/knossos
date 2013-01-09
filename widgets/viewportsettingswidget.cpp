#include "viewportsettingswidget.h"

ViewportSettingsWidget::ViewportSettingsWidget(QWidget *parent) :
    QDialog(parent)
{

    setWindowTitle("Viewport Settings");
    tabs = new QTabWidget(this);
    tabs->addTab(new VPGeneralTabWidget(), "General");
    tabs->addTab(new VPSlicePlaneViewportWidget(), "Slice Plane Viewports");
    tabs->addTab(new VPSkeletonViewportWidget(), "Skeleton Viewport");

}

void ViewportSettingsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
