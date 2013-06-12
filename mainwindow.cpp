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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GUIConstants.h"
#include <QEvent>
#include <QMenu>
#include <QAction>
#include <QLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QStringList>
#include <QToolBar>
#include <QSpinBox>
#include <QLabel>
#include <QQueue>
#include <QKeySequence>
#include <QSettings>
#include <QDir>
#include <QAction>
#include <QThread>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "knossos-global.h"
#include "knossos.h"
#include "viewport.h"
#include "skeletonizer.h"

#include "widgetcontainer.h"
#include "widgets/console.h"
#include "widgets/tracingtimewidget.h"
#include "widgets/commentswidget.h"
#include "widgets/zoomandmultireswidget.h"
#include "widgets/navigationwidget.h"
#include "widgets/toolswidget.h"
#include "widgets/viewportsettingswidget.h"
#include "widgets/datasavingwidget.h"
#include "widgets/synchronizationwidget.h"
#include "widgets/splashscreenwidget.h"

extern struct stateInfo *state;
extern struct stateInfo *tempConfig;

// -- Constructor and destroyer -- //
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("KnossosQT");
    this->setWindowIcon(QIcon(":/images/logo.ico"));

    skeletonFileHistory = new QQueue<QString>();
    skeletonFileHistory->reserve(FILE_DIALOG_HISTORY_MAX_ENTRIES);

    state->viewerState->gui->oneShiftedCurrPos.x =
        state->viewerState->currentPosition.x + 1;
    state->viewerState->gui->oneShiftedCurrPos.y =
        state->viewerState->currentPosition.y + 1;
    state->viewerState->gui->oneShiftedCurrPos.z =
        state->viewerState->currentPosition.z + 1;

    state->viewerState->gui->activeTreeID = 1;
    state->viewerState->gui->activeNodeID = 1;

    state->viewerState->gui->totalNodes = 0;
    state->viewerState->gui->totalTrees = 0;

    state->viewerState->gui->zoomOrthoVPs =
        state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel;

    state->viewerState->gui->radioSkeletonDisplayMode = 0;

    /* init here instead of initSkeletonizer to fix some init order issue */
    state->skeletonState->displayMode = 0;
    state->skeletonState->displayMode |= DSP_SKEL_VP_WHOLE;

    state->viewerState->gui->radioRenderingModel = 1;

    state->viewerState->gui->commentBuffer = (char*)malloc(10240 * sizeof(char));
    memset(state->viewerState->gui->commentBuffer, '\0', 10240 * sizeof(char));

    state->viewerState->gui->commentSearchBuffer = (char*)malloc(2048 * sizeof(char));
    memset(state->viewerState->gui->commentSearchBuffer, '\0', 2048 * sizeof(char));

    state->viewerState->gui->treeCommentBuffer = (char*)malloc(8192 * sizeof(char));
    memset(state->viewerState->gui->treeCommentBuffer, '\0', 8192 * sizeof(char));

    state->viewerState->gui->comment1 = (char*)malloc(10240 * sizeof(char));
    memset(state->viewerState->gui->comment1, '\0', 10240 * sizeof(char));

    state->viewerState->gui->comment2 = (char*)malloc(10240 * sizeof(char));
    memset(state->viewerState->gui->comment2, '\0', 10240 * sizeof(char));

    state->viewerState->gui->comment3 = (char*)malloc(10240 * sizeof(char));
    memset(state->viewerState->gui->comment3, '\0', 10240 * sizeof(char));

    state->viewerState->gui->comment4 = (char*)malloc(10240 * sizeof(char));
    memset(state->viewerState->gui->comment4, '\0', 10240 * sizeof(char));

    state->viewerState->gui->comment5 = (char*)malloc(10240 * sizeof(char));
    memset(state->viewerState->gui->comment5, '\0', 10240 * sizeof(char));

    createActions();
    createMenus();

    widgetContainer = new WidgetContainer(this);
    widgetContainer->createWidgets();
    createCoordBarWin(); /* @todo make a CoordBarWidget class and push it to widgetContainer */


    mainWidget = new QWidget(this);
    gridLayout = new QGridLayout();
    mainWidget->setLayout(gridLayout);
    setCentralWidget(mainWidget);

    connect(widgetContainer->toolsWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckToolsAction()));
    connect(widgetContainer->viewportSettingsWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckViewportSettingAction()));
    connect(widgetContainer->commentsWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckCommentShortcutsAction()));
    connect(widgetContainer->console, SIGNAL(uncheckSignal()), this, SLOT(uncheckConsoleAction()));
    connect(widgetContainer->tracingTimeWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckTracingTimeAction()));
    connect(widgetContainer->zoomAndMultiresWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckZoomAndMultiresAction()));
    connect(widgetContainer->dataSavingWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckDataSavingAction()));
    connect(widgetContainer->navigationWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckNavigationAction()));
    connect(widgetContainer->synchronizationWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckSynchronizationAction()));
    updateTitlebar(false);
    loadSettings();
}

