#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

#include <QObject>

class Console;
class TracingTimeWidget;
class CommentsWidget;
class DataSavingWidget;
class SplashScreenWidget;
class SynchronizationWidget;
class ToolsWidget;
class ViewportSettingsWidget;
class ZoomAndMultiresWidget;
class Viewport;
class NavigationWidget;
class MainWindow;
class CoordinateBarWidget;
class WidgetContainer : public QObject
{
    Q_OBJECT
public:
    explicit WidgetContainer(MainWindow *parent = 0);
    void createConsoleWidget();
    void showConsoleWidget();
    void createTracingTimeWidget();
    void showTracingTimeWidget();
    void createCommentsWidget();
    void showCommentsWidget();
    void createZoomAndMultiresWidget();

    void createNavigationWidget();
    void createViewportSettingsWidget();
    void createToolWidget();
    void createDataSavingWidget();
    void createSychronizationWidget();
    void showSplashScreenWidget();
    void createWidgets();
    void createCoordBarWidget();

    Console *console;
    TracingTimeWidget *tracingTimeWidget;
    CommentsWidget *commentsWidget;
    ZoomAndMultiresWidget *zoomAndMultiresWidget;
    NavigationWidget *navigationWidget;
    ToolsWidget *toolsWidget;
    ViewportSettingsWidget *viewportSettingsWidget;
    CoordinateBarWidget *coordBarWidget;
    DataSavingWidget *dataSavingWidget;
    SynchronizationWidget *synchronizationWidget;
    SplashScreenWidget *splashWidget;
signals:
    
public slots:
    
};

#endif // WIDGETCONTAINER_H
