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
#include "mainwindow.h"
#include "network.h"
#include "skeleton/node.h"
#include "version.h"
#include "viewer.h"
#include "viewport.h"
#include "scriptengine/scripting.h"
#include "skeleton/skeletonizer.h"
#include "widgetcontainer.h"

#include <PythonQt/PythonQt.h>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopWidget>
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
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QRegExp>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringList>
#include <QThread>
#include <QToolButton>
#include <QQueue>

// default position of xy viewport and default viewport size
#define DEFAULT_VP_MARGIN 5
#define DEFAULT_VP_SIZE 350

class CoordinateSpin : public QSpinBox {
public:
    CoordinateSpin(const QString & prefix, MainWindow & mainWindow) : QSpinBox(&mainWindow) {
        setPrefix(prefix);
        setRange(0, 1000000);//allow min 0 as bogus value, we don’t adjust the max anyway
        setValue(0 + 1);//inintialize for {0, 0, 0}
        QObject::connect(this, &QSpinBox::editingFinished, &mainWindow, &MainWindow::coordinateEditingFinished);
    }
    virtual void fixup(QString & input) const override {
        input = QString::number(0);//let viewer reset the value
    }
};

template<typename Menu, typename Receiver, typename Slot>
QAction & addApplicationShortcut(Menu & menu, const QIcon & icon, const QString & caption, const Receiver * receiver, const Slot slot, const QKeySequence & keySequence) {
    auto & action = *menu.addAction(icon, caption);
    action.setShortcut(keySequence);
    action.setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect(&action, &QAction::triggered, receiver, slot);
    return action;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), widgetContainer(this) {
    state->mainWindow = this;
    updateTitlebar();
    this->setWindowIcon(QIcon(":/resources/icons/logo.ico"));

    skeletonFileHistory.reserve(FILE_DIALOG_HISTORY_MAX_ENTRIES);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::propertiesChanged, &widgetContainer.appearanceWidget.nodesTab, &NodesTab::updateProperties);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::guiModeLoaded, [this]() { setProofReadingUI(Session::singleton().guiMode == GUIMode::ProofReading); });
    QObject::connect(&widgetContainer.appearanceWidget.viewportTab, &ViewportTab::setViewportDecorations, this, &MainWindow::showVPDecorationClicked);
    QObject::connect(&widgetContainer.appearanceWidget.viewportTab, &ViewportTab::resetViewportPositions, this, &MainWindow::resetViewports);
    QObject::connect(&widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this](bool showOverlays) {
        const auto currentMode = workModeModel.at(modeCombo.currentIndex()).first;
        std::map<AnnotationMode, QString> rawModes = workModes;
        AnnotationMode defaultMode = AnnotationMode::Mode_Tracing;
        if (!showOverlays) {
             rawModes = {{AnnotationMode::Mode_Tracing, workModes[AnnotationMode::Mode_Tracing]}, {AnnotationMode::Mode_TracingAdvanced, workModes[AnnotationMode::Mode_TracingAdvanced]}};
        }
        else if (Session::singleton().guiMode == GUIMode::ProofReading) {
            rawModes = {{AnnotationMode::Mode_Merge, workModes[AnnotationMode::Mode_Merge]}, {AnnotationMode::Mode_Paint, workModes[AnnotationMode::Mode_Paint]}};
            defaultMode = AnnotationMode::Mode_Merge;
        }
        workModeModel.recreate(rawModes);
        setWorkMode((rawModes.find(currentMode) != std::end(rawModes))? currentMode : defaultMode);

        widgetContainer.annotationWidget.setSegmentationVisibility(showOverlays);
    });
    QObject::connect(&widgetContainer.snapshotWidget, &SnapshotWidget::snapshotRequest,
        [this](const QString & path, const ViewportType vpType, const int size, const bool withAxes, const bool withOverlay, const bool withSkeleton, const bool withScale, const  bool withVpPlanes) {
            viewport(vpType)->takeSnapshot(path, size, withAxes, withOverlay, withSkeleton, withScale, withVpPlanes);
        });
    QObject::connect(&Segmentation::singleton(), &Segmentation::appendedRow, this, &MainWindow::notifyUnsavedChanges);
    QObject::connect(&Segmentation::singleton(), &Segmentation::changedRow, this, &MainWindow::notifyUnsavedChanges);
    QObject::connect(&Segmentation::singleton(), &Segmentation::removedRow, this, &MainWindow::notifyUnsavedChanges);
    QObject::connect(&Segmentation::singleton(), &Segmentation::todosLeftChanged, this, &MainWindow::updateTodosLeft);

    QObject::connect(&Session::singleton(), &Session::autoSaveSignal, [this](){ save(); });

    createToolbars();
    createMenus();
    setCentralWidget(new QWidget(this));
    setGeometry(0, 0, width(), height());

    createViewports();
    setAcceptDrops(true);

    statusBar()->setSizeGripEnabled(false);
    GUIModeLabel.setVisible(false);
    statusBar()->addWidget(&GUIModeLabel);
    statusBar()->addWidget(&cursorPositionLabel);
    statusBar()->addPermanentWidget(&segmentStateLabel);
    statusBar()->addPermanentWidget(&unsavedChangesLabel);
    statusBar()->addPermanentWidget(&annotationTimeLabel);

    QObject::connect(&Session::singleton(), &Session::annotationTimeChanged, &annotationTimeLabel, &QLabel::setText);

    {
        auto & action = *new QAction(this);
        action.setShortcut(Qt::Key_7);
        QObject::connect(&action, &QAction::triggered, [](){
            state->viewer->gpuRendering = !state->viewer->gpuRendering;
        });
        addAction(&action);
    }
}

