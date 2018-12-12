/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cheatsheet.h"
#include "scriptengine/proxies/skeletonproxy.h"
#include "session.h"
#include "viewports/viewportarb.h"
#include "viewports/viewportbase.h"
#include "viewports/viewportortho.h"
#include "viewports/viewport3d.h"
#include "widgetcontainer.h"
#include "widgets/coordinatespins.h"

#include <QComboBox>
#include <QDropEvent>
#include <QList>
#include <QMenu>
#include <QMainWindow>
#include <QParallelAnimationGroup>
#include <QProgressBar>
#include <QToolBar>

#include <array>
#include <memory>

enum class SynapseState {
    Off, SynapticCleft, PostSynapse
};

enum class SegmentState {
    On, Off, Off_Once
};

class WorkModeModel : public QAbstractListModel {
    Q_OBJECT
    std::vector<std::pair<AnnotationMode, QString>> workModes;
public:
    virtual int rowCount(const QModelIndex &) const override {
        return workModes.size();
    }
    virtual QVariant data(const QModelIndex & index, int role) const override {
        if (role == Qt::DisplayRole) {
            return workModes[index.row()].second;
        }
        return QVariant();
    }
    void recreate(const std::map<AnnotationMode, QString> & modes) {
        beginResetModel();
        workModes.clear();
        for (const auto & mode : modes) {
            workModes.emplace_back(mode);
        }
        std::sort(std::begin(workModes), std::end(workModes), [](const std::pair<AnnotationMode, QString> elemA, const std::pair<AnnotationMode, QString> elemB) {
            return elemA.second < elemB.second;
        });
        for (uint i = 1; i <= modes.size(); ++i) {
            workModes[i - 1].second = tr("%1. ").arg(i) + workModes[i -1].second;
        }
        endResetModel();
    }

    std::pair<AnnotationMode, QString> at(const int index) const {
        return workModes[index];
    }

    int indexOf(const AnnotationMode & mode) const {
        for (uint i = 0; i < workModes.size(); ++i) {
            if (workModes[i].first == mode) {
                return i;
            }
        }
        return -1;
    }
};

struct LoadingCursor {
    LoadingCursor();
    LoadingCursor(LoadingCursor&) = delete;
    ~LoadingCursor();
};

class MainWindow : public QMainWindow {
    const bool evilHack;
    Q_OBJECT
    friend class TaskManagementWidget;
    friend class SkeletonProxy;

    QToolBar basicToolbar{"Basic Functionality"};
    QToolBar defaultToolbar{"Tools"};

    std::map<AnnotationMode, QString> workModes{ {AnnotationMode::Mode_MergeTracing, tr("Merge Tracing")},
                                                 {AnnotationMode::Mode_Tracing, tr("Tracing")},
                                                 {AnnotationMode::Mode_TracingAdvanced, tr("Tracing Advanced")},
                                                 {AnnotationMode::Mode_Merge, tr("Segmentation Merge")},
                                                 {AnnotationMode::Mode_Paint, tr("Segmentation Paint")},
                                                 {AnnotationMode::Mode_Selection, tr("Review")},
                                               };
    WorkModeModel workModeModel;
    QComboBox modeCombo;
    QAction *cheatsheetAction;
    QAction *clearMergelistAction;
    QAction *clearSkeletonAction;
    QAction *compressionToggleAction;
    QAction *createSynapse;
    static constexpr int FILE_DIALOG_HISTORY_MAX_ENTRIES = 10;
    std::array<QAction*, FILE_DIALOG_HISTORY_MAX_ENTRIES> historyEntryActions;
    QAction *newTreeAction;
    QAction *popBranchAction;
    QAction *pushBranchAction;
    QAction *swapSynapticNodes;
    QAction *toggleSegmentsAction;

    QAction *newObjectAction;
    QAction *decreaseOpacityAction;
    QAction *increaseOpacityAction;
    QAction *enlargeBrushAction;
    QAction *shrinkBrushAction;
    // convenience mode switch actions for proof reading mode
    QAction *modeSwitchSeparator{nullptr};
    QAction *setMergeModeAction{nullptr};
    QAction *setPaintModeAction{nullptr};

    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
    void resizeToFitViewports(int width, int height);

    CoordinateSpins currentPosSpins;
    QMenu fileMenu{"&File"};
    QMenu actionMenu{"&Action"};
    QMenu *pluginMenu;
    QString openFileDirectory;
    QString saveFileDirectory;
    QList<QString> skeletonFileHistory;

    std::vector<QAction*> commentActions;

    QProgressBar networkProgressBar;
    QPushButton networkProgressAbortButton{"Abort"};

    int loaderLastProgress;
    QLabel *loaderProgress;

    // segmentation job mode
    QToolBar segJobModeToolbar{"Job Navigation"};
    QLabel todosLeftLabel{"<font color='green'>  0 more left</font>"};
    void updateTodosLeft();

