#include "widgetcontainer.h"
#include "mainwindow.h"

WidgetContainer::WidgetContainer(MainWindow *parent) :
    QObject(parent)
{

}

void WidgetContainer::rewire() {

    connect(this->toolsWidget->toolsNodesTabWidget->activeNodeIdSpinBox, SIGNAL(valueChanged(int)), this->toolsWidget->toolsQuickTabWidget, SLOT(activeNodeIdChanged(int)));
    connect(this->toolsWidget->toolsTreesTabWidget->activeTreeSpinBox, SIGNAL(valueChanged(int)), this->toolsWidget->toolsQuickTabWidget, SLOT(activeTreeIdChanged(int)));
    connect(this->toolsWidget->toolsNodesTabWidget->commentField, SIGNAL(textChanged(QString)), this->toolsWidget->toolsQuickTabWidget, SLOT(commentChanged(QString)));
    connect(this->toolsWidget->toolsNodesTabWidget->searchForField, SIGNAL(textChanged(QString)), this->toolsWidget->toolsQuickTabWidget, SLOT(searchForChanged(QString)));
    connect(this->commentsWidget->nodeCommentsTab, SIGNAL(updateTools()), this->toolsWidget, SLOT(updateDisplayedTree()));
    connect(this->toolsWidget->toolsQuickTabWidget, SIGNAL(updateToolsSignal()), this->toolsWidget, SLOT(updateDisplayedTree()));
    connect(this->toolsWidget->toolsTreesTabWidget, SIGNAL(updateToolsSignal()), this->toolsWidget, SLOT(updateDisplayedTree()));
    connect(this->toolsWidget->toolsNodesTabWidget, SIGNAL(updateCommentsTableSignal()), this->commentsWidget->nodeCommentsTab, SLOT(updateCommentsTable()));
    connect(this->toolsWidget->toolsQuickTabWidget, SIGNAL(updateCommentsTableSignal()), this->commentsWidget->nodeCommentsTab, SLOT(updateCommentsTable()));
}

void WidgetContainer::createConsoleWidget() {
    console = new Console();
    console->setWindowFlags(Qt::WindowStaysOnTopHint);
    console->setMinimumSize(200, 100);

}

void WidgetContainer::createTracingTimeWidget() {
    tracingTimeWidget = new TracingTimeWidget();
    tracingTimeWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    tracingTimeWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    tracingTimeWidget->setMinimumSize(200, 100);

}

void WidgetContainer::createCommentsWidget() {
    commentsWidget = new CommentsWidget();
    commentsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    commentsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createZoomAndMultiresWidget() {
    zoomAndMultiresWidget = new ZoomAndMultiresWidget();
    zoomAndMultiresWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    zoomAndMultiresWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createNavigationWidget() {
    navigationWidget = new NavigationWidget();
    navigationWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    navigationWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createToolWidget() {
    toolsWidget = new ToolsWidget();
    toolsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    toolsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

}

void WidgetContainer::createViewportSettingsWidget() {
    viewportSettingsWidget = new ViewportSettingsWidget();
    viewportSettingsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    viewportSettingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    viewportSettingsWidget->setGeometry(QRect(100, 100, 500, 500));
}

void WidgetContainer::createDataSavingWidget() {
    dataSavingWidget = new DataSavingWidget();
    dataSavingWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    dataSavingWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createSychronizationWidget() {
    synchronizationWidget = new SynchronizationWidget();
    synchronizationWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    synchronizationWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

}

void WidgetContainer::showSplashScreenWidget() {
    splashWidget = new SplashScreenWidget();
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