void MainWindow::updateCursorLabel(const Coordinate & position, const ViewportType vpType) {
    cursorPositionLabel.setHidden(vpType == VIEWPORT_SKELETON || vpType == VIEWPORT_UNDEFINED);
    cursorPositionLabel.setText(QString("%1, %2, %3").arg(position.x + 1).arg(position.y + 1).arg(position.z + 1));
}

void MainWindow::resetTextureProperties() {
    state->viewerState->voxelDimX = state->scale.x;
    state->viewerState->voxelDimY = state->scale.y;
    state->viewerState->voxelDimZ = state->scale.z;
    state->viewerState->voxelXYRatio = state->scale.x / state->scale.y;
    state->viewerState->voxelXYtoZRatio = state->scale.x / state->scale.z;
    //reset viewerState texture properties
    forEachOrthoVPDo([](ViewportOrtho & orthoVP) {
        orthoVP.texture.texUnitsPerDataPx = 1. / state->viewerState->texEdgeLength;
        orthoVP.texture.texUnitsPerDataPx /= static_cast<float>(state->magnification);
        orthoVP.texture.usedTexLengthDc = state->M;
        orthoVP.texture.edgeLengthPx = state->viewerState->texEdgeLength;
        orthoVP.texture.edgeLengthDc = state->viewerState->texEdgeLength / state->cubeEdgeLength;
        orthoVP.texture.FOV = 1;
    });
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
    if (ViewportBase::oglDebug) {
        format.setOption(QSurfaceFormat::DebugContext);
    }
    QSurfaceFormat::setDefaultFormat(format);

    viewportXY = std::unique_ptr<ViewportOrtho>(new ViewportOrtho(centralWidget(), VIEWPORT_XY));
    viewportXZ = std::unique_ptr<ViewportOrtho>(new ViewportOrtho(centralWidget(), VIEWPORT_XZ));
    viewportZY = std::unique_ptr<ViewportOrtho>(new ViewportOrtho(centralWidget(), VIEWPORT_ZY));
    viewportArb = std::unique_ptr<ViewportArb>(new ViewportArb(centralWidget(), VIEWPORT_ARBITRARY));
    viewport3D = std::unique_ptr<Viewport3D>(new Viewport3D(centralWidget(), VIEWPORT_SKELETON));
    viewportXY->upperLeftCorner = {5, 30, 0};
    viewportXZ->upperLeftCorner = {5, 385, 0};
    viewportZY->upperLeftCorner = {360, 30, 0};
    viewportArb->upperLeftCorner = {715, 30, 0};
    viewport3D->upperLeftCorner = {360, 385, 0};
    resetTextureProperties();
    forEachVPDo([this](ViewportBase & vp) { QObject::connect(&vp, &ViewportBase::cursorPositionChanged, this, &MainWindow::updateCursorLabel); });

}

ViewportBase * MainWindow::viewport(const ViewportType vpType) {
    return (viewportXY->viewportType == vpType)? static_cast<ViewportBase *>(viewportXY.get()) :
           (viewportXZ->viewportType == vpType)? static_cast<ViewportBase *>(viewportXZ.get()) :
           (viewportZY->viewportType == vpType)? static_cast<ViewportBase *>(viewportZY.get()) :
           (viewportArb->viewportType == vpType)? static_cast<ViewportBase *>(viewportArb.get()) :
           static_cast<ViewportBase *>(viewport3D.get());
}

ViewportOrtho * MainWindow::viewportOrtho(const ViewportType vpType) {
    return (viewportXY->viewportType == vpType)? viewportXY.get() :
           (viewportXZ->viewportType == vpType)? viewportXZ.get() :
           (viewportZY->viewportType == vpType)? viewportZY.get() :
           viewportArb.get();
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
                         "<b>" + workModes[AnnotationMode::Mode_TracingAdvanced] + ":</b> Unrestricted skeletonization<br/>");
    basicToolbar.addSeparator();
    basicToolbar.addAction(QIcon(":/resources/icons/edit-copy.png"), "Copy", this, SLOT(copyClipboardCoordinates()));
    basicToolbar.addAction(QIcon(":/resources/icons/edit-paste.png"), "Paste", this, SLOT(pasteClipboardCoordinates()));

    xField = new CoordinateSpin("x: ", *this);
    yField = new CoordinateSpin("y: ", *this);
    zField = new CoordinateSpin("z: ", *this);

    basicToolbar.addWidget(xField);
    basicToolbar.addWidget(yField);
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
    auto appearanceButton = createToolToogleButton(":/resources/icons/view-list-icons-symbolic.png", "Appearance Settings");
    auto annotationButton = createToolToogleButton(":/resources/icons/graph.png", "Annotation");
    auto pythonInterpreterButton = createToolToogleButton(":/resources/icons/python.png", "Python Interpreter");
    auto snapshotButton = createToolToogleButton(":/resources/icons/camera.png", "Snapshot");
    //button → visibility
    QObject::connect(taskManagementButton, &QToolButton::clicked, [this, &taskManagementButton](const bool down){
        if (down) {
            widgetContainer.taskManagementWidget.updateAndRefreshWidget();
        } else {
            widgetContainer.taskManagementWidget.hide();
        }
    });
    QObject::connect(pythonInterpreterButton, &QToolButton::clicked, &widgetContainer.pythonInterpreterWidget, &PythonInterpreterWidget::setVisible);
    QObject::connect(annotationButton, &QToolButton::clicked, &widgetContainer.annotationWidget, &AnnotationWidget::setVisible);
    QObject::connect(appearanceButton, &QToolButton::clicked, &widgetContainer.appearanceWidget, &AppearanceWidget::setVisible);
    QObject::connect(zoomAndMultiresButton, &QToolButton::clicked, &widgetContainer.datasetOptionsWidget, &DatasetOptionsWidget::setVisible);
    QObject::connect(snapshotButton, &QToolButton::clicked, &widgetContainer.snapshotWidget, &SnapshotWidget::setVisible);
    //visibility → button
    QObject::connect(&widgetContainer.taskManagementWidget, &TaskManagementWidget::visibilityChanged, taskManagementButton, &QToolButton::setChecked);
    QObject::connect(&widgetContainer.annotationWidget, &AnnotationWidget::visibilityChanged, annotationButton, &QToolButton::setChecked);
    QObject::connect(&widgetContainer.pythonInterpreterWidget, &PythonInterpreterWidget::visibilityChanged, pythonInterpreterButton, &QToolButton::setChecked);
    QObject::connect(&widgetContainer.appearanceWidget, &AppearanceWidget::visibilityChanged, appearanceButton, &QToolButton::setChecked);
    QObject::connect(&widgetContainer.datasetOptionsWidget, &DatasetOptionsWidget::visibilityChanged, zoomAndMultiresButton, &QToolButton::setChecked);
    QObject::connect(&widgetContainer.snapshotWidget, &SnapshotWidget::visibilityChanged, snapshotButton, &QToolButton::setChecked);

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