void MainWindow::createViewports() {
    viewports = new Viewport*[4];
    viewports[0] = new Viewport(this, VIEWPORT_XY);
    viewports[1] = new Viewport(this, VIEWPORT_YZ);
    viewports[2] = new Viewport(this, VIEWPORT_XZ);
    viewports[3] = new Viewport(this, VIEWPORT_SKELETON);

    viewports[0]->setGeometry(5, 40, 350,350);
    viewports[1]->setGeometry(355, 40, 350, 350);
    viewports[2]->setGeometry(5, 400, 350, 350);
    viewports[3]->setGeometry(355, 400, 350, 350);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow:: createCoordBarWin() {
    copyButton = new QPushButton("Copy");
    pasteButton = new QPushButton("Paste");

    this->toolBar = new QToolBar();
    this->addToolBar(toolBar);
    this->toolBar->addWidget(copyButton);
    this->toolBar->addWidget(pasteButton);

    xField = new QSpinBox();
    xField->setMaximum(10000);
    xField->setMinimumWidth(75);

    xField->setValue(state->viewerState->currentPosition.x);
    yField = new QSpinBox();
    yField->setMaximum(10000);
    yField->setMinimumWidth(75);
    yField->setValue(state->viewerState->currentPosition.y);
    zField = new QSpinBox();
    zField->setMaximum(10000);
    zField->setMinimumWidth(75);
    zField->setValue(state->viewerState->currentPosition.z);

    xLabel = new QLabel("x");
    yLabel = new QLabel("y");
    zLabel = new QLabel("z");

    this->toolBar->addWidget(xLabel);
    this->toolBar->addWidget(xField);
    this->toolBar->addWidget(yLabel);
    this->toolBar->addWidget(yField);
    this->toolBar->addWidget(zLabel);
    this->toolBar->addWidget(zField);


    connect(copyButton, SIGNAL(clicked()), this, SLOT(copyClipboardCoordinates()));
    connect(pasteButton, SIGNAL(clicked()), this, SLOT(pasteClipboardCoordinates()));
    //connect(xField, SIGNAL(editingFinished()), this, SLOT(coordinateEditingFinished()));
    //connect(yField, SIGNAL(editingFinished()), this, SLOT(coordinateEditingFinished()));
    //connect(zField, SIGNAL(editingFinished()), this, SLOT(coordinateEditingFinished()));
}

/**
  * This function is a replacement for the updateAgConfig() function in KNOSSOS 3.2
  * @todo Replacements for AG_Numerical
  */
static void updateGuiconfig() {
    state->viewerState->gui->totalTrees = state->skeletonState->treeElements;
    state->viewerState->gui->totalNodes = state->skeletonState->totalNodeElements;
    if(state->skeletonState->totalNodeElements == 0) {
        //AG_NumericalSetWriteable(state->viewerState->gui->actNodeIDWdgt1, false);
        //AG_NumericalSetWriteable(state->viewerState->gui->actNodeIDWdgt2, false);
        state->viewerState->gui->activeNodeID = 0;
        state->viewerState->gui->activeNodeCoord.x = 0;
        state->viewerState->gui->activeNodeCoord.y = 0;
        state->viewerState->gui->activeNodeCoord.z = 0;
    }
    else {
        //AG_NumericalSetWriteable(state->viewerState->gui->actNodeIDWdgt1, true);
        //AG_NumericalSetWriteable(state->viewerState->gui->actNodeIDWdgt2, true);
    }

    if(state->skeletonState->activeNode) {
        SET_COORDINATE(state->viewerState->gui->activeNodeCoord,
            state->skeletonState->activeNode->position.x + 1,
            state->skeletonState->activeNode->position.y + 1,
            state->skeletonState->activeNode->position.z + 1)
        state->viewerState->gui->actNodeRadius =
            state->skeletonState->activeNode->radius;
    }

    if(state->skeletonState->activeTree) {
        state->viewerState->gui->actTreeColor =
            state->skeletonState->activeTree->color;
        strncpy(state->viewerState->gui->treeCommentBuffer,
                state->skeletonState->activeTree->comment,
                8192);
    }

    SET_COORDINATE(state->viewerState->gui->oneShiftedCurrPos,
        state->viewerState->currentPosition.x + 1,
        state->viewerState->currentPosition.y + 1,
        state->viewerState->currentPosition.z + 1)

    state->viewerState->gui->numBranchPoints =
        state->skeletonState->branchStack->elementsOnStack;

    strncpy(state->viewerState->gui->commentBuffer,
        state->skeletonState->commentBuffer,
        10240);
}



void MainWindow::updateTitlebar(bool useFilename) {
    char *filename;
    if(state->skeletonState->skeletonFile) {
#ifdef Q_OS_UNIX
    filename = strrchr(state->skeletonState->skeletonFile, '/');
#else
    filename = strrchr(state->skeletonState->skeletonFile, '\\');
#endif
    }

    if(!useFilename ||!filename) {
        snprintf(state->viewerState->gui->titleString, 2047, "KNOSSOS %s showing %s [%s]", KVERSION, state->datasetBaseExpName, "no skeleton file");
    }
    else {
        snprintf(state->viewerState->gui->titleString, 2047, "KNOSSOS %s showing %s [%s]", KVERSION, state->datasetBaseExpName, ++filename);
    }

    QString title(state->viewerState->gui->titleString);
    setWindowTitle(title);
}

void MainWindow::showSplashScreen() {
    QSplashScreen splashScreen(QPixmap("../splash"), Qt::WindowStaysOnTopHint);
    splashScreen.show();

}

// -- static methods -- //

bool MainWindow::cpBaseDirectory(char *target, char *path, size_t len){

    char *hit;
        int baseLen;

    #ifdef Q_OS_UNIX
        hit = strrchr(path, '/');
    #else
        hit = strrchr(path, '\\');
    #endif

        if(hit == NULL) {
            LOG("Cannot find a path separator char in %s\n", path);
            return false;
        }

        baseLen = (int)(hit - path);
        if(baseLen > 2047) {
            LOG("Path too long\n");
            return false;
        }

        strncpy(target, path, baseLen);
        target[baseLen] = '\0';

        return true;

}


bool MainWindow::addRecentFile(const QString &fileName) {
    if(skeletonFileHistory->size() < FILE_DIALOG_HISTORY_MAX_ENTRIES) {
        skeletonFileHistory->enqueue(fileName);
    } else {
        skeletonFileHistory->dequeue();
        skeletonFileHistory->enqueue(fileName);
    }

    updateFileHistoryMenu();
    return true;
}


void MainWindow::saveSkeleton(QString fileName, int increment) {
    if(fileName.isNull()) {
        return;
    }

    QFile saveFile(fileName);
    char *cpath = const_cast<char *>(fileName.toStdString().c_str());

    if(saveFile.open(QIODevice::ReadWrite)) {
        emit updateSkeletonFileNameSignal(CHANGE_MANUAL, state->skeletonState->autoFilenameIncrementBool, cpath);
        int saved = Skeletonizer::saveSkeleton();

        if(saved == FAIL) {
            QMessageBox box;
            box.setText("The skeleton was not saved successfully. Check disk space and access rights");
            box.show();
        } else if(!saved) {
            qDebug() << "No skeleton was found. Not saving";
        } else {
            updateTitlebar(true);
            qDebug("Successfully saved to %s", state->skeletonState->skeletonFile);
            state->skeletonState->unsavedChanges = false;
            addRecentFile(fileName);
        }

    } else {

    }
}

/* */
void MainWindow::UI_saveSkeleton(int increment) { }
void MainWindow::UI_saveSettings(){ }


void MainWindow::loadSkeleton(char *fileName) {
    strncpy(state->skeletonState->prevSkeletonFile, state->skeletonState->skeletonFile, 8192);
    strncpy(state->skeletonState->skeletonFile, fileName, 8192);

    if(Skeletonizer::loadSkeleton()) {
        updateTitlebar(true);
        linkWithActiveNodeSlot();
        qDebug() << "Successfully loded";
    } else {
        qDebug() << "Error";
        strncpy(state->skeletonState->skeletonFile, state->skeletonState->prevSkeletonFile, 8192);
    }
}

/**
  * @todo Replacements for the Labels
  * Maybe functionality of Viewport
  */
void MainWindow::reloadDataSizeWin(){
    float heightxy = state->viewerState->vpConfigs[0].displayedlengthInNmY*0.001;
    float widthxy = state->viewerState->vpConfigs[0].displayedlengthInNmX*0.001;
    float heightxz = state->viewerState->vpConfigs[1].displayedlengthInNmY*0.001;
    float widthxz = state->viewerState->vpConfigs[1].displayedlengthInNmX*0.001;
    float heightyz = state->viewerState->vpConfigs[2].displayedlengthInNmY*0.001;
    float widthyz = state->viewerState->vpConfigs[2].displayedlengthInNmX*0.001;

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

void MainWindow::treeColorAdjustmentsChanged(){

    //user lut activated
        if(state->viewerState->treeColortableOn) {
            //user lut selected
            if(state->viewerState->treeLutSet) {
                memcpy(state->viewerState->treeAdjustmentTable,
                state->viewerState->treeColortable,
                RGB_LUTSIZE * sizeof(float));
                Skeletonizer::updateTreeColors();
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
                    Skeletonizer::updateTreeColors();
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
        }
        else {
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
}

//-- private methods --//

void MainWindow::createActions()
{
    /* file actions */
    historyEntryActions = new QAction*[FILE_DIALOG_HISTORY_MAX_ENTRIES];
    for(int i = 0; i < FILE_DIALOG_HISTORY_MAX_ENTRIES; i++) {
        historyEntryActions[i] = new QAction("", this);
        historyEntryActions[i]->installEventFilter(this);
    }

    /* edit skeleton actions */
    addNodeAction = new QAction(tr("&Add Node"), this);
    addNodeAction->setCheckable(true);
    linkWithActiveNodeAction = new QAction(tr("&Link with Active Node"), this);
    linkWithActiveNodeAction->setCheckable(true);
    dropNodesAction = new QAction(tr("&Drop Nodes"), this);
    dropNodesAction->setCheckable(true);
    skeletonStatisticsAction = new QAction(tr("&Skeleton Statistics"), this);
    clearSkeletonAction =  new QAction(tr("&Clear Skeleton"), this);

    if(state->skeletonState->workMode == SKELETONIZER_ON_CLICK_ADD_NODE) {
        addNodeAction->setChecked(true);
    } else if(state->skeletonState->workMode == SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE) {
        linkWithActiveNodeAction->setChecked(true);
    } else if(state->skeletonState->workMode == SKELETONIZER_ON_CLICK_DROP_NODE) {
        dropNodesAction->setChecked(true);
    }

    connect(addNodeAction, SIGNAL(triggered()), this, SLOT(addNodeSlot()));
    connect(linkWithActiveNodeAction, SIGNAL(triggered()), this, SLOT(linkWithActiveNodeSlot()));
    connect(dropNodesAction, SIGNAL(triggered()), this, SLOT(dropNodesSlot()));
    connect(skeletonStatisticsAction, SIGNAL(triggered()), this, SLOT(skeletonStatisticsSlot()));
    connect(clearSkeletonAction, SIGNAL(triggered()), this, SLOT(clearSkeletonSlot()));

    /* view actions */
    workModeViewAction = new QAction(tr("&Work Mode"), this);
    dragDatasetAction = new QAction(tr("&Drag Dataset"), this);
    dragDatasetAction->setCheckable(true);
    recenterOnClickAction = new QAction(tr("&Recenter on Click"), this);
    recenterOnClickAction->setCheckable(true);
    zoomAndMultiresAction = new QAction(tr("Zoom and Multires.."), this);
    zoomAndMultiresAction->setCheckable(true);
    tracingTimeAction = new QAction(tr("&Tracing Time"), this);
    tracingTimeAction->setCheckable(true);

    if(state->viewerState->workMode == ON_CLICK_DRAG) {
        dragDatasetAction->setChecked(true);
    } else if(state->viewerState->workMode == ON_CLICK_RECENTER) {
        recenterOnClickAction->setChecked(true);
    }

    connect(dragDatasetAction, SIGNAL(triggered()), this, SLOT(dragDatasetSlot()));
    connect(recenterOnClickAction, SIGNAL(triggered()), this, SLOT(recenterOnClickSlot()));
    connect(zoomAndMultiresAction, SIGNAL(triggered()), this, SLOT(zoomAndMultiresSlot()));
    connect(tracingTimeAction, SIGNAL(triggered()), this, SLOT(tracingTimeSlot()));

    /* preferences actions */
    loadCustomPreferencesAction = new QAction(tr("&Load Custom Preferences"), this);
    saveCustomPreferencesAction = new QAction(tr("&Save Custom Preferences"), this);
    defaultPreferencesAction = new QAction(tr("&Default Preferences"), this);
    datasetNavigationAction = new QAction(tr("&Dataset Navigation"), this);
    synchronizationAction = new QAction(tr("&Synchronization"), this);
    synchronizationAction->setCheckable(true);
    dataSavingOptionsAction = new QAction(tr("&Data Saving Options"), this);
    dataSavingOptionsAction->setCheckable(true);
    viewportSettingsAction = new QAction(tr("&Viewport Settings"), this);
    viewportSettingsAction->setCheckable(true);

    connect(loadCustomPreferencesAction, SIGNAL(triggered()), this, SLOT(loadCustomPreferencesSlot()));
    connect(saveCustomPreferencesAction, SIGNAL(triggered()), this, SLOT(saveCustomPreferencesSlot()));
    connect(defaultPreferencesAction, SIGNAL(triggered()), this, SLOT(defaultPreferencesSlot()));
    connect(datasetNavigationAction, SIGNAL(triggered()), this, SLOT(datatasetNavigationSlot()));
    connect(synchronizationAction, SIGNAL(triggered()), this, SLOT(synchronizationSlot()));
    connect(dataSavingOptionsAction, SIGNAL(triggered()), this, SLOT(dataSavingOptionsSlot()));
    connect(viewportSettingsAction, SIGNAL(triggered()), this, SLOT(viewportSettingsSlot()));

    /* window actions */
    toolsAction = new QAction(tr("&Tools"), this);
    toolsAction->setCheckable(true);
    logAction = new QAction(tr("&Log"), this);
    logAction->setCheckable(true);
    commentShortcutsAction = new QAction(tr("&Comment Shortcuts"), this);
    commentShortcutsAction->setCheckable(true);

    connect(toolsAction, SIGNAL(triggered()), this, SLOT(toolsSlot()));
    connect(logAction, SIGNAL(triggered()), this, SLOT(logSlot()));
    connect(commentShortcutsAction, SIGNAL(triggered()), this, SLOT(commentShortcutsSlots()));

    /* Help actions */
    aboutAction = new QAction(tr("&About"), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutSlot()));

}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(QIcon("open"), "&Open", this, SLOT(openSlot()), QKeySequence(tr("CTRL+O", "File|Open")));
    recentFileMenu = fileMenu->addMenu("Recent File(s)");

    /* History Entries */
    for(int i = 0; i < FILE_DIALOG_HISTORY_MAX_ENTRIES; i++) {
        ////historyEntryActions[i]->setText(skeletonFileHistory->at(i));
        historyEntryActions[i]->installEventFilter(this);
        //recentFileMenu->addAction(historyEntryActions[i]);
    }

    fileMenu->addAction(QIcon("save"), "&Save", this, SLOT(saveSlot()), QKeySequence(tr("CTRL+S", "File|Save")));
    fileMenu->addAction(QIcon("save_as"), "&Save As", this, SLOT(saveAsSlot()), QKeySequence(tr("CTRL+?", "File|Save As")));
    fileMenu->addSeparator();
    fileMenu->addAction(QIcon("quit"), "&Quit", this, SLOT(quitSlot()), QKeySequence(tr("CTRL+Q", "File|Quit")));

    editMenu = menuBar()->addMenu("&Edit Skeleton");
    workModeEditMenu = editMenu->addMenu("&Work Mode");
        workModeEditMenu->addAction(addNodeAction);
        workModeEditMenu->addAction(linkWithActiveNodeAction);
        workModeEditMenu->addAction(dropNodesAction);
    editMenu->addAction(skeletonStatisticsAction);
    editMenu->addAction(clearSkeletonAction);

    viewMenu = menuBar()->addMenu("&View");
    workModeViewMenu = viewMenu->addMenu("&Work Mode");
        workModeViewMenu->addAction(dragDatasetAction);
        workModeViewMenu->addAction(recenterOnClickAction);
    viewMenu->addAction(zoomAndMultiresAction);
    viewMenu->addAction(tracingTimeAction);

    preferenceMenu = menuBar()->addMenu("&Preferences");
    preferenceMenu->addAction(loadCustomPreferencesAction);
    preferenceMenu->addAction(saveCustomPreferencesAction);
    preferenceMenu->addAction(defaultPreferencesAction);
    preferenceMenu->addAction(datasetNavigationAction);
    preferenceMenu->addAction(synchronizationAction);
    preferenceMenu->addAction(dataSavingOptionsAction);
    preferenceMenu->addAction(viewportSettingsAction);

    windowMenu = menuBar()->addMenu("&Windows");
    windowMenu->addAction(toolsAction);
    windowMenu->addAction(logAction);
    windowMenu->addAction(commentShortcutsAction);

    helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAction);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSettings();

    if(state->skeletonState->unsavedChanges) {

        prompt = new QMessageBox(this);
        prompt->setWindowTitle("Confirmation");
        prompt->setText("There are unsaved changes. Really Quit?");

        QPushButton *yesButton = prompt->addButton(tr("Yes"), QMessageBox::ActionRole);
        QPushButton *noButton = prompt->addButton(tr("No"), QMessageBox::ActionRole);
        prompt->exec();


        if((QPushButton *) prompt->clickedButton() == yesButton) {            
            event->accept();
        }

        if((QPushButton *) prompt->clickedButton() == noButton) {
            event->ignore();
            prompt->close();
        }

    }
}



//file menu functionality

/**
  * This method opens the file dialog and receives a skeleton file name path. If the file dialog is not cancelled
  * the skeletonFileHistory Queue is updated with the file name entry. The history entries are compared to the the
  * selected file names. If the file is already loaded it will not be put to the queue
  * @todo lookup in skeleton directory, extend the file dialog with merge option
  *
  */
void MainWindow::openSlot() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Skeleton File", state->viewerState->gui->skeletonDirectory, "KNOSSOS Skeleton file(*.nml)");

    if(!fileName.isNull()) {
        QFileInfo info(fileName);
        QString path = info.canonicalPath();

        char *cpath = const_cast<char *>(info.canonicalPath().toStdString().c_str());
        MainWindow::cpBaseDirectory(state->viewerState->gui->skeletonDirectory, cpath, 2048);

        int ret = QMessageBox::question(this, "", "Do you like to merge the new skeleton into the currently loaded one?", QMessageBox::Yes | QMessageBox::No);

        if(ret == QMessageBox::Yes) {
            state->skeletonState->mergeOnLoadFlag = true;

        } else {
            state->skeletonState->mergeOnLoadFlag = false;
        }


        loadSkeleton(const_cast<char *>(fileName.toStdString().c_str()));
        if(!alreadyInMenu(path)) {
            addRecentFile(fileName);
        }
    }
}

