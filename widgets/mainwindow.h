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

#include <array>
#include <memory>

#define FILE_DIALOG_HISTORY_MAX_ENTRIES 10
#include <QMainWindow>
#include <QDropEvent>
#include <QQueue>
#include <QComboBox>
#include <QUndoStack>
#include "knossos-global.h"

namespace Ui {
    class MainWindow;
}

class QLabel;
class QToolBar;
class QToolButton;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QRadioButton;
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

    static bool cpBaseDirectory(char *target, QString path);
    static void reloadDataSizeWin();
    static void datasetColorAdjustmentsChanged();

signals:
    bool changeDatasetMagSignal(uint serverMovement);
    void recalcTextureOffsetsSignal();    
    void clearAnnotationSignal(int targetRevision, int loadingSkeleton);
    void updateSkeletonFileNameSignal(int targetRevision, int increment, char *filename);
    bool loadSkeletonSignal(QString fileName);
    bool saveSkeletonSignal(QString fileName);
    void recentFileSelectSignal(int index);
    void updateToolsSignal();
    void updateTreeviewSignal();
    void updateCommentsTableSignal();
    void userMoveSignal(int x, int y, int z, int serverMovement);

    void stopRenderTimerSignal();
    void startRenderTimerSignal(int frequency);
    void updateTreeColorsSignal();
    void loadTreeLUTFallback();

    treeListElement *addTreeListElementSignal(int sync, int targetRevision, int treeID, color4F color, int serialize);
    void nextCommentSignal(QString searchString);
    void previousCommentSignal(QString searchString);
    /* */
    void moveToPrevNodeSignal();
    void moveToNextNodeSignal();
    void moveToPrevTreeSignal();
    void moveToNextTreeSignal();
    void updateTools();
    bool popBranchNodeSignal();
    bool pushBranchNodeSignal(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, int branchNodeID, int serialize);
    void jumpToActiveNodeSignal();

    void jumpToActiveLoopSignal();
    void deactivateLoopSignal();

    bool addCommentSignal(int targetRevision, QString content, nodeListElement *node, int nodeID, int serialize);
    bool editCommentSignal(int targetRevision, commentListElement *currentComment, int nodeID, QString newContent, nodeListElement *newNode, int newNodeID, int serialize);

    void updateTaskDescriptionSignal(QString description);
    void updateTaskCommentSignal(QString comment);

    void keyPressSignal(QKeyEvent *event);
    void treeAddedSignal(treeListElement *tree);
    void branchPushedSignal();
    void branchPoppedSignal();
    void unselectNodesSignal();
    void nodeCommentChangedSignal(nodeListElement *node);
    void viewportDecorationSignal(bool visible);
