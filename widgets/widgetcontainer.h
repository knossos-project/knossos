#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

/* Include your widget and your subwidgets and you have easy access via this class */

#include <QObject>

#include "tracingtimewidget.h"
#include "datasavingwidget.h"
#include "splashscreenwidget.h"

#include "commentswidget.h"
#include "commentshortcuts/commentshortcutstab.h"
#include "commentshortcuts/commentshighlightingtab.h"
#include "commentshortcuts/commentsnodecommentstab.h"

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
    void createTracingTimeWidget(QWidget *parent);
    void showTracingTimeWidget();
    void createCommentsWidget(QWidget *parent);
    void showCommentsWidget();
    void createZoomAndMultiresWidget(QWidget *parent);

    void createNavigationWidget(QWidget *parent);
    void createViewportSettingsWidget(QWidget *parent);
    void createDataSavingWidget(QWidget *parent);
    void createSychronizationWidget(QWidget *parent);
    void createSplashScreenWidget(QWidget *parent);
    void createDatasetPropertyWidget(QWidget *parent);
    void createTaskWidgets(QWidget *parent);
    void createDocumentationWidget(QWidget *parent);
    void createAnnotationWidget(QWidget *parent);
    void createWidgets(QWidget *parent);

    TracingTimeWidget *tracingTimeWidget;
    CommentsWidget *commentsWidget;
    ZoomAndMultiresWidget *zoomAndMultiresWidget;
    NavigationWidget *navigationWidget;
    ViewportSettingsWidget *viewportSettingsWidget;
    DataSavingWidget *dataSavingWidget;
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