    // status bar
    QLabel annotationTimeLabel;
    QLabel cursorPositionLabel;
    QLabel GUIModeLabel{""};
    QLabel nodeLockingLabel;
    QLabel segmentStateLabel;
    QLabel synapseStateLabel;
    QLabel unsavedChangesLabel;
    QLabel warningDisabledFeaturesLabel;
    QPixmap warnPixmap{":/resources/icons/warning_disabled_features.png"};

    QTimer activityTimer;
    QParallelAnimationGroup activityAnimation;
    QLabel activityLabel;

    // for creating action, menus and the toolbar
    void createMenus();
    void createToolbars();

    bool placeComment(const int index);
    void toggleSegments();

    virtual bool event(QEvent *event) override;

    Cheatsheet cheatsheet;
public:
    static constexpr auto docUrl = "https://knossos.app/documentation/KNOSSOS_full_doc.html";

    std::unique_ptr<ViewportOrtho> viewportXY;
    std::unique_ptr<ViewportOrtho> viewportXZ;
    std::unique_ptr<ViewportOrtho> viewportZY;
    std::unique_ptr<ViewportArb> viewportArb;
    std::unique_ptr<Viewport3D> viewport3D;

    // contains all widgets
    WidgetContainer widgetContainer;

    SkeletonProxy *skeletonProxy;

    void createViewports();
    // for save, load and clear settings
    void saveSettings();
    void loadSettings();
    void clearSettings();

    explicit MainWindow(QWidget *parent = nullptr);
    template<typename Function> void forEachVPDo(Function func) {
        for (auto * vp : { static_cast<ViewportBase *>(viewportXY.get()), static_cast<ViewportBase *>(viewportXZ.get()), static_cast<ViewportBase *>(viewportZY.get()), static_cast<ViewportBase *>(viewportArb.get()), static_cast<ViewportBase *>(viewport3D.get()) }) {
            func(*vp);
        }
    }
    template<typename Function> void forEachOrthoVPDo(Function func) {
        for (auto * vp : { viewportXY.get(), viewportXZ.get(), viewportZY.get(), static_cast<ViewportOrtho *>(viewportArb.get()) }) {
            func(*vp);
        }
    }
    void resetTextureProperties();
    ViewportBase *viewport(const ViewportType vpType);
    ViewportOrtho *viewportOrtho(const ViewportType vpType);
    virtual void closeEvent(QCloseEvent *event) override;
    void notifyUnsavedChanges();
    void updateTitlebar();

    SegmentState segmentState{SegmentState::On};
    void setSegmentState(const SegmentState newState);
    SynapseState synapseState{SynapseState::Off};
    void setSynapseState(const SynapseState newState);
    void toggleSynapseState();

    void loadCustomPreferences(const QString & fileName);

signals:
    void aboutToSave();
    void datasetDropped(const QUrl &);
    void overlayOpacityChanged();
public slots:
    void refreshPluginMenu();
    void setProofReadingUI(const bool on);
    void setJobModeUI(bool enabled);
    void updateLoaderProgress(int refCount);
    void updateCursorLabel(const Coordinate & position, const ViewportType vpType);
    // for the recent file menu
    bool openFileDispatch(QStringList fileNames, const bool mergeAll = false, const bool silent = false);
    void updateRecentFile(const QString &fileName);

    /* skeleton menu */
    bool newAnnotationSlot();
    void openSlot();
    void saveSlot();
    void saveAsSlotWrap();
    void saveAsSlot(const bool onlySelectedTrees = false);
    void save(QString filename = Session::singleton().annotationFilename, const bool silent = false, const bool allocIncrement = true, const bool onlySelectedTrees = false);
    void exportToNml();
    void updateCommentShortcut(const int index, const QString & comment);

    /* edit skeleton menu*/
    void setWorkMode(AnnotationMode workMode);
    void clearSkeletonSlot();

    /* preferences menu */
    void loadCustomPreferencesSlot();
    void saveCustomPreferencesSlot();
    void defaultPreferencesSlot();

    /* toolbar slots */
    void coordinateEditingFinished();
    void updateCoordinateBar(int x, int y, int z);
    // viewports
    void resetViewports();

    // from the event handler
    void newTreeSlot();
    void pushBranchNodeSlot();
    void pythonPropertiesSlot();
    void pythonFileSlot();
    void pythonInterpreterSlot();
    void pythonPluginMgrSlot();
    // dataset load widget
    void updateCompressionRatioDisplay();
};

auto createGlobalAction = [](auto parent, auto key, auto todo){
    auto & action = *new QAction(parent);
    action.setShortcut(key);
    action.setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect(&action, &QAction::triggered, todo);
    parent->addAction(&action);
};

#endif // MAINWINDOW_H
