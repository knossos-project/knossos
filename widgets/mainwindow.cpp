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

#include "file_io.h"
#include "GuiConstants.h"
#include "knossos.h"
#include "mainwindow.h"
#include "network.h"
#include "skeleton/node.h"
#include "version.h"
#include "viewer.h"
#include "viewport.h"
#include "scriptengine/scripting.h"
#include "skeleton/skeletonizer.h"
#include "widgets/viewportsettings/vpgeneraltabwidget.h"
#include "widgetcontainer.h"

#include <PythonQt/PythonQt.h>
#include <QAction>
#include <QCheckBox>
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QRegExp>
#include <QSettings>
#include <QSpinBox>
#include <QStringList>
#include <QThread>
#include <QToolButton>
#include <QQueue>

// default position of xy viewport and default viewport size
#define DEFAULT_VP_MARGIN 5
#define DEFAULT_VP_SIZE 350

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), widgetContainerObject(this), widgetContainer(&widgetContainerObject) {
    updateTitlebar();
    this->setWindowIcon(QIcon(":/resources/icons/logo.ico"));

    skeletonFileHistory.reserve(FILE_DIALOG_HISTORY_MAX_ENTRIES);

    QObject::connect(widgetContainer->viewportSettingsWidget->generalTabWidget, &VPGeneralTabWidget::setViewportDecorations, this, &MainWindow::showVPDecorationClicked);
    QObject::connect(widgetContainer->viewportSettingsWidget->generalTabWidget, &VPGeneralTabWidget::resetViewportPositions, this, &MainWindow::resetViewports);
    QObject::connect(widgetContainer->datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this](bool showOverlays) {
        const auto currentMode = workModeModel.at(modeCombo.currentIndex()).first;
        if (!showOverlays) {
            const std::map<AnnotationMode, QString> rawModes{{AnnotationMode::Mode_Tracing, workModes[AnnotationMode::Mode_Tracing]},
                                                             {AnnotationMode::Mode_TracingAdvanced, workModes[AnnotationMode::Mode_TracingAdvanced]},
                                                             {AnnotationMode::Mode_TracingUnlinked, workModes[AnnotationMode::Mode_TracingUnlinked]}};
            workModeModel.recreate(rawModes);
            setWorkMode((rawModes.find(currentMode) != std::end(rawModes))? currentMode : AnnotationMode::Mode_Tracing);
        }
        else {
            workModeModel.recreate(workModes);
            setWorkMode(currentMode);
        }
        widgetContainer->annotationWidget->setSegmentationVisibility(showOverlays);
    });
    QObject::connect(widgetContainer->snapshotWidget, &SnapshotWidget::snapshotRequest,
        [this](const QString & path, ViewportType vp, const int size, const bool withAxes, const bool withOverlay, const bool withSkeleton, const bool withScale, const  bool withVpPlanes) {
            viewports[vp]->takeSnapshot(path, size, withAxes, withOverlay, withSkeleton, withScale, withVpPlanes);
        });
    QObject::connect(&Segmentation::singleton(), &Segmentation::appendedRow, this, &MainWindow::notifyUnsavedChanges);
    QObject::connect(&Segmentation::singleton(), &Segmentation::changedRow, this, &MainWindow::notifyUnsavedChanges);
    QObject::connect(&Segmentation::singleton(), &Segmentation::removedRow, this, &MainWindow::notifyUnsavedChanges);
    QObject::connect(&Segmentation::singleton(), &Segmentation::todosLeftChanged, this, &MainWindow::updateTodosLeft);


    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::autosaveSignal, this, &MainWindow::autosaveSlot);

    createToolbars();
    createMenus();
    setCentralWidget(new QWidget(this));
    setGeometry(0, 0, width(), height());

    createViewports();
    setAcceptDrops(true);

    statusBar()->setSizeGripEnabled(false);
    statusBar()->addPermanentWidget(&unsavedChangesLabel);
    statusBar()->addPermanentWidget(&annotationTimeLabel);

#ifdef Q_OS_WIN
    //manually tweak padding between widgets and both window borders and statusbar seperators
    unsavedChangesLabel.setContentsMargins(0, 0, 3, 0);
    annotationTimeLabel.setContentsMargins(0, 0, 4, 0);
#endif

    QObject::connect(&Session::singleton(), &Session::annotationTimeChanged, &annotationTimeLabel, &QLabel::setText);
}

void MainWindow::createViewports() {
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setMajorVersion(2);
    format.setMinorVersion(0);
    format.setDepthBufferSize(24);
//    format.setSwapInterval(0);
//    format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setOption(QSurfaceFormat::DeprecatedFunctions);
    if (Viewport::oglDebug) {
        format.setOption(QSurfaceFormat::DebugContext);
    }
    QSurfaceFormat::setDefaultFormat(format);

    viewports[VP_UPPERLEFT] = std::unique_ptr<Viewport>(new Viewport(centralWidget(), VIEWPORT_XY, VP_UPPERLEFT));
    viewports[VP_LOWERLEFT] = std::unique_ptr<Viewport>(new Viewport(centralWidget(), VIEWPORT_XZ, VP_LOWERLEFT));
    viewports[VP_UPPERRIGHT] = std::unique_ptr<Viewport>(new Viewport(centralWidget(), VIEWPORT_YZ, VP_UPPERRIGHT));
    viewports[VP_LOWERRIGHT] = std::unique_ptr<Viewport>(new Viewport(centralWidget(), VIEWPORT_SKELETON, VP_LOWERRIGHT));
}