bool MainWindow::alreadyInMenu(const QString &path) {
    for(int i = 0; i < this->skeletonFileHistory->size(); i++) {
        if(QString::compare(skeletonFileHistory->at(i), path, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}


/**
  * This method puts the history entries of the loaded skeleton files to the recent file menu section
  */
void MainWindow::updateFileHistoryMenu() {
    QQueue<QString>::iterator it;
    int i = 0;
    for(it = skeletonFileHistory->begin(); it != skeletonFileHistory->end(); it++) {
        QString path = *it;
        historyEntryActions[i]->setText(path);
        recentFileMenu->addAction(historyEntryActions[i]);
        i++;
    }
}

/**
 * @todo
 */
void MainWindow::saveSlot()
{

}

void MainWindow::saveAsSlot()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save the KNOSSOS Skeleton file", QDir::homePath(), "KNOSSOS Skeleton file(*.nml)");
    if(!fileName.isEmpty()) {
        QFileInfo info(fileName);

        // override the skeletonDirectory
        memset(state->viewerState->gui->skeletonDirectory, '\0', 8192);
        strcpy(state->viewerState->gui->skeletonDirectory, const_cast<char *>(info.canonicalPath().toStdString().c_str()));

        QFile file(fileName);
        if(file.open(QIODevice::ReadWrite)) {
            saveSkeleton(fileName, false);
        }

        file.close();
    }

}

void MainWindow::quitSlot()
{
   saveSettings();
   QApplication::closeAllWindows();

}

/* edit skeleton functionality */

void MainWindow::addNodeSlot()
{
    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;

    if(linkWithActiveNodeAction->isChecked()) {
        linkWithActiveNodeAction->setChecked(false);
    }
    if(dropNodesAction->isChecked()) {
        dropNodesAction->setChecked(false);
    }
}

void MainWindow::linkWithActiveNodeSlot()
{
    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE;

    if(addNodeAction->isChecked()) {
        addNodeAction->setChecked(false);
    }
    if(dropNodesAction->isChecked()) {
        dropNodesAction->setChecked(false);
    }
}

void MainWindow::dropNodesSlot()
{
    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_DROP_NODE;

    if(addNodeAction->isChecked()) {
        addNodeAction->setChecked(false);
    }

    if(linkWithActiveNodeAction->isChecked()) {
        linkWithActiveNodeAction->setChecked(false);
    }
}


void MainWindow::skeletonStatisticsSlot()
{
    QMessageBox::information(this, "Information", "This feature is not yet implemented", QMessageBox::Ok);
}

/**
 * @todo Invokation of Skeleton::clearSkeleton leads to crashing the application
 */
void MainWindow::clearSkeletonSlot()
{
    int ret = QMessageBox::question(this, "", "Really clear the skeleton (you can not undo this) ?", QMessageBox::Ok | QMessageBox::No);

    switch(ret) {
        case QMessageBox::Ok:
            emit clearSkeletonSignal(CHANGE_MANUAL, false);
            updateTitlebar(false);
    }
}

/* view menu functionality */

void MainWindow::dragDatasetSlot()
{
   tempConfig->viewerState->workMode = ON_CLICK_DRAG;
   if(recenterOnClickAction->isChecked()) {
       recenterOnClickAction->setChecked(false);
   }
}

void MainWindow::recenterOnClickSlot()
{
   tempConfig->viewerState->workMode = ON_CLICK_RECENTER;
   if(dragDatasetAction->isChecked()) {
       dragDatasetAction->setChecked(true);
   }
}

void MainWindow::zoomAndMultiresSlot()
{
    this->widgetContainer->zoomAndMultiresWidget->show();
    zoomAndMultiresAction->setChecked(true);
}

void MainWindow::tracingTimeSlot()
{
    this->widgetContainer->tracingTimeWidget->show();
    tracingTimeAction->setChecked(true);
}

/* preference menu functionality */
/**
 * @todo UI_loadSettings
 */
void MainWindow::loadCustomPreferencesSlot()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Custom Preferences File", QDir::homePath(), "KNOSOS GUI preferences File (*.xml)");
    if(!fileName.isEmpty()) {
        cpBaseDirectory(state->viewerState->gui->settingsFile, const_cast<char *>(fileName.toStdString().c_str()), 2048);
        strncpy(state->viewerState->gui->settingsFile, fileName.toStdString().c_str(), 2048);
        /** @todo UI_loadSettings */

    }
}

/**
 * @todo UI_saveSettings
 */
void MainWindow::saveCustomPreferencesSlot()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Custom Preferences File As", QDir::homePath(), "KNOSSOS GUI preferences File (*.xml)");
    if(!fileName.isEmpty()) {
        cpBaseDirectory(state->viewerState->gui->customPrefsDirectory, const_cast<char *>(fileName.toStdString().c_str()), 2048);
        strncpy(state->viewerState->gui->settingsFile, fileName.toStdString().c_str(), 2048);
        /** @todo UI_saveSettings */
    }
}