void MainWindow::setProofReadingUI(const bool on) {
    const auto currentMode = workModeModel.at(modeCombo.currentIndex()).first;
    if (on) {
        workModeModel.recreate({{AnnotationMode::Mode_Merge, workModes[AnnotationMode::Mode_Merge]}, {AnnotationMode::Mode_Paint, workModes[AnnotationMode::Mode_Paint]}});
        if (currentMode == AnnotationMode::Mode_Merge || currentMode == AnnotationMode::Mode_Paint) {
            setWorkMode(currentMode);
        } else {
            setWorkMode(AnnotationMode::Mode_Merge);
        }
    } else {
        workModeModel.recreate(workModes);
        setWorkMode(currentMode);
    }

    modeSwitchSeparator->setVisible(on);
    setMergeModeAction->setVisible(on);
    setPaintModeAction->setVisible(on);

    viewportXZ->setHidden(on);
    viewportZY->setHidden(on);
    viewportArb->setHidden(on);
    viewport3D->setHidden(on);
    resetViewports();
    GUIModeLabel.setText(on ? "Proof Reading Mode" : "");
    GUIModeLabel.setVisible(on);
}

void MainWindow::setJobModeUI(bool enabled) {
    if(enabled) {
        setWorkMode(AnnotationMode::Mode_MergeSimple);
        menuBar()->hide();
        widgetContainer.hideAll();
        removeToolBar(&defaultToolbar);
        addToolBar(&segJobModeToolbar);
        segJobModeToolbar.show(); // toolbar is hidden by removeToolBar
        forEachVPDo([] (ViewportBase & vp) { vp.hide(); });
        viewportXY.get()->resize(centralWidget()->height() - DEFAULT_VP_MARGIN, centralWidget()->height() - DEFAULT_VP_MARGIN);
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
            annotationFileSave(finishedJobPath);
            Network::singleton().submitSegmentationJob(finishedJobPath);
        }
    }
}

void MainWindow::notifyUnsavedChanges() {
    Session::singleton().unsavedChanges = true;
    updateTitlebar();
}

