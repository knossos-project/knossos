#include "widgetcontainer.h"
#include "mainwindow.h"

// for mac it is necessary to set following flags: Qt::WindowStaysOnTop and Qt::Tool,
// so that they don't disappear behind main window. Under Windows these flags lead to minor bugs in prompt focussing

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
#ifdef Q_OS_MAC
    tracingTimeWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    tracingTimeWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    tracingTimeWidget->setMinimumSize(200, 100);
}

void WidgetContainer::createCommentsWidget(QWidget *parent) {
    commentsWidget = new CommentsWidget(parent);
#ifdef Q_OS_MAC
    commentsWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    commentsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createZoomAndMultiresWidget(QWidget *parent) {
    zoomAndMultiresWidget = new ZoomAndMultiresWidget(parent);
#ifdef Q_OS_MAC
    zoomAndMultiresWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    zoomAndMultiresWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createNavigationWidget(QWidget *parent) {
    navigationWidget = new NavigationWidget(parent);
#ifdef Q_OS_MAC
    navigationWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    navigationWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createToolWidget(QWidget *parent) {
    toolsWidget = new ToolsWidget(parent);
#ifdef Q_OS_MAC
    toolsWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    toolsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

}

void WidgetContainer::createViewportSettingsWidget(QWidget *parent) {
    viewportSettingsWidget = new ViewportSettingsWidget(parent);
#ifdef Q_OS_MAC
    viewportSettingsWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    viewportSettingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    viewportSettingsWidget->setGeometry(QRect(100, 100, 500, 500));
}

void WidgetContainer::createDataSavingWidget(QWidget *parent) {
    dataSavingWidget = new DataSavingWidget(parent);
#ifdef Q_OS_MAC
    dataSavingWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    dataSavingWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void WidgetContainer::createSychronizationWidget(QWidget *parent) {
    synchronizationWidget = new SynchronizationWidget(parent);
#ifdef Q_OS_MAC
    synchronizationWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    synchronizationWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

}

void WidgetContainer::createDatasetPropertyWidget(QWidget *parent) {
    datasetPropertyWidget = new DatasetPropertyWidget(parent);
#ifdef Q_OS_MAC
    datasetPropertyWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
}

void WidgetContainer::createTaskWidgets(QWidget *parent) {
    taskLoginWidget = new TaskLoginWidget(parent);
#ifdef Q_OS_MAC
    taskLoginWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    taskLoginWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    taskManagementWidget = new TaskManagementWidget(taskLoginWidget, parent);
#ifdef Q_OS_MAC
    taskManagementWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    taskManagementWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    taskLoginWidget->setTaskManagementWidget(taskManagementWidget);
}

void WidgetContainer::createSplashScreenWidget(QWidget *parent) {
    splashWidget = new SplashScreenWidget(parent);
#ifdef Q_OS_MAC
    splashWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
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