/**
  * @todo the implementation for defaultPreferences
  */
void MainWindow::defaultPreferencesSlot() {
    int ret = QMessageBox::question(this, "", "Do you really want to load the default preferences ?", QMessageBox::Yes | QMessageBox::No);

    switch(ret) {
        case QMessageBox::Yes:
            break;
    case QMessageBox::No:
           break;
    }

}

void MainWindow::datatasetNavigationSlot()
{
    this->widgetContainer->navigationWidget->show();
    datasetNavigationAction->setChecked(true);
}

void MainWindow::synchronizationSlot()
{
    this->widgetContainer->synchronizationWidget->show();
    synchronizationAction->setChecked(true);
}

void MainWindow::dataSavingOptionsSlot()
{
    this->widgetContainer->dataSavingWidget->show();
    dataSavingOptionsAction->setChecked(true);
}

void MainWindow::viewportSettingsSlot()
{
    this->widgetContainer->viewportSettingsWidget->show();
    viewportSettingsAction->setChecked(true);
}

/* window menu functionality */

void MainWindow::toolsSlot()
{
    this->widgetContainer->toolsWidget->show();
    toolsAction->setChecked(true);
}

void MainWindow::logSlot()
{
    this->widgetContainer->console->show();
    logAction->setChecked(true);
}

