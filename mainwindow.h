//the functions formerly placed in gui.h/gui.c are now in mainwindow
//yesNoPrompt should be replaced, saveSkeletonCallback is replaced by QAction saveAction
//ui_loadskeleton receives a QEvent instead of AG_Event now
//updateAGconfig is now updateGuiConfig()
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStringList>
#include <QToolBar>
#include <QSpinbox>
#include <QLabel>
#include "viewport.h"

#include "knossos-global.h"
#include <dirent.h>

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

    static void quitKnossos(); //not needed in qt
    static bool initGUI();
    static bool cpBaseDirectory(char *target, char *path, size_t len);
    static void yesNoPrompt(QWidget *par, char *promptString, void (*yesCb)(), void (*noCb)());
    static uint32_t addRecentFile(char *path, uint32_t pos);
    static void UI_workModeAdd();
    static void UI_workModeLink();
    static void UI_workModeDrop();
    //static void saveSkelCallback(AG_Event *event);
    static void UI_saveSkeleton(int32_t increment);
    static void UI_saveSettings();
    static void UI_loadSkeleton(QEvent *event);
    static void UI_pasteClipboardCoordinates();
    static void UI_zoomOrthogonals(float step);
    static void reloadDataSizeWin();
    static void treeColorAdjustmentsChanged();

    void copyClipboardCoordinates();
    void pasteClipboardCoordinates();

    void showSplashScreen();
    void createConsoleWidget();
    void createTracingTimeWidget();
    void createCommentsWidget();
    void createZoomAndMultiresWidget();
    void createNavigationWidget();
    void createViewportSettingsWidget();
    void createToolWidget();

    void createXYViewport();
    void createXZViewport();
    void createYZViewport();
    void createSkeletonViewport();

private:
    Ui::MainWindow *ui;

    QToolBar *toolBar;
    QPushButton *copyButton;
    QPushButton *pasteButton;
    QLabel *xLabel, *yLabel, *zLabel;
    QSpinBox *xField, *yField, *zField;

    QMessageBox *prompt;

    QWidget *mainWidget;
    QGridLayout *gridLayout;
    Viewport **viewports;

    /* Dialogs */
    Console *console;
    TracingTimeWidget *tracingTimeWidget;
    CommentsWidget *commentsWidget;
    ZoomAndMultiresWidget *zoomAndMultiresWidget;

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
    QAction *viewportSettingsAction;

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

    QFileDialog *loadFileDialog;
    QFileDialog *saveFileDialog;
    QFile *loadedFile;

    void createActions();
    void createMenus();
    void createCoordBarWin();

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
    void viewportSettingsSlot();

    /* window menu */
    void toolsSlot();
    void logSlot();
    void commentShortcutsSlots();

    /* help menu */
    void aboutSlot();

};

#endif // MAINWINDOW_H
