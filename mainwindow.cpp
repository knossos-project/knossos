//the functions formerly placed in gui.h/gui.c are now in mainwindow
//yesNoPrompt should be replaced, saveSkeletonCallback is replaced by QAction saveAction
//AG_Events are replaced by QEvent
#include "mainwindow.h"
#include "ui_mainwindow.h"


extern struct stateInfo *state;
extern struct stateInfo *tempConfig;

//-- static functions --//

static void setGUIcolors(){}
// general callbacks
static void OkfileDlgOpenSkel(QEvent *event){}
static void OkfileDlgSaveAsSkel(QEvent *event){}
static void OkfileDlgLoadDatasetLUT(QEvent *event){}
static void OkfileDlgLoadTreeLUT(QEvent *event){}
static void OkfileDlgOpenPrefs(QEvent *event){}
static void OkfileDlgSavePrefsAs(QEvent *event){}

static void UI_checkQuitKnossos(){}
static void resizeCallback(uint32_t newWinLenX, uint32_t newWinLenY){}
static void resizeWindows(){}
static void agInputWdgtGainedFocus(QEvent *event){}
static void agInputWdgtLostFocus(QEvent *event){}

static void currPosWdgtModified(QEvent *event){}
static void actNodeIDWdgtModified(QEvent *event){}
static void actTreeIDWdgtModified(QEvent *event){}
static void actTreeColorWdgtModified(QEvent *event){}
static void actNodeCommentWdgtModified(QEvent *event){}
static void actTreeCommentWdgtModified(QEvent *event){}

// menu callbacks
static void fileOpenSkelFile(QEvent *event){}
static void fileSaveAsSkelFile(QEvent *event){}

//static void viewDataSetStats(QEvent *event){}
static void viewZooming(QEvent *event){}
static void viewTracingTime(){}
static void viewComments(){}
//static void viewLoadImgJTable(QEvent *event){}

static void prefNavOptions(QEvent *event){}
static void prefLoadCustomPrefs(QEvent *event){}
static void prefSaveCustomPrefsAs(QEvent *event){}
static void prefSyncOptions(QEvent *event){}
//static void prefRenderingQualityOptions(QEvent *event){}
//static void prefVolTracingOptions(QEvent *event){}
//static void prefSpatialLockingOptions(QEvent *event){}
//static void prefAGoptions(QEvent *event){}
static void prefViewportPrefs(){}
static void prefSaveOptions(){}

//static void winShowNavigator(QEvent *event){}
static void winShowTools(QEvent *event){}
static void winShowConsole(QEvent *event){}

// functions generating gui elements

static void createToolsWin() {}
static void createViewportPrefWin() {}



void MainWindow:: createCoordBarWin() {
    copyButton = new QPushButton("Copy");
    pasteButton = new QPushButton("Paste");

    this->toolBar = new QToolBar();
    this->addToolBar(toolBar);
    this->toolBar->addWidget(copyButton);
    this->toolBar->addWidget(pasteButton);

    xField = new QSpinBox();
    yField = new QSpinBox();
    zField = new QSpinBox();

    xLabel = new QLabel("x");
    yLabel = new QLabel("y");
    zLabel = new QLabel("z");

    this->toolBar->addWidget(xLabel);
    this->toolBar->addWidget(xField);
    this->toolBar->addWidget(yLabel);
    this->toolBar->addWidget(yField);
    this->toolBar->addWidget(zLabel);
    this->toolBar->addWidget(zField);
}


static void createSkeletonVpToolsWin() {

}

static void createDataSizeWin() {}
static void createNavWin() {}

void MainWindow::createConsoleWidget() {

    console = new Console(this);
    console->setGeometry(800, 500, 200, 120);
    console->show();

}

void MainWindow::createTracingTimeWidget() {
    tracingTimeWidget = new TracingTimeWidget(this);
    tracingTimeWidget->setGeometry(800, 350, 200, 100);
    tracingTimeWidget->show();
}

void MainWindow::createCommentsWidget() {
    commentsWidget = new CommentsWidget(this);
    commentsWidget->setGeometry(800, 100, 200, 150);
    commentsWidget->show();
}

static void createNavOptionsWin() {}
static void createDisplayOptionsWin() {}
static void createSaveOptionsWin() {}
static void createSyncOptionsWin() {}
static void createRenderingOptionsWin() {}
static void createVolTracingOptionsWin() {}
static void createSpatialLockingOptionsWin() {}

