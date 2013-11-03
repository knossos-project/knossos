#include "widgetcontainer.h"
#include "mainwindow.h"

WidgetContainer::WidgetContainer(MainWindow *parent) :
    QObject(parent)
{

}

void WidgetContainer::rewire() {

    connect(this->toolsWidget->toolsNodesTabWidget->activeNodeIdSpinBox, SIGNAL(valueChanged(int)), this->toolsWidget->toolsQuickTabWidget, SLOT(activeNodeIdChanged(int)));
    connect(this->toolsWidget->toolsTreesTabWidget->activeTreeSpinBox, SIGNAL(valueChanged(int)), this->toolsWidget->toolsQuickTabWidget, SLOT(activeTreeIdChanged(int)));
    connect(this->toolsWidget->toolsNodesTabWidget->findNextButton, SIGNAL(clicked()), this->toolsWidget->toolsQuickTabWidget, SLOT(findNextButtonClicked()));
    connect(this->toolsWidget->toolsNodesTabWidget->findPreviousButton, SIGNAL(clicked()), this->toolsWidget->toolsQuickTabWidget, SLOT(findPreviousButtonClicked()));
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

void WidgetContainer::createTracingTimeWidget(QWidget *parent) {
    tracingTimeWidget = new TracingTimeWidget(parent);
    //tracingTimeWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
    tracingTimeWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    tracingTimeWidget->setMinimumSize(200, 100);
}

void WidgetContainer::createCommentsWidget(QWidget *parent) {
    commentsWidget = new CommentsWidget(parent);
    commentsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createZoomAndMultiresWidget(QWidget *parent) {
    zoomAndMultiresWidget = new ZoomAndMultiresWidget(parent);
    zoomAndMultiresWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createNavigationWidget(QWidget *parent) {
    navigationWidget = new NavigationWidget(parent);
    navigationWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createToolWidget(QWidget *parent) {
    toolsWidget = new ToolsWidget(parent);
    toolsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

}

void WidgetContainer::createViewportSettingsWidget(QWidget *parent) {
    viewportSettingsWidget = new ViewportSettingsWidget(parent);
    viewportSettingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    viewportSettingsWidget->setGeometry(QRect(100, 100, 500, 500));
}

void WidgetContainer::createDataSavingWidget(QWidget *parent) {
    dataSavingWidget = new DataSavingWidget(parent);
    dataSavingWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createSychronizationWidget(QWidget *parent) {
    synchronizationWidget = new SynchronizationWidget(parent);
    synchronizationWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

}

void WidgetContainer::createDatasetPropertyWidget(QWidget *parent) {
    datasetPropertyWidget = new DatasetPropertyWidget(parent);
}

void WidgetContainer::createTaskWidgets(QWidget *parent) {
    taskLoginWidget = new TaskLoginWidget(parent);
    taskLoginWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    taskManagementWidget = new TaskManagementWidget(taskLoginWidget, parent);
    taskManagementWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    taskManagementWidget->resize(300, 240);

    taskLoginWidget->setTaskManagementWidget(taskManagementWidget);
}

void WidgetContainer::createSplashScreenWidget(QWidget *parent) {
    splashWidget = new SplashScreenWidget(parent);
}

void WidgetContainer::createWidgets(QWidget *parent) {
    createConsoleWidget();
    createTracingTimeWidget(parent);
    createCommentsWidget(parent);
    createViewportSettingsWidget(parent);
    createZoomAndMultiresWidget(parent);
    createNavigationWidget(parent);
    createToolWidget(parent);
    createDataSavingWidget(parent);
    createSychronizationWidget(parent);
    createDatasetPropertyWidget(parent);
    createTaskWidgets(parent);
    createSplashScreenWidget(parent);
    rewire();
}