void MainWindow::createToolbars() {
    basicToolbar.setObjectName(basicToolbar.windowTitle());
    defaultToolbar.setObjectName(defaultToolbar.windowTitle());

    basicToolbar.setMovable(false);
    basicToolbar.setFloatable(false);
    basicToolbar.setMaximumHeight(45);

    basicToolbar.addAction(QIcon(":/resources/icons/open-annotation.png"), "Open Annotation", this, SLOT(openSlot()));
    basicToolbar.addAction(QIcon(":/resources/icons/document-save.png"), "Save Annotation", this, SLOT(saveSlot()));
    basicToolbar.addSeparator();
    workModeModel.recreate(workModes);
    modeCombo.setModel(&workModeModel);
    modeCombo.setCurrentIndex(static_cast<int>(AnnotationMode::Mode_Tracing));
    basicToolbar.addWidget(&modeCombo);
    QObject::connect(&modeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](int index) { setWorkMode(workModeModel.at(index).first); });
    modeCombo.setToolTip("<b>Select a work mode:</b><br/>"
                         "<b>" + workModes[AnnotationMode::Mode_MergeTracing] + ":</b> Merge segmentation objects by tracing<br/>"
                         "<b>" + workModes[AnnotationMode::Mode_Merge] + ":</b> Segmentation by merging objects<br/>"
                         "<b>" + workModes[AnnotationMode::Mode_Paint] + ":</b> Segmentation by painting<br/>"
                         "<b>" + workModes[AnnotationMode::Mode_Tracing] + ":</b> Skeletonization on one tree<br/>"
                         "<b>" + workModes[AnnotationMode::Mode_TracingAdvanced] + ":</b> Unrestricted skeletonization<br/>"
                         "<b>" + workModes[AnnotationMode::Mode_TracingUnlinked] + ":</b> Skeletonization with unlinked nodes<br/>");
    basicToolbar.addSeparator();
    basicToolbar.addAction(QIcon(":/resources/icons/edit-copy.png"), "Copy", this, SLOT(copyClipboardCoordinates()));
    basicToolbar.addAction(QIcon(":/resources/icons/edit-paste.png"), "Paste", this, SLOT(pasteClipboardCoordinates()));

    xField = new QSpinBox();
    xField->setRange(1, 1000000);
    xField->setMinimumWidth(75);
    xField->setValue(state->viewerState->currentPosition.x + 1);

    yField = new QSpinBox();
    yField->setRange(1, 1000000);
    yField->setMinimumWidth(75);
    yField->setValue(state->viewerState->currentPosition.y + 1);

    zField = new QSpinBox();
    zField->setRange(1, 1000000);
    zField->setMinimumWidth(75);
    zField->setValue(state->viewerState->currentPosition.z + 1);

    QObject::connect(xField, &QSpinBox::editingFinished, this, &MainWindow::coordinateEditingFinished);
    QObject::connect(yField, &QSpinBox::editingFinished, this, &MainWindow::coordinateEditingFinished);
    QObject::connect(zField, &QSpinBox::editingFinished, this, &MainWindow::coordinateEditingFinished);

    basicToolbar.addWidget(new QLabel("<font color='black'>x</font>"));
    basicToolbar.addWidget(xField);
    basicToolbar.addWidget(new QLabel("<font color='black'>y</font>"));
    basicToolbar.addWidget(yField);
    basicToolbar.addWidget(new QLabel("<font color='black'>z</font>"));
    basicToolbar.addWidget(zField);

    addToolBar(&basicToolbar);
    addToolBar(&defaultToolbar);

    auto createToolToogleButton = [&](const QString & icon, const QString & tooltip){
        auto button = new QToolButton();
        button->setIcon(QIcon(icon));
        button->setToolTip(tooltip);
        button->setCheckable(true);
        defaultToolbar.addWidget(button);
        return button;
    };
    auto taskManagementButton = createToolToogleButton(":/resources/icons/task.png", "Task Management");
    auto zoomAndMultiresButton = createToolToogleButton(":/resources/icons/zoom-in.png", "Dataset Options");
    auto viewportSettingsButton = createToolToogleButton(":/resources/icons/view-list-icons-symbolic.png", "Viewport Settings");
    auto annotationButton = createToolToogleButton(":/resources/icons/graph.png", "Annotation");

    //button → visibility
    QObject::connect(taskManagementButton, &QToolButton::clicked, [this, &taskManagementButton](const bool down){
        if (down) {
            widgetContainer->taskManagementWidget->updateAndRefreshWidget();
        } else {
            widgetContainer->taskManagementWidget->hide();
        }
    });
    QObject::connect(annotationButton, &QToolButton::clicked, widgetContainer->annotationWidget, &AnnotationWidget::setVisible);
    QObject::connect(viewportSettingsButton, &QToolButton::clicked, widgetContainer->viewportSettingsWidget, &ViewportSettingsWidget::setVisible);
    QObject::connect(zoomAndMultiresButton, &QToolButton::clicked, widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::setVisible);
    //visibility → button
    QObject::connect(widgetContainer->taskManagementWidget, &TaskManagementWidget::visibilityChanged, taskManagementButton, &QToolButton::setChecked);
    QObject::connect(widgetContainer->annotationWidget, &AnnotationWidget::visibilityChanged, annotationButton, &QToolButton::setChecked);
    QObject::connect(widgetContainer->viewportSettingsWidget, &ViewportSettingsWidget::visibilityChanged, viewportSettingsButton, &QToolButton::setChecked);
    QObject::connect(widgetContainer->datasetOptionsWidget, &DatasetOptionsWidget::visibilityChanged, zoomAndMultiresButton, &QToolButton::setChecked);

    defaultToolbar.addSeparator();

    auto * const pythonButton = new QToolButton();
    pythonButton->setMenu(new QMenu());
    pythonButton->setIcon(QIcon(":/resources/icons/python.png"));
    pythonButton->setPopupMode(QToolButton::MenuButtonPopup);
    QObject::connect(pythonButton, &QToolButton::clicked, this, &MainWindow::pythonSlot);
    pythonButton->menu()->addAction(QIcon(":/resources/icons/python.png"), "Python Properties", this, SLOT(pythonPropertiesSlot()));
    pythonButton->menu()->addAction(QIcon(":/resources/icons/python.png"), "Python File", this, SLOT(pythonFileSlot()));
    defaultToolbar.addWidget(pythonButton);

    defaultToolbar.addSeparator();

    auto resetVPsButton = new QPushButton("Reset VP Positions", this);
    resetVPsButton->setToolTip("Reset viewport positions and sizes");
    defaultToolbar.addWidget(resetVPsButton);
    QObject::connect(resetVPsButton, &QPushButton::clicked, this, &MainWindow::resetViewports);

    defaultToolbar.addWidget(new QLabel(" Loader pending: "));
    loaderProgress = new QLabel();
    defaultToolbar.addWidget(loaderProgress);
    loaderLastProgress = 0;
    loaderProgress->setFixedWidth(25);
    loaderProgress->setAlignment(Qt::AlignCenter);
    QObject::connect(&Loader::Controller::singleton(), &Loader::Controller::progress, this, &MainWindow::updateLoaderProgress);

    // segmentation task mode toolbar
    auto prevBtn = new QPushButton("< Last");
    auto nextBtn = new QPushButton("(N)ext >");
    auto splitBtn = new QPushButton("Split required >");
    prevBtn->setToolTip("Go back to last task.");
    nextBtn->setToolTip("Mark current task as finished and go to next one.");
    nextBtn->setShortcut(QKeySequence(Qt::Key_N));
    QObject::connect(prevBtn, &QPushButton::clicked, [](bool) { Segmentation::singleton().selectPrevTodoObject(); });
    QObject::connect(nextBtn, &QPushButton::clicked, [](bool) { Segmentation::singleton().selectNextTodoObject(); });
    QObject::connect(splitBtn, &QPushButton::clicked, [](bool) { Segmentation::singleton().markSelectedObjectForSplitting(state->viewerState->currentPosition); });

    segJobModeToolbar.addWidget(prevBtn);
    segJobModeToolbar.addWidget(nextBtn);
    segJobModeToolbar.addWidget(splitBtn);
    segJobModeToolbar.addSeparator();
    segJobModeToolbar.addWidget(&todosLeftLabel);
}

