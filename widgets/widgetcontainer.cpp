#include "widgetcontainer.h"
#include "mainwindow.h"

// for mac it is necessary to set following flags: Qt::WindowStaysOnTop and Qt::Tool,
// so that they don't disappear behind main window. Under Windows these flags lead to minor bugs in prompt focussing

WidgetContainer::WidgetContainer(MainWindow *parent) :
    QObject(parent)
{

}

/**
 * @brief WidgetContainer::rewire connects signals and slots in different widgets with each other
 */
void WidgetContainer::rewire() {
//    connect(toolsWidget, SIGNAL(updateCommentsTableSignal()), commentsWidget->nodeCommentsTab, SLOT(updateCommentsTable()));
//    connect(toolsWidget, SIGNAL(updateTreeviewSignal()), annotationWidget->treeviewTab, SLOT(update()));
//    connect(toolsWidget->toolsQuickTabWidget, SIGNAL(updateTreeviewSignal()), annotationWidget->treeviewTab, SLOT(update()));
//    connect(toolsWidget->toolsNodesTabWidget, SIGNAL(updateTreeviewSignal()), annotationWidget->treeviewTab, SLOT(update()));
//    connect(toolsWidget->toolsTreesTabWidget, SIGNAL(updateTreeviewSignal()), annotationWidget->treeviewTab, SLOT(update()));
    //connect(annotationWidget->treeviewTab, SIGNAL(updateToolsSignal()), toolsWidget, SLOT(updateToolsSlot()));
//    connect(commentsWidget->nodeCommentsTab, SIGNAL(updateTreeviewSignal()), annotationWidget->treeviewTab, SLOT(update()));
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
    tracingTimeWidget->layout()->update();
    tracingTimeWidget->layout()->activate();
    tracingTimeWidget->move(QApplication::desktop()->screen()->rect().center().x() - tracingTimeWidget->width() - 20,
                            QApplication::desktop()->screen()->rect().bottomRight().y() - tracingTimeWidget->height() - 50);
    tracingTimeWidget->setFixedSize(tracingTimeWidget->size());
}

void WidgetContainer::createCommentsWidget(QWidget *parent) {
    commentsWidget = new CommentsWidget(parent);
#ifdef Q_OS_MAC
    commentsWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    // update and activate, so that widget geometry is calculated
    commentsWidget->layout()->update();
    commentsWidget->layout()->activate();
    commentsWidget->move(QApplication::desktop()->screen()->rect().topRight().x() - commentsWidget->width() - 20,
                         QApplication::desktop()->screen()->rect().topRight().y() + 50);
    commentsWidget->setFixedSize(commentsWidget->size());
}

void WidgetContainer::createZoomAndMultiresWidget(QWidget *parent) {
    zoomAndMultiresWidget = new ZoomAndMultiresWidget(parent);
#ifdef Q_OS_MAC
    zoomAndMultiresWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    zoomAndMultiresWidget->layout()->update();
    zoomAndMultiresWidget->layout()->activate();
    zoomAndMultiresWidget->move(QApplication::desktop()->screen()->rect().center().x(),
                         QApplication::desktop()->screen()->rect().center().y());
    zoomAndMultiresWidget->setFixedSize(zoomAndMultiresWidget->size());
}

void WidgetContainer::createNavigationWidget(QWidget *parent) {
    navigationWidget = new NavigationWidget(parent);
#ifdef Q_OS_MAC
    navigationWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    navigationWidget->layout()->update();
    navigationWidget->layout()->activate();
    navigationWidget->move(QApplication::desktop()->screen()->rect().topRight().x() - navigationWidget->width() - 20,
                         QApplication::desktop()->screen()->rect().topRight().y() + 50);
    navigationWidget->setFixedSize(navigationWidget->size());
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
    viewportSettingsWidget->layout()->update();
    viewportSettingsWidget->layout()->activate();
    viewportSettingsWidget->move(QApplication::desktop()->screen()->rect().topRight().x() - viewportSettingsWidget->width() - 20,
                                 QApplication::desktop()->screen()->rect().topRight().y() + viewportSettingsWidget->height());
    viewportSettingsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    viewportSettingsWidget->setFixedSize(viewportSettingsWidget->sizeHint());
}

void WidgetContainer::createDataSavingWidget(QWidget *parent) {
    dataSavingWidget = new DataSavingWidget(parent);
#ifdef Q_OS_MAC
    dataSavingWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    dataSavingWidget->layout()->update();
    dataSavingWidget->layout()->activate();
    dataSavingWidget->move(QApplication::desktop()->screen()->rect().center().x(),
                           QApplication::desktop()->screen()->rect().topRight().y() + 50);
    dataSavingWidget->setFixedSize(dataSavingWidget->size());
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

void WidgetContainer::createDocumentationWidget(QWidget *parent) {
    docWidget = new DocumentationWidget(parent);
    docWidget->resize(1112, 600);
#ifdef Q_OS_MAC
    docWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
}

void WidgetContainer::createAnnotationWidget(QWidget *parent) {
    annotationWidget = new AnnotationWidget(parent);
#ifdef Q_OS_MAC
    annotationWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    annotationWidget->layout()->update();
    annotationWidget->layout()->activate();
    annotationWidget->resize(700, annotationWidget->height());
    annotationWidget->move(QApplication::desktop()->screen()->rect().bottomRight().x() - annotationWidget->width() - 20,
                           QApplication::desktop()->screen()->rect().bottomRight().y() - annotationWidget->height() - 50);
}

void WidgetContainer::createWidgets(QWidget *parent) {
    createConsoleWidget();
    createTracingTimeWidget(parent);
    createCommentsWidget(parent);
    createViewportSettingsWidget(parent);
    createZoomAndMultiresWidget(parent);
    createNavigationWidget(parent);
    //createToolWidget(parent);
    createDataSavingWidget(parent);
    createSychronizationWidget(parent);
    createDatasetPropertyWidget(parent);
    createTaskWidgets(parent);
    createSplashScreenWidget(parent);
    createDocumentationWidget(parent);
    createAnnotationWidget(parent);
    rewire();

    connect(this->datasetPropertyWidget, SIGNAL(datasetSwitchZoomDefaults()), this->zoomAndMultiresWidget, SLOT(zoomDefaultsClicked()));
}