void MainWindow::commentShortcutsSlots()
{
    this->widgetContainer->commentsWidget->show();
    commentShortcutsAction->setChecked(true);
}

/* help menu functionality */

void MainWindow::aboutSlot()
{
    this->widgetContainer->showSplashScreenWidget();
}

/* toolbar slots */

void MainWindow::copyClipboardCoordinates() {
   char copyString[8192];

   memset(copyString, '\0', 8192);

   snprintf(copyString,
                 8192,
                 "%d, %d, %d",
                 this->xField->value() + 1,
                 this->yField->value() + 1,
                 this->zField->value()) + 1;
   QString coords(copyString);
   QApplication::clipboard()->setText(coords);
}

/**
  * @todo uncommented method call refreshViewports can first be used when the viewports instances are saved in viewer-state
  * @bug updatePosition leads to application will be terminated
  * Why are the values saved in tempConfig and not state ?
  */
void MainWindow::pasteClipboardCoordinates(){
    QString text = QApplication::clipboard()->text();

    if(text.size() > 0) {
      char *pasteBuffer = const_cast<char *> (text.toStdString().c_str());

      Coordinate *extractedCoords = NULL;

      if((extractedCoords = Coordinate::parseRawCoordinateString(pasteBuffer))) {

            state->viewerState->currentPosition.x = extractedCoords->x;
            state->viewerState->currentPosition.y = extractedCoords->y;
            state->viewerState->currentPosition.z = extractedCoords->z;

            this->xField->setValue(extractedCoords->x);
            this->yField->setValue(extractedCoords->y);
            this->zField->setValue(extractedCoords->z);

            emit updatePositionSignal(TELL_COORDINATE_CHANGE);
            emit runSignal();

            free(extractedCoords);

      } else {
          qDebug("Unexpected Error in MainWindow::pasteCliboardCoordinates");
      }

    } else {
       LOG("Unable to fetch text from clipboard");
    }

}