void MainWindow::updateLoaderProgress(int refCount) {
    if ((refCount % 5 > 0) && (loaderLastProgress > 0)) {
        return;
    }
    loaderLastProgress = refCount;
    QPalette pal;
    pal.setColor(QPalette::WindowText, Qt::black);
    pal.setColor(loaderProgress->backgroundRole(), QColor(refCount > 0 ? Qt::red : Qt::green).lighter());
    loaderProgress->setAutoFillBackground(true);
    loaderProgress->setPalette(pal);
    loaderProgress->setText(QString::number(refCount));
}

void MainWindow::setJobModeUI(bool enabled) {
    if(enabled) {
        setWorkMode(AnnotationMode::Mode_MergeSimple);
        menuBar()->hide();
        widgetContainer->hideAll();
        removeToolBar(&defaultToolbar);
        addToolBar(&segJobModeToolbar);
        segJobModeToolbar.show(); // toolbar is hidden by removeToolBar
        for(uint i = 1; i < Viewport::numberViewports; ++i) {
            viewports[i].get()->hide();
        }
        viewports[VIEWPORT_XY].get()->resize(centralWidget()->height() - DEFAULT_VP_MARGIN, centralWidget()->height() - DEFAULT_VP_MARGIN);
    } else {
        menuBar()->show();
        removeToolBar(&segJobModeToolbar);
        addToolBar(&defaultToolbar);
        defaultToolbar.show();
        resetViewports();
    }
}

void MainWindow::updateTodosLeft() {
    int todosLeft = Segmentation::singleton().todolist().size();
    auto & job = Segmentation::singleton().job;

    if(todosLeft > 0) {
        todosLeftLabel.setText(QString("<font color='red'>  %1 more left</font>").arg(todosLeft));
    }
    else if(Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeSimple)) {
        todosLeftLabel.setText(QString("<font color='green'>  %1 more left</font>").arg(todosLeft));
        // submit work
        QRegExp regex("\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}\\b");
        if(regex.exactMatch(job.submitPath)) { // submission by email
            QMessageBox msgBox(QMessageBox::Information,
                               "Good Job, you're done!", QString("Please save your work and send it to:\n%0").arg(job.submitPath));
            msgBox.exec();
            return;
        }
        // submission by upload
        QMessageBox msgBox(QMessageBox::Question,
                           "Good Job, you're done!", "Submit your work now to receive a verification?",
                           QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if(msgBox.exec() == QMessageBox::Yes) {
            auto jobFilename = "final_" + QFileInfo(Session::singleton().annotationFilename).fileName();
            auto finishedJobPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/segmentationJobs/" + jobFilename;
            annotationFileSave(finishedJobPath, nullptr);
            Network::singleton().submitSegmentationJob(finishedJobPath);
        }
    }
}

void MainWindow::notifyUnsavedChanges() {
    state->skeletonState->unsavedChanges = true;
    updateTitlebar();
}

void MainWindow::updateTitlebar() {
    QString title = qApp->applicationDisplayName() + " showing ";
    if (!Session::singleton().annotationFilename.isEmpty()) {
        title.append(Session::singleton().annotationFilename);
    } else {
        title.append("no annotation file");
    }
    if (state->skeletonState->unsavedChanges) {
        title.append("*");
        const auto autosave = tr(" (") + (!state->skeletonState->autoSaveBool ? "<b>NOT</b> " : "") + "autosaving)";
        unsavedChangesLabel.setText(tr("unsaved changes") + autosave);
    } else {
        unsavedChangesLabel.setText("saved");
    }
    //don’t display if there are no changes and no file is loaded
    if (!state->skeletonState->unsavedChanges && Session::singleton().annotationFilename.isEmpty()) {
        unsavedChangesLabel.hide();
        annotationTimeLabel.hide();
    } else {
        unsavedChangesLabel.show();
        annotationTimeLabel.show();
    }

    if(!state->skeletonState->autoSaveBool) {
        title.append(" Autosave: OFF");
    }

    setWindowTitle(title);
}

void MainWindow::updateRecentFile(const QString & fileName) {
    int pos = skeletonFileHistory.indexOf(fileName);
    if (pos != -1) {//move to front if already existing
        skeletonFileHistory.move(pos, 0);
    } else {
        if (skeletonFileHistory.size() == FILE_DIALOG_HISTORY_MAX_ENTRIES) {//shrink if necessary
           skeletonFileHistory.pop_back();
        }
        skeletonFileHistory.push_front(fileName);
    }
    //update the menu
    int i = 0;
    for (const auto & path : skeletonFileHistory) {
        historyEntryActions[i]->setText(path);
        historyEntryActions[i]->setVisible(!path.isEmpty());
        ++i;
    }
}

void MainWindow::treeColorAdjustmentsChanged() {
    if (state->viewerState->treeColortableOn && state->viewerState->treeLutSet) {//user lut activated and  selected
        memcpy(state->viewerState->treeAdjustmentTable, state->viewerState->treeColortable, RGB_LUTSIZE * sizeof(float));
        Skeletonizer::singleton().updateTreeColors();
    } else {//use of default lut
        memcpy(state->viewerState->treeAdjustmentTable, state->viewerState->defaultTreeTable, RGB_LUTSIZE * sizeof(float));
        Skeletonizer::singleton().updateTreeColors();
    }
}

void MainWindow::datasetColorAdjustmentsChanged() {
    bool doAdjust = false;
    int i = 0;
    int dynIndex;
    GLuint tempTable[3][256];

    if(state->viewerState->datasetColortableOn) {
        memcpy(state->viewerState->datasetAdjustmentTable,
               state->viewerState->datasetColortable,
               RGB_LUTSIZE * sizeof(GLuint));
        doAdjust = true;
    } else {
        memcpy(state->viewerState->datasetAdjustmentTable,
               state->viewerState->neutralDatasetTable,
               RGB_LUTSIZE * sizeof(GLuint));
    }

    /*
     * Apply the dynamic range settings to the adjustment table
     *
     */
    if((state->viewerState->luminanceBias != 0) ||
       (state->viewerState->luminanceRangeDelta != MAX_COLORVAL)) {
        for(i = 0; i < 256; i++) {
            dynIndex = (int)((i - state->viewerState->luminanceBias) /
                                 (state->viewerState->luminanceRangeDelta / MAX_COLORVAL));

            if(dynIndex < 0)
                dynIndex = 0;
            if(dynIndex > MAX_COLORVAL)
                dynIndex = MAX_COLORVAL;

            tempTable[0][i] = state->viewerState->datasetAdjustmentTable[0][dynIndex];
            tempTable[1][i] = state->viewerState->datasetAdjustmentTable[1][dynIndex];
            tempTable[2][i] = state->viewerState->datasetAdjustmentTable[2][dynIndex];
        }

        for(i = 0; i < 256; i++) {
            state->viewerState->datasetAdjustmentTable[0][i] = tempTable[0][i];
            state->viewerState->datasetAdjustmentTable[1][i] = tempTable[1][i];
            state->viewerState->datasetAdjustmentTable[2][i] = tempTable[2][i];
        }

        doAdjust = true;
    }
    state->viewerState->datasetAdjustmentOn = doAdjust;

    state->viewer->dc_reslice_notify_visible();
}

