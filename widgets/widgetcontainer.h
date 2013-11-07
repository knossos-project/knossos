#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

/* Include your widget and your subwidgets and you have easy access via this class */

#include <QObject>

#include "widgets/console.h"
#include "widgets/tracingtimewidget.h"
#include "widgets/synchronizationwidget.h"
#include "widgets/datasavingwidget.h"
#include "widgets/splashscreenwidget.h"

#include "widgets/commentswidget.h"
#include "widgets/commentshortcuts/commentshortcutstab.h"
#include "widgets/commentshortcuts/commentshighlightingtab.h"
#include "widgets/commentshortcuts/commentsnodecommentstab.h"

#include "widgets/toolswidget.h"
#include "widgets/tools/toolsnodestabwidget.h"
#include "widgets/tools/toolsquicktabwidget.h"
#include "widgets/tools/toolstreestabwidget.h"

#include "widgets/viewportsettingswidget.h"
#include "widgets/viewportsettings/vpgeneraltabwidget.h"
#include "widgets/viewportsettings/vpskeletonviewportwidget.h"
#include "widgets/viewportsettings/vpsliceplaneviewportwidget.h"

#include "widgets/zoomandmultireswidget.h"
#include "widgets/navigationwidget.h"
#include "widgets/datasetpropertywidget.h"
#include "widgets/task/taskloginwidget.h"
#include "widgets/task/taskmanagementwidget.h"
#include "widgets/documentationwidget.h"

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

signals:

public slots:
    
};

#endif // WIDGETCONTAINER_H
