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
#include "color4F.h"
#include "viewport.h"
#include "session.h"
#include "widgetcontainer.h"

#include <array>
#include <memory>

#define FILE_DIALOG_HISTORY_MAX_ENTRIES 10

#include <QMainWindow>
#include <QDropEvent>
#include <QQueue>
#include <QComboBox>
#include <QToolBar>
#include <QUndoStack>
#include "scriptengine/proxies/skeletonproxy.h"

class QLabel;
class QToolButton;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QMessageBox;
class QGridLayout;
class QFile;

class MainWindow : public QMainWindow {
    Q_OBJECT
    friend TaskManagementWidget;
    friend SkeletonProxy;

    void resizeEvent(QResizeEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void resizeToFitViewports(int width, int height);

    QSpinBox *xField, *yField, *zField;
    QMenu fileMenu{"File"};
    QMenu *skelEditMenu;
    QMenu *segEditMenu;
    QString annotationFilename;
    QString openFileDirectory;
    QString saveFileDirectory;

    std::vector<QAction*> commentActions;

    QToolBar basicToolbar{"Basic Functionality"};
    QToolBar defaultToolbar{"Tools"};

    // segmentation job mode
    QToolBar segJobModeToolbar{"Job Navigation"};
    QLabel todosLeftLabel{"<font color='green'>  0 more left</font>"};
    void updateTodosLeft();

    void placeComment(const int index);
public:
    std::array<std::unique_ptr<Viewport>, Viewport::numberViewports> viewports;

    // contains all widgets
    WidgetContainer widgetContainerObject;
    WidgetContainer *widgetContainer;

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

    SkeletonProxy *skeletonProxy;

    QLabel unsavedChangesLabel;
    QLabel annotationTimeLabel;

    void createViewports();

    // for creating action, menus and the toolbar
    void createMenus();
    void createToolbars();

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
    void recentFileSelectSignal(int index);
    void userMoveSignal(int x, int y, int z, UserMoveType userMoveType, ViewportType viewportType);

    void stopRenderTimerSignal();
    void startRenderTimerSignal(int frequency);
    void updateTreeColorsSignal();

    treeListElement *addTreeListElementSignal(int treeID, color4F color);

    void updateTaskDescriptionSignal(QString description);
    void updateTaskCommentSignal(QString comment);

    void resetRotationSignal();
public slots:
    void setJobModeUI(bool enabled);

    // for the recent file menu
    bool openFileDispatch(QStringList fileNames);
    void updateRecentFile(const QString &fileName);

    /* skeleton menu */
    void newAnnotationSlot();
    void openSlot();
    void autosaveSlot();
    void saveSlot();
    void saveAsSlot();
    void exportToNml();

    /* edit skeleton menu*/
    void setAnnotationMode(AnnotationMode mode);
    void clearSkeletonSlotNoGUI();
    void clearSkeletonSlotGUI();
    void setSimpleTracing(bool simple);

    /* view menu */
    void dragDatasetSlot();
    void recenterOnClickSlot();

    /* preferences menu */
    void loadCustomPreferencesSlot();
    void saveCustomPreferencesSlot();
    void defaultPreferencesSlot();

    /* toolbar slots */
    void copyClipboardCoordinates();
    void pasteClipboardCoordinates();
    void coordinateEditingFinished();

    void updateCoordinateBar(int x, int y, int z);
    void recentFileSelected();
    void treeColorAdjustmentsChanged();
    // viewports
    void resetViewports();
    void showVPDecorationClicked();

    // from the event handler
    void newTreeSlot();
    void nextCommentNodeSlot();
    void previousCommentNodeSlot();
    void pushBranchNodeSlot();
    void popBranchNodeSlot();
    void pythonSlot();
    void pythonPropertiesSlot();
    void pythonFileSlot();
};

#endif // MAINWINDOW_H