/** This slot is called if one of the entries is clicked in the recent file menue */
void MainWindow::recentFileSelected() {
    QAction *action = (QAction *)sender();

    QString fileName = action->text();
    if(fileName.isNull() == false) {
        openFileDispatch(QStringList(fileName));
    }
}

template<typename Menu, typename Receiver, typename Slot>
QAction & addApplicationShortcut(Menu & menu, const QIcon & icon, const QString & caption, const Receiver * receiver, const Slot slot, const QKeySequence & keySequence) {
    auto & action = *menu.addAction(icon, caption);
    action.setShortcut(keySequence);
    action.setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect(&action, &QAction::triggered, receiver, slot);
    return action;
}

void MainWindow::createMenus() {
    menuBar()->addMenu(&fileMenu);
    fileMenu.addAction(QIcon(":/resources/icons/open-dataset.png"), tr("Choose Dataset…"), this->widgetContainer->datasetLoadWidget, SLOT(show()));
    fileMenu.addSeparator();
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/graph.png"), tr("Create New Annotation"), this, &MainWindow::newAnnotationSlot, QKeySequence::New);
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/open-annotation.png"), tr("Load Annotation…"), this, &MainWindow::openSlot, QKeySequence::Open);
    auto & recentfileMenu = *fileMenu.addMenu(QIcon(":/resources/icons/document-open-recent.png"), tr("Recent Annotation File(s)"));
    for (auto & elem : historyEntryActions) {
        elem = recentfileMenu.addAction(QIcon(":/resources/icons/document-open-recent.png"), "");
        elem->setVisible(false);
        QObject::connect(elem, &QAction::triggered, this, &MainWindow::recentFileSelected);
    }
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/document-save.png"), tr("Save Annotation"), this, &MainWindow::saveSlot, QKeySequence::Save);
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/document-save-as.png"), tr("Save Annotation As…"), this, &MainWindow::saveAsSlot, QKeySequence::SaveAs);
    fileMenu.addSeparator();
    fileMenu.addAction(tr("Export to NML..."), this, SLOT(exportToNml()));
    fileMenu.addSeparator();
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/system-shutdown.png"), tr("Quit"), this, &MainWindow::close, QKeySequence::Quit);

    newTreeAction = &addApplicationShortcut(actionMenu, QIcon(), tr("New Tree"), this, &MainWindow::newTreeSlot, Qt::Key_C);
    pushBranchAction = &addApplicationShortcut(actionMenu, QIcon(), tr("Push Branch Node"), this, &MainWindow::pushBranchNodeSlot, Qt::Key_B);
    popBranchAction = &addApplicationShortcut(actionMenu, QIcon(), tr("Pop Branch Node"), this, &MainWindow::popBranchNodeSlot, Qt::Key_J);
    clearSkeletonAction = actionMenu.addAction(QIcon(":/resources/icons/user-trash.png"), "Clear Skeleton", this, SLOT(clearSkeletonSlotGUI()));
    clearMergelistAction = actionMenu.addAction(QIcon(":/resources/icons/user-trash.png"), "Clear Merge List", &Segmentation::singleton(), SLOT(clear()));

    menuBar()->addMenu(&actionMenu);

    auto viewMenu = menuBar()->addMenu("Navigation");

    QActionGroup* workModeViewMenuGroup = new QActionGroup(this);
    dragDatasetAction = workModeViewMenuGroup->addAction(tr("Drag Dataset"));
    dragDatasetAction->setCheckable(true);

    recenterOnClickAction = workModeViewMenuGroup->addAction(tr("Recenter on Click"));
    recenterOnClickAction->setCheckable(true);

    if(state->viewerState->clickReaction == ON_CLICK_DRAG) {
        dragDatasetAction->setChecked(true);
    } else if(state->viewerState->clickReaction == ON_CLICK_RECENTER) {
        recenterOnClickAction->setChecked(true);
    }

    QObject::connect(dragDatasetAction, &QAction::triggered, this, &MainWindow::dragDatasetSlot);
    QObject::connect(recenterOnClickAction, &QAction::triggered, this, &MainWindow::recenterOnClickSlot);

    viewMenu->addActions({dragDatasetAction, recenterOnClickAction});

    viewMenu->addSeparator();

    QAction * penmodeAction = new QAction("Pen-Mode", this);

    penmodeAction->setCheckable(true);

    QObject::connect(penmodeAction, &QAction::triggered, [this, penmodeAction]() {
        state->viewerState->penmode = penmodeAction->isChecked();
    });

    viewMenu->addActions({penmodeAction});

    viewMenu->addSeparator();

    addApplicationShortcut(*viewMenu, QIcon(), tr("Jump To Active Node"), &Skeletonizer::singleton(), []() {
        if(state->skeletonState->activeNode) {
            Skeletonizer::singleton().jumpToNode(*state->skeletonState->activeNode);
        }
    }, Qt::Key_S);
    addApplicationShortcut(*viewMenu, QIcon(), tr("Move To Next Node"), &Skeletonizer::singleton(), &Skeletonizer::moveToNextNode, Qt::Key_X);
    addApplicationShortcut(*viewMenu, QIcon(), tr("Move To Previous Node"), &Skeletonizer::singleton(), &Skeletonizer::moveToPrevNode, Qt::SHIFT + Qt::Key_X);
    addApplicationShortcut(*viewMenu, QIcon(), tr("Move To Next Tree"), &Skeletonizer::singleton(), &Skeletonizer::moveToNextTree, Qt::Key_Z);
    addApplicationShortcut(*viewMenu, QIcon(), tr("Move To Previous Tree"), &Skeletonizer::singleton(), &Skeletonizer::moveToPrevTree, Qt::SHIFT + Qt::Key_Z);

    viewMenu->addSeparator();

    viewMenu->addAction(tr("Dataset Navigation Options"), widgetContainer->navigationWidget, SLOT(show()));

    auto commentsMenu = menuBar()->addMenu("Comments");

    addApplicationShortcut(*commentsMenu, QIcon(), tr("Next Comment"), this, &MainWindow::nextCommentNodeSlot, Qt::Key_N);
    addApplicationShortcut(*commentsMenu, QIcon(), tr("Previous Comment"), this, &MainWindow::previousCommentNodeSlot, Qt::Key_P);

    commentsMenu->addSeparator();

    auto addCommentShortcut = [&](const int number, const QKeySequence key, const QString & description){
        auto & action = addApplicationShortcut(*commentsMenu, QIcon(), description, this, [this, number](){placeComment(number-1);}, key);
        commentActions.push_back(&action);
    };
    for (int number = 1; number < 11; ++number) {
        const auto numberString = QString::number(number) + (number == 1 ? "st" : number == 2 ? "nd" : number == 3 ? "rd" : "th");
        addCommentShortcut(number, QKeySequence(QString("F%1").arg(number)), numberString + tr(" Comment Shortcut"));
    }

    commentsMenu->addSeparator();

    auto preferenceMenu = menuBar()->addMenu("Preferences");
    preferenceMenu->addAction(tr("Load Custom Preferences"), this, SLOT(loadCustomPreferencesSlot()));
    preferenceMenu->addAction(tr("Save Custom Preferences"), this, SLOT(saveCustomPreferencesSlot()));
    preferenceMenu->addAction(tr("Reset to Default Preferences"), this, SLOT(defaultPreferencesSlot()));
    preferenceMenu->addSeparator();
    preferenceMenu->addAction(tr("Data Saving Options"), widgetContainer->dataSavingWidget, SLOT(show()));
    preferenceMenu->addAction(QIcon(":/resources/icons/view-list-icons-symbolic.png"), "Viewport Settings", widgetContainer->viewportSettingsWidget, SLOT(show()));

    auto windowMenu = menuBar()->addMenu("Windows");
    windowMenu->addAction(QIcon(":/resources/icons/task.png"), "Task Management", widgetContainer->taskManagementWidget, SLOT(updateAndRefreshWidget()));
    windowMenu->addAction(QIcon(":/resources/icons/graph.png"), "Annotation Window", widgetContainer->annotationWidget, SLOT(show()));
    windowMenu->addAction(QIcon(":/resources/icons/zoom-in.png"), "Dataset Options", widgetContainer->datasetOptionsWidget, SLOT(show()));
    windowMenu->addAction(tr("Take a snapshot"), widgetContainer->snapshotWidget, SLOT(show()));

    auto helpMenu = menuBar()->addMenu("Help");
    addApplicationShortcut(*helpMenu, QIcon(":/resources/icons/edit-select-all.png"), tr("Documentation"), widgetContainer->docWidget, &DocumentationWidget::show, Qt::CTRL + Qt::Key_H);
    helpMenu->addAction(QIcon(":/resources/icons/knossos.png"), "About", widgetContainer->splashWidget, SLOT(show()));
}

