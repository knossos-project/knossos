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

    skeletonFileHistory = new QQueue<QString>();
    skeletonFileHistory->reserve(FILE_DIALOG_HISTORY_MAX_ENTRIES);

    QObject::connect(widgetContainer->viewportSettingsWidget->generalTabWidget, &VPGeneralTabWidget::setViewportDecorations, this, &MainWindow::showVPDecorationClicked);
    QObject::connect(widgetContainer->viewportSettingsWidget->generalTabWidget, &VPGeneralTabWidget::resetViewportPositions, this, &MainWindow::resetViewports);
    QObject::connect(widgetContainer->datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this](bool showOverlays) {
        skelEditSegModeAction->setEnabled(showOverlays);
        if(showOverlays == false && Session::singleton().annotationMode == SegmentationMode) {
            setAnnotationMode(SkeletonizationMode);
        }
        widgetContainer->annotationWidget->setSegmentationVisibility(showOverlays);
    });
    QObject::connect(widgetContainer->snapshotWidget, &SnapshotWidget::snapshotRequest, [this](QString path, ViewportType vp, bool withOverlay, bool withSkeleton, bool withScale) {
       viewports[vp]->takeSnapshot(path, withOverlay, withSkeleton, withScale);
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
    basicToolbar.setMovable(false);
    basicToolbar.setFloatable(false);
    basicToolbar.setMaximumHeight(45);

    basicToolbar.addAction(QIcon(":/resources/icons/open-annotation.png"), "Open Annotation", this, SLOT(openSlot()));
    basicToolbar.addAction(QIcon(":/resources/icons/document-save.png"), "Save Annotation", this, SLOT(saveSlot()));
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

void MainWindow::setJobModeUI(bool enabled) {
    if(enabled) {
        setAnnotationMode(SegmentationMode);
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
    else if(job.active) {
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
            auto jobFilename = "final_" + QFileInfo(annotationFilename).fileName();
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
    if (!annotationFilename.isEmpty()) {
        title.append(annotationFilename);
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
    if (!state->skeletonState->unsavedChanges && annotationFilename.isEmpty()) {
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

// -- static methods -- //

void MainWindow::updateRecentFile(const QString & fileName) {
    bool notAlreadyExists = std::find(std::begin(*skeletonFileHistory), std::end(*skeletonFileHistory), fileName) == std::end(*skeletonFileHistory);
    if (notAlreadyExists) {
        if (skeletonFileHistory->size() < FILE_DIALOG_HISTORY_MAX_ENTRIES) {
            skeletonFileHistory->enqueue(fileName);
        } else {//shrink if necessary
            skeletonFileHistory->dequeue();
            skeletonFileHistory->enqueue(fileName);
        }
    } else {//move to front if already existing
        skeletonFileHistory->move(skeletonFileHistory->indexOf(fileName), 0);
    }
    //update the menu
    int i = 0;
    for (const auto & path : *skeletonFileHistory) {
        historyEntryActions[i]->setText(path);
        historyEntryActions[i]->setVisible(!path.isEmpty());
        ++i;
    }
}

/**
  * @todo Replacements for the Labels
  * Maybe functionality of Viewport
  */
void MainWindow::reloadDataSizeWin(){
    float heightxy = state->viewerState->vpConfigs[VIEWPORT_XY].displayedlengthInNmY*0.001;
    float widthxy = state->viewerState->vpConfigs[VIEWPORT_XY].displayedlengthInNmX*0.001;
    float heightxz = state->viewerState->vpConfigs[VIEWPORT_XZ].displayedlengthInNmY*0.001;
    float widthxz = state->viewerState->vpConfigs[VIEWPORT_XZ].displayedlengthInNmX*0.001;
    float heightyz = state->viewerState->vpConfigs[VIEWPORT_YZ].displayedlengthInNmY*0.001;
    float widthyz = state->viewerState->vpConfigs[VIEWPORT_YZ].displayedlengthInNmX*0.001;

    if ((heightxy > 1.0) && (widthxy > 1.0)){
        //AG_LabelText(state->viewerState->gui->dataSizeLabelxy, "Height %.2f \u00B5m, Width %.2f \u00B5m", heightxy, widthxy);
    }
    else{
        //AG_LabelText(state->viewerState->gui->dataSizeLabelxy, "Height %.0f nm, Width %.0f nm", heightxy*1000, widthxy*1000);
    }
    if ((heightxz > 1.0) && (widthxz > 1.0)){
        //AG_LabelText(state->viewerState->gui->dataSizeLabelxz, "Height %.2f \u00B5m, Width %.2f \u00B5m", heightxz, widthxz);
    }
    else{
       // AG_LabelText(state->viewerState->gui->dataSizeLabelxz, "Height %.0f nm, Width %.0f nm", heightxz*1000, widthxz*1000);
    }

    if ((heightyz > 1.0) && (widthyz > 1.0)){
        //AG_LabelText(state->viewerState->gui->dataSizeLabelyz, "Height %.2f \u00B5m, Width %.2f \u00B5m", heightyz, widthyz);
    }
    else{
        //AG_LabelText(state->viewerState->gui->dataSizeLabelyz, "Height %.0f nm, Width %.0f nm", heightyz*1000, widthyz*1000);
    }
}

void MainWindow::treeColorAdjustmentsChanged() {
    //user lut activated
        if(state->viewerState->treeColortableOn) {
            //user lut selected
            if(state->viewerState->treeLutSet) {
                memcpy(state->viewerState->treeAdjustmentTable,
                state->viewerState->treeColortable,
                RGB_LUTSIZE * sizeof(float));
                emit updateTreeColorsSignal();
            }
            else {
                memcpy(state->viewerState->treeAdjustmentTable,
                state->viewerState->defaultTreeTable,
                RGB_LUTSIZE * sizeof(float));
            }
        }
        //use of default lut
        else {
            memcpy(state->viewerState->treeAdjustmentTable,
            state->viewerState->defaultTreeTable,
            RGB_LUTSIZE * sizeof(float));
                    emit updateTreeColorsSignal();
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
};

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

    segEditMenu = new QMenu("Edit Segmentation");
    auto segAnnotationModeGroup = new QActionGroup(this);
    segEditSegModeAction = segAnnotationModeGroup->addAction(tr("Segmentation Mode"));
    segEditSegModeAction->setCheckable(true);
    segEditSkelModeAction = segAnnotationModeGroup->addAction(tr("Skeletonization Mode"));
    segEditSkelModeAction->setCheckable(true);
    connect(segEditSegModeAction, &QAction::triggered, [this]() { setAnnotationMode(SegmentationMode); });
    connect(segEditSkelModeAction, &QAction::triggered, [this]() { setAnnotationMode(SkeletonizationMode); });
    segEditMenu->addActions({segEditSegModeAction, segEditSkelModeAction});
    segEditMenu->addSeparator();


    const QString branchToolTip{"Create Nodes when merging."};
    const QString mergeToolTip{"Right click to merge.\nHold SHIFT to unmerge.\nHold CTRL to work on object parts only."};
    const QString paintToolTip{"Create overlay data for the selected object.\nHold SHIFT to erase.\nErase any if none is selected."};

    auto & brushModeGroup = *new QActionGroup(this);
    auto & hybridModeAction = *brushModeGroup.addAction(tr("Hybrid Mode"));
    hybridModeAction.setCheckable(true);
    hybridModeAction.setStatusTip(branchToolTip);
    auto & mergeModeAction = *brushModeGroup.addAction(tr("Merge/Unmerge Mode"));
    mergeModeAction.setCheckable(true);
    mergeModeAction.setStatusTip(mergeToolTip);
    auto & paintModeAction = *brushModeGroup.addAction(tr("Paint/Erase Mode"));
    paintModeAction.setCheckable(true);
    paintModeAction.setStatusTip(paintToolTip);

    segEditMenu->addActions({&hybridModeAction, &mergeModeAction, &paintModeAction});
    segEditMenu->addSeparator();

    addApplicationShortcut(*segEditMenu, QIcon(), tr("Push Branch"), this, [this](){
        Skeletonizer::singleton().pushBranchNode(true, true, state->skeletonState->activeNode, 0);
    }, Qt::Key_B);
    addApplicationShortcut(*segEditMenu, QIcon(), tr("Pop Branch"), this, [this](){
        Skeletonizer::singleton().popBranchNodeAfterConfirmation(this);
    }, Qt::Key_J);

    QObject::connect(&Segmentation::singleton().brush, &brush_t::toolChanged, [&hybridModeAction, &mergeModeAction, &paintModeAction](brush_t::tool_t value){
        hybridModeAction.setChecked(value == brush_t::tool_t::hybrid);
        mergeModeAction.setChecked(value == brush_t::tool_t::merge);
        paintModeAction.setChecked(value == brush_t::tool_t::add);
    });
    QObject::connect(&brushModeGroup, &QActionGroup::triggered, [&hybridModeAction, &mergeModeAction, &paintModeAction](QAction * action){
        const auto tool = action == &hybridModeAction ? brush_t::tool_t::hybrid : action == &mergeModeAction ? brush_t::tool_t::merge : brush_t::tool_t::add;
        if (tool != Segmentation::singleton().brush.getTool()) {//no ping pong
            Segmentation::singleton().brush.setTool(tool);
        }
    });

    segEditMenu->addSeparator();
    segEditMenu->addAction(QIcon(":/resources/icons/user-trash.png"), "Clear Merge List", &Segmentation::singleton(), SLOT(clear()));

    skelEditMenu = new QMenu("Edit Skeleton");
    auto skelAnnotationModeGroup = new QActionGroup(this);
    skelEditSegModeAction = skelAnnotationModeGroup->addAction(tr("Segmentation Mode"));
    skelEditSegModeAction->setCheckable(true);
    skelEditSkelModeAction = skelAnnotationModeGroup->addAction(tr("Skeletonization Mode"));
    skelEditSkelModeAction->setCheckable(true);
    skelEditSkelModeAction->setChecked(true);
    connect(skelEditSegModeAction, &QAction::triggered, [this]() { setAnnotationMode(SegmentationMode); });
    connect(skelEditSkelModeAction, &QAction::triggered, [this]() { setAnnotationMode(SkeletonizationMode); });
    skelEditMenu->addActions({skelEditSegModeAction, skelEditSkelModeAction});

    skelEditMenu->addSeparator();
    auto simpleTracingAction = skelEditMenu->addAction(tr("Deactivate Simple Tracing"));
    connect(simpleTracingAction, &QAction::triggered, [this](bool) { setSimpleTracing(!Skeletonizer::singleton().simpleTracing); });
    skelEditMenu->addSeparator();

    auto workModeEditMenuGroup = new QActionGroup(this);

    addNodeAction = &addApplicationShortcut(*workModeEditMenuGroup, QIcon(), tr("Add one unlinked Node"), this, [this](){
        if(Skeletonizer::singleton().simpleTracing) {
            QMessageBox::information(this, "Not available in Simple Tracing mode",
                                     "Please deactivate Simple Tracing under 'Edit Skeleton' for this function.");
            linkWithActiveNodeAction->trigger();
            return;
        }
        state->viewer->skeletonizer->setTracingMode(Skeletonizer::TracingMode::skipNextLink);
    }, Qt::Key_A);
    addNodeAction->setCheckable(true);

    linkWithActiveNodeAction = workModeEditMenuGroup->addAction(tr("Add linked Nodes"));
    linkWithActiveNodeAction->setCheckable(true);

    dropNodesAction = workModeEditMenuGroup->addAction(tr("Add unlinked Nodes"));
    dropNodesAction->setCheckable(true);

    QObject::connect(linkWithActiveNodeAction, &QAction::triggered, [](){
        state->viewer->skeletonizer->setTracingMode(Skeletonizer::TracingMode::linkedNodes);
    });
    QObject::connect(dropNodesAction, &QAction::triggered, [this](){
        if(Skeletonizer::singleton().simpleTracing) {
            QMessageBox::information(this, "Not available in Simple Tracing mode",
                                     "Please deactivate Simple Tracing under 'Edit Skeleton' for this function.");
            linkWithActiveNodeAction->trigger();
            return;
        }
        state->viewer->skeletonizer->setTracingMode(Skeletonizer::TracingMode::unlinkedNodes);
    });

    skelEditMenu->addActions({addNodeAction, linkWithActiveNodeAction, dropNodesAction});//can’t add the group, must add all actions separately
    skelEditMenu->addSeparator();

    addApplicationShortcut(*skelEditMenu, QIcon(), tr("New Tree"), this, &MainWindow::newTreeSlot, Qt::Key_C);
    addApplicationShortcut(*skelEditMenu, QIcon(), tr("Push Branch Node"), this, &MainWindow::pushBranchNodeSlot, Qt::Key_B);
    addApplicationShortcut(*skelEditMenu, QIcon(), tr("Pop Branch Node"), this, &MainWindow::popBranchNodeSlot, Qt::Key_J);

    skelEditMenu->addSeparator();
    skelEditMenu->addAction(QIcon(":/resources/icons/user-trash.png"), "Clear Skeleton", this, SLOT(clearSkeletonSlotGUI()));

    menuBar()->addMenu(skelEditMenu);

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

    addApplicationShortcut(*viewMenu, QIcon(), tr("Jump To Active Node"), &Skeletonizer::singleton(), &Skeletonizer::jumpToActiveNode, Qt::Key_S);
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
    EmitOnCtorDtor eocd(&SignalRelay::Signal_MainWindow_closeEvent, state->scripting->signalRelay, event);
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

    auto nmls = std::vector<QString>(std::begin(fileNames), nmlEndIt);
    const auto blockState = state->viewer->skeletonizer->signalsBlocked();
    for (const auto & filename : nmls) {
        if(filename == fileNames.last()) { //enable signals only at last element
            state->viewer->skeletonizer->blockSignals(false);
        } else {
            state->viewer->skeletonizer->blockSignals(true);
        }
        const QString treeCmtOnMultiLoad = multipleFiles ? QFileInfo(filename).fileName() : "";
        QFile file(filename);
        if (success &= state->viewer->skeletonizer->loadXmlSkeleton(file, treeCmtOnMultiLoad)) {
            updateRecentFile(filename);
        }
        state->skeletonState->mergeOnLoadFlag = true;//multiple files have to be merged
    }
    state->viewer->skeletonizer->blockSignals(blockState);

    auto zips = std::vector<QString>(nmlEndIt, std::end(fileNames));
    for (const auto & filename : zips) {
        const QString treeCmtOnMultiLoad = multipleFiles ? QFileInfo(filename).fileName() : "";
        annotationFileLoad(filename, treeCmtOnMultiLoad);
        updateRecentFile(filename);
        state->skeletonState->mergeOnLoadFlag = true;//multiple files have to be merged
    }

    state->skeletonState->unsavedChanges = mergeSkeleton || mergeSegmentation;//merge implies changes

    annotationFilename = "";
    if (success && !multipleFiles) { // either an .nml or a .k.zip was loaded
        annotationFilename = nmls.empty() ? zips.front() : nmls.front();
    }
    updateTitlebar();

    if (Segmentation::singleton().job.active) { // we need to apply job mode here to ensure that all necessary parts are loaded by now.
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
    Skeletonizer::singleton().clearSkeleton(false);
    Segmentation::singleton().clear();
    state->skeletonState->unsavedChanges = false;
    annotationFilename = "";
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
    if (annotationFilename.isEmpty()) {
        annotationFilename = annotationFileDefaultPath();
    }
    saveSlot();
}

void MainWindow::saveSlot() {
    if (annotationFilename.isEmpty()) {
        saveAsSlot();
    } else {
        if(annotationFilename.endsWith(".nml")) {
            annotationFilename.chop(4);
            annotationFilename += ".k.zip";
        }
        if (state->skeletonState->autoFilenameIncrementBool) {
            int index = skeletonFileHistory->indexOf(annotationFilename);
            updateFileName(annotationFilename);
            if (index != -1) {//replace old filename with updated one
                skeletonFileHistory->replace(index, annotationFilename);
                historyEntryActions[index]->setText(skeletonFileHistory->at(index));
            }
        }
        annotationFileSave(annotationFilename);

        updateRecentFile(annotationFilename);
        updateTitlebar();
    }
}

void MainWindow::saveAsSlot() {
    auto *seg = &Segmentation::singleton();
    const auto & suggestedFile = saveFileDirectory.isEmpty() ? annotationFileDefaultPath() : saveFileDirectory + '/' + annotationFileDefaultName();

    state->viewerState->renderInterval = SLOW;
#ifdef Q_OS_MAC
    QString fileName = QFileDialog::getSaveFileName(this, "Save the KNOSSSOS Annotation file", suggestedFile);
#else
    QString fileName = QFileDialog::getSaveFileName(this, "Save the KNOSSSOS Annotation file", suggestedFile, "KNOSSOS Annotation file (*.k.zip)");
#endif
    state->viewerState->renderInterval = FAST;

    if (!fileName.isEmpty()) {
        if (!fileName.contains(".k.zip")) {
            fileName += ".k.zip";
        }
        QRegExp wrongSuffixRegex(R"regex((\.k(?!\.zip$))|(?:[^k](\.zip)))regex");
        if (fileName.contains(wrongSuffixRegex)) {
            const auto prevFilename = fileName;
            fileName.remove(wrongSuffixRegex);
            const auto message = tr("The supplied filename has been changed: \n") + prevFilename + " → " + fileName;
            QMessageBox::information(this, tr("Fixed filename"), message);
        }

        annotationFilename = fileName;
        saveFileDirectory = QFileInfo(fileName).absolutePath();

        annotationFileSave(annotationFilename);

        updateRecentFile(annotationFilename);
        updateTitlebar();
    }
}

void MainWindow::exportToNml() {
    if(!state->skeletonState->firstTree) {
        QMessageBox::information(this, "No Save", "No skeleton was found. Not saving!");
        return;
    }
    auto info = QFileInfo(annotationFilename);
    auto defaultpath = annotationFileDefaultPath();
    defaultpath.chop(6);
    defaultpath += ".nml";
    const auto & suggestedFilepath = annotationFilename.isEmpty() ? defaultpath : info.absoluteDir().path() + "/" + info.baseName() + ".nml";
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

void MainWindow::setAnnotationMode(AnnotationMode mode) {
    if (Session::singleton().annotationMode == mode) {
        return;
    }
    Session::singleton().annotationMode = mode;
    if (mode == SkeletonizationMode) {
        menuBar()->insertMenu(segEditMenu->menuAction(), skelEditMenu);
        menuBar()->removeAction(segEditMenu->menuAction());
        skelEditSkelModeAction->setChecked(true);
    } else {
        menuBar()->insertMenu(skelEditMenu->menuAction(), segEditMenu);
        menuBar()->removeAction(skelEditMenu->menuAction());
        segEditSegModeAction->setChecked(true);
    }
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
    Skeletonizer::singleton().clearSkeleton(false);
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
        Knossos::loadTreeLUTFallback();
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
    emit userMoveSignal(viewer_offset_x, viewer_offset_y, viewer_offset_z, USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
}

void MainWindow::saveSettings() {
    QSettings settings;

    settings.beginGroup(MAIN_WINDOW);
    settings.setValue(WIDTH, this->geometry().width());
    settings.setValue(HEIGHT, this->geometry().height());
    settings.setValue(POS_X, this->geometry().x());
    settings.setValue(POS_Y, this->geometry().y());

    // viewport position and sizes
    settings.setValue(VP_DEFAULT_POS_SIZE, state->viewerState->defaultVPSizeAndPos);
    settings.setValue(VPXY_SIZE, viewports[VIEWPORT_XY]->dockSize.isEmpty() ? viewports[VIEWPORT_XY]->size() : viewports[VIEWPORT_XY]->dockSize);
    settings.setValue(VPXZ_SIZE, viewports[VIEWPORT_XZ]->dockSize.isEmpty() ? viewports[VIEWPORT_XZ]->size() : viewports[VIEWPORT_XZ]->dockSize);
    settings.setValue(VPYZ_SIZE, viewports[VIEWPORT_YZ]->dockSize.isEmpty() ? viewports[VIEWPORT_YZ]->size() : viewports[VIEWPORT_YZ]->dockSize);
    settings.setValue(VPSKEL_SIZE, viewports[VIEWPORT_SKELETON]->dockSize.isEmpty() ? viewports[VIEWPORT_SKELETON]->size() : viewports[VIEWPORT_SKELETON]->dockSize);

    settings.setValue(VPXY_COORD, viewports[VIEWPORT_XY]->dockPos.isNull() ? viewports[VIEWPORT_XY]->pos() : viewports[VIEWPORT_XY]->dockPos);
    settings.setValue(VPXZ_COORD, viewports[VIEWPORT_XZ]->dockPos.isNull() ? viewports[VIEWPORT_XZ]->pos() : viewports[VIEWPORT_XZ]->dockPos);
    settings.setValue(VPYZ_COORD, viewports[VIEWPORT_YZ]->dockPos.isNull() ? viewports[VIEWPORT_YZ]->pos() : viewports[VIEWPORT_YZ]->dockPos);
    settings.setValue(VPSKEL_COORD, viewports[VIEWPORT_SKELETON]->dockPos.isNull() ? viewports[VIEWPORT_SKELETON]->pos() : viewports[VIEWPORT_SKELETON]->dockPos);

    settings.setValue(VPXY_VISIBLE, viewports[VIEWPORT_XY]->isVisible());
    settings.setValue(VPXZ_VISIBLE, viewports[VIEWPORT_XZ]->isVisible());
    settings.setValue(VPYZ_VISIBLE, viewports[VIEWPORT_YZ]->isVisible());
    settings.setValue(VPSKEL_VISIBLE, viewports[VIEWPORT_SKELETON]->isVisible());

    settings.setValue(TRACING_MODE, static_cast<int>(state->viewer->skeletonizer->getTracingMode()));
    settings.setValue(SIMPLE_TRACING, Skeletonizer::singleton().simpleTracing);
    settings.setValue(ANNOTATION_MODE, static_cast<int>(Session::singleton().annotationMode));
    settings.setValue(SEGMENTATION_TOOL, static_cast<int>(Segmentation::singleton().brush.getTool()));

    int i = 0;
    for (const auto & path : *skeletonFileHistory) {
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
    int width = (settings.value(WIDTH).isNull())? 1024 : settings.value(WIDTH).toInt();
    int height = (settings.value(HEIGHT).isNull())? 800 : settings.value(HEIGHT).toInt();
    int x, y;
    if(settings.value(POS_X).isNull() || settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().topLeft().x() + 20;
        y = QApplication::desktop()->screen()->rect().topLeft().y() + 50;
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }

    state->viewerState->defaultVPSizeAndPos = settings.value(VP_DEFAULT_POS_SIZE, true).toBool();
    if(state->viewerState->defaultVPSizeAndPos == false) {
        viewports[VIEWPORT_XY]->resize(settings.value(VPXY_SIZE).toSize());
        viewports[VIEWPORT_XZ]->resize(settings.value(VPXZ_SIZE).toSize());
        viewports[VIEWPORT_YZ]->resize(settings.value(VPYZ_SIZE).toSize());
        viewports[VIEWPORT_SKELETON]->resize(settings.value(VPSKEL_SIZE).toSize());

        viewports[VIEWPORT_XY]->move(settings.value(VPXY_COORD).toPoint());
        viewports[VIEWPORT_XZ]->move(settings.value(VPXZ_COORD).toPoint());
        viewports[VIEWPORT_YZ]->move(settings.value(VPYZ_COORD).toPoint());
        viewports[VIEWPORT_SKELETON]->move(settings.value(VPSKEL_COORD).toPoint());

        bool defaultViewportVisibility = true;
        viewports[VIEWPORT_XY]->setVisible(settings.value(VPXY_VISIBLE,defaultViewportVisibility).toBool());
        viewports[VIEWPORT_XZ]->setVisible(settings.value(VPXZ_VISIBLE,defaultViewportVisibility).toBool());
        viewports[VIEWPORT_YZ]->setVisible(settings.value(VPYZ_VISIBLE,defaultViewportVisibility).toBool());
        viewports[VIEWPORT_SKELETON]->setVisible(settings.value(VPSKEL_VISIBLE,defaultViewportVisibility).toBool());
    }

    auto autosaveLocation = QFileInfo(annotationFileDefaultPath()).dir().absolutePath();
    QDir().mkpath(autosaveLocation);

    openFileDirectory = settings.value(OPEN_FILE_DIALOG_DIRECTORY, autosaveLocation).toString();

    saveFileDirectory = settings.value(SAVE_FILE_DIALOG_DIRECTORY, autosaveLocation).toString();

    const auto tracingMode = static_cast<Skeletonizer::TracingMode>(settings.value(TRACING_MODE, Skeletonizer::TracingMode::linkedNodes).toInt());
    state->viewer->skeletonizer->setTracingMode(tracingMode);
    setSimpleTracing(settings.value(SIMPLE_TRACING, true).toBool());

    setAnnotationMode(static_cast<AnnotationMode>(settings.value(ANNOTATION_MODE, SkeletonizationMode).toInt()));

    const auto segmentationTool = static_cast<brush_t::tool_t>(settings.value(SEGMENTATION_TOOL, static_cast<int>(brush_t::tool_t::merge)).toInt());
    Segmentation::singleton().brush.setTool(segmentationTool);

    updateRecentFile(settings.value(LOADED_FILE1, "").toString());
    updateRecentFile(settings.value(LOADED_FILE2, "").toString());
    updateRecentFile(settings.value(LOADED_FILE3, "").toString());
    updateRecentFile(settings.value(LOADED_FILE4, "").toString());
    updateRecentFile(settings.value(LOADED_FILE5, "").toString());
    updateRecentFile(settings.value(LOADED_FILE6, "").toString());
    updateRecentFile(settings.value(LOADED_FILE7, "").toString());
    updateRecentFile(settings.value(LOADED_FILE8, "").toString());
    updateRecentFile(settings.value(LOADED_FILE9, "").toString());
    updateRecentFile(settings.value(LOADED_FILE10, "").toString());

    settings.endGroup();
    setGeometry(x, y, width, height);

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

    skeletonFileHistory->clear();

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
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    QStringList files;
    for (auto && url : event->mimeData()->urls()) {
        files.append(url.toLocalFile());
    }
    openFileDispatch(files);
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
    if(Skeletonizer::singleton().simpleTracing) {
        QMessageBox::information(this, "Not available in Simple Tracing mode",
                                 "Please deactivate Simple Tracing under 'Edit Skeleton' for this function.");
        return;
    }
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
    if(Session::singleton().annotationMode == SegmentationMode) {
        Segmentation::singleton().placeCommentForSelectedObject(CommentSetting::comments[index].text);
        return;
    }
    if(!state->skeletonState->activeNode) {
        return;
    }
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

void MainWindow::setSimpleTracing(bool simple) {
    Skeletonizer::singleton().simpleTracing = simple;
    skelEditMenu->actions().at(3)->setText(simple ? "Deactivate Simple Tracing" : "Activate Simple Tracing");
    widgetContainer->annotationWidget->commandsTab.setSimpleTracing(simple);
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
    QFile pyFile(pyFileName);
    pyFile.open(QIODevice::ReadOnly);
    QString s;
    QTextStream textStream(&pyFile);
    s.append(textStream.readAll());
    pyFile.close();

    state->scripting->_ctx.evalScript(s, Py_file_input);
}