static void createDataSetStatsWin() {}
static void createZoomingWin() {}


static void createLoadDatasetImgJTableWin() {}
static void createLoadTreeImgJTableWin() {}
static void createSetDynRangeWin() {}

static void createOpenFileDlgWin(){}
static void createSaveAsFileDlgWin(){}

static void createCurrPosWdgt(AG_Window *parent){}
static void createSkeletonVpToolsWdgt(AG_Window *parent){}
static void createActNodeWdgt(AG_Widget *parent){}
static void datasetColorAdjustmentsChanged(){}

static void createOpenCustomPrefsDlgWin(){}
static void createSaveCustomPrefsAsDlgWin(){}


//  Wrapper functions around KNOSSOS internals for use by the UI
// (GUI / Keyboard / (Mouse))


static void WRAP_loadSkeleton(){}
static void WRAP_saveSkeleton(){}
static void updateTitlebar(int32_t useFilename){}
static void UI_clearSkeleton(){}
static void WRAP_clearSkeleton(){}
static void UI_setViewModeDrag() {}
static void UI_setViewModeRecenter(){}
static void UI_unimplemented(){}
static void UI_lockActiveNodeBtnPressed(){}
static void UI_disableLockingBtnPressed(){}
static void UI_commentLockWdgtModified(QEvent *event){}
static void UI_setDefaultZoom(){}
static void UI_SyncConnect(){}
static void UI_SyncDisconnect(){}
static void UI_findNextBtnPressed(){}
static void UI_findPrevBtnPressed(){}
static void UI_deleteNodeBtnPressed(){}
static void UI_jumpToNodeBtnPressed(){}
static void UI_linkActiveNodeWithBtnPressed(){}
static void UI_actNodeRadiusWdgtModified(){}
static void UI_deleteTreeBtnPressed(){}
static void UI_newTreeBtnPressed(){}
static void UI_splitTreeBtnPressed(){}
static void UI_mergeTreesBtnPressed(){}
static void UI_helpDeleteTree(){}
static void UI_renderModelRadioModified(){}
static void UI_displayModeRadioModified(){}
static void UI_setHighlightActiveTree(){}
static void UI_setShowVPLabels(){}
static void UI_setShowNodeIDs(){}
static void UI_skeletonChanged(){}
static void UI_enableSliceVPOverlayModified(){}
static void UI_pushBranchBtnPressed(){}
static void UI_popBranchBtnPressed(){}
static void UI_enableLinearFilteringModified(){}
static void UI_helpShowAbout(){}

static void UI_loadSettings(){

}

static void UI_setSkeletonPerspective(QEvent *event){}
static void UI_orthoVPzoomSliderModified(){}
static void UI_lockCurrentMagModified(QEvent *event){}
static void UI_deleteCommentBoxesBtnPressed(){}
static void UI_deleteCommentBoxes(){}
static void UI_changeViewportPosSiz(){}
static void UI_changeViewportPosSizCheckbox(){}

/**
  * This function is a replacement for the updateAgConfig() function in KNOSSOS 3.2
  * @todo Replacements for AG_Numerical
  */
