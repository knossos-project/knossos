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

#include "viewport.h"
#include "knossos-global.h"
#include "widgetcontainer.h"

#include <array>
#include <memory>

#define FILE_DIALOG_HISTORY_MAX_ENTRIES 10
#define LOCK_VP_ORIENTATION_DEFAULT (true)

#include <QMainWindow>
#include <QDropEvent>
#include <QQueue>
#include <QComboBox>
#include <QUndoStack>
#include "scriptengine/proxies/skeletonproxy.h"

class QLabel;
class QToolBar;
class QToolButton;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QMessageBox;
class QGridLayout;
class QFile;

class MainWindow : public QMainWindow {
    Q_OBJECT
    friend class TaskManagementMainTab;
    friend SkeletonProxy;

protected:
    void resizeEvent(QResizeEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    void resizeViewports(int width, int height);
    QMenu fileMenu{"File"};
    QMenu *skelEditMenu;
    QMenu *segEditMenu;
    QString annotationFilename;
    QString openFileDirectory;
    QString saveFileDirectory;
public:
    QSpinBox *xField, *yField, *zField;
    std::array<std::unique_ptr<Viewport>, NUM_VP> viewports;

    // contains all widgets
    WidgetContainer widgetContainerObject;
    WidgetContainer * widgetContainer;

    std::array<QAction*, FILE_DIALOG_HISTORY_MAX_ENTRIES> historyEntryActions;

    QAction *segEditSegModeAction;
    QAction *segEditSkelModeAction;
    QAction *skelEditSegModeAction;
    QAction *skelEditSkelModeAction;
    QAction *addNodeAction;
    QAction *linkWithActiveNodeAction;
    QAction *dropNodesAction;

    QAction *dragDatasetAction;
    QAction *recenterOnClickAction;

    QQueue<QString> *skeletonFileHistory;
    QFile *loadedFile;

    QCheckBox *lockVPOrientationCheckbox;

    void createViewports();

    // for creating action, menus and the toolbar
    void createMenus();
    void createToolBar();

    // for save, load and clear settings
    void saveSettings();
    void loadSettings();
    void clearSettings();

    explicit MainWindow(QWidget *parent = 0);

    void closeEvent(QCloseEvent *event);
    void notifyUnsavedChanges();
    void updateTitlebar();

    static void reloadDataSizeWin();
    static void datasetColorAdjustmentsChanged();

signals:
    bool changeDatasetMagSignal(uint upOrDownFlag);
    void recalcTextureOffsetsSignal();
    void clearSkeletonSignal(int loadingSkeleton);
    void recentFileSelectSignal(int index);
    void updateToolsSignal();
    void updateTreeviewSignal();
    void updateCommentsTableSignal();
    void userMoveSignal(int x, int y, int z);

    void stopRenderTimerSignal();
    void startRenderTimerSignal(int frequency);
    void updateTreeColorsSignal();
    void loadTreeLUTFallback();

    treeListElement *addTreeListElementSignal(int treeID, color4F color);
    void nextCommentSignal(QString searchString);
    void previousCommentSignal(QString searchString);
    /* */
    void moveToPrevNodeSignal();
    void moveToNextNodeSignal();
    void moveToPrevTreeSignal(bool *isSuccess = NULL);
    void moveToNextTreeSignal(bool *isSuccess = NULL);
    bool popBranchNodeSignal();
    bool pushBranchNodeSignal(int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, uint branchNodeID);
    void jumpToActiveNodeSignal(bool *isSuccess = NULL);

    bool addCommentSignal(QString content, nodeListElement *node, uint nodeID);
    bool editCommentSignal(commentListElement *currentComment, uint nodeID, QString newContent, nodeListElement *newNode, uint newNodeID);

    void updateTaskDescriptionSignal(QString description);
    void updateTaskCommentSignal(QString comment);

    void treeAddedSignal(treeListElement *tree);
    void branchPushedSignal();
    void branchPoppedSignal();
    void nodeCommentChangedSignal(nodeListElement *node);

public slots:
    // for the recent file menu
    bool openFileDispatch(QStringList fileNames);
    void updateRecentFile(const QString &fileName);

    /* skeleton menu */
    void openSlot();
    void autosaveSlot();
    void saveSlot();
    void saveAsSlot();

    /* edit skeleton menu*/
    void segModeSelected();
    void skelModeSelected();
    void skeletonStatisticsSlot();
    void clearSkeletonSlotNoGUI();
    void clearSkeletonSlotGUI();
    void clearSkeletonWithoutConfirmation();
    void setSimpleTracing(bool simple);

    /* view menu */
    void dragDatasetSlot();
    void recenterOnClickSlot();

    /* preferences menu */
    void loadCustomPreferencesSlot();
    void saveCustomPreferencesSlot();
    void defaultPreferencesSlot();

    /* window menu */
    void taskSlot();

    /* toolbar slots */
    void copyClipboardCoordinates();
    void pasteClipboardCoordinates();
    void coordinateEditingFinished();

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
    void F1Slot();
    void F2Slot();
    void F3Slot();
    void F4Slot();
    void F5Slot();
    void pythonSlot();
    void pythonPropertiesSlot();
};

#endif // MAINWINDOW_H