void MainWindow::closeEvent(QCloseEvent *event) {
    EmitOnCtorDtor eocd(&SignalRelay::Signal_MainWindow_closeEvent, state->signalRelay, event);
    saveSettings();

    if(state->skeletonState->unsavedChanges) {
         QMessageBox question;
         question.setWindowFlags(Qt::WindowStaysOnTopHint);
         question.setIcon(QMessageBox::Question);
         question.setWindowTitle("Confirmation required.");
         question.setText("There are unsaved changes. Really Quit?");
         question.addButton("Yes", QMessageBox::ActionRole);
         const QPushButton * const no = question.addButton("No", QMessageBox::ActionRole);
         question.exec();
         if(question.clickedButton() == no) {
             event->ignore();
             return;//we changed our mind – we dont want to quit anymore
         }
    }

    Knossos::sendQuitSignal();
    for(int i = 0; i < 4; i++) {
        viewports[i].get()->setParent(NULL);
    }

    event->accept();//mainwindow takes the qapp with it
}

//file menu functionality
bool MainWindow::openFileDispatch(QStringList fileNames) {
    if (fileNames.empty()) {
        return false;
    }

    bool mergeSkeleton = false;
    bool mergeSegmentation = false;
    if (state->skeletonState->treeElements > 0) {
        const auto text = tr("Which Action do you like to choose?<ul>")
            + tr("<li>Merge the new Skeleton into the current one</li>")
            + tr("<li>Override the current Skeleton</li>")
            + tr("</ul>");
        const auto button = QMessageBox::question(this, tr("Existing Skeleton"), text, tr("Merge"), tr("Override"), tr("Cancel"), 0, 2);

        if (button == 0) {
            mergeSkeleton = true;
        } else if(button == 1) {//clear skeleton
            mergeSkeleton = false;
        } else {
            return false;
        }
    }
    if (Segmentation::singleton().hasObjects()) {
        const auto text = tr("Which Action do you like to choose?<ul>")
            + tr("<li>Merge the new Mergelist into the current one?</li>")
            + tr("<li>Override the current Segmentation</li>")
            + tr("</ul>");
        const auto button = QMessageBox::question(this, tr("Existing Merge List"), text, tr("Merge"), tr("Clear and Load"), tr("Cancel"), 0, 2);

        if (button == 0) {
            mergeSegmentation = true;
        } else if (button == 1) {//clear segmentation
            Segmentation::singleton().clear();
            mergeSegmentation = false;
        } else if (button == 2) {
            return false;
        }
    }

    bool multipleFiles = fileNames.size() > 1;
    bool success = true;

    auto nmlEndIt = std::stable_partition(std::begin(fileNames), std::end(fileNames), [](const QString & elem){
        return QFileInfo(elem).suffix() == "nml";
    });

    state->skeletonState->mergeOnLoadFlag = mergeSkeleton;

    const auto skeletonSignalBlockState = Skeletonizer::singleton().signalsBlocked();
    Skeletonizer::singleton().blockSignals(true);
    auto nmls = std::vector<QString>(std::begin(fileNames), nmlEndIt);
    for (const auto & filename : nmls) {
        const QString treeCmtOnMultiLoad = multipleFiles ? QFileInfo(filename).fileName() : "";
        QFile file(filename);
        if (success &= state->viewer->skeletonizer->loadXmlSkeleton(file, treeCmtOnMultiLoad)) {
            updateRecentFile(filename);
        }
        state->skeletonState->mergeOnLoadFlag = true;//multiple files have to be merged
    }

    auto zips = std::vector<QString>(nmlEndIt, std::end(fileNames));
    for (const auto & filename : zips) {
        const QString treeCmtOnMultiLoad = multipleFiles ? QFileInfo(filename).fileName() : "";
        annotationFileLoad(filename, treeCmtOnMultiLoad);
        updateRecentFile(filename);
        state->skeletonState->mergeOnLoadFlag = true;//multiple files have to be merged
    }
    Skeletonizer::singleton().blockSignals(skeletonSignalBlockState);
    Skeletonizer::singleton().resetData();

    state->skeletonState->unsavedChanges = mergeSkeleton || mergeSegmentation;//merge implies changes

    Session::singleton().annotationFilename = "";
    if (success && !multipleFiles) { // either an .nml or a .k.zip was loaded
        Session::singleton().annotationFilename = nmls.empty() ? zips.front() : nmls.front();
    }
    updateTitlebar();

    if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeSimple)) { // we need to apply job mode here to ensure that all necessary parts are loaded by now.
        setJobModeUI(true);
        Segmentation::singleton().startJobMode();
    }
    return success;
}

