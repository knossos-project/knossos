#include <QtGui/QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QImage>
#include <QRect>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "mainwindow.h"
#include "knossos-global.h"
#include "viewer.h"
#include "loader.h"
#include "remote.h"
#include "client.h"

#include "knossos.h"

//#include "../treeLUT_fallback.h"
//#include "y.tab.h"
//#include "lex.yy.h"


struct stateInfo *tempConfig = NULL;
struct stateInfo *state = NULL;

static int32_t stripNewlines(char *string) {
    int32_t i = 0;

    for(i = 0; string[i] != '\0'; i++) {
        if(string[i] == '\n')
            string[i] = ' ';
        }

    return TRUE;

}

int32_t readConfigFile(char *path) {
    // TODO
    /*
    FILE *configFile;
        size_t bytesRead;
        char fileBuffer[16384];
        YY_BUFFER_STATE confParseBuffer;

        configFile = fopen(path, "r");

        if(configFile != NULL) {
            memset(fileBuffer, '\0', 16384);
            bytesRead = fread(fileBuffer, 1, 16383, configFile);
            if(bytesRead > 0) {
                stripNewlines(fileBuffer);
                confParseBuffer = yy_scan_string(fileBuffer);
                yyparse(state);
                yy_delete_buffer(confParseBuffer);
                fclose(configFile);
                return TRUE;
            }
        }
        */
        return FALSE;
}

static struct stateInfo *emptyState() {

    struct stateInfo *state = NULL;

    state = (stateInfo *) malloc(sizeof(struct stateInfo));
    if(state == NULL)
        return FALSE; // FALSE ?
     memset(state, '\0', sizeof(struct stateInfo));

     state->viewerState = (viewerState *) malloc(sizeof(struct viewerState));
     if(state->viewerState == NULL)
        return FALSE;
     memset(state->viewerState, '\0', sizeof(struct viewerState));

     /*
     // TODO Probleme mit agConfig
     state->viewerState->ag = (agConfig *) malloc(sizeof(struct agConfig));
     if(state->viewerState->ag == NULL)
         return FALSE;
     memset(state->viewerState->ag, '\0', sizeof(struct agConfig));
    */

     state->remoteState = (remoteState *)malloc(sizeof(struct remoteState));
     if(state->remoteState == NULL)
        return FALSE;
     memset(state->remoteState, '\0', sizeof(struct remoteState));

     state->clientState = (clientState *)malloc(sizeof(struct clientState));
     if(state->clientState == NULL)
        return FALSE;
     memset(state->clientState, '\0', sizeof(struct clientState));

     state->loaderState = (loaderState *)malloc(sizeof(struct loaderState));
     if(state->loaderState == NULL)
         return FALSE;
     memset(state->loaderState, '\0', sizeof(struct loaderState));

     state->skeletonState = (skeletonState *) malloc(sizeof(struct skeletonState));
     if(state->skeletonState == NULL)
        return FALSE;
     memset(state->skeletonState, '\0', sizeof(struct skeletonState));

