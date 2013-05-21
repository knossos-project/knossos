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
#include <QSpinbox>
#include <QLabel>
#include <QQueue>
#include <QKeySequence>
#include <QSettings>
#include <QDir>
#include <QThread>
#include "knossos-global.h"
#include "knossos.h"
#include "viewport.h"
#include "skeletonizer.h"
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

    settings = new QSettings();
    settings->beginGroup("MainWindow");
    skeletonFileHistory = new QQueue<QString>();

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

    createCoordBarWin();
    createConsoleWidget();
    createTracingTimeWidget();
    createCommentsWidget();
    createViewportSettingsWidget();
    createZoomAndMultiresWidget();
    createNavigationWidget();
    createToolWidget();

    createDataSavingWidget();
    createSychronizationWidget();


    mainWidget = new QWidget(this);
    gridLayout = new QGridLayout();
    mainWidget->setLayout(gridLayout);
    setCentralWidget(mainWidget);


    connect(toolsWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckToolsAction()));
    connect(viewportSettingsWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckViewportSettingAction()));
    connect(commentsWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckCommentShortcutsAction()));
    connect(console, SIGNAL(uncheckSignal()), this, SLOT(uncheckConsoleAction()));
    connect(tracingTimeWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckTracingTimeAction()));
    connect(zoomAndMultiresWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckZoomAndMultiresAction()));
    connect(dataSavingWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckDataSavingAction()));
    connect(navigationWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckNavigationAction()));
    connect(synchronizationWidget, SIGNAL(uncheckSignal()), this, SLOT(uncheckSynchronizationAction()));
    /*
    viewports = new Viewport*[NUM_VP];

    for(int i = 0; i < NUM_VP; i++) {
        viewports[i] = new Viewport(this, i);
    }

    */
    /*
    viewports[0]->setGeometry(5, 40, 500, 500);
    viewports[1]->setGeometry(510, 40, 500, 500);
    viewports[2]->setGeometry(5, 545, 500, 500);
    viewports[3]->setGeometry(510, 545, 500, 500);



    for(int i = 0; i < NUM_VP; i++) {
        SET_COORDINATE(state->viewerState->vpConfigs[i].upperLeftCorner,
                       viewports[i]->geometry().topLeft().x(),
                       viewports[i]->geometry().topLeft().y(),
                       0);
        state->viewerState->vpConfigs[i].edgeLength = viewports[i]->width();
    } */


    this->skeletonFileDialog = new QFileDialog(this);
    skeletonFileDialog->setWindowTitle(tr("Open Skeleton File"));
    skeletonFileDialog->setDirectory(QDir::home());
    skeletonFileDialog->setNameFilter(tr("KNOSSOS Skeleton File(*.nml)"));

    this->saveFileDialog = new QFileDialog(this);
    saveFileDialog->setWindowTitle(tr("Save Skeleton File"));
    saveFileDialog->setDirectory(QDir::home());
    saveFileDialog->setFileMode(QFileDialog::AnyFile);
    saveFileDialog->setNameFilter(tr("Knossos Skeleton File(*.nml)"));

    updateTitlebar(false);
}

MainWindow::~MainWindow()
{
    settings->endGroup();
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

    connect(xField, SIGNAL(valueChanged(int)), this, SLOT(xCoordinateChanged(int)));
    connect(yField, SIGNAL(valueChanged(int)), this, SLOT(yCoordinateChanged(int)));
    connect(zField, SIGNAL(valueChanged(int)), this, SLOT(zCoordinateChanged(int)));
}

// Dialogs
void MainWindow::createConsoleWidget() {
    console = new Console();
    console->setWindowFlags(Qt::WindowStaysOnTopHint);
    console->setGeometry(800, 500, 200, 120);

}

void MainWindow::createTracingTimeWidget() {
    tracingTimeWidget = new TracingTimeWidget();
    tracingTimeWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    tracingTimeWidget->setGeometry(800, 350, 200, 100);


}

void MainWindow::createCommentsWidget() {
    commentsWidget = new CommentsWidget();
    commentsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    commentsWidget->setGeometry(800, 100, 200, 150);

}

void MainWindow::createZoomAndMultiresWidget() {
    zoomAndMultiresWidget = new ZoomAndMultiresWidget();
    zoomAndMultiresWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    zoomAndMultiresWidget->setGeometry(1024, 100, 380, 200);

}

void MainWindow::createNavigationWidget() {
    navigationWidget = new NavigationWidget();
    navigationWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    navigationWidget->setGeometry(1024, 350, 200, 200);

}