void MainWindow::newAnnotationSlot() {
    if (state->skeletonState->unsavedChanges) {
        const auto text = tr("There are unsaved changes. \nCreating a new annotation will make you lose what you’ve done.");
        const auto button = QMessageBox::question(this, tr("Unsaved changes"), text, tr("Abandon changes – Start from scratch"), tr("Cancel"), QString(), 0);
        if (button == 1) {
            return;
        }
    }
    Skeletonizer::singleton().clearSkeleton();
    Segmentation::singleton().clear();
    state->skeletonState->unsavedChanges = false;
    Session::singleton().annotationFilename = "";
}

/**
  * opens the file dialog and receives a skeleton file name path. If the file dialog is not cancelled
  * the skeletonFileHistory queue is updated with the file name entry. The history entries are compared to the
  * selected file names. If the file is already loaded it will not be put into the queue
  *
  */
void MainWindow::openSlot() {
#ifdef Q_OS_MAC
    QString choices = "KNOSSOS Annotation file(s) (*.zip *.nml)";
#else
    QString choices = "KNOSSOS Annotation file(s) (*.k.zip *.nml)";
#endif
    state->viewerState->renderInterval = SLOW;
    QStringList fileNames = QFileDialog::getOpenFileNames(this, "Open Annotation File(s)", openFileDirectory, choices);
    state->viewerState->renderInterval = FAST;
    if (fileNames.empty() == false) {
        openFileDirectory = QFileInfo(fileNames.front()).absolutePath();
        openFileDispatch(fileNames);
    }
}

void MainWindow::autosaveSlot() {
    if (Session::singleton().annotationFilename.isEmpty()) {
        Session::singleton().annotationFilename = annotationFileDefaultPath();
    }
    saveSlot();
}

void MainWindow::saveSlot() {
    auto & annotationFilename = Session::singleton().annotationFilename;
    if (annotationFilename.isEmpty()) {
        saveAsSlot();
    } else {
        if(annotationFilename.endsWith(".nml")) {
            annotationFilename.chop(4);
            annotationFilename += ".k.zip";
        }
        if (state->skeletonState->autoFilenameIncrementBool) {
            int index = skeletonFileHistory.indexOf(annotationFilename);
            updateFileName(annotationFilename);
            if (index != -1) {//replace old filename with updated one
                skeletonFileHistory.replace(index, annotationFilename);
                historyEntryActions[index]->setText(annotationFilename);
            }
        }
        annotationFileSave(annotationFilename);

        updateRecentFile(annotationFilename);
        updateTitlebar();
    }
}

void MainWindow::saveAsSlot() {
    const auto & suggestedFile = saveFileDirectory.isEmpty() ? annotationFileDefaultPath() : saveFileDirectory + '/' + annotationFileDefaultName();

    state->viewerState->renderInterval = SLOW;
#ifdef Q_OS_MAC
    QString fileName = QFileDialog::getSaveFileName(this, "Save the KNOSSSOS Annotation file", suggestedFile);
#else
    QString fileName = QFileDialog::getSaveFileName(this, "Save the KNOSSSOS Annotation file", suggestedFile, "KNOSSOS Annotation file (*.k.zip)");
#endif
    state->viewerState->renderInterval = FAST;

    if (!fileName.isEmpty()) {
        const auto prevFilename = fileName;
        QRegExp kzipRegex(R"regex((\.k)|(\.zip))regex"); // any occurance of .k and .zip
        if (fileName.contains(kzipRegex)) {
            fileName.remove(kzipRegex);

            if (prevFilename != fileName + ".k.zip") {
                const auto message = tr("The supplied filename has been changed: \n") + prevFilename + " to\n" + fileName + ".k.zip";
                QMessageBox::information(this, tr("Fixed filename"), message);
            }
        }
        fileName += ".k.zip";

        Session::singleton().annotationFilename = fileName;
        saveFileDirectory = QFileInfo(fileName).absolutePath();

        annotationFileSave(Session::singleton().annotationFilename);

        updateRecentFile(Session::singleton().annotationFilename);
        updateTitlebar();
    }
}

void MainWindow::exportToNml() {
    if(!state->skeletonState->firstTree) {
        QMessageBox::information(this, "No Save", "No skeleton was found. Not saving!");
        return;
    }
    auto info = QFileInfo(Session::singleton().annotationFilename);
    auto defaultpath = annotationFileDefaultPath();
    defaultpath.chop(6);
    defaultpath += ".nml";
    const auto & suggestedFilepath = Session::singleton().annotationFilename.isEmpty() ? defaultpath : info.absoluteDir().path() + "/" + info.baseName() + ".nml";
    state->viewerState->renderInterval = SLOW;
    auto filename = QFileDialog::getSaveFileName(this, "Export to Skeleton file", suggestedFilepath, "KNOSSOS Skeleton file (*.nml)");
    state->viewerState->renderInterval = FAST;
    if(filename.isEmpty() == false) {
        if(filename.endsWith(".nml") == false) {
            filename += ".nml";
        }
        nmlExport(filename);
    }
}

void MainWindow::setWorkMode(AnnotationMode workMode) {
    if(workModes.find(workMode) == std::end(workModes)) {
        workMode = AnnotationMode::Mode_Tracing;
    }
    modeCombo.setCurrentText(workModes[workMode]);
    auto & mode = Session::singleton().annotationMode;
    mode = workMode;
    const bool trees = mode.testFlag(AnnotationMode::Mode_TracingAdvanced) || mode.testFlag(AnnotationMode::Mode_TracingUnlinked) || mode.testFlag(AnnotationMode::Mode_MergeTracing);
    const bool skeleton = mode.testFlag(AnnotationMode::Mode_Tracing) || mode.testFlag(AnnotationMode::Mode_TracingAdvanced) || mode.testFlag(AnnotationMode::Mode_TracingUnlinked) || mode.testFlag(AnnotationMode::Mode_MergeTracing);
    const bool segmentation = mode.testFlag(AnnotationMode::Brush) || mode.testFlag(AnnotationMode::Mode_MergeTracing);
    newTreeAction->setVisible(trees);
    widgetContainer->annotationWidget->commandsTab.enableNewTreeButton(trees);
    pushBranchAction->setVisible(mode.testFlag(AnnotationMode::NodeEditing));
    popBranchAction->setVisible(mode.testFlag(AnnotationMode::NodeEditing));
    clearSkeletonAction->setVisible(skeleton);
    clearMergelistAction->setVisible(segmentation);
}

void MainWindow::clearSkeletonSlotGUI() {
    if(state->skeletonState->unsavedChanges || state->skeletonState->treeElements > 0) {
        QMessageBox question;
        question.setWindowFlags(Qt::WindowStaysOnTopHint);
        question.setIcon(QMessageBox::Question);
        question.setWindowTitle("Confirmation required");
        question.setText("Really clear skeleton (you cannot undo this)?");
        QPushButton *ok = question.addButton(QMessageBox::Ok);
        question.addButton(QMessageBox::No);
        question.exec();
        if(question.clickedButton() == ok) {
            clearSkeletonSlotNoGUI();
        }
    }
}