        return state;


}
//static uint32_t isPathString(char *string);
//static uint32_t printUsage();
static int32_t initStates() { return 0;}
static int32_t printConfigValues() { return 0;}
static bool cleanUpMain() {


    free(tempConfig->viewerState);
    free(tempConfig->remoteState);
    free(tempConfig->clientState);
    free(tempConfig->loaderState);
    free(tempConfig);
    free(state->viewerState);
    free(state->remoteState);
    free(state->clientState);
    free(state->loaderState);
    free(state);

    return true;
}
static int32_t tempConfigDefaults() {

    uint32_t i = 0;

        if(tempConfig == NULL) {
            tempConfig = emptyState();
            if(tempConfig == NULL)
                return FALSE;
        }

        // General stuff
        tempConfig->boergens = FALSE;
        tempConfig->boundary.x = 1000;
        tempConfig->boundary.y = 1000;
        tempConfig->boundary.z = 1000;
        tempConfig->scale.x = 1.;
        tempConfig->scale.y = 1.;
        tempConfig->scale.z = 1.;
        tempConfig->offset.x = 0;
        tempConfig->offset.y = 0;
        tempConfig->offset.z = 0;
        tempConfig->cubeEdgeLength = 128;
        tempConfig->M = 3;
        tempConfig->magnification = 1;
        tempConfig->overlay = FALSE;

        // For the viewer
        tempConfig->viewerState->highlightVp = VIEWPORT_UNDEFINED;
        tempConfig->viewerState->vpKeyDirection[VIEWPORT_XY] = 1;
        tempConfig->viewerState->vpKeyDirection[VIEWPORT_XZ] = 1;
        tempConfig->viewerState->vpKeyDirection[VIEWPORT_YZ] = 1;
        tempConfig->viewerState->overlayVisible = FALSE;
        tempConfig->viewerState->datasetColortableOn = FALSE;
        tempConfig->viewerState->datasetAdjustmentOn = FALSE;
        tempConfig->viewerState->treeColortableOn = FALSE;
        tempConfig->viewerState->viewerReady = FALSE;
        tempConfig->viewerState->drawVPCrosshairs = TRUE;
        tempConfig->viewerState->showVPLabels = FALSE;
        tempConfig->viewerState->stepsPerSec = 40;
        tempConfig->viewerState->numberViewPorts = 4;
        tempConfig->viewerState->inputmap = NULL;
        tempConfig->viewerState->dropFrames = 1;
        tempConfig->viewerState->userMove = FALSE;
        tempConfig->viewerState->screenSizeX = 1024;
        tempConfig->viewerState->screenSizeY = 740;
        tempConfig->viewerState->filterType = GL_LINEAR;
        tempConfig->viewerState->currentPosition.x = 0;
        tempConfig->viewerState->currentPosition.y = 0;
        tempConfig->viewerState->currentPosition.z = 0;
        tempConfig->viewerState->voxelDimX = tempConfig->scale.x;
        tempConfig->viewerState->voxelDimY = tempConfig->scale.y;
        tempConfig->viewerState->voxelDimZ = tempConfig->scale.z;
        tempConfig->viewerState->voxelXYRatio = tempConfig->viewerState->voxelDimX / tempConfig->viewerState->voxelDimY;
        tempConfig->viewerState->voxelXYtoZRatio = tempConfig->viewerState->voxelDimX / tempConfig->viewerState->voxelDimZ;
        tempConfig->viewerState->depthCutOff = 5.;
        tempConfig->viewerState->luminanceBias = 0;
        tempConfig->viewerState->luminanceRangeDelta = 255;
        tempConfig->viewerState->autoTracingEnabled = FALSE;
        tempConfig->viewerState->autoTracingDelay = 50;
        tempConfig->viewerState->autoTracingSteps = 10;
        tempConfig->viewerState->recenteringTimeOrth = 500;
        tempConfig->viewerState->walkOrth = FALSE;

        tempConfig->viewerState->viewPorts = (viewPort *) malloc(tempConfig->viewerState->numberViewPorts * sizeof(struct viewPort));
        if(tempConfig->viewerState->viewPorts == NULL) {
            LOG("Out of memory.");
            return FALSE;
        }

        memset(tempConfig->viewerState->viewPorts, '\0', tempConfig->viewerState->numberViewPorts * sizeof(struct viewPort));

        for(i = 0; i < tempConfig->viewerState->numberViewPorts; i++) {
            switch(i) {
            case VIEWPORT_XY:
                SET_COORDINATE(tempConfig->viewerState->viewPorts[i].upperLeftCorner, 5, 30, 0);
                tempConfig->viewerState->viewPorts[i].type = VIEWPORT_XY;
                break;
            case VIEWPORT_XZ:
                SET_COORDINATE(tempConfig->viewerState->viewPorts[i].upperLeftCorner, 5, 385, 0);
                tempConfig->viewerState->viewPorts[i].type = VIEWPORT_XZ;
                break;
            case VIEWPORT_YZ:
                SET_COORDINATE(tempConfig->viewerState->viewPorts[i].upperLeftCorner, 360, 30, 0);
                tempConfig->viewerState->viewPorts[i].type = VIEWPORT_YZ;
                break;
            case VIEWPORT_SKELETON:
                SET_COORDINATE(tempConfig->viewerState->viewPorts[i].upperLeftCorner, 360, 385, 0);
                tempConfig->viewerState->viewPorts[i].type = VIEWPORT_SKELETON;
                break;
            }

            tempConfig->viewerState->viewPorts[i].draggedNode = NULL;
            tempConfig->viewerState->viewPorts[i].userMouseSlideX = 0.;
            tempConfig->viewerState->viewPorts[i].userMouseSlideY = 0.;
            tempConfig->viewerState->viewPorts[i].edgeLength = 350;
            tempConfig->viewerState->viewPorts[i].texture.texUnitsPerDataPx = 1. / TEXTURE_EDGE_LEN;
            tempConfig->viewerState->viewPorts[i].texture.edgeLengthPx = TEXTURE_EDGE_LEN;
            tempConfig->viewerState->viewPorts[i].texture.edgeLengthDc = TEXTURE_EDGE_LEN / tempConfig->cubeEdgeLength;

            //This variable indicates the current zoom value for a viewport.
            //Zooming is continous, 1: max zoom out, 0.1: max zoom in (adjust values..)
            tempConfig->viewerState->viewPorts[i].texture.zoomLevel = VPZOOMMIN;
        }

        // For the GUI
        // TODO instruction leads to crash since agConfig is not used, will be replaced
        // snprintf(tempConfig->viewerState->ag->settingsFile, 2048, "defaultSettings.xml");

        // For the client
        tempConfig->clientState->connectAsap = FALSE;
        tempConfig->clientState->connectionTimeout = 3000;
        tempConfig->clientState->remotePort = 7890;
        strncpy(tempConfig->clientState->serverAddress, "localhost", 1024);
        tempConfig->clientState->connected = FALSE;
        tempConfig->clientState->synchronizeSkeleton = TRUE;
        tempConfig->clientState->synchronizePosition = TRUE;
        tempConfig->clientState->saveMaster = FALSE;

        // For the remote
        tempConfig->remoteState->activeTrajectory = 0;
        tempConfig->remoteState->maxTrajectories = 16;
        tempConfig->remoteState->type = FALSE;

        // For the skeletonizer
        tempConfig->skeletonState->lockRadius = 100;
        tempConfig->skeletonState->lockPositions = FALSE;

        strncpy(tempConfig->skeletonState->onCommentLock, "seed", 1024);
        tempConfig->skeletonState->branchpointUnresolved = FALSE;
        tempConfig->skeletonState->autoFilenameIncrementBool = TRUE;
        tempConfig->skeletonState->autoSaveBool = TRUE;
        tempConfig->skeletonState->autoSaveInterval = 5;
        tempConfig->skeletonState->skeletonTime = 0;
        //tempConfig->skeletonState->skeletonTimeCorrection = SDL_GetTicks(); TODO SDL_GetTicks
        tempConfig->skeletonState->definedSkeletonVpView = 0;

        //This number is currently arbitrary, but high values ensure a good performance
        tempConfig->skeletonState->skeletonDCnumber = 8000;
        tempConfig->skeletonState->workMode = ON_CLICK_DRAG;

        return TRUE;

}

