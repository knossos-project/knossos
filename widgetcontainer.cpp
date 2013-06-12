#include "widgetcontainer.h"
#include "widgets/console.h"
#include "widgets/tracingtimewidget.h"
#include "widgets/commentswidget.h"
#include "widgets/zoomandmultireswidget.h"
#include "widgets/navigationwidget.h"
#include "widgets/toolswidget.h"
#include "widgets/viewportsettingswidget.h"
#include "widgets/datasavingwidget.h"
#include "widgets/synchronizationwidget.h"
#include "widgets/splashscreenwidget.h"
#include "widgets/coordinatebarwidget.h"
#include "mainwindow.h"

WidgetContainer::WidgetContainer(MainWindow *parent) :
    QObject(parent)
{
}

void WidgetContainer::createConsoleWidget() {
    console = new Console();
    console->setWindowFlags(Qt::WindowStaysOnTopHint);
    console->setGeometry(800, 500, 200, 120);
}

void WidgetContainer::createTracingTimeWidget() {
    tracingTimeWidget = new TracingTimeWidget();
    tracingTimeWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    tracingTimeWidget->setGeometry(800, 350, 200, 100);
}

void WidgetContainer::createCommentsWidget() {
    commentsWidget = new CommentsWidget();
    commentsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    commentsWidget->setGeometry(800, 100, 470, 300);
}

void WidgetContainer::createZoomAndMultiresWidget() {
    zoomAndMultiresWidget = new ZoomAndMultiresWidget();
    zoomAndMultiresWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    zoomAndMultiresWidget->setGeometry(1024, 100, 380, 200);
}

void WidgetContainer::createNavigationWidget() {
    navigationWidget = new NavigationWidget();
    navigationWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    navigationWidget->setGeometry(1024, 350, 200, 200);
}

void WidgetContainer::createToolWidget() {
    toolsWidget = new ToolsWidget();
    toolsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    toolsWidget->setGeometry(500, 100, 430, 610);
}

void WidgetContainer::createViewportSettingsWidget() {
    viewportSettingsWidget = new ViewportSettingsWidget();
    viewportSettingsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    viewportSettingsWidget->setGeometry(500, 500, 680, 400);
}

void WidgetContainer::createDataSavingWidget() {
    dataSavingWidget = new DataSavingWidget();
    dataSavingWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    dataSavingWidget->setGeometry(100, 100, 100, 90);
}

void WidgetContainer::createSychronizationWidget() {
    synchronizationWidget = new SynchronizationWidget();
    synchronizationWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    synchronizationWidget->setGeometry(100, 350, 150, 100);
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
}