void MainWindow::clearSkeletonSlotNoGUI() {
    Skeletonizer::singleton().clearSkeleton();
    updateTitlebar();
}

/* view menu functionality */

void MainWindow::dragDatasetSlot() {
   state->viewerState->clickReaction = ON_CLICK_DRAG;
   if(recenterOnClickAction->isChecked()) {
       recenterOnClickAction->setChecked(false);
   }
}

void MainWindow::recenterOnClickSlot() {
   state->viewerState->clickReaction = ON_CLICK_RECENTER;
   if(dragDatasetAction->isChecked()) {
       dragDatasetAction->setChecked(false);
   }
}

/* preference menu functionality */
void MainWindow::loadCustomPreferencesSlot()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Custom Preferences File", QDir::homePath(), "KNOSOS GUI preferences File (*.ini)");
    if(!fileName.isEmpty()) {
        QSettings settings;

        QSettings settingsToLoad(fileName, QSettings::IniFormat);
        QStringList keys = settingsToLoad.allKeys();
        for(int i = 0; i < keys.size(); i++) {
            settings.setValue(keys.at(i), settingsToLoad.value(keys.at(i)));
        }

        loadSettings();
    }
}

void MainWindow::saveCustomPreferencesSlot()
{
    saveSettings();
    QSettings settings;
    QString originSettings = settings.fileName();

    QString fileName = QFileDialog::getSaveFileName(this, "Save Custom Preferences File As", QDir::homePath(), "KNOSSOS GUI preferences File (*.ini)");
    if(!fileName.isEmpty()) {
        QFile file;
        file.setFileName(originSettings);
        file.copy(fileName);
    }
}

void MainWindow::defaultPreferencesSlot() {
    QMessageBox question;
    question.setWindowFlags(Qt::WindowStaysOnTopHint);
    question.setIcon(QMessageBox::Question);
    question.setWindowTitle("Confirmation required");
    question.setText("Do you really want to load the default preferences?");
    QPushButton *yes = question.addButton(QMessageBox::Yes);
    question.addButton(QMessageBox::No);
    question.exec();

    if(question.clickedButton() == yes) {
        clearSettings();
        loadSettings();
        Knossos::loadDefaultTreeLUT();
        treeColorAdjustmentsChanged();
        datasetColorAdjustmentsChanged();
        this->setGeometry(QApplication::desktop()->availableGeometry().topLeft().x() + 20,
                          QApplication::desktop()->availableGeometry().topLeft().y() + 50, 1024, 800);
    }
}

/* toolbar slots */

void MainWindow::copyClipboardCoordinates() {
    const auto content = QString("%0, %1, %2").arg(xField->value()).arg(yField->value()).arg(zField->value());
    QApplication::clipboard()->setText(content);
}

void MainWindow::pasteClipboardCoordinates(){
    const QString clipboardContent = QApplication::clipboard()->text();

    const QRegExp clipboardRegEx(R"regex((\d+)\D+(\d+)\D+(\d+))regex");//match 3 groups of digits separated by non-digits
    if (clipboardRegEx.indexIn(clipboardContent) != -1) {//also fails if clipboard is empty
        const auto x = clipboardRegEx.cap(1).toInt();//index 0 is the whole matched text
        const auto y = clipboardRegEx.cap(2).toInt();
        const auto z = clipboardRegEx.cap(3).toInt();

        xField->setValue(x);
        yField->setValue(y);
        zField->setValue(z);

        coordinateEditingFinished();
    } else {
        qDebug() << "Unable to extract coordinates from clipboard content" << clipboardContent;
    }
}