void MainWindow::updateTitlebar() {
    const auto & session = Session::singleton();
    QString title = qApp->applicationDisplayName() + " showing ";
    if (!session.annotationFilename.isEmpty()) {
        title.append(Session::singleton().annotationFilename);
    } else {
        title.append("no annotation file");
    }
    unsavedChangesLabel.setToolTip("");
    if (session.unsavedChanges) {
        title.append("*");
        auto autosave = tr("<font color='red'>(autosave: off)</font>");
        if (session.autoSaveTimer.isActive()) {
            autosave = tr("<font color='green'>(autosave: on)</font>");
            const auto minutes = session.autoSaveTimer.remainingTime() / 1000 / 60;
            const auto seconds = session.autoSaveTimer.remainingTime() / 1000 % 60;
            unsavedChangesLabel.setToolTip(tr("Next autosave in %1:%2 min").arg(minutes).arg(seconds, 2, 10, QChar('0')));
        }
        unsavedChangesLabel.setText(tr("unsaved changes ") + autosave);
    } else {
        unsavedChangesLabel.setText("saved");
    }
    //don’t display if there are no changes and no file is loaded
    if (session.unsavedChanges == false && session.annotationFilename.isEmpty()) {
        unsavedChangesLabel.hide();
        annotationTimeLabel.hide();
    } else {
        unsavedChangesLabel.show();
        annotationTimeLabel.show();
    }

    if(session.autoSaveTimer.isActive() == false) {
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

void MainWindow::createMenus() {
    menuBar()->addMenu(&fileMenu);
    fileMenu.addAction(QIcon(":/resources/icons/open-dataset.png"), tr("Choose Dataset…"), &widgetContainer.datasetLoadWidget, SLOT(show()));
    fileMenu.addSeparator();
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/graph.png"), tr("Create New Annotation"), this, &MainWindow::newAnnotationSlot, QKeySequence::New);
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/open-annotation.png"), tr("Load Annotation…"), this, &MainWindow::openSlot, QKeySequence::Open);
    auto & recentfileMenu = *fileMenu.addMenu(QIcon(":/resources/icons/document-open-recent.png"), tr("Recent Annotation File(s)"));
    int i = 0;
    for (auto & elem : historyEntryActions) {
        elem = recentfileMenu.addAction(QIcon(":/resources/icons/document-open-recent.png"), "");
        elem->setVisible(false);
        QObject::connect(elem, &QAction::triggered, [this, i](){
            openFileDispatch({skeletonFileHistory.at(i)});
        });
        ++i;
    }
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/document-save.png"), tr("Save Annotation"), this, &MainWindow::saveSlot, QKeySequence::Save);
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/document-save-as.png"), tr("Save Annotation As…"), this, &MainWindow::saveAsSlot, QKeySequence::SaveAs);
    fileMenu.addSeparator();
    fileMenu.addAction(tr("Export to NML..."), this, SLOT(exportToNml()));
    fileMenu.addSeparator();
    addApplicationShortcut(fileMenu, QIcon(":/resources/icons/system-shutdown.png"), tr("Quit"), this, &MainWindow::close, QKeySequence::Quit);

    const QString segStateString = segmentState == SegmentState::On ? tr("On") : tr("Off");
    //advanced skeleton
    toggleSegmentsAction = &addApplicationShortcut(actionMenu, QIcon(), tr("Segments: ") + segStateString, this, &MainWindow::toggleSegments, Qt::Key_A);
    newTreeAction = &addApplicationShortcut(actionMenu, QIcon(), tr("New Tree"), this, &MainWindow::newTreeSlot, Qt::Key_C);
    //skeleton
    pushBranchAction = &addApplicationShortcut(actionMenu, QIcon(), tr("Push Branch Node"), this, &MainWindow::pushBranchNodeSlot, Qt::Key_B);
    popBranchAction = &addApplicationShortcut(actionMenu, QIcon(), tr("Pop Branch Node"), this, &MainWindow::popBranchNodeSlot, Qt::Key_J);
    clearSkeletonAction = actionMenu.addAction(QIcon(":/resources/icons/user-trash.png"), "Clear Skeleton", this, SLOT(clearSkeletonSlot()));
    //segmentation
    clearMergelistAction = actionMenu.addAction(QIcon(":/resources/icons/user-trash.png"), "Clear Merge List", &Segmentation::singleton(), SLOT(clear()));
    //proof reading mode
    modeSwitchSeparator = actionMenu.addSeparator();
    setMergeModeAction = &addApplicationShortcut(actionMenu, QIcon(), tr("Switch to Segmentation Merge mode"), this, [this]() { setWorkMode(AnnotationMode::Mode_Merge); }, Qt::Key_1);
    setPaintModeAction = &addApplicationShortcut(actionMenu, QIcon(), tr("Switch to Paint mode"), this, [this]() { setWorkMode(AnnotationMode::Mode_Paint); }, Qt::Key_2);

    menuBar()->addMenu(&actionMenu);

    auto viewMenu = menuBar()->addMenu("Navigation");

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

    viewMenu->addAction(tr("Dataset Navigation Options"), &widgetContainer.navigationWidget, SLOT(show()));

    auto commentsMenu = menuBar()->addMenu("Comments");

    addApplicationShortcut(*commentsMenu, QIcon(), tr("Next Comment"), &Skeletonizer::singleton(), [this] () {
        Skeletonizer::singleton().gotoComment(widgetContainer.annotationWidget.skeletonTab.getFilterComment(), true);
    }, Qt::Key_N);
    addApplicationShortcut(*commentsMenu, QIcon(), tr("Previous Comment"), &Skeletonizer::singleton(), [this] () {
        Skeletonizer::singleton().gotoComment(widgetContainer.annotationWidget.skeletonTab.getFilterComment(), false);
    }, Qt::Key_P);

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
    preferenceMenu->addAction(tr("Data Saving Options"), &widgetContainer.dataSavingWidget, SLOT(show()));
    preferenceMenu->addAction(QIcon(":/resources/icons/view-list-icons-symbolic.png"), "Appearance Settings", &widgetContainer.appearanceWidget, SLOT(show()));

    auto windowMenu = menuBar()->addMenu("Windows");
    windowMenu->addAction(QIcon(":/resources/icons/task.png"), tr("Task Management"), &widgetContainer.taskManagementWidget, SLOT(updateAndRefreshWidget()));
    windowMenu->addAction(QIcon(":/resources/icons/graph.png"), tr("Annotation Window"), &widgetContainer.annotationWidget, SLOT(show()));
    windowMenu->addAction(QIcon(":/resources/icons/zoom-in.png"), tr("Dataset Options"), &widgetContainer.datasetOptionsWidget, SLOT(show()));
    windowMenu->addAction(QIcon(":/resources/icons/camera.png"), tr("Take a snapshot"), &widgetContainer.snapshotWidget, SLOT(show()));

    auto scriptingMenu = menuBar()->addMenu("Scripting");
    scriptingMenu->addAction("Properties", this, SLOT(pythonPropertiesSlot()));
    scriptingMenu->addAction("Run File", this, SLOT(pythonFileSlot()));
    scriptingMenu->addAction("Plugin Manager", this, SLOT(pythonPluginMgrSlot()));
    scriptingMenu->addAction("Interpreter", this, SLOT(pythonInterpreterSlot()));
    pluginMenu = scriptingMenu->addMenu("Plugins");
    refreshPluginMenu();

    auto helpMenu = menuBar()->addMenu("Help");
    addApplicationShortcut(*helpMenu, QIcon(":/resources/icons/edit-select-all.png"), tr("Documentation"), &widgetContainer.docWidget, &DocumentationWidget::show, Qt::CTRL + Qt::Key_H);
    helpMenu->addAction(QIcon(":/resources/icons/knossos.png"), "About", &widgetContainer.splashWidget, SLOT(show()));
}

void MainWindow::closeEvent(QCloseEvent *event) {
    EmitOnCtorDtor eocd(&SignalRelay::Signal_MainWindow_closeEvent, state->signalRelay, event);
    saveSettings();

    if (Session::singleton().unsavedChanges) {
         QMessageBox question(this);
         question.setIcon(QMessageBox::Question);
         question.setText(tr("Really quit KNOSSOS despite changes?"));
         question.setInformativeText(tr("All unsaved changes will be lost."));
         question.addButton(tr("&Quit"), QMessageBox::AcceptRole);
         const QPushButton * const cancelButton = question.addButton(QMessageBox::Cancel);
         question.exec();
         if (question.clickedButton() == cancelButton) {
             event->ignore();
             return;//we changed our mind – we dont want to quit anymore
         }
    }

    state->quitSignal = true;
    QApplication::processEvents();//ensure everything’s done
    Loader::Controller::singleton().suspendLoader();

    event->accept();//mainwindow takes the qapp with it
}

//file menu functionality
bool MainWindow::openFileDispatch(QStringList fileNames, const bool mergeAll, const bool silent) {
    if (fileNames.empty()) {
        return false;
    }

    bool mergeSkeleton = mergeAll;
    bool mergeSegmentation = mergeAll;
    if (!mergeAll && !state->skeletonState->trees.empty()) {
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
    if (!mergeAll && Segmentation::singleton().hasObjects()) {
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
    Session::singleton().guiMode = GUIMode::None; // always reset to default gui
    auto nmlEndIt = std::stable_partition(std::begin(fileNames), std::end(fileNames), [](const QString & elem){
        return QFileInfo(elem).suffix() == "nml";
    });

    state->skeletonState->mergeOnLoadFlag = mergeSkeleton;

    const auto skeletonSignalBlockState = Skeletonizer::singleton().blockSignals(true);
    auto nmls = std::vector<QString>(std::begin(fileNames), nmlEndIt);
    auto zips = std::vector<QString>(nmlEndIt, std::end(fileNames));
    try {
        for (const auto & filename : nmls) {
            const QString treeCmtOnMultiLoad = multipleFiles ? QFileInfo(filename).fileName() : "";
            QFile file(filename);
            state->viewer->skeletonizer->loadXmlSkeleton(file, treeCmtOnMultiLoad);
            updateRecentFile(filename);
            state->skeletonState->mergeOnLoadFlag = true;//multiple files have to be merged
        }
        for (const auto & filename : zips) {
            const QString treeCmtOnMultiLoad = multipleFiles ? QFileInfo(filename).fileName() : "";
            annotationFileLoad(filename, treeCmtOnMultiLoad);
            updateRecentFile(filename);
            state->skeletonState->mergeOnLoadFlag = true;//multiple files have to be merged
        }
    } catch (std::runtime_error & error) {
        if (!silent) {
            QMessageBox fileErrorBox(this);
            fileErrorBox.setIcon(QMessageBox::Warning);
            fileErrorBox.setText(tr("Loading failed."));
            fileErrorBox.setInformativeText(tr("One of the files could not be successfully loaded."));
            fileErrorBox.setDetailedText(error.what());
            fileErrorBox.exec();
        }

        Skeletonizer::singleton().blockSignals(skeletonSignalBlockState);
        Session::singleton().unsavedChanges = false;// meh, don’t ask
        newAnnotationSlot();
        if (silent) {
            throw;
        }
        return false;
    }
    Skeletonizer::singleton().blockSignals(skeletonSignalBlockState);
    Skeletonizer::singleton().resetData();
    widgetContainer.appearanceWidget.nodesTab.updateProperties(Skeletonizer::singleton().getNumberProperties());

    Session::singleton().unsavedChanges = multipleFiles || mergeSkeleton || mergeSegmentation;//merge implies changes

    Session::singleton().annotationFilename = multipleFiles ? "" : !nmls.empty() ? nmls.front() : zips.front();// either an .nml or a .k.zip was loaded
    updateTitlebar();

    setProofReadingUI(Session::singleton().guiMode == GUIMode::ProofReading);
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeSimple)) { // we need to apply job mode here to ensure that all necessary parts are loaded by now.
        setJobModeUI(true);
        Segmentation::singleton().startJobMode();
    }

    state->viewer->window->widgetContainer.datasetOptionsWidget.update();

    return true;
}

void MainWindow::newAnnotationSlot() {
    if (Session::singleton().unsavedChanges) {
        const auto text = tr("There are unsaved changes. \nCreating a new annotation will make you lose what you’ve done.");
        const auto button = QMessageBox::question(this, tr("Unsaved changes"), text, tr("Abandon changes – Start from scratch"), tr("&Cancel"), {}, 0);
        if (button == 1) {
            return;
        }
    }
    Skeletonizer::singleton().clearSkeleton();
    Segmentation::singleton().clear();
    Session::singleton().unsavedChanges = false;
    Session::singleton().annotationFilename = "";
    updateTitlebar();
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

void MainWindow::saveSlot() {
    auto annotationFilename = Session::singleton().annotationFilename;
    if (annotationFilename.isEmpty()) {
        saveAsSlot();
    } else {
        if(annotationFilename.endsWith(".nml")) {
            annotationFilename.chop(4);
            annotationFilename += ".k.zip";
        }
        if (Session::singleton().autoFilenameIncrementBool) {
            int index = skeletonFileHistory.indexOf(annotationFilename);
            annotationFilename = updatedFileName(annotationFilename);
            if (index != -1) {//replace old filename with updated one
                skeletonFileHistory.replace(index, annotationFilename);
                historyEntryActions[index]->setText(annotationFilename);
            }
        }
        save(annotationFilename);
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
                const auto message = tr("The supplied filename has been changed: \n%1 to\n%2.k.zip").arg(prevFilename).arg(fileName);
                QMessageBox::information(this, tr("Fixed filename"), message);
            }
        }
        saveFileDirectory = QFileInfo(fileName).absolutePath();
        save(fileName + ".k.zip");
    }
}

void MainWindow::save(QString filename, const bool silent)
try {
    if (filename.isEmpty()) {
        filename = annotationFileDefaultPath();
    }
    annotationFileSave(filename);
    Session::singleton().annotationFilename = filename;
    updateRecentFile(filename);
    updateTitlebar();
} catch (std::runtime_error & error) {
    if (silent) {
        throw;
    } else {
        QMessageBox errorBox(this);
        errorBox.setIcon(QMessageBox::Critical);
        errorBox.setText("File save failed");
        errorBox.setInformativeText(filename);
        errorBox.setDetailedText(error.what());
        errorBox.exec();
    }
}

void MainWindow::exportToNml() {
    if (!state->skeletonState->trees.empty()) {
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
    if (workModes.find(workMode) == std::end(workModes)) {
        workMode = AnnotationMode::Mode_Tracing;
    }
    modeCombo.setCurrentIndex(workModeModel.indexOf(workMode));
    auto & mode = Session::singleton().annotationMode;
    mode = workMode;
    const bool trees = mode.testFlag(AnnotationMode::Mode_TracingAdvanced) || mode.testFlag(AnnotationMode::Mode_MergeTracing);
    const bool skeleton = mode.testFlag(AnnotationMode::Mode_Tracing) || mode.testFlag(AnnotationMode::Mode_TracingAdvanced) || mode.testFlag(AnnotationMode::Mode_MergeTracing);
    const bool segmentation = mode.testFlag(AnnotationMode::Brush) || mode.testFlag(AnnotationMode::Mode_MergeTracing);
    toggleSegmentsAction->setVisible(mode.testFlag(AnnotationMode::Mode_TracingAdvanced));
    segmentStateLabel.setVisible(mode.testFlag(AnnotationMode::Mode_TracingAdvanced));
    if (mode.testFlag(AnnotationMode::Mode_TracingAdvanced)) {
        setSegmentState(segmentState);
    } else if (mode.testFlag(AnnotationMode::Mode_Tracing)) {
        setSegmentState(SegmentState::On);
    }
    newTreeAction->setVisible(trees);
    widgetContainer.annotationWidget.commandsTab.enableNewTreeButton(trees);
    pushBranchAction->setVisible(mode.testFlag(AnnotationMode::NodeEditing));
    popBranchAction->setVisible(mode.testFlag(AnnotationMode::NodeEditing));
    clearSkeletonAction->setVisible(skeleton);
    clearMergelistAction->setVisible(segmentation);
}

void MainWindow::setSegmentState(const SegmentState newState) {
    segmentState = newState;
    QString stateName = "";
    QString nextState = "";
    switch(segmentState) {
    case SegmentState::On:
        stateName = tr("<font color='green'>On</font>");
        nextState = tr("off once");
        Session::singleton().annotationMode |= AnnotationMode::LinkedNodes;
        break;
    case SegmentState::Off_Once:
        stateName = tr("<font color='darkGoldenRod'>Off once</font>");
        nextState = tr("off");
        Session::singleton().annotationMode &= ~QFlags<AnnotationMode>(AnnotationMode::LinkedNodes);
        break;
    case SegmentState::Off:
        stateName = tr("<font color='blue'>Off</font>");
        nextState = tr("on");
        Session::singleton().annotationMode &= ~QFlags<AnnotationMode>(AnnotationMode::LinkedNodes);
        break;
    }
    toggleSegmentsAction->setText(tr("Turn segments %1").arg(nextState));
    segmentStateLabel.setText(tr("Segments (toggle with a): ") + stateName);
}

void MainWindow::toggleSegments() {
    switch(segmentState) {
    case SegmentState::On:
        setSegmentState(SegmentState::Off_Once);
        break;
    case SegmentState::Off_Once:
        setSegmentState(SegmentState::Off);
        break;
    case SegmentState::Off:
        setSegmentState(SegmentState::On);
        break;
    }
}

void MainWindow::clearSkeletonSlot() {
    if(Session::singleton().unsavedChanges || !state->skeletonState->trees.empty()) {
        QMessageBox question(this);
        question.setIcon(QMessageBox::Question);
        question.setText(tr("Do you really want to clear the skeleton?"));
        question.setInformativeText(tr("This erases all trees and their nodes and cannot be undone."));
        QPushButton const * const ok = question.addButton(tr("Clear Skeleton"), QMessageBox::AcceptRole);
        question.addButton(QMessageBox::Cancel);
        question.exec();
        if (question.clickedButton() == ok) {
            Skeletonizer::singleton().clearSkeleton();
            updateTitlebar();
        }
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
        state->viewer->loadTreeLUT();
        state->viewer->defaultDatasetLUT();
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
    floatCoordinate inputCoord{xField->value() - 1.f, yField->value() - 1.f, zField->value() - 1.f};
    state->viewer->userMove(inputCoord - state->viewerState->currentPosition, USERMOVE_NEUTRAL);
}

void MainWindow::saveSettings() {
    QSettings settings;

    settings.beginGroup(MAIN_WINDOW);
    settings.setValue(GEOMETRY, saveGeometry());
    settings.setValue(STATE, saveState());

    // viewport position and sizes
    settings.setValue(VP_DEFAULT_POS_SIZE, state->viewerState->defaultVPSizeAndPos);

    forEachVPDo([&settings] (ViewportBase & vp) {
        settings.setValue(VP_I_POS.arg(vp.viewportType), vp.dockPos.isNull() ? vp.pos() : vp.dockPos);
        settings.setValue(VP_I_SIZE.arg(vp.viewportType), vp.dockSize.isEmpty() ? vp.size() : vp.dockSize);
        settings.setValue(VP_I_VISIBLE.arg(vp.viewportType), vp.isVisible());

    });
    QList<QVariant> order;
    for (const auto & w : centralWidget()->children()) {
        forEachVPDo([&w, &order](ViewportBase & vp) {
            if (w == &vp) {
                order.append(vp.viewportType);
            }
        });
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

    settings.setValue(GUI_MODE, static_cast<int>(Session::singleton().guiMode));

    settings.endGroup();

    widgetContainer.datasetLoadWidget.saveSettings();
    widgetContainer.dataSavingWidget.saveSettings();
    widgetContainer.datasetOptionsWidget.saveSettings();
    widgetContainer.appearanceWidget.saveSettings();
    widgetContainer.navigationWidget.saveSettings();
    widgetContainer.annotationWidget.saveSettings();
    widgetContainer.pythonPropertyWidget.saveSettings();
    widgetContainer.pythonInterpreterWidget.saveSettings();
    widgetContainer.snapshotWidget.saveSettings();
    widgetContainer.taskManagementWidget.taskLoginWidget.saveSettings();
    //widgetContainer.toolsWidget->saveSettings();
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
        forEachVPDo([&settings](ViewportBase & vp) {
            vp.move(settings.value(VP_I_POS.arg(vp.viewportType)).toPoint());
            vp.resize(settings.value(VP_I_SIZE.arg(vp.viewportType)).toSize());
            vp.setVisible(settings.value(VP_I_VISIBLE.arg(vp.viewportType), true).toBool());
        });
    }
    for (const auto & i : settings.value(VP_ORDER).toList()) {
        viewport(static_cast<ViewportType>(i.toInt()))->raise();
    }

    auto autosaveLocation = QFileInfo(annotationFileDefaultPath()).dir().absolutePath();
    QDir().mkpath(autosaveLocation);

    openFileDirectory = settings.value(OPEN_FILE_DIALOG_DIRECTORY, autosaveLocation).toString();

    saveFileDirectory = settings.value(SAVE_FILE_DIALOG_DIRECTORY, autosaveLocation).toString();

    setWorkMode(static_cast<AnnotationMode>(settings.value(ANNOTATION_MODE, static_cast<int>(AnnotationMode::Mode_Tracing)).toInt()));

    for (int nr = 10; nr != 0; --nr) {//reverse, because new ones are added in front
        updateRecentFile(settings.value(QString("loaded_file%1").arg(nr), "").toString());
    }

    setSegmentState(static_cast<SegmentState>(settings.value(SEGMENT_STATE, static_cast<int>(SegmentState::On)).toInt()));

    Session::singleton().guiMode = static_cast<GUIMode>(settings.value(GUI_MODE, static_cast<int>(GUIMode::None)).toInt());
    setProofReadingUI(Session::singleton().guiMode == GUIMode::ProofReading);

    settings.endGroup();

    widgetContainer.datasetLoadWidget.loadSettings();
    widgetContainer.dataSavingWidget.loadSettings();
    widgetContainer.datasetOptionsWidget.loadSettings();
    widgetContainer.appearanceWidget.loadSettings();
    widgetContainer.navigationWidget.loadSettings();
    widgetContainer.annotationWidget.loadSettings();
    widgetContainer.pythonPropertyWidget.loadSettings();
    widgetContainer.pythonInterpreterWidget.loadSettings();
    widgetContainer.snapshotWidget.loadSettings();
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
        forEachVPDo([](ViewportBase & vp) {
            vp.posAdapt();
            vp.sizeAdapt();
        });
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
    if (Session::singleton().guiMode == GUIMode::ProofReading) {
        viewportXY.get()->setDock(true);
        viewportXY.get()->show();
    } else {
        forEachVPDo([](ViewportBase & vp) {
            vp.setDock(true);
            vp.show();
        });
    }
    resizeToFitViewports(centralWidget()->width(), centralWidget()->height());
    state->viewerState->defaultVPSizeAndPos = true;
}

void MainWindow::showVPDecorationClicked() {
    bool isShow = widgetContainer.appearanceWidget.viewportTab.showVPDecorationCheckBox.isChecked();
    forEachVPDo([&isShow](ViewportBase & vp) { vp.showHideButtons(isShow); });
}

void MainWindow::newTreeSlot() {
    Skeletonizer::singleton().addTreeListElement();
}

void MainWindow::pushBranchNodeSlot() {
    if(state->skeletonState->activeNode) {
        Skeletonizer::singleton().pushBranchNode(*state->skeletonState->activeNode);
    }
}

void MainWindow::popBranchNodeSlot() {
    Skeletonizer::singleton().popBranchNodeAfterConfirmation(this);
}

void MainWindow::placeComment(const int index) {
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing) && state->skeletonState->activeNode != nullptr) {
        CommentSetting comment = CommentSetting::comments[index];
        if (!comment.text.isEmpty()) {
            if(comment.appendComment) {
                auto activeNodeComment = state->skeletonState->activeNode->getComment();
                if(activeNodeComment.isEmpty() == false) {
                    activeNodeComment.append(' ');
                    comment.text.prepend(activeNodeComment);
                }
            }
            Skeletonizer::singleton().setComment(*state->skeletonState->activeNode, comment.text);
        }
    } else {
        Segmentation::singleton().placeCommentForSelectedObject(CommentSetting::comments[index].text);
    }
}

void MainWindow::resizeToFitViewports(int width, int height) {
    width = (width - DEFAULT_VP_MARGIN);
    height = (height - DEFAULT_VP_MARGIN);
    int mindim = std::min(width, height);
    if (Session::singleton().guiMode == GUIMode::ProofReading) {
        viewportXY->setGeometry(DEFAULT_VP_MARGIN, DEFAULT_VP_MARGIN, mindim - DEFAULT_VP_MARGIN, mindim - DEFAULT_VP_MARGIN);
    } else {
        mindim /= 2;
        viewportXY->move(DEFAULT_VP_MARGIN, DEFAULT_VP_MARGIN);
        viewportXZ->move(DEFAULT_VP_MARGIN, DEFAULT_VP_MARGIN + mindim);
        viewportZY->move(DEFAULT_VP_MARGIN + mindim, DEFAULT_VP_MARGIN);
        viewportArb->move(DEFAULT_VP_MARGIN + 2 * mindim, DEFAULT_VP_MARGIN);
        viewport3D->move(DEFAULT_VP_MARGIN + mindim, DEFAULT_VP_MARGIN + mindim);
        forEachVPDo([&mindim](ViewportBase & vp) { vp.resize(mindim - DEFAULT_VP_MARGIN, mindim - DEFAULT_VP_MARGIN); });
    }
}

void MainWindow::pythonPropertiesSlot() {
    widgetContainer.pythonPropertyWidget.show();
}

void MainWindow::pythonFileSlot() {
    state->viewerState->renderInterval = SLOW;
    QString pyFileName = QFileDialog::getOpenFileName(this, "Select python file", QDir::homePath(), "*.py");
    state->viewerState->renderInterval = FAST;
    state->scripting->runFile(pyFileName);
}

void MainWindow::refreshPluginMenu() {
    auto actions = pluginMenu->actions();
    for (auto action : actions) {
        pluginMenu->removeAction(action);
    }
    QStringList pluginNames = state->scripting->getPluginNames().split(";");
    for (auto pluginName : pluginNames) {
        if (pluginName.isEmpty()) {
            continue;
        }
        auto pluginSubMenu = pluginMenu->addMenu(pluginName);
        QObject::connect(pluginSubMenu->addAction("Open"), &QAction::triggered,
                         [pluginName](){state->scripting->openPlugin(pluginName,false);});
        QObject::connect(pluginSubMenu->addAction("Close"), &QAction::triggered,
                         [pluginName](){state->scripting->closePlugin(pluginName,false);});
        QObject::connect(pluginSubMenu->addAction("Show"), &QAction::triggered,
                         [pluginName](){state->scripting->showPlugin(pluginName,false);});
        QObject::connect(pluginSubMenu->addAction("Hide"), &QAction::triggered,
                         [pluginName](){state->scripting->hidePlugin(pluginName,false);});
        QObject::connect(pluginSubMenu->addAction("Reload"), &QAction::triggered,
                         [pluginName](){state->scripting->reloadPlugin(pluginName,false);});
        QObject::connect(pluginSubMenu->addAction("Instantiate"), &QAction::triggered,
                         [pluginName](){state->scripting->instantiatePlugin(pluginName,false);});
        QObject::connect(pluginSubMenu->addAction("Remove Instance"), &QAction::triggered,
                         [pluginName](){state->scripting->removePluginInstance(pluginName,false);});
        QObject::connect(pluginSubMenu->addAction("Import"), &QAction::triggered,
                         [pluginName](){state->scripting->importPlugin(pluginName,false);});
        QObject::connect(pluginSubMenu->addAction("Remove Import"), &QAction::triggered,
                         [pluginName](){state->scripting->removePluginImport(pluginName,false);});
    }
}

void MainWindow::pythonInterpreterSlot() {
    widgetContainer.pythonInterpreterWidget.show();
    widgetContainer.pythonInterpreterWidget.activateWindow();
}

void MainWindow::pythonPluginMgrSlot() {
    auto showError = [this](const QString &errorStr) {
        QMessageBox errorBox(QMessageBox::Warning, "Python Plugin Manager: Error", errorStr, QMessageBox::Ok, this);
        errorBox.exec();
        return;
    };
    auto pluginDir = state->scripting->getPluginDir();
    auto pluginMgrFn = PLUGIN_MGR_NAME + ".py";
    auto pluginMgrPath = QString("%1/%2").arg(pluginDir).arg(pluginMgrFn);
    auto existed = QFile(pluginMgrPath).exists();
    if (!existed && !QFile::copy(QString(":/resources/plugins/%1").arg(pluginMgrFn), pluginMgrPath)) {
        return showError(QString("Cannot temporarily place default plugin manager in plugin directory:\n%1").arg(pluginMgrPath));
    }
    state->scripting->importPlugin(PLUGIN_MGR_NAME, false);
    if (!existed) {
        QFile pluginMgrFile(pluginMgrPath);
        if (!pluginMgrFile.setPermissions(QFile::WriteOther)) {
            return showError(QString("Cannot set write permissions for temporarily placed plugin manager in plugin directory:\n%1").arg(pluginMgrPath));
        }
        if (!pluginMgrFile.remove()) {
            return showError(QString("Cannot remove temporarily placed plugin manager from plugin directory:\n%1").arg(pluginMgrPath));
        }
    }
    state->scripting->openPlugin(PLUGIN_MGR_NAME, false);
}
