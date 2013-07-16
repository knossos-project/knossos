#include "widgetcontainer.h"
#include "mainwindow.h"

WidgetContainer::WidgetContainer(MainWindow *parent) :
    QObject(parent)
{

}

void WidgetContainer::rewire() {
    connect(this->toolsWidget->toolsQuickTabWidget, SIGNAL(updateToolsSignal()), this->toolsWidget, SLOT(updateDisplayedTree()));
    connect(this->toolsWidget->toolsTreesTabWidget, SIGNAL(updateToolsSignal()), this->toolsWidget, SLOT(updateDisplayedTree()));
}

void WidgetContainer::createConsoleWidget() {
    console = new Console();
    console->setWindowFlags(Qt::WindowStaysOnTopHint);
    console->setMinimumSize(200, 100);
    console->setGeometry(800, 500, 200, 120);
}

void WidgetContainer::createTracingTimeWidget() {
    tracingTimeWidget = new TracingTimeWidget();
    tracingTimeWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    tracingTimeWidget->setMinimumSize(200, 100);
    tracingTimeWidget->setGeometry(800, 350, 200, 100);
}

void WidgetContainer::createCommentsWidget() {
    commentsWidget = new CommentsWidget();
    commentsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    commentsWidget->setMinimumSize(470, 300);
    commentsWidget->setGeometry(800, 100, 470, 300);
}

void WidgetContainer::createZoomAndMultiresWidget() {
    zoomAndMultiresWidget = new ZoomAndMultiresWidget();
    zoomAndMultiresWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    zoomAndMultiresWidget->setMinimumSize(380, 200);
    zoomAndMultiresWidget->setGeometry(1024, 100, 380, 200);
}

void WidgetContainer::createNavigationWidget() {
    navigationWidget = new NavigationWidget();
    navigationWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    navigationWidget->setMinimumSize(200, 200);
    navigationWidget->setGeometry(1024, 350, 200, 200);
}

void WidgetContainer::createToolWidget() {
    toolsWidget = new ToolsWidget();
    toolsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    toolsWidget->setMinimumSize(430, 610);
    toolsWidget->setGeometry(500, 100, 430, 610);
}

void WidgetContainer::createViewportSettingsWidget() {
    viewportSettingsWidget = new ViewportSettingsWidget();
    viewportSettingsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    viewportSettingsWidget->setMinimumSize(680, 400);
    viewportSettingsWidget->setGeometry(500, 500, 680, 400);
}

void WidgetContainer::createDataSavingWidget() {
    dataSavingWidget = new DataSavingWidget();
    dataSavingWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    dataSavingWidget->setMinimumSize(220, 120);
    dataSavingWidget->setGeometry(100, 100, 220, 120);
}

void WidgetContainer::createSychronizationWidget() {
    synchronizationWidget = new SynchronizationWidget();
    synchronizationWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    synchronizationWidget->setMinimumSize(200, 120);
    synchronizationWidget->setGeometry(100, 350, 200, 120);
}

void WidgetContainer::showSplashScreenWidget() {
    splashWidget = new SplashScreenWidget();
    splashWidget->setGeometry(400, 100, 500, 500);
    splashWidget->show();
}

void WidgetContainer::createCoordBarWidget() {
    this->coordBarWidget = new CoordinateBarWidget();
}

void WidgetContainer::createWidgets() {
    createConsoleWidget();
    createTracingTimeWidget();
    createCommentsWidget();
    createViewportSettingsWidget();
    createZoomAndMultiresWidget();
    createNavigationWidget();
    createToolWidget();
    createDataSavingWidget();
    createSychronizationWidget();
    createCoordBarWidget();
    rewire();
}
