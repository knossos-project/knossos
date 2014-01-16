#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

/* Include your widget and your subwidgets and you have easy access via this class */

#include <QObject>

#include "console.h"
#include "tracingtimewidget.h"
#include "synchronizationwidget.h"
#include "datasavingwidget.h"
#include "splashscreenwidget.h"

#include "commentswidget.h"
#include "commentshortcuts/commentshortcutstab.h"
#include "commentshortcuts/commentshighlightingtab.h"
#include "commentshortcuts/commentsnodecommentstab.h"

#include "toolswidget.h"
#include "tools/toolsnodestabwidget.h"
#include "tools/toolsquicktabwidget.h"
#include "tools/toolstreestabwidget.h"

#include "viewportsettingswidget.h"
#include "viewportsettings/vpgeneraltabwidget.h"
#include "viewportsettings/vpskeletonviewportwidget.h"
#include "viewportsettings/vpsliceplaneviewportwidget.h"

#include "zoomandmultireswidget.h"
#include "navigationwidget.h"
#include "datasetpropertywidget.h"
#include "task/taskloginwidget.h"
#include "task/taskmanagementwidget.h"
#include "documentationwidget.h"
#include "annotationwidget.h"

class Viewport;
class MainWindow;
class WidgetContainer : public QObject
{
    Q_OBJECT
public:
    explicit WidgetContainer(MainWindow *parent = 0);
    void rewire();
    void createConsoleWidget();
    void showConsoleWidget();
    void createTracingTimeWidget(QWidget *parent);
    void showTracingTimeWidget();
    void    createCommentsWidget(QWidget *parent);
    void showCommentsWidget();
    void createZoomAndMultiresWidget(QWidget *parent);

    void createNavigationWidget(QWidget *parent);
    void createViewportSettingsWidget(QWidget *parent);
    void createToolWidget(QWidget *parent);
    void createDataSavingWidget(QWidget *parent);
    void createSychronizationWidget(QWidget *parent);
    void createSplashScreenWidget(QWidget *parent);
    void createDatasetPropertyWidget(QWidget *parent);
    void createTaskWidgets(QWidget *parent);
    void createDocumentationWidget(QWidget *parent);
    void createAnnotationWidget(QWidget *parent);
    void createWidgets(QWidget *parent);

    Console *console;
    TracingTimeWidget *tracingTimeWidget;
    CommentsWidget *commentsWidget;
    ZoomAndMultiresWidget *zoomAndMultiresWidget;
    NavigationWidget *navigationWidget;
    ToolsWidget *toolsWidget;
    ViewportSettingsWidget *viewportSettingsWidget;
    DataSavingWidget *dataSavingWidget;
    SynchronizationWidget *synchronizationWidget;
    SplashScreenWidget *splashWidget;
    DatasetPropertyWidget *datasetPropertyWidget;
    TaskLoginWidget *taskLoginWidget;
    TaskManagementWidget *taskManagementWidget;
    DocumentationWidget *docWidget;
    AnnotationWidget *annotationWidget;

signals:

public slots:
    
};

#endif // WIDGETCONTAINER_H