static void updateGuiconfig() {
    state->viewerState->gui->totalTrees = state->skeletonState->treeElements;
    state->viewerState->gui->totalNodes = state->skeletonState->totalNodeElements;
    if(state->skeletonState->totalNodeElements == 0) {
        //AG_NumericalSetWriteable(state->viewerState->gui->actNodeIDWdgt1, FALSE);
        //AG_NumericalSetWriteable(state->viewerState->gui->actNodeIDWdgt2, FALSE);
        state->viewerState->gui->activeNodeID = 0;
        state->viewerState->gui->activeNodeCoord.x = 0;
        state->viewerState->gui->activeNodeCoord.y = 0;
        state->viewerState->gui->activeNodeCoord.z = 0;
    }
    else {
        //AG_NumericalSetWriteable(state->viewerState->gui->actNodeIDWdgt1, TRUE);
        //AG_NumericalSetWriteable(state->viewerState->gui->actNodeIDWdgt2, TRUE);
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

static Coordinate *parseRawCoordinateString(char *string){
    Coordinate *extractedCoords = NULL;
    char coordStr[strlen(string)];
    strcpy(coordStr, string);
    char delims[] = "[]()./,; -";
    char* result = NULL;
    char* coords[3];
    int i = 0;

    if(!(extractedCoords = (Coordinate *)malloc(sizeof(Coordinate)))) {
        LOG("Out of memory");
        _Exit(FALSE);
    }

    result = strtok(coordStr, delims);
    while(result != NULL && i < 4) {
        coords[i] = (char *)malloc(strlen(result)+1);
        strcpy(coords[i], result);
        result = strtok(NULL, delims);
        ++i;
    }

    if(i < 2) {
        LOG("Paste string doesn't contain enough delimiter-separated elements");
        goto fail;
    }

    if((extractedCoords->x = atoi(coords[0])) < 1) {
        LOG("Error converting paste string to coordinate");
        goto fail;
    }
    if((extractedCoords->y = atoi(coords[1])) < 1) {
        LOG("Error converting paste string to coordinate");
        goto fail;
    }
    if((extractedCoords->z = atoi(coords[2])) < 1) {
        LOG("Error converting paste string to coordinate");
        goto fail;
    }

    return extractedCoords;

fail:
    free(extractedCoords);
    return NULL;

}

static void prefDefaultPrefsWindow(){}
static void prefDefaultPrefs(){}
static void resetViewportPosSiz(){}


// -- Constructor and destroyer -- //
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("KnossosQT");

    createActions();
    createMenus();
    createCoordBarWin();
    createConsoleWidget();
    createTracingTimeWidget();
    createCommentsWidget();

    mainWidget = new QWidget(this);
    gridLayout = new QGridLayout();
    mainWidget->setLayout(gridLayout);
    setCentralWidget(mainWidget);

    viewports = new Viewport*[4];
    for(int i = 0; i < 4; i++) {
        viewports[i] = new Viewport(this, i);
        viewports[i]->setStyleSheet("background:black");
    }

    gridLayout->addWidget(viewports[0], 0, 1);
    gridLayout->addWidget(viewports[1], 0, 2);
    gridLayout->addWidget(viewports[2], 1, 1);
    gridLayout->addWidget(viewports[3], 1, 2);

    this->loadFileDialog = new QFileDialog(this);
    loadFileDialog->setWindowTitle(tr("Open Skeleton File"));
    loadFileDialog->setDirectory(QDir::home());
    loadFileDialog->setNameFilter(tr("KNOSSOS Skeleton File(*.nml)"));

    this->saveFileDialog = new QFileDialog(this);
    saveFileDialog->setWindowTitle(tr("Save Skeleton File"));
    saveFileDialog->setDirectory(QDir::home());
    saveFileDialog->setFileMode(QFileDialog::AnyFile);
    saveFileDialog->setNameFilter(tr("Knossos Skeleton File(*.nml)"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showSplashScreen() {

    QSplashScreen splashScreen(QPixmap("../splash"), Qt::WindowStaysOnTopHint);
    splashScreen.show();

}

// -- static methods -- //

bool MainWindow::initGUI() {
    /* set the window caption */
    updateTitlebar(FALSE);

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

    //createMenuBar();
    //createCoordBarWin();
    createSkeletonVpToolsWin();
    createDataSizeWin();
    createNavWin();
    createToolsWin();
    //createConsoleWin();

    createNavOptionsWin();
    createSyncOptionsWin();
    createSaveOptionsWin();

    //createAboutWin();
    /* all following 4 unused
    createDisplayOptionsWin();
    createRenderingOptionsWin();
    createSpatialLockingOptionsWin();
    createVolTracingOptionsWin();   */

    createDataSetStatsWin();
    createViewportPrefWin();
    createZoomingWin();
    //createTracingTimeWin();
    //createCommentsWin();
    /*createSetDynRangeWin(); */           /* Unused. */

    //createVpXzWin();
    //createVpXyWin();
    //createVpYzWin();
    //createVpSkelWin();

    UI_loadSettings();

    return true;
}

void MainWindow::quitKnossos(){} //not needed in qt


bool MainWindow::cpBaseDirectory(char *target, char *path, size_t len){

    char *hit;
        int32_t baseLen;

    #ifdef LINUX
        hit = strrchr(path, '/');
    #else
        hit = strrchr(path, '\\');
    #endif

        if(hit == NULL) {
            LOG("Cannot find a path separator char in %s\n", path);
            return FALSE;
        }

        baseLen = (int32_t)(hit - path);
        if(baseLen > 2047) {
            LOG("Path too long\n");
            return FALSE;
        }

        strncpy(target, path, baseLen);
        target[baseLen] = '\0';

        return true;

}

void MainWindow::yesNoPrompt(QWidget *par, char *promptString, void (*yesCb)(), void (*noCb)()){}
uint32_t MainWindow::addRecentFile(char *path, uint32_t pos){return FALSE;}

void MainWindow::UI_workModeAdd(){
   tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
}
void MainWindow::UI_workModeLink(){
   tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE;
}
void MainWindow::UI_workModeDrop(){
    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_DROP_NODE;
}

//void MainWindow::saveSkelCallback(QEvent *event){}
void MainWindow::UI_saveSkeleton(int32_t increment){

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
        yesNoPrompt(NULL, "Overwrite existing skeleton file?", WRAP_saveSkeleton, NULL);
        fclose(saveFile);
        return;
    }

    WRAP_saveSkeleton();

}

void MainWindow::UI_saveSettings(){

}

/**
  * @todo Replacement for AG_String(X)
  * No QEvents
  */
void MainWindow::UI_loadSkeleton(QEvent *event){
    char *path; // = AG_STRING(1);
    char *msg; // = AG_STRING(2);


    strncpy(state->skeletonState->prevSkeletonFile, state->skeletonState->skeletonFile, 8192);
    strncpy(state->skeletonState->skeletonFile, path, 8192);

    if(state->skeletonState->totalNodeElements != 0) {
        yesNoPrompt(NULL, msg, WRAP_loadSkeleton, NULL);
    }
    else {
        WRAP_loadSkeleton();
    }
}

void MainWindow::copyClipboardCoordinates() {
   char copyString[8192];

   memset(copyString, '\0', 8192);

   snprintf(copyString,
                 8192,
                 "%d, %d, %d",
                 state->viewerState->currentPosition.x + 1,
                 state->viewerState->currentPosition.y + 1,
                 state->viewerState->currentPosition.z + 1);
   QString coords(copyString);
   QApplication::clipboard()->setText(coords);
}

void MainWindow::pasteClipboardCoordinates(){
    QString text = QApplication::clipboard()->text();
    if(text.size() > 0) {
      char *pasteBuffer = const_cast<char *> (text.toStdString().c_str());
      Coordinate *extractedCoords = NULL;

      if((extractedCoords = parseRawCoordinateString(pasteBuffer))) {
            tempConfig->viewerState->currentPosition.x = extractedCoords->x - 1;
            tempConfig->viewerState->currentPosition.y = extractedCoords->y - 1;
            tempConfig->viewerState->currentPosition.z = extractedCoords->z - 1;

            Viewer::updatePosition(TELL_COORDINATE_CHANGE);
            Viewer::refreshViewports();

            free(extractedCoords);
            QApplication::clipboard()->clear();
      }

    } else {
       LOG("Unable to fetch text from clipboard");
    }
}
void MainWindow::UI_zoomOrthogonals(float step){
    int32_t i = 0;
        int32_t triggerMagChange = FALSE;

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

        if(triggerMagChange) Viewer::changeDatasetMag(triggerMagChange);

        Viewer::recalcTextureOffsets();
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

void MainWindow::createXYViewport() {

}

void MainWindow::createXZViewport() {

}

void MainWindow::createYZViewport() {

}

void MainWindow::createSkeletonViewport() {

}

//-- private methods --//

void MainWindow::createActions()
{
    /* file actions */
    openAction = new QAction(tr("&Open"), this);
    recentFileAction = new QAction(tr("&Recent Files"), this);
    saveAction = new QAction(tr("&Save (CTRL+s)"), this);
    saveAsAction = new QAction(tr("&Save as"), this);
    quitAction = new QAction(tr("&Quit"), this);


    connect(openAction, SIGNAL(triggered()), this, SLOT(openSlot()));
    connect(recentFileAction, SIGNAL(triggered()), this, SLOT(recentFilesSlot()));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(saveSlot()));
    connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAsSlot()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(quitSlot()));

    /* edit skeleton actions */
    addNodeAction = new QAction(tr("&Add Node"), this);
    linkWithActiveNodeAction = new QAction(tr("&Link with Active Node"), this);
    dropNodesAction = new QAction(tr("&Drop Nodes"), this);
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
    recenterOnClickAction = new QAction(tr("&Recenter on Click"), this);
    zoomAndMultiresAction = new QAction(tr("Zoom and Multires.."), this);
    tracingTimeAction = new QAction(tr("&Tracing Time"), this);

    connect(workModeViewAction, SIGNAL(triggered()), this, SLOT(workModeViewSlot()));
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
    dataSavingOptionsAction = new QAction(tr("&Data Saving Options"), this);
    viewportSettingsAction = new QAction(tr("&Viewport Settings"), this);

    connect(loadCustomPreferencesAction, SIGNAL(triggered()), this, SLOT(loadCustomPreferencesSlot()));
    connect(saveCustomPreferencesAction, SIGNAL(triggered()), this, SLOT(saveCustomPreferencesSlot()));
    connect(defaultPreferencesAction, SIGNAL(triggered()), this, SLOT(defaultPreferencesSlot()));
    connect(datasetNavigationAction, SIGNAL(triggered()), this, SLOT(datatasetNavigationSlot()));
    connect(dataSavingOptionsAction, SIGNAL(triggered()), this, SLOT(dataSavingOptionsSlot()));
    connect(viewportSettingsAction, SIGNAL(triggered()), this, SLOT(viewportSettingsSlot()));

    /* window actions */
    toolsAction = new QAction(tr("&Tools"), this);
    logAction = new QAction(tr("&Log"), this);
    commentShortcutsAction = new QAction(tr("&Comment Shortcuts"), this);

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
    fileMenu->addAction(openAction);
    fileMenu->addAction(recentFileAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    editMenu = menuBar()->addMenu("&Edit Skeleton");
    workModeMenu = editMenu->addMenu("&Work Mode");
        workModeMenu->addAction(addNodeAction);
        workModeMenu->addAction(linkWithActiveNodeAction);
        workModeMenu->addAction(dropNodesAction);
    editMenu->addAction(skeletonStatisticsAction);
    editMenu->addAction(clearSkeletonAction);

    viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(workModeViewAction);
    viewMenu->addAction(dragDatasetAction);
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
    // TODO temporary condition
    if(5 > 0) {
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

void MainWindow::openSlot()
{
    QStringList fileList;
    if(loadFileDialog->exec()) {

        fileList = loadFileDialog->selectedFiles();

    }

    if(fileList.size() > 0) {
        QString fileName = fileList.at(0);
        loadedFile = new QFile(fileName);
    }

}

void MainWindow::recentFilesSlot()
{
    QStringList fileList = loadFileDialog->history();

}

void MainWindow::saveSlot()
{



}

void MainWindow::saveAsSlot()
{
    QStringList fileList;
    if(saveFileDialog->exec()) {
        fileList = saveFileDialog->selectedFiles();
    }
    qDebug() << fileList.at(0);
}

void MainWindow::quitSlot()
{
   QApplication::closeAllWindows();
}

/* edit skeleton functionality */

void MainWindow::workModeEditSlot()
{

}

void MainWindow::addNodeSlot()
{

}

void MainWindow::linkWithActiveNodeSlot()
{

}

void MainWindow::dropNodesSlot()
{

}


void MainWindow::skeletonStatisticsSlot()
{

}

void MainWindow::clearSkeletonSlot()
{

}

/* view menu functionality */

void MainWindow::workModeViewSlot()
{

}

void MainWindow::dragDatasetSlot()
{

}

void MainWindow::recenterOnClickSlot()
{

}

void MainWindow::zoomAndMultiresSlot()
{

}

void MainWindow::tracingTimeSlot()
{

}

/* preference menu functionality */

void MainWindow::loadCustomPreferencesSlot()
{

}

void MainWindow::saveCustomPreferencesSlot()
{

}

void MainWindow::defaultPreferencesSlot()
{

}

void MainWindow::datatasetNavigationSlot()
{

}

void MainWindow::synchronizationSlot()
{

}

void MainWindow::dataSavingOptionsSlot()
{

}

void MainWindow::viewportSettingsSlot()
{

}

/* window menu functionality */

void MainWindow::toolsSlot()
{

}

void MainWindow::logSlot()
{

}

void MainWindow::commentShortcutsSlots()
{

}

/* help menu functionality */

void MainWindow::aboutSlot()
{
    showSplashScreen();
}