#include "viewer.h"
void MainWindow::coordinateEditingFinished() {
    qDebug() << xField->value();
    qDebug() << yField->value();
    qDebug() << zField->value();

    state->viewerState->currentPosition.x = xField->value();
    state->viewerState->currentPosition.y = yField->value();
    state->viewerState->currentPosition.z = zField->value();

    state->currentPositionX.x = xField->value();
    state->currentPositionX.y = yField->value();
    state->currentPositionX.z = zField->value();
    state->loadSignal = true;

    //emit updatePositionSignal(TELL_COORDINATE_CHANGE);
}

void MainWindow::saveSettings() {
    QSettings settings;
    settings.beginGroup(MAIN_WINDOW);
    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());

    for(int i = 0; i < skeletonFileHistory->size(); i++) {
        settings.setValue(QString("loaded_file%1").arg(i+1), this->skeletonFileHistory->at(i));
    }

    settings.endGroup();

    widgetContainer->commentsWidget->saveSettings();
    widgetContainer->console->saveSettings();
    widgetContainer->dataSavingWidget->saveSettings();
    widgetContainer->zoomAndMultiresWidget->saveSettings();
    widgetContainer->viewportSettingsWidget->saveSettings();
    widgetContainer->navigationWidget->saveSettings();
    widgetContainer->toolsWidget->saveSettings();
}

