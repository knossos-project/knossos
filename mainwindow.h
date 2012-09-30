#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QDebug>

#include "openglwidget.h"



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);
private:
    Ui::MainWindow *ui;

    QMessageBox *prompt;

    QWidget *mainWidget;
    QGridLayout *gridLayout;

    OpenGLWidget **glWidgets;

    /* file actions */
    QAction *openAction;
    QAction *recentFileAction;
    QAction *saveAction;
    QAction *saveAsAction;
    QAction *quitAction;

    /* edit skeleton actions */
    QAction *addNodeAction;
    QAction *linkWithActiveNodeAction;
    QAction *dropNodesAction;
    QAction *skeletonStatisticsAction;
    QAction *clearSkeletonAction;

    /* view actions */
    QAction *workModeViewAction;
    QAction *dragDatasetAction;
    QAction *recenterOnClickAction;
    QAction *zoomAndMultiresAction;
    QAction *tracingTimeAction;

    /* preferences actions */
    QAction *loadCustomPreferencesAction;
    QAction *saveCustomPreferencesAction;
    QAction *defaultPreferencesAction;
    QAction *datasetNavigationAction;
    QAction *synchronizationAction;
    QAction *dataSavingOptionsAction;
    QAction *viewPortSettingsAction;

    /* window actions */
    QAction *toolsAction;
    QAction *logAction;
    QAction *commentShortcutsAction;

    /* help actions */
    QAction *aboutAction;

    /* Qmenu-points */
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *workModeMenu;
    QMenu *viewMenu;
    QMenu *preferenceMenu;
    QMenu *windowMenu;
    QMenu *helpMenu;

    void createActions();
    void createMenus();

private slots:
    /* file menu */
    void openSlot();
    void recentFilesSlot();
    void saveSlot();
    void saveAsSlot();
    void quitSlot();

    /* edit skeleton menu*/
    void workModeEditSlot();
    void addNodeSlot();
    void linkWithActiveNodeSlot();
    void dropNodesSlot();
    void skeletonStatisticsSlot();
    void clearSkeletonSlot();

    /* view menu */
    void workModeViewSlot();
    void dragDatasetSlot();
    void recenterOnClickSlot();
    void zoomAndMultiresSlot();
    void tracingTimeSlot();

    /* preferences menu */
    void loadCustomPreferencesSlot();
    void saveCustomPreferencesSlot();
    void defaultPreferencesSlot();
    void datatasetNavigationSlot();
    void synchronizationSlot();
    void dataSavingOptionsSlot();
    void viewPortSettingsSlot();

    /* window menu */
    void toolsSlot();
    void logSlot();
    void commentShortcutsSlots();

    /* help menu */
    void aboutSlot();

};

#endif // MAINWINDOW_H