void MainWindow::coordinateEditingFinished() {
    const auto viewer_offset_x = xField->value() - 1 - state->viewerState->currentPosition.x;
    const auto viewer_offset_y = yField->value() - 1 - state->viewerState->currentPosition.y;
    const auto viewer_offset_z = zField->value() - 1 - state->viewerState->currentPosition.z;
    state->viewer->userMove(viewer_offset_x, viewer_offset_y, viewer_offset_z, USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
}

void MainWindow::saveSettings() {
    QSettings settings;

    settings.beginGroup(MAIN_WINDOW);
    settings.setValue(GEOMETRY, saveGeometry());
    settings.setValue(STATE, saveState());

    // viewport position and sizes
    settings.setValue(VP_DEFAULT_POS_SIZE, state->viewerState->defaultVPSizeAndPos);
    for (int i = 0; i < Viewport::numberViewports; ++i) {
        const Viewport & vp = *viewports[i];
        settings.setValue(VP_I_POS.arg(i), vp.dockPos.isNull() ? vp.pos() : vp.dockPos);
        settings.setValue(VP_I_SIZE.arg(i), vp.dockSize.isEmpty() ? vp.size() : vp.dockSize);
        settings.setValue(VP_I_VISIBLE.arg(i), vp.isVisible());
    }
    QList<QVariant> order;
    for (const auto & w : centralWidget()->children()) {
        for (int i = 0; i < Viewport::numberViewports; ++i) {
            if (w == viewports[i].get()) {
                order.append(i);
            }
        }
    }
    settings.setValue(VP_ORDER, order);

    settings.setValue(ANNOTATION_MODE, static_cast<int>(workModeModel.at(modeCombo.currentIndex()).first));

    int i = 0;
    for (const auto & path : skeletonFileHistory) {
        settings.setValue(QString("loaded_file%1").arg(i+1), path);
        ++i;
    }

    settings.setValue(OPEN_FILE_DIALOG_DIRECTORY, openFileDirectory);
    settings.setValue(SAVE_FILE_DIALOG_DIRECTORY, saveFileDirectory);

    settings.endGroup();

    widgetContainer->datasetLoadWidget->saveSettings();
    widgetContainer->dataSavingWidget->saveSettings();
    widgetContainer->datasetOptionsWidget->saveSettings();
    widgetContainer->viewportSettingsWidget->saveSettings();
    widgetContainer->navigationWidget->saveSettings();
    widgetContainer->annotationWidget->saveSettings();
    widgetContainer->pythonPropertyWidget->saveSettings();
    widgetContainer->snapshotWidget->saveSettings();
    widgetContainer->taskManagementWidget->taskLoginWidget.saveSettings();
    //widgetContainer->toolsWidget->saveSettings();
}

/**
 * this method is a proposal for the qsettings variant
 */
void MainWindow::loadSettings() {
    QSettings settings;
    settings.beginGroup(MAIN_WINDOW);
    restoreGeometry(settings.value(GEOMETRY).toByteArray());
    restoreState(settings.value(STATE).toByteArray());

    state->viewerState->defaultVPSizeAndPos = settings.value(VP_DEFAULT_POS_SIZE, true).toBool();
    if (state->viewerState->defaultVPSizeAndPos) {
        resetViewports();
    } else {
        for (int i = 0; i < Viewport::numberViewports; ++i) {
            Viewport & vp = *viewports[i];
            vp.move(settings.value(VP_I_POS.arg(i)).toPoint());
            vp.resize(settings.value(VP_I_SIZE.arg(i)).toSize());
            vp.setVisible(settings.value(VP_I_VISIBLE.arg(i), true).toBool());
        }
    }
    for (const auto & i : settings.value(VP_ORDER).toList()) {
        viewports[i.toInt()]->raise();
    }

    auto autosaveLocation = QFileInfo(annotationFileDefaultPath()).dir().absolutePath();
    QDir().mkpath(autosaveLocation);

    openFileDirectory = settings.value(OPEN_FILE_DIALOG_DIRECTORY, autosaveLocation).toString();

    saveFileDirectory = settings.value(SAVE_FILE_DIALOG_DIRECTORY, autosaveLocation).toString();

    setWorkMode(static_cast<AnnotationMode>(settings.value(ANNOTATION_MODE, static_cast<int>(AnnotationMode::Mode_Tracing)).toInt()));

    for (int nr = 10; nr != 0; --nr) {//reverse, because new ones are added in front
        updateRecentFile(settings.value(QString("loaded_file%1").arg(nr), "").toString());
    }

    settings.endGroup();

    widgetContainer->datasetLoadWidget->loadSettings();
    widgetContainer->dataSavingWidget->loadSettings();
    widgetContainer->datasetOptionsWidget->loadSettings();
    widgetContainer->viewportSettingsWidget->loadSettings();
    widgetContainer->navigationWidget->loadSettings();
    widgetContainer->annotationWidget->loadSettings();
    widgetContainer->pythonPropertyWidget->loadSettings();
    widgetContainer->snapshotWidget->loadSettings();
}

void MainWindow::clearSettings() {
    QSettings settings;

    skeletonFileHistory.clear();

    for(int i = 0; i < FILE_DIALOG_HISTORY_MAX_ENTRIES; i++) {
        historyEntryActions[i]->setVisible(false);
    }

    for(auto a : settings.allKeys()) {
        settings.remove(a);
    }
}

void MainWindow::updateCoordinateBar(int x, int y, int z) {
    xField->setValue(x + 1);
    yField->setValue(y + 1);
    zField->setValue(z + 1);
}

void MainWindow::resizeEvent(QResizeEvent *) {
    if(state->viewerState->defaultVPSizeAndPos) {
        // don't resize viewports when user positioned and resized them manually
        resetViewports();
    } else {//ensure viewports fit the window
        for (auto & vp : viewports) {
            vp->posAdapt();
            vp->sizeAdapt();
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    QStringList files;
    for (auto && url : event->mimeData()->urls()) {
        files.append(url.toLocalFile());
    }
    QTimer::singleShot(0, [this, files](){
        openFileDispatch(files);
    });
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent * event) {
    if(event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        for (auto && url : urls) {
            qDebug() << url;//in case its no working
            if (url.isLocalFile()) {
                const auto fileName(url.toLocalFile());
                if (fileName.endsWith(".k.zip") || fileName.endsWith(".nml")) {
                    event->accept();
                }
            }
        }
    }
}

void MainWindow::resetViewports() {
    for (auto & vp : viewports) {
        vp->setDock(true);
        vp->setVisible(true);
    }
    resizeToFitViewports(centralWidget()->width(), centralWidget()->height());
    state->viewerState->defaultVPSizeAndPos = true;
}

void MainWindow::showVPDecorationClicked() {
    bool isShow = widgetContainer->viewportSettingsWidget->generalTabWidget->showVPDecorationCheckBox->isChecked();
    for(uint i = 0; i < Viewport::numberViewports; i++) {
        viewports[i]->showHideButtons(isShow);
    }
}

void MainWindow::newTreeSlot() {
    color4F treeCol;
    treeCol.r = -1.;
    Skeletonizer::singleton().addTreeListElement(0, treeCol);
}

void MainWindow::nextCommentNodeSlot() {
    Skeletonizer::singleton().nextComment(state->skeletonState->nodeCommentFilter);
}

void MainWindow::previousCommentNodeSlot() {
    Skeletonizer::singleton().previousComment(state->skeletonState->nodeCommentFilter);
}

void MainWindow::pushBranchNodeSlot() {
    Skeletonizer::singleton().pushBranchNode(true, true, state->skeletonState->activeNode, 0);
}

void MainWindow::popBranchNodeSlot() {
    Skeletonizer::singleton().popBranchNodeAfterConfirmation(this);
}

void MainWindow::placeComment(const int index) {
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing) && state->skeletonState->activeNode != nullptr) {
        CommentSetting comment = CommentSetting::comments[index];
        if (!comment.text.isEmpty()) {
            if(comment.appendComment) {
                if(state->skeletonState->activeNode->comment) {
                    QString text(state->skeletonState->activeNode->comment->content);
                    text.append(' ');
                    comment.text.prepend(text);
                }
            }
            Skeletonizer::singleton().setComment(comment.text, state->skeletonState->activeNode, 0);
        }
    } else {
        Segmentation::singleton().placeCommentForSelectedObject(CommentSetting::comments[index].text);
    }
}

void MainWindow::resizeToFitViewports(int width, int height) {
    width = (width - DEFAULT_VP_MARGIN) / 2;
    height = (height - DEFAULT_VP_MARGIN) / 2;
    int mindim = std::min(width, height);
    viewports[VIEWPORT_XY]->move(DEFAULT_VP_MARGIN, DEFAULT_VP_MARGIN);
    viewports[VIEWPORT_XZ]->move(DEFAULT_VP_MARGIN, DEFAULT_VP_MARGIN + mindim);
    viewports[VIEWPORT_YZ]->move(DEFAULT_VP_MARGIN + mindim, DEFAULT_VP_MARGIN);
    viewports[VIEWPORT_SKELETON]->move(DEFAULT_VP_MARGIN + mindim, DEFAULT_VP_MARGIN + mindim);
    for(int i = 0; i < 4; i++) {
        viewports[i]->resize(mindim-DEFAULT_VP_MARGIN, mindim-DEFAULT_VP_MARGIN);
    }
}

void MainWindow::pythonSlot() {
    widgetContainer->pythonPropertyWidget->openTerminal();
}

void MainWindow::pythonPropertiesSlot() {
    widgetContainer->pythonPropertyWidget->show();
}

void MainWindow::pythonFileSlot() {
    state->viewerState->renderInterval = SLOW;
    QString pyFileName = QFileDialog::getOpenFileName(this, "Select python file", QDir::homePath(), "*.py");
    state->viewerState->renderInterval = FAST;
    state->scripting->runFile(pyFileName);
}