/**
 * this method is a proposal for the qsettings variant
 */
void MainWindow::loadSettings() {
    QSettings settings;
    settings.beginGroup("MainWindow");
    int width = settings.value(WIDTH).toInt();
    int height = settings.value(HEIGHT).toInt();
    int x = settings.value(POS_X).toInt();
    int y = settings.value(POS_Y).toInt();

    if(!settings.value(LOADED_FILE1).isNull() and !settings.value(LOADED_FILE1).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE1).toString());
        this->skeletonFileHistory->dequeue();
    }
    if(!settings.value(LOADED_FILE2).isNull() and !settings.value(LOADED_FILE2).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE2).toString());
        this->skeletonFileHistory->dequeue();
    }
    if(!settings.value(LOADED_FILE3).isNull() and !settings.value(LOADED_FILE3).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE3).toString());
        this->skeletonFileHistory->dequeue();
    }
    if(!settings.value(LOADED_FILE4).isNull() and !settings.value(LOADED_FILE4).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE4).toString());
        this->skeletonFileHistory->dequeue();
    }
    if(!settings.value(LOADED_FILE5).isNull() and !settings.value(LOADED_FILE5).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE5).toString());
        this->skeletonFileHistory->dequeue();
    }
    if(!settings.value(LOADED_FILE6).isNull() and !settings.value(LOADED_FILE6).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE6).toString());
        this->skeletonFileHistory->dequeue();
    }
    if(!settings.value(LOADED_FILE7).isNull() and !settings.value(LOADED_FILE7).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE7).toString());
        this->skeletonFileHistory->dequeue();
    }
    if(!settings.value(LOADED_FILE8).isNull() and !settings.value(LOADED_FILE8).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE8).toString());
        this->skeletonFileHistory->dequeue();
    }
    if(!settings.value(LOADED_FILE9).isNull() and !settings.value(LOADED_FILE9).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE9).toString());
        this->skeletonFileHistory->dequeue();
    }
    if(!settings.value(LOADED_FILE10).isNull() and !settings.value(LOADED_FILE10).toString().isEmpty()) {
        this->skeletonFileHistory->enqueue(settings.value(LOADED_FILE10).toString());
        this->skeletonFileHistory->dequeue();
    }
    this->updateFileHistoryMenu();

    settings.endGroup();

    bool visible;
    this->setGeometry(x, y, width, height);

    widgetContainer->commentsWidget->loadSettings();
    widgetContainer->console->loadSettings();
    widgetContainer->dataSavingWidget->loadSettings();
    widgetContainer->zoomAndMultiresWidget->loadSettings();
    widgetContainer->viewportSettingsWidget->loadSettings();
    widgetContainer->navigationWidget->loadSettings();
    widgetContainer->toolsWidget->loadSettings();
}