static int32_t readDataConfAndLocalConf() { return 0;}

static int32_t configFromCli(int argCount, char *arguments[]) {

#define NUM_PARAMS 15

 char *lval = NULL, *rval = NULL;
 char *equals = NULL;
 int32_t i = 0, j = 0, llen = 0;
 char *lvals[NUM_PARAMS] = {
                             "--data-path",       // 0  Do not forget
                             "--connect-asap",    // 1  to also modify
                             "--scale-x",         // 2  NUM_PARAMS
                             "--scale-y",         // 3  above when adding
                             "--scale-z",         // 4  switches!
                             "--boundary-x",      // 5
                             "--boundary-y",      // 6
                             "--boundary-z",      // 7
                             "--experiment-name", // 8
                             "--supercube-edge",  // 9
                             "--color-table",     // 10
                             "--config-file",     // 11
                             "--magnification",   // 12
                             "--overlay",         // 13
                             "--profile"          // 14
                           };

 for(i = 1; i < argCount; i++) {
     /*
      * Everything right of the equals sign is the rvalue, everythin left of
      * it the lvalue, or, if there is no equals sign, everything is lvalue.
      */

     llen = strlen(arguments[i]);
     equals = strchr(arguments[i], '=');

     if(equals) {
         if(equals[1] != '\0') {
             llen = strlen(arguments[i]) - strlen(equals);
             equals++;
             rval = (char *)malloc((strlen(equals) + 1) * sizeof(char));
             if(rval == NULL) {
                 LOG("Out of memory.");
                 _Exit(FALSE);
             }
             memset(rval, '\0', strlen(equals) + 1);

             strncpy(rval, equals, strlen(equals));
         }
         else
             llen = strlen(arguments[i]) - 1;
     }
     lval = (char *) malloc((llen + 1) * sizeof(char));
     if(lval == NULL) {
         LOG("Out of memory.");
         _Exit(FALSE);
     }
     memset(lval, '\0', llen + 1);

     strncpy(lval, arguments[i], llen);

     for(j = 0; j < NUM_PARAMS; j++) {
         if(strcmp(lval, lvals[j]) == 0) {
             switch(j) {
                 case 0:
                     strncpy(tempConfig->path, rval, 1023);
                     break;
                 case 1:
                     tempConfig->clientState->connectAsap = TRUE;
                     break;
                 case 2:
                     tempConfig->scale.x = (float)strtod(rval, NULL);
                     break;
                 case 3:
                     tempConfig->scale.y = (float)strtod(rval, NULL);
                     break;
                 case 4:
                     tempConfig->scale.z = (float)strtod(rval, NULL);
                     break;
                 case 5:
                     tempConfig->boundary.x = (int32_t)atoi(rval);
                     break;
                 case 6:
                     tempConfig->boundary.y = (int32_t)atoi(rval);
                     break;
                 case 7:
                     tempConfig->boundary.z = (int32_t)atoi(rval);
                     break;
                 case 8:
                     strncpy(tempConfig->name, rval, 1023);
                     break;
                 case 9:
                     tempConfig->M = (int32_t)atoi(rval);
                     break;
                 case 10:
                     Viewer::loadDatasetColorTable(rval, &(state->viewerState->datasetColortable[0][0]), GL_RGB);
                     break;
                 case 11:
                     readConfigFile(rval);
                     break;
                 case 12:
                     tempConfig->magnification = (int32_t)atoi(rval);
                     break;
                 case 13:
                     tempConfig->overlay = TRUE;
                     tempConfig->viewerState->overlayVisible = TRUE;
                     break;
                 case 14:
                     strncpy(tempConfig->viewerState->ag->settingsFile, rval, 2000);
                     strcpy(tempConfig->viewerState->ag->settingsFile + strlen(rval), ".xml");
                     break;
             }
         }
     }
 }

 return TRUE;

}
static int32_t loadNeutralDatasetLUT(GLuint *lut) { return 0;}


