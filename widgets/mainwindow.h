#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#define FILE_DIALOG_HISTORY_MAX_ENTRIES 10
#include <QMainWindow>
#include <QQueue>

namespace Ui {
    class MainWindow;
}

class QLabel;
class QToolBar;
class QToolButton;
class QPushButton;
class QSpinBox;
class QMessageBox;
class QGridLayout;
class QFile;
class Viewport;
class WidgetContainer;
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void updateSkeletonFileName(QString &fileName);

    void closeEvent(QCloseEvent *event);
    void updateTitlebar(bool useFilename);

    static bool cpBaseDirectory(char *target, char *path, size_t len);
    static void reloadDataSizeWin();

    static void datasetColorAdjustmentsChanged();
    void showSplashScreen();

signals:
    bool changeDatasetMagSignal(uint serverMovement);
    void recalcTextureOffsetsSignal();    
    void clearSkeletonSignal(int targetRevision, int loadingSkeleton);
    void updateSkeletonFileNameSignal(int targetRevision, int increment, char *filename);
    bool loadSkeletonSignal(QString fileName);
    bool saveSkeletonSignal(QString fileName);
    void recentFileSelectSignal(int index);
    void updateToolsSignal();
    void updateCommentsTableSignal();
    void userMoveSignal(int x, int y, int z, int serverMovement);

    void stopRenderTimerSignal();
    void startRenderTimerSignal(int frequency);
    void updateTreeColorsSignal();
    void loadTreeLUTFallback();
protected:

public:
    Ui::MainWindow *ui;

    QToolBar *toolBar;
    QToolButton *copyButton;
    QToolButton *pasteButton;
    QLabel *xLabel, *yLabel, *zLabel;
    QSpinBox *xField, *yField, *zField;

    QMessageBox *prompt;

    QWidget *mainWidget;
    QGridLayout *gridLayout;
    Viewport **viewports;

    // contains all widgets
    WidgetContainer *widgetContainer;

    /* file actions */
    QAction *recentFileAction;
    QAction **historyEntryActions;

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
    QMenu *recentFileMenu;
    QMenu *editMenu;
    QMenu *workModeEditMenu;
    QMenu *viewMenu;
    QMenu *workModeViewMenu;
    QMenu *preferenceMenu;
    QMenu *windowMenu;
    QMenu *helpMenu;

    QQueue<QString> *skeletonFileHistory;
    QFile *loadedFile;

    QLabel *lol;

    QToolButton *open, *save;
    QToolButton *pythonButton;
    QToolButton *tracingTimeButton;
    QToolButton *zoomAndMultiresButton;
    QToolButton *syncButton;
    QToolButton *viewportSettingsButton;
    QToolButton *toolsButton;
    QToolButton *commentShortcutsButton;

    void updateFileHistoryMenu();
    void createActions();
    void createMenus();
    void createToolBar();
    void createViewports();

    void saveSettings();
    void loadSettings();
    void clearSettings();

    bool alreadyInMenu(const QString &path);
    bool addRecentFile(const QString &fileName);

public slots:
    void setCoordinates(int x, int y, int z);

    /* file menu */
    void openSlot();
    void saveSlot();
    void saveAsSlot();
    void quitSlot();

    /* edit skeleton menu*/
    void addNodeSlot();
    void linkWithActiveNodeSlot();
    void dropNodesSlot();
    void skeletonStatisticsSlot();
    void clearSkeletonSlot();
    void clearSkeletonWithoutConfirmation();



    /* view menu */
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

    /* toolbar slots */
    void copyClipboardCoordinates();
    void pasteClipboardCoordinates();
    void coordinateEditingFinished();

    void uncheckToolsAction();
    void uncheckViewportSettingAction();
    void uncheckCommentShortcutsAction();
    void uncheckConsoleAction();    
    void uncheckDataSavingAction();

    void uncheckSynchronizationAction();
    void uncheckNavigationAction();
    void updateCoordinateBar(int x, int y, int z);  
    void recentFileSelected();
    void treeColorAdjustmentsChanged();

};

#endif // MAINWINDOW_H