void MainWindow::createToolWidget() {
    toolsWidget = new ToolsWidget();
    toolsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    toolsWidget->setGeometry(500, 100, 430, 610);

}

void MainWindow::createViewportSettingsWidget() {
    viewportSettingsWidget = new ViewportSettingsWidget();
    viewportSettingsWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    viewportSettingsWidget->setGeometry(500, 500, 680, 400);

}

void MainWindow::createDataSavingWidget() {
    dataSavingWidget = new DataSavingWidget();
    dataSavingWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    dataSavingWidget->setGeometry(100, 100, 100, 90);

}

void MainWindow::createSychronizationWidget() {
    synchronizationWidget = new SynchronizationWidget();
    synchronizationWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    synchronizationWidget->setGeometry(100, 350, 150, 100);

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

void MainWindow::showAboutScreen() {
    this->splashWidget = new SplashScreenWidget(this);
    splashWidget->setGeometry(400, 400, 500, 500);
    splashWidget->show();
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


bool MainWindow::addRecentFile(char *path, uint pos){return false;}

bool MainWindow::addRecentFile(QString fileName) {
    QQueue<QString>::iterator it;
    for(it = skeletonFileHistory->begin(); it != skeletonFileHistory->end(); it++) {
        QString path = *it;
        if(path.compare(fileName), Qt::CaseInsensitive == 0) {
            return false;
        }
    }

    if(skeletonFileHistory->size() < FILE_DIALOG_HISTORY_MAX_ENTRIES) {
        skeletonFileHistory->enqueue(fileName);
    } else {
        skeletonFileHistory->dequeue();
        skeletonFileHistory->enqueue(fileName);
    }

    return true;
}

void MainWindow::UI_saveSkeleton(int increment){
    /*
    //create directory if it does not exist
    DIR *skelDir;
    cpBaseDirectory(state->viewerState->gui->skeletonDirectory, state->skeletonState->skeletonFile, 2048);
    skelDir = opendir(state->viewerState->gui->skeletonDirectory);
    if(!skelDir) {
        #ifdef LINUX
            mkdir(state->viewerState->gui->skeletonDirectory, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP);
        #else
            mkdir(state->viewerState->gui->skeletonDirectory);
        #endif
    }

    FILE *saveFile;
    if(increment) {
        increment = state->skeletonState->autoFilenameIncrementBool;
    }

    Skeletonizer::updateSkeletonFileName(CHANGE_MANUAL,
                           increment,
                           state->skeletonState->skeletonFile);

    saveFile = fopen(state->skeletonState->skeletonFile, "r");
    if(saveFile) {
        //yesNoPrompt(NULL, "Overwrite existing skeleton file?", WRAP_saveSkeleton, NULL);
        fclose(saveFile);
        return;
    }

    //WRAP_saveSkeleton();
    */

}

void MainWindow::UI_saveSettings(){

}

/**
  * @todo Replacement for AG_String(X)
  * Prompt and Wrap_loadSkeleton
  */
void MainWindow::loadSkeleton() {
    char *path; // = AG_STRING(1);
    char *msg; // = AG_STRING(2);

    strncpy(state->skeletonState->prevSkeletonFile, state->skeletonState->skeletonFile, 8192);
    strncpy(state->skeletonState->skeletonFile, path, 8192);

    if(state->skeletonState->totalNodeElements != 0) {
        //yesNoPrompt(NULL, msg, WRAP_loadSkeleton, NULL);
    }
    else {
        //WRAP_loadSkeleton(); /**@todo */
    }
}


void MainWindow::zoomOrthogonals(float step){
    int i = 0;
        int triggerMagChange = false;

        for(i = 0; i < state->viewerState->numberViewports; i++) {
            if(state->viewerState->vpConfigs[i].type != VIEWPORT_SKELETON) {

                /* check if mag is locked */
                if(state->viewerState->datasetMagLock) {
                    if(!(state->viewerState->vpConfigs[i].texture.zoomLevel + step < VPZOOMMAX) &&
                       !(state->viewerState->vpConfigs[i].texture.zoomLevel + step > VPZOOMMIN)) {
                        state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                    }
                }
                else {
                    /* trigger a mag change when possible */
                    if((state->viewerState->vpConfigs[i].texture.zoomLevel + step < 0.5)
                        && (state->viewerState->vpConfigs[i].texture.zoomLevel >= 0.5)
                        && (state->magnification != state->lowestAvailableMag)) {
                        state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                        triggerMagChange = MAG_DOWN;
                    }
                    if((state->viewerState->vpConfigs[i].texture.zoomLevel + step > 1.0)
                        && (state->viewerState->vpConfigs[i].texture.zoomLevel <= 1.0)
                        && (state->magnification != state->highestAvailableMag)) {
                        state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                        triggerMagChange = MAG_UP;
                    }
                    /* performe normal zooming otherwise. This case also covers
                    * the special case of zooming in further than 0.5 on mag1 */
                    if(!triggerMagChange) {
                        if(!(state->viewerState->vpConfigs[i].texture.zoomLevel + step < 0.09999) &&
                           !(state->viewerState->vpConfigs[i].texture.zoomLevel + step > 1.0000)) {
                            state->viewerState->vpConfigs[i].texture.zoomLevel += step;
                        }
                    }

                }
            }
        }

        /* keep the agar slider / numerical widget informed */
        state->viewerState->gui->zoomOrthoVPs =
            state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel;

        if(triggerMagChange)
            emit changeDatasetMagSignal(triggerMagChange);

       emit recalcTextureOffsetsSignal();
       emit runSignal();

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
    for(int i = 0; i < skeletonFileHistory->size(); i++) {

        historyEntryActions[i]->setText(skeletonFileHistory->at(i));
        historyEntryActions[i]->installEventFilter(this);
        recentFileMenu->addAction(historyEntryActions[i]);
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

void MainWindow::closeEvent(QCloseEvent *event)
{
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
  *
  */
void MainWindow::openSlot()
{

    QString fileName = QFileDialog::getOpenFileName(this, "Open Skeleton File", QDir::homePath(), "KNOSSOS Skeleton file(*.nml)");
    int decision;

    if(!fileName.isNull()) {
        char *fileNameAsCharArray = const_cast<char *>(fileName.toStdString().c_str());
        MainWindow::cpBaseDirectory(state->viewerState->gui->skeletonDirectory, fileNameAsCharArray, 2048);

        int ret = QMessageBox::question(this, "", "Should the loaded skeleton be merged with the current skeleton?", QMessageBox::Yes | QMessageBox::No);



        loadedFile = new QFile(fileName);

    }
}

/**
  * This method puts the history entries of the loaded skeleton files to the recent file menu section
  */
void MainWindow::updateFileHistoryMenu() {
    QQueue<QString>::iterator it;
    int i = 0;
    for(it = skeletonFileHistory->begin(); it != skeletonFileHistory->end(); it++) {
        QString path = *it;
        historyEntryActions[i++]->setText(path);

    }
}

/**
 * @todo
 *
 */
void MainWindow::recentFilesSlot(int index)
{
    QString fileName = skeletonFileHistory->at(index - 1);


}

/**
 * @todo
 */
void MainWindow::saveSlot()
{



}

/**
  * @todo message box other parameter, save as logic
  */
void MainWindow::saveAsSlot()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save the KNOSSOS Skeleton file", QDir::homePath(), "KNOSSOS Skeleton file(*.nml)");
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)) {
        QMessageBox box(this);
    } else {
        // Save as logic
        file.close();
    }


}

void MainWindow::quitSlot()
{
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
    this->zoomAndMultiresWidget->show();
    zoomAndMultiresAction->setChecked(true);
}

void MainWindow::tracingTimeSlot()
{
    this->tracingTimeWidget->show();
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
void MainWindow::defaultPreferencesSlot()
{

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
    this->navigationWidget->show();
    datasetNavigationAction->setChecked(true);
}

void MainWindow::synchronizationSlot()
{
    this->synchronizationWidget->show();
    synchronizationAction->setChecked(true);
}

void MainWindow::dataSavingOptionsSlot()
{
    this->dataSavingWidget->show();
    dataSavingOptionsAction->setChecked(true);
}

void MainWindow::viewportSettingsSlot()
{
    this->viewportSettingsWidget->show();
    viewportSettingsAction->setChecked(true);
}

/* window menu functionality */

void MainWindow::toolsSlot()
{
    this->toolsWidget->show();
    toolsAction->setChecked(true);
}

void MainWindow::logSlot()
{
    this->console->show();
    logAction->setChecked(true);
}

void MainWindow::commentShortcutsSlots()
{
    this->commentsWidget->show();
    commentShortcutsAction->setChecked(true);
}

/* help menu functionality */

void MainWindow::aboutSlot()
{
    this->showAboutScreen();
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

void MainWindow::xCoordinateChanged(int value) {
    state->viewerState->currentPosition.x = value;

}

void MainWindow::yCoordinateChanged(int value) {
    state->viewerState->currentPosition.y = value;
}

void MainWindow::zCoordinateChanged(int value) {
    state->viewerState->currentPosition.z = value;
}

void MainWindow::saveSettings() {
    settings->setValue("main_window.width", this->width());
    settings->setValue("main_indow.height", this->height());
    settings->setValue("main_window.position.x", this->x());
    settings->setValue("main_window.position.y", this->y());

    settings->setValue("comments_widget.width", this->commentsWidget->width());
    settings->setValue("comments_widget.height", this->commentsWidget->height());
    settings->setValue("comment_widget.position.x", this->commentsWidget->x());
    settings->setValue("comments_widget.position.y", this->commentsWidget->y());
    settings->setValue("comments_widget.visible", this->commentsWidget->isVisible());
    settings->setValue("comments_widget.comment1", commentsWidget->textFields[0]->text());
    settings->setValue("comments_widget.comment2", this->commentsWidget->textFields[1]->text());
    settings->setValue("comments_widget.comment3", this->commentsWidget->textFields[2]->text());
    settings->setValue("comments_widget.comment4", this->commentsWidget->textFields[3]->text());
    settings->setValue("comments_widget.comment5", this->commentsWidget->textFields[4]->text());

    settings->setValue("console_widget.width", this->console->width());
    settings->setValue("console_widget.height", this->console->height());
    settings->setValue("console_widget.position.x", this->console->x());
    settings->setValue("console_widget.position.y", this->console->y());
    settings->setValue("console_widget.visible", this->console->isVisible());

    settings->setValue("data_saving_widget.width", this->dataSavingWidget->width());
    settings->setValue("data_saving_widget.height", this->dataSavingWidget->height());
    settings->setValue("data_saving_widget.position.x", this->dataSavingWidget->x());
    settings->setValue("data_saving_widget.position.y", this->dataSavingWidget->y());
    settings->setValue("data_saving_widget.visible", this->dataSavingWidget->isVisible());
    settings->setValue("data_saving_widget.auto_saving", this->dataSavingWidget->autosaveButton->isChecked());
    settings->setValue("data_saving_widget.autosave_interval", this->dataSavingWidget->autosaveIntervalSpinBox->value());
    settings->setValue("data_saving_widget.autoincrement_filename", this->dataSavingWidget->autoincrementFileNameButton->isChecked());

    settings->setValue("navigation_widget.width", this->navigationWidget->width());
    settings->setValue("navigation_widget.height", this->navigationWidget->height());
    settings->setValue("navigation_widget.position.x", this->navigationWidget->x());
    settings->setValue("navigation_widget.position.y", this->navigationWidget->y());
    settings->setValue("navigation_widget.visible", this->navigationWidget->isVisible());
    settings->setValue("navigation_widget.general.movement_speed", this->navigationWidget->movementSpeedSpinBox->value());
    settings->setValue("navigation_widget.general.jump_frames", this->navigationWidget->jumpFramesSpinBox->value());
    settings->setValue("navigation_widget.general.recenter_parallel", this->navigationWidget->recenterTimeParallelSpinBox->value());
    settings->setValue("navigation_widget.general.recenter_ortho", this->navigationWidget->recenterTimeOrthoSpinBox->value());
    settings->setValue("navigation_widget.advanced.normal_mode", this->navigationWidget->normalModeButton->isChecked());
    settings->setValue("navigation_widget.advanced.additional_viewport_direction_move", this->navigationWidget->additionalViewportDirectionMoveButton->isChecked());
    settings->setValue("navigation_widget.advanced.additional_tracing_direction_move", this->navigationWidget->additionalTracingDirectionMoveButton->isChecked());
    settings->setValue("navigation_widget.advanced.additional_mirrored_move", this->navigationWidget->additionalMirroredMoveButton->isChecked());
    settings->setValue("navigation_widget.advanced.delay_time_per_step", this->navigationWidget->delayTimePerStepSpinBox->value());
    settings->setValue("navigation_widget.advanced.number_of_steps", this->navigationWidget->numberOfStepsSpinBox->value());

    settings->setValue("synchronization_widget.width", this->synchronizationWidget->width());
    settings->setValue("synchronization_widget.height", this->synchronizationWidget->height());
    settings->setValue("synchronization_widget.position.x", this->synchronizationWidget->x());
    settings->setValue("synchronization_widget.position.y", this->synchronizationWidget->y());
    settings->setValue("synchronization_widget.visible", this->synchronizationWidget->isVisible());
    settings->setValue("synchronization_widget.remote_port", this->synchronizationWidget->remotePortSpinBox->value());
    settings->setValue("synchronization-widget.connected", this->synchronizationWidget->connected);

    settings->setValue("tools_widget.width", this->toolsWidget->width());
    settings->setValue("tools_widget.height", this->toolsWidget->height());
    settings->setValue("tools_widget.position.x", this->toolsWidget->x());
    settings->setValue("tools_widget.position.y", this->toolsWidget->y());
    settings->setValue("tools_widget.visible", this->toolsWidget->isVisible());
    //settings->setValue("tools_widget.selected_tab", this->toolsWidget->);
    //settings->setValue("tools_widget.quick.active_tree_id", this->toolsWidget->q TODO

    settings->setValue("tracing_time_widget.width", this->tracingTimeWidget->width());
    settings->setValue("tracing_time_widget.height", this->tracingTimeWidget->height());
    settings->setValue("tracing_time_widget.position.x", this->tracingTimeWidget->x());
    settings->setValue("tracing_time_widget.position.y", this->tracingTimeWidget->y());
    settings->setValue("tracing_time_widget.visible", this->tracingTimeWidget->isVisible());

    settings->setValue("viewport_settings_widget.width", this->viewportSettingsWidget->width());
    settings->setValue("viewport_settings_widget.height", this->viewportSettingsWidget->height());
    settings->setValue("viewport_settings_widget.position.x", this->viewportSettingsWidget->x());
    settings->setValue("viewport_settings_widget.position.y", this->viewportSettingsWidget->y());
    settings->setValue("viewport_settings_widget.visible", this->viewportSettingsWidget->isVisible());
    //settings->setValue("viewport_settings_widget.selected_tab", this->viewportSettingsWidget->);

    settings->setValue("zoom_and_multires_widget.width", this->zoomAndMultiresWidget->width());
    settings->setValue("zoom_and_multires_widget.height", this->zoomAndMultiresWidget->height());
    settings->setValue("zoom_and_multires_widget.position.x", this->zoomAndMultiresWidget->x());
    settings->setValue("zoom_and_multires_widget.position.y", this->zoomAndMultiresWidget->y());
    settings->setValue("zoom_and_multires_widget.visible", this->zoomAndMultiresWidget->isVisible());
    settings->setValue("zoom_and_multires_widget.orthogonal_data_viewport", this->zoomAndMultiresWidget->orthogonalDataViewportSpinBox->value());
    settings->setValue("zoom_and_multires_widget.skeleton_view_data_viewport", this->zoomAndMultiresWidget->orthogonalDataViewportSpinBox->value());
    settings->setValue("zoom_and_multires_widget.lock_dataset_to_current_mag", this->zoomAndMultiresWidget->lockDatasetCheckBox->isChecked());

}

/**
 * this method is a proposal for the qsettings variant
 */
void MainWindow::loadSettings() {
    int width = settings->value("main_window.width").toInt();
    int height = settings->value("main_window.height").toInt();
    int x = settings->value("main_window.position.x").toInt();
    int y = settings->value("main_window.position.y").toInt();
    bool visible;

    this->setGeometry(x, y, width, height);

    width = settings->value("comments_widget.width").toInt();
    height = settings->value("comments_widget.height").toInt();
    x = settings->value("comments_widget.position.x").toInt();
    y = settings->value("comments_widget.position.y").toInt();
    visible = settings->value("comments_widget.visible").toBool();

    this->commentsWidget->setGeometry(x, y, width, height);
    this->commentsWidget->setVisible(visible);

    width = settings->value("console_widget.width").toInt();
    height = settings->value("console_widget.width").toInt();
    x = settings->value("console_widget.position.x").toInt();
    y = settings->value("console_widget.position.y").toInt();
    visible = settings->value("console_widget.visible").toBool();

    this->console->setGeometry(x, y, width, height);
    this->console->setVisible(visible);

    width = settings->value("data_saving_widget.width").toInt();
    height = settings->value("data_saving_widget.height").toInt();
    x = settings->value("data_saving_widget.position.x").toInt();
    y = settings->value("data_saving_widget.position.y").toInt();
    visible = settings->value("data_saving_widget.visible").toBool();

    this->dataSavingWidget->setGeometry(x, y, width, height);
    this->dataSavingWidget->setVisible(visible);

    width = settings->value("navigation_widget").toInt();

}

/**
  * This Event Filter is used to find out which of the elements of the 2d array historyEntryAction is clicked
  * @todo load the corresponding skeleton
  */
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    for(int i = 0; i < FILE_DIALOG_HISTORY_MAX_ENTRIES; i++) {
        if(historyEntryActions[i] == obj) {
           QString fileName = historyEntryActions[i]->text();
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