protected:
    void clearAnnotation(bool isGUI);
    void resizeEvent(QResizeEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    void resizeViewports(int width, int height);
    void becomeFirstEntry(const QString &entry);
    QString *openFileDirectory;
    QString *saveFileDirectory;        
public:
    Ui::MainWindow *ui;

    QToolBar *toolBar;
    QToolButton *copyButton;
    QToolButton *pasteButton;
    QLabel *xLabel, *yLabel, *zLabel;
    QSpinBox *xField, *yField, *zField;
    QLabel *loaderThreadsLabel;
    QSpinBox *loaderThreadsField;

    QRadioButton *patchModeRadio;
    QRadioButton *skeletonModeRadio;
    QMessageBox *prompt;

    QWidget *mainWidget;
    QGridLayout *gridLayout;
    std::array<std::unique_ptr<Viewport>, NUM_VP> viewports;

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
    QAction *clearAnnotationAction;

    QAction *newTreeAction;
    QAction *nextCommentAction;
    QAction *previousCommentAction;
    QAction *pushBranchNodeAction;
    QAction *popBranchNodeAction;
    QAction *moveToPrevNodeAction;
    QAction *moveToNextNodeAction;
    QAction *moveToPrevTreeAction;
    QAction *moveToNextTreeAction;
    QAction *jumpToActiveNodeAction;

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
    QAction *F1Action;
    QAction *F2Action;
    QAction *F3Action;
    QAction *F4Action;
    QAction *F5Action;

    /* window actions */
    QAction *toolsAction;
    QAction *taskAction;
    QAction *logAction;
    QAction *commentShortcutsAction;
    QAction *annotationAction;

    /* help actions */
    QAction *aboutAction;

    /* Qmenu-points */
    QMenuBar *customBar;
    QMenu *dataSetMenu;
    QMenu *fileMenu;
    QMenu *recentFileMenu;
    QMenu *skelMenu; //! action menu for skeleton mode (placing nodes)
    QMenu *patchMenu; //! action menu for patch mode (volume annotation)
    QMenu *workModeEditMenu;
    QMenu *viewMenu;
    QMenu *workModeViewMenu;
    QMenu *preferenceMenu;
    QMenu *windowMenu;
    QMenu *helpMenu;

    QQueue<QString> *skeletonFileHistory;
    QFile *loadedFile;


    QToolButton *open, *save;
    QToolButton *pythonButton;
    QToolButton *tracingTimeButton;
    QToolButton *zoomAndMultiresButton;
    QToolButton *syncButton;
    QToolButton *viewportSettingsButton;
    QToolButton *toolsButton;
    QToolButton *commentShortcutsButton;
    QPushButton *resetVPsButton;
    QPushButton *resetVPOrientButton;
    QCheckBox *lockVPOrientationCheckbox;
    QToolButton *taskManagementButton;
    QToolButton *annotationButton;

    void createViewports();

    // for creating action, menus and the toolbar
    void createActions();
    void createMenus();
    void createToolBar();


    // for save, load and clear settings
    void saveSettings();
    void loadSettings();
    void clearSettings();

public slots:
    // for the recent file menu
    bool loadSkeletonAfterUserDecision(const QString &fileName);
    void updateFileHistoryMenu();
    bool alreadyInMenu(const QString &path);
    bool addRecentFile(const QString &fileName);    
    //QUndoStack *undoStack;

    /* dataset */
    void openDatasetSlot();
    /* skeleton menu */
    void openSlot();
    void openSlot(const QString &fileName); // for the drag n drop version
    void saveSlot();
    void saveAsSlot();
    void quitSlot();

    /* edit skeleton menu*/
    void addNodeSlot();
    void linkWithActiveNodeSlot();
    void dropNodesSlot();
    void skeletonStatisticsSlot();
    void clearAnnotationSlotNoGUI();
    void clearAnnotationSlotGUI();
    void clearAnnotationWithoutConfirmation();

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
    void taskSlot();
    void logSlot();
    void commentShortcutsSlots();
    void annotationSlot();

    /* help menu */
    void aboutSlot();
    void documentationSlot();

    /* toolbar slots */
    void copyClipboardCoordinates();
    void pasteClipboardCoordinates();
    void coordinateEditingFinished();
    void loaderThreadsFieldChanged();
    void patchModeSelected();
    void skeletonModeSelected();

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
    // viewports
    void resetViewports();
    void resetVPOrientation();
    void lockVPOrientation(bool lock);
    void showVPDecorationClicked();

    // from the event handler
    void newTreeSlot();
    void nextCommentNodeSlot();
    void previousCommentNodeSlot();
    void pushBranchNodeSlot();
    void popBranchNodeSlot();
    void moveToPrevNodeSlot();
    void moveToNextNodeSlot();
    void moveToPrevTreeSlot();
    void moveToNextTreeSlot();
    void jumpToActiveNodeSlot();
    void deselectNodesSlot();
    void F1Slot();
    void F2Slot();
    void F3Slot();
    void F4Slot();
    void F5Slot();

    void jumpToActiveLoopSlot();
    void deactivateLoopSlot();
};

#endif // MAINWINDOW_H
