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
//    connect(commentsWidget->nodeCommentsTab, SIGNAL(updateTreeviewSignal()), annotationWidget->treeviewTab, SLOT(update()));
}

#include <QGraphicsBlurEffect>

void WidgetContainer::createTracingTimeWidget(QWidget *parent) {
    tracingTimeWidget = new TracingTimeWidget(parent);
#ifdef Q_OS_UNIX
    tracingTimeWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    tracingTimeWidget->layout()->update();
    tracingTimeWidget->layout()->activate();
    tracingTimeWidget->move(QApplication::desktop()->screen()->rect().center().x() - tracingTimeWidget->width() - 20,
                            QApplication::desktop()->screen()->rect().bottomRight().y() - tracingTimeWidget->height() - 50);
    tracingTimeWidget->adjustSize();
    tracingTimeWidget->setFixedSize(tracingTimeWidget->size());

}

void WidgetContainer::createCommentsWidget(QWidget *parent) {
    commentsWidget = new CommentsWidget(parent);
#ifdef Q_OS_UNIX
    commentsWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    // update and activate, so that widget geometry is calculated
    commentsWidget->layout()->update();
    commentsWidget->layout()->activate();
    commentsWidget->move(QApplication::desktop()->screen()->rect().topRight().x() - commentsWidget->width() - 20,
                         QApplication::desktop()->screen()->rect().topRight().y() + 50);
    commentsWidget->setFixedHeight(commentsWidget->height());
}

void WidgetContainer::createDatasetOptionsWidget(QWidget *parent) {
    datasetOptionsWidget = new DatasetOptionsWidget(parent);
#ifdef Q_OS_UNIX
    datasetOptionsWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    datasetOptionsWidget->layout()->update();
    datasetOptionsWidget->layout()->activate();
    datasetOptionsWidget->move(QApplication::desktop()->screen()->rect().center().x(),
                         QApplication::desktop()->screen()->rect().center().y());
    datasetOptionsWidget->setFixedSize(datasetOptionsWidget->size());
}

void WidgetContainer::createNavigationWidget(QWidget *parent) {
    navigationWidget = new NavigationWidget(parent);
#ifdef Q_OS_UNIX
    navigationWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    navigationWidget->layout()->update();
    navigationWidget->layout()->activate();
    navigationWidget->move(QApplication::desktop()->screen()->rect().topRight().x() - navigationWidget->width() - 20,
                         QApplication::desktop()->screen()->rect().topRight().y() + 50);
    navigationWidget->setFixedSize(navigationWidget->size());
}

void WidgetContainer::createViewportSettingsWidget(QWidget *parent) {
    viewportSettingsWidget = new ViewportSettingsWidget(parent);
#ifdef Q_OS_UNIX
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
#ifdef Q_OS_UNIX
    dataSavingWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    dataSavingWidget->layout()->update();
    dataSavingWidget->layout()->activate();
    dataSavingWidget->move(QApplication::desktop()->screen()->rect().center().x(),
                           QApplication::desktop()->screen()->rect().topRight().y() + 50);
    dataSavingWidget->setFixedSize(dataSavingWidget->size());
}

void WidgetContainer::createDatasetLoadWidget(QWidget *parent) {
    datasetLoadWidget = new DatasetLoadWidget(parent);
#ifdef Q_OS_UNIX
    datasetLoadWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    datasetLoadWidget->setFixedHeight(datasetLoadWidget->sizeHint().height());
}

void WidgetContainer::createTaskWidgets(QWidget *parent) {
    taskLoginWidget = new TaskLoginWidget(parent);
#ifdef Q_OS_UNIX
    taskLoginWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    taskLoginWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    taskLoginWidget->setFixedHeight(taskLoginWidget->height());
    taskManagementWidget = new TaskManagementWidget(taskLoginWidget, parent);
#ifdef Q_OS_UNIX
    taskManagementWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    taskManagementWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    taskLoginWidget->setTaskManagementWidget(taskManagementWidget);
}

void WidgetContainer::createSplashScreenWidget(QWidget *parent) {
    splashWidget = new SplashScreenWidget(parent);
#ifdef Q_OS_UNIX
    splashWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    splashWidget->setFixedSize(splashWidget->sizeHint());
}

void WidgetContainer::createDocumentationWidget(QWidget *parent) {
    docWidget = new DocumentationWidget(parent);
    docWidget->resize(1112, 600);
#ifdef Q_OS_UNIX
    docWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
}

void WidgetContainer::createAnnotationWidget(QWidget *parent) {
    annotationWidget = new AnnotationWidget(parent);
#ifdef Q_OS_UNIX
    annotationWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    annotationWidget->layout()->update();
    annotationWidget->layout()->activate();
    annotationWidget->resize(700, annotationWidget->height());
    annotationWidget->move(QApplication::desktop()->screen()->rect().bottomRight().x() - annotationWidget->width() - 20,
                           QApplication::desktop()->screen()->rect().bottomRight().y() - annotationWidget->height() - 50);
}

void WidgetContainer::createWidgets(QWidget *parent) {
    createTracingTimeWidget(parent);
    createCommentsWidget(parent);
    createViewportSettingsWidget(parent);
    createDatasetOptionsWidget(parent);
    createNavigationWidget(parent);
    createDataSavingWidget(parent);
    createDatasetLoadWidget(parent);
    createTaskWidgets(parent);
    createSplashScreenWidget(parent);
    createDocumentationWidget(parent);
    createAnnotationWidget(parent);
    rewire();

    connect(this->datasetLoadWidget, SIGNAL(datasetSwitchZoomDefaults()), this->datasetOptionsWidget, SLOT(zoomDefaultsClicked()));
}