/**
  * This Event Filter is used to find out which of the elements of the 2d array historyEntryAction is clicked
  */
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    for(int i = 0; i < FILE_DIALOG_HISTORY_MAX_ENTRIES; i++) {
        if(historyEntryActions[i] == obj) {
           QString fileName = historyEntryActions[i]->text();
           char *cname = const_cast<char *>(fileName.toStdString().c_str());
           loadSkeleton(cname);
           event->accept();
        }
    }

    return false;
}

/**
  * @todo
  */
void MainWindow::loadDefaultPrefs() {
    this->showMaximized();

    Knossos::loadTreeLUTFallback();
    treeColorAdjustmentsChanged();
    datasetColorAdjustmentsChanged();
}

void MainWindow::uncheckToolsAction() {
    this->toolsAction->setChecked(false);
}

void MainWindow::uncheckViewportSettingAction() {
    this->viewportSettingsAction->setChecked(false);
}

void MainWindow::uncheckCommentShortcutsAction() {
    this->commentShortcutsAction->setChecked(false);
}

void MainWindow::uncheckConsoleAction() {
    this->logAction->setChecked(false);
}

void MainWindow::uncheckDataSavingAction() {
    this->dataSavingOptionsAction->setChecked(false);
}

void MainWindow::uncheckSynchronizationAction() {
    this->synchronizationAction->setChecked(false);
}

void MainWindow::uncheckTracingTimeAction() {
    this->tracingTimeAction->setChecked(false);
}

void MainWindow::uncheckZoomAndMultiresAction() {
    this->zoomAndMultiresAction->setChecked(false);
}

void MainWindow::uncheckNavigationAction() {
    this->datasetNavigationAction->setChecked(false);
}

void MainWindow::updateCoordinateBar(int x, int y, int z) {
    xField->setValue(x);
    yField->setValue(y);
    zField->setValue(z);
}

void MainWindow::saveSkelCallback() {
    if(state->skeletonState->firstTree != NULL) {
        if(state->skeletonState->unsavedChanges) {
            UI_saveSkeleton(true);
        } else {
            qDebug("No changes since last save event. Not saving.");
            return;
        }
    } else {
        qDebug("No skeleton was found. Not saving.");
    }
}