static int32_t findAndRegisterAvailableDatasets() { return 0;}
#ifdef LINUX
static int32_t catchSegfault(int signum) { return 0;}
#endif

//IMPORTANT. SDL redefines main
#ifdef main
#undef main
#endif



int main(int argc, char *argv[])
{
    qDebug("foo");
    QApplication a(argc, argv);


    // displaying the splash
    MainWindow window;
    window.showMaximized();

    QSplashScreen splashScreen(QPixmap("../splash"), Qt::WindowStaysOnTopHint);
    splashScreen.show();

    // legacy init code
    // TODO SDL_INIT Alternative für QT/C++
    Loader *loadingThread;
    Viewer *viewingThread;
    Remote *remoteThread;
    Client *clientThread;


    state = emptyState();

    state->loadSignal = FALSE;
    state->remoteSignal = FALSE;
    state->quitSignal = FALSE;
    state->clientSignal = FALSE;
    state->conditionLoadSignal = new QWaitCondition();
    state->conditionRemoteSignal = new QWaitCondition();
    state->conditionClientSignal = new QWaitCondition();
    state->protectSkeleton = new QMutex();
    state->protectLoadSignal = new QMutex();
    state->protectRemoteSignal = new QMutex();
    state->protectClientSignal = new QMutex();
    state->protectCube2Pointer = new QMutex();
    state->protectPeerList = new QMutex();
    state->protectOutBuffer = new QMutex();



    if(tempConfigDefaults() != TRUE) {
        LOG("Error loading default parameters.");
        _Exit(FALSE);
    }
        // TODO

        if(argc >= 2)
            configFromCli(argc, argv);

        if(tempConfig->path[0] != '\0') {
            // Got a path from cli.
            readDataConfAndLocalConf();
            // We need to read the specified config file again because it should
            // override all parameters from other config files.
            configFromCli(argc, argv);
        }
        else
            readConfigFile("knossos.conf");


        state->viewerState->voxelDimX = tempConfig->scale.x;
        state->viewerState->voxelDimY = tempConfig->scale.y;
        state->viewerState->voxelDimZ = tempConfig->scale.z;

        if(argc >= 2) {
            if(configFromCli(argc, argv) == FALSE) {
                LOG("Error reading configuration from command line.");
            }
        }

        // TODO
        //if(initStates(state) != TRUE) {
        //    LOG("Error during initialization of the state struct.");
        //    _Exit(FALSE);
        //}


        printConfigValues();

        viewingThread = new Viewer(); // viewer() is called in constructor
        loadingThread = new Loader(); // TODO call loader() in constructor
        remoteThread = new Remote(); // TODO call remote() in constructor
        clientThread = new Client(); // TODO call client() in constructor

        loadingThread->wait();
        remoteThread->wait();
        viewingThread->wait();
        clientThread->wait();


        //SDL_Quit(); // SQL_QUIT

        cleanUpMain();


    return a.exec();
}

/* http://aggregate.org/MAGIC/#Log2%20of%20an%20Integer */
uint32_t Knossos::ones32(register uint32_t x) {
        /* 32-bit recursive reduction using SWAR...
       but first step is mapping 2-bit values
       into sum of 2 1-bit values in sneaky way
    */
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return(x & 0x0000003f);
}


/* copied from http://aggregate.org/MAGIC/#Log2%20of%20an%20Integer;  */
uint32_t Knossos::log2uint32(register uint32_t x) {

    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);

    return(ones32(x >> 1));
}

bool Knossos::lockSkeleton(int32_t targetRevision) { return true;}

bool Knossos::unlockSkeleton(int32_t increment) { return true;}

bool Knossos::sendClientSignal() { return true;}

bool Knossos::sendRemoteSignal() { return true;}

void Knossos::sendDatasetChangeSignal(uint32_t upOrDownFlag) {}

bool Knossos::sendLoadSignal(uint32_t x, uint32_t y, uint32_t z) { return true;}

bool Knossos::sendQuitSignal() { return true;}
