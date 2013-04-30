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

#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QImage>
#include <QRect>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>

#include "mainwindow.h"
#include "knossos-global.h"
#include "sleeper.h"
#include "viewer.h"
#include "loader.h"
#include "remote.h"
#include "client.h"
#include "knossos.h"

//#include "y.tab.h"
//#include "lex.yy.h"


#define NUMTHREADS 4

struct stateInfo *tempConfig = NULL;
struct stateInfo *state = NULL;
//Knossos *knossos;
Viewer *viewer;

//static uint32_t isPathString(char *string);
//static uint32_t printUsage();


int main(int argc, char *argv[])
{ 
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.");
    QCoreApplication::setApplicationName("Knossos QT");


    // The idea behind all this is that we have four sources of
    // configuration data:
    //
    //  * Arguments passed by KUKU
    //  * Local knossos.conf
    //  * knossos.conf that comes with the data
    //  * Default parameters
    //
    // All this config data should be used. Command line overrides
    // local knossos.conf and local knossos.conf overrides knossos.conf
    // from data and knossos.conf from data overrides defaults.

    state = Knossos::emptyState();
    state->loadSignal = false;
    state->remoteSignal = false;
    state->quitSignal = false;
    state->clientSignal = false;
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

    if(Knossos::tempConfigDefaults() != true) {
        LOG("Error loading default parameters.");
        _Exit(false);
    }    

    if(argc >= 2) {
        Knossos::configFromCli(argc, argv);
    }

    if(tempConfig->path[0] != '\0') {
        // Got a path from cli.
        Knossos::readDataConfAndLocalConf();
        // We need to read the specified config file again because it should
        // override all parameters from other config files.
        Knossos::configFromCli(argc, argv);
    }
    else {
        Knossos::readConfigFile("knossos.conf");
    }
    state->viewerState->voxelDimX = tempConfig->scale.x;
    state->viewerState->voxelDimY = tempConfig->scale.y;
    state->viewerState->voxelDimZ = tempConfig->scale.z;

    if(argc >= 2) {
        if(Knossos::configFromCli(argc, argv) == false) {
            LOG("Error reading configuration from command line.");
        }
    }

    //2012.12.11 HARDCODED FOR TESTING LOADER
    strncpy(tempConfig->path, "../../../../../e1088_mag1/", 1024);
    strncpy(tempConfig->name, "070317_e1088", 1024);
    tempConfig->boundary.x = 2048;
    tempConfig->boundary.y = 1792;
    tempConfig->boundary.z = 2048;
    tempConfig->scale.x = 22.0;
    tempConfig->scale.y = 22.0;
    tempConfig->scale.z = 33.0;
    tempConfig->cubeEdgeLength = 128;
    tempConfig->cubeBytes = tempConfig->cubeEdgeLength * tempConfig->cubeEdgeLength * tempConfig->cubeEdgeLength;

    tempConfig->cubeSliceArea = tempConfig->cubeEdgeLength * tempConfig->cubeEdgeLength;
    tempConfig->M = 3;
    tempConfig->cubeSetElements = tempConfig->M * tempConfig->M  * tempConfig->M;
    tempConfig->cubeSetBytes = tempConfig->cubeSetElements* tempConfig->cubeBytes;
    tempConfig->boergens = 0;

    if(Knossos::initStates() != true) {
       LOG("Error during initialization of the state struct.");
        _Exit(false);
    }

    Knossos::printConfigValues();
    /*
    MainWindow window;
    window.showMaximized();
    */
    // built up threads. Do not follow instructions of qt documentation on QThread
    // as they are outdated since qt 4.4!
    // Instead of subclassing a QThread, normal QObjects are to be moved onto threads


    //knossos = new Knossos();
    viewer = new Viewer();
    Loader *loader = new Loader();
    Remote *remote = new Remote();
    Client *client = new Client();


    //QObject::connect(knossos, SIGNAL(calcDisplayedEdgeLengthSignal()), viewer, SLOT(calcDisplayedEdgeLength()));
    QObject::connect(viewer, SIGNAL(loadSignal()), loader, SLOT(load()));
    //viewer->start();
    loader->start();
    viewer->run();


    /*
    for(int i = 0; i < 100; i++) {
        int x = 830 + i * 5;
        int y = 1000 + i * 5;
        int z = 830 + i * 5;


        SET_COORDINATE(state->viewerState->currentPosition, x, y, z)
        Viewer::sendLoadSignal(x, y, z, NO_MAG_CHANGE);
        Sleeper::msleep(250);
        viewer->run();

    }   */
    //viewer->sendLoadSignal(829, 1000, 832, NO_MAG_CHANGE);


    //viewer->start();

    /*
    for(int i = 0; i < 2; i++) {
        threadObjs[i]->moveToThread(threads[i]);
        QObject::connect(threads[i], SIGNAL(started()), threadObjs[i], SLOT(start()));

        QObject::connect(threadObjs[i], SIGNAL(finished()), threads[i], SLOT(quit()));
        QObject::connect(threadObjs[i], SIGNAL(finished()), threadObjs[i], SLOT(deleteLater()));
        QObject::connect(threadObjs[i], SIGNAL(finished()), threads[i], SLOT(deleteLater()));
        threads[i]->start();
    }
    */

    return a.exec();
}

/**
 * This function overwrites the values of state with the value of tempConfig
 * Beyond it allocates the dynamic data structures
 */
int32_t Knossos::initStates() {
   state->time.start();

   //General stuff
   state->boergens = tempConfig->boergens;
   strncpy(state->path, tempConfig->path, 1024);
   strncpy(state->name, tempConfig->name, 1024);
   state->boundary.x = tempConfig->boundary.x;
   state->boundary.y = tempConfig->boundary.y;
   state->boundary.z = tempConfig->boundary.z;
   state->scale.x = tempConfig->scale.x;
   state->scale.y = tempConfig->scale.y;
   state->scale.z = tempConfig->scale.z;
   state->offset.x = tempConfig->offset.x;
   state->offset.y = tempConfig->offset.y;
   state->offset.z = tempConfig->offset.z;
   state->cubeEdgeLength = tempConfig->cubeEdgeLength;

   if(tempConfig->M % 2 == 0) {
       state->M = tempConfig->M - 1;
       tempConfig->M = state->M;
   }
   else {
       state->M = tempConfig->M;
   }

   state->magnification = tempConfig->magnification;
   state->overlay = tempConfig->overlay;

   // For the viewer
   state->viewerState->highlightVp = tempConfig->viewerState->highlightVp;
   state->viewerState->vpKeyDirection[VIEWPORT_XY] = tempConfig->viewerState->vpKeyDirection[VIEWPORT_XY];
   state->viewerState->vpKeyDirection[VIEWPORT_XZ] = tempConfig->viewerState->vpKeyDirection[VIEWPORT_XZ];
   state->viewerState->vpKeyDirection[VIEWPORT_YZ] = tempConfig->viewerState->vpKeyDirection[VIEWPORT_YZ];
   state->viewerState->overlayVisible = tempConfig->viewerState->overlayVisible;
   state->viewerState->datasetColortableOn = tempConfig->viewerState->datasetColortableOn;
   state->viewerState->datasetAdjustmentOn = tempConfig->viewerState->datasetAdjustmentOn;
   state->viewerState->treeColortableOn = tempConfig->viewerState->treeColortableOn;
   state->viewerState->drawVPCrosshairs = tempConfig->viewerState->drawVPCrosshairs;
   state->viewerState->showVPLabels = tempConfig->viewerState->showVPLabels;
   state->viewerState->viewerReady = tempConfig->viewerState->viewerReady;
   state->viewerState->stepsPerSec = tempConfig->viewerState->stepsPerSec;
   state->viewerState->numberViewports = tempConfig->viewerState->numberViewports;
   state->viewerState->inputmap = tempConfig->viewerState->inputmap;
   state->viewerState->dropFrames = tempConfig->viewerState->dropFrames;
   state->viewerState->userMove = tempConfig->viewerState->userMove;
   state->viewerState->screenSizeX = tempConfig->viewerState->screenSizeX;
   state->viewerState->screenSizeY = tempConfig->viewerState->screenSizeY;
   state->viewerState->filterType = tempConfig->viewerState->filterType;
   state->viewerState->currentPosition.x = tempConfig->viewerState->currentPosition.x;
   state->viewerState->currentPosition.y = tempConfig->viewerState->currentPosition.y;
   state->viewerState->currentPosition.z = tempConfig->viewerState->currentPosition.z;
   state->viewerState->recenteringTimeOrth = tempConfig->viewerState->recenteringTimeOrth;
   state->viewerState->walkOrth = tempConfig->viewerState->walkOrth;
   state->viewerState->autoTracingMode = 0;
   state->viewerState->autoTracingDelay = 50;
   state->viewerState->autoTracingSteps = 10;
   // the voxel dim stuff needs an cleanup. this is such a mess. fuck
   state->viewerState->voxelDimX = state->scale.x;
   state->viewerState->voxelDimY = state->scale.y;
   state->viewerState->voxelDimZ = state->scale.z;
   state->viewerState->voxelXYRatio = state->scale.x / state->scale.y;
   state->viewerState->voxelXYtoZRatio = state->scale.x / state->scale.z;
   state->viewerState->depthCutOff = tempConfig->viewerState->depthCutOff;
   state->viewerState->luminanceBias = tempConfig->viewerState->luminanceBias;
   state->viewerState->luminanceRangeDelta = tempConfig->viewerState->luminanceRangeDelta;
   Knossos::loadNeutralDatasetLUT(&(state->viewerState->neutralDatasetTable[0][0]));
   Knossos::loadTreeLUTFallback();

   state->viewerState->treeLutSet = false;

   state->viewerState->vpConfigs = (vpConfig *)malloc(state->viewerState->numberViewports * sizeof(struct vpConfig));
   if(state->viewerState->vpConfigs == NULL)
       return false;
   memset(state->viewerState->vpConfigs, '\0', state->viewerState->numberViewports * sizeof(struct vpConfig));

   for(int i = 0; i < state->viewerState->numberViewports; i++) {
       state->viewerState->vpConfigs[i].upperLeftCorner = tempConfig->viewerState->vpConfigs[i].upperLeftCorner;
       state->viewerState->vpConfigs[i].type = tempConfig->viewerState->vpConfigs[i].type;
       state->viewerState->vpConfigs[i].draggedNode = tempConfig->viewerState->vpConfigs[i].draggedNode;
       state->viewerState->vpConfigs[i].userMouseSlideX = tempConfig->viewerState->vpConfigs[i].userMouseSlideX;
       state->viewerState->vpConfigs[i].userMouseSlideY = tempConfig->viewerState->vpConfigs[i].userMouseSlideY;
       state->viewerState->vpConfigs[i].edgeLength = tempConfig->viewerState->vpConfigs[i].edgeLength;
       state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx =
           tempConfig->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
           / (float)state->magnification;

       state->viewerState->vpConfigs[i].texture.edgeLengthPx = tempConfig->viewerState->vpConfigs[i].texture.edgeLengthPx;
       state->viewerState->vpConfigs[i].texture.edgeLengthDc = tempConfig->viewerState->vpConfigs[i].texture.edgeLengthDc;
       state->viewerState->vpConfigs[i].texture.zoomLevel = tempConfig->viewerState->vpConfigs[i].texture.zoomLevel;
       state->viewerState->vpConfigs[i].texture.usedTexLengthPx = tempConfig->M * tempConfig->cubeEdgeLength;
       state->viewerState->vpConfigs[i].texture.usedTexLengthDc = tempConfig->M;

   }

   if(state->M * state->cubeEdgeLength >= TEXTURE_EDGE_LEN) {
       LOG("Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!");
       return false;
   }

   /* @todo todo emitting signals out of class seems to be problematic
   emit knossos->calcDisplayedEdgeLengthSignal();
   */


   // For the GUI
   strncpy(state->viewerState->gui->settingsFile,
           tempConfig->viewerState->gui->settingsFile,
           2048);

   // For the client

   state->clientState->connectAsap = tempConfig->clientState->connectAsap;
   state->clientState->connectionTimeout = tempConfig->clientState->connectionTimeout;
   state->clientState->remotePort = tempConfig->clientState->remotePort;
   strncpy(state->clientState->serverAddress, tempConfig->clientState->serverAddress, 1024);
   state->clientState->connected = tempConfig->clientState->connected;

   state->clientState->inBuffer = (IOBuffer *)malloc(sizeof(struct IOBuffer));
   if(state->clientState->inBuffer == NULL) {
       LOG("Out of memory.");
       return false;
   }
   memset(state->clientState->inBuffer, '\0', sizeof(struct IOBuffer));

   state->clientState->inBuffer->data = (Byte *)malloc(128);
   if(state->clientState->inBuffer->data == NULL) {
       LOG("Out of memory.");
       return false;
   }
   memset(state->clientState->inBuffer->data, '\0', 128);

   state->clientState->inBuffer->size = 128;
   state->clientState->inBuffer->length = 0;

   state->clientState->outBuffer = (IOBuffer *)malloc(sizeof(struct IOBuffer));
   if(state->clientState->outBuffer == NULL) {
       LOG("Out of memory.");
       return false;
   }
   memset(state->clientState->outBuffer, '\0', sizeof(struct IOBuffer));

   state->clientState->outBuffer->data = (Byte *) malloc(128);
   if(state->clientState->outBuffer->data == NULL) {
       LOG("Out of memory.");
       return false;
   }
   memset(state->clientState->outBuffer->data, '\0', 128);

   state->clientState->outBuffer->size = 128;
   state->clientState->outBuffer->length = 0;
   state->clientState->synchronizeSkeleton = tempConfig->clientState->synchronizeSkeleton;
   state->clientState->synchronizePosition = tempConfig->clientState->synchronizePosition;
   state->clientState->saveMaster = tempConfig->clientState->saveMaster;

   // For the skeletonizer
   state->skeletonState->lockPositions = tempConfig->skeletonState->lockPositions;
   state->skeletonState->positionLocked = tempConfig->skeletonState->positionLocked;
   state->skeletonState->lockRadius = tempConfig->skeletonState->lockRadius;
   SET_COORDINATE(state->skeletonState->lockedPosition,
                  tempConfig->skeletonState->lockedPosition.x,
                  tempConfig->skeletonState->lockedPosition.y,
                  tempConfig->skeletonState->lockedPosition.y);
   strcpy(state->skeletonState->onCommentLock, tempConfig->skeletonState->onCommentLock);
   state->skeletonState->branchpointUnresolved = tempConfig->skeletonState->branchpointUnresolved;
   state->skeletonState->autoFilenameIncrementBool = tempConfig->skeletonState->autoFilenameIncrementBool;
   state->skeletonState->autoSaveBool = tempConfig->skeletonState->autoSaveBool;
   state->skeletonState->autoSaveInterval = tempConfig->skeletonState->autoSaveInterval;
   state->skeletonState->skeletonTime = tempConfig->skeletonState->skeletonTime;
   state->skeletonState->skeletonTimeCorrection = tempConfig->skeletonState->skeletonTimeCorrection;
   state->skeletonState->definedSkeletonVpView = tempConfig->skeletonState->definedSkeletonVpView;
   strcpy(state->skeletonState->skeletonCreatedInVersion, "3.2");
   state->skeletonState->idleTime = 0;
   state->skeletonState->idleTimeNow = 0;
   state->skeletonState->idleTimeLast = 0;

   // For the remote
   state->remoteState->activeTrajectory = tempConfig->remoteState->activeTrajectory;
   state->remoteState->maxTrajectories = tempConfig->remoteState->maxTrajectories;
   state->remoteState->type = tempConfig->remoteState->type;

   // Those values can be calculated from given parameters
   state->cubeSliceArea = state->cubeEdgeLength * state->cubeEdgeLength;
   state->cubeBytes = state->cubeEdgeLength * state->cubeEdgeLength * state->cubeEdgeLength;
   state->cubeSetElements = state->M * state->M * state->M;
   state->cubeSetBytes = state->cubeSetElements * state->cubeBytes;

   SET_COORDINATE(state->currentPositionX, 0, 0, 0);

   // We're not doing stuff in parallel, yet. So we skip the locking
   // part.
   // This *10 thing is completely arbitrary. The larger the table size,
   // the lower the chance of getting collisions and the better the loading
   // order will be respected. *10 doesn't seem to have much of an effect
   // on performance but we should try to find the optimal value some day.
   // Btw: A more clever implementation would be to use an array exactly the
   // size of the supercube and index using the modulo operator.
   // sadly, that realization came rather late. ;)

   // creating the hashtables is cheap, keeping the datacubes is
   // memory expensive..
   for(int i = 0; i <= NUM_MAG_DATASETS; i = i * 2) {
       state->Dc2Pointer[Knossos::log2uint32(i)] = Hashtable::ht_new(state->cubeSetElements * 10);
       state->Oc2Pointer[Knossos::log2uint32(i)] = Hashtable::ht_new(state->cubeSetElements * 10);
       if(i == 0) i = 1;
   }

   // searches for multiple mag datasets and enables multires if more
   //  than one was found


   Knossos::findAndRegisterAvailableDatasets();

   return true;

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

bool Knossos::lockSkeleton(uint32_t targetRevision) {
    /*
     * If a skeleton modifying function is called on behalf of the network client,
     * targetRevision should be set to the appropriate remote value and lockSkeleton()
     * will decide whether to commit the change or if the skeletons have gone out of sync.
     * (This means that the return value of this function if very important and always
     * has to be checked. If the function returns a failure, the skeleton change cannot
     * proceed.)
     * If the function is being called on behalf of the user, targetRevision should be
     * set to CHANGE_MANUAL (== 0).
     *
     */

    state->protectSkeleton->lock();

    if(targetRevision != CHANGE_MANUAL) {
        // We can only commit a remote skeleton change if the remote revision count
        // is exactly the local revision count plus 1.
        // If the function changing the skeleton encounters an error, unlockSkeleton() has
        // to be called without incrementing the local revision count and the skeleton
        // synchronization has to be cancelled

        // printf("Recieved skeleton delta to revision %d, local revision is %d.\n",
        //       targetRevision, state->skeletonState->skeletonRevision);
        //

       if(targetRevision != state->skeletonState->skeletonRevision + 1) {
           // Local and remote skeletons have gone out of sync.
           Client::skeletonSyncBroken();
           return false;
       }
    }
    return true;
}
bool Knossos::unlockSkeleton(int32_t increment) {
    /* We cannot increment the revision count if the skeleton change was
     * not successfully commited (i.e. the skeleton changing function encountered
     * an error). In that case, the connection has to be closed and the user
     * must be notified. */

     /*
      * Increment signals either success or failure of the operation that made
      * locking the skeleton necessary.
      * It's here as a parameter for historical reasons and will be removed soon
      * unless it turns out to be useful for something else...
      *
      */
    state->protectSkeleton->unlock();
    return true;
}

bool Knossos::sendClientSignal() {
    state->protectClientSignal->lock();
    state->clientSignal = true;
    state->protectClientSignal->unlock();

    state->conditionClientSignal->wakeOne();

    return true;
}

bool Knossos::sendRemoteSignal() {
    state->protectRemoteSignal->lock();
    state->remoteSignal = true;
    state->protectRemoteSignal->unlock();

    state->conditionRemoteSignal->wakeOne();

    return true;
}

bool Knossos::sendQuitSignal() {
    MainWindow::UI_saveSettings();

    state->quitSignal = true;

    Knossos::sendRemoteSignal();
    Knossos::sendClientSignal();

    state->protectLoadSignal->lock();
    state->loadSignal = true;
    state->protectLoadSignal->unlock();

    state->conditionLoadSignal->wakeOne();
    return true;
}

/* Removes the new line symbols from a string */
bool Knossos::stripNewlines(char *string) {
    int32_t i = 0;

    for(i = 0; string[i] != '\0'; i++) {
        if(string[i] == '\n') // ? os specific ?
            string[i] = ' ';
        }

    return true;

}

/** @todo function yyparse is not found. Functionality is temporarily uncommented  */
bool Knossos::readConfigFile(const char *path) {
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
                return true;
            }
        }
        */
        return false;
}

bool Knossos::printConfigValues() {
    qDebug("Configuration:\n\tExperiment:\n\t\tPath: %s\n\t\tName: %s\n\t\tBoundary (x): %d\n\t\tBoundary (y): %d\n\t\tBoundary (z): %d\n\t\tScale (x): %f\n\t\tScale (y): %f\n\t\tScale (z): %f\n\n\tData:\n\t\tCube bytes: %d\n\t\tCube edge length: %d\n\t\tCube slice area: %d\n\t\tM (cube set edge length): %d\n\t\tCube set elements: %d\n\t\tCube set bytes: %d\n\t\tZ-first cube order: %d\n",
               state->path,
               state->name,
               state->boundary.x,
               state->boundary.y,
               state->boundary.z,
               state->scale.x,
               state->scale.y,
               state->scale.z,
               state->cubeBytes,
               state->cubeEdgeLength,
               state->cubeSliceArea,
               state->M,
               state->cubeSetElements,
               state->cubeSetBytes,
               state->boergens);

        return true;
}

bool Knossos::loadNeutralDatasetLUT(GLuint *datasetLut) {

    int32_t i;

    for(i = 0; i < 256; i++) {
        datasetLut[0 * 256 + i] = i;
        datasetLut[1 * 256 + i] = i;
        datasetLut[2 * 256 + i] = i;
   }

    return true;
}

struct stateInfo *Knossos::emptyState() {

    struct stateInfo *state = NULL;

    state = new stateInfo();

    state->viewerState = new viewerState();
    state->viewerState->gui = new guiConfig();
    state->remoteState = new remoteState();
    state->clientState = new clientState();
    state->loaderState = new loaderState();
    state->skeletonState = new skeletonState();

    return state;
}

/**
 * This function checks the selected dataset directory for
 * available magnifications(subfolder ending with magX)
 * The main directory name should be end with mag1. Dont forget the
 * path separator
 * Otherwise some strange behaviour has been observed in single cases.
 */
bool Knossos::findAndRegisterAvailableDatasets() {
    /* state->path stores the path to the dataset K was launched with */
    uint32_t currMag, i;
    char currPath[1024];
    char levelUpPath[1024];
    char currKconfPath[1024];
    char datasetBaseDirName[1024];
    char datasetBaseExpName[1024];
    int32_t isPathSepTerminated = false;
    uint32_t pathLen;

    memset(currPath, '\0', 1024);
    memset(levelUpPath, '\0', 1024);
    memset(currKconfPath, '\0', 1024);
    memset(datasetBaseDirName, '\0', 1024);
    memset(datasetBaseExpName, '\0', 1024);


    /* Analyze state->name to find out whether K was launched with
     * a dataset that allows multires. */

    /* Multires is only enabled if K is launched with mag1!
    * Launching it with another dataset than mag1 leads to the old
    * behavior, that only this mag is shown, this happens also
    * when the path contains no mag string. */

    if((strstr(state->name, "mag") != NULL) && (state->magnification == 1)) {

        /* take base path and go one level up */
        pathLen = strlen(state->path);

        for(i = 1; i < pathLen; i++) {
            if((state->path[pathLen-i] == '\\') || (state->path[pathLen-i] == '/')) {
                if(i == 1) {
                    /* This is the trailing path separator, ignore. */
                    isPathSepTerminated = true;
                    continue;
                }
                /* this contains the path "one level up" */
                strncpy(levelUpPath, state->path, pathLen - i + 1);
                levelUpPath[pathLen - i + 1] = '\0';
                /* this contains the dataset dir without "mag1"
                 * K must be launched with state->path set to the
                 * mag1 dataset for multires to work! This is by convention. */
                if(isPathSepTerminated) {
                    strncpy(datasetBaseDirName, state->path + pathLen - i + 1, i - 6);
                    datasetBaseDirName[i - 6] = '\0';
                }
                else {
                    strncpy(datasetBaseDirName, state->path + pathLen - i + 1, i - 5);
                    datasetBaseDirName[i - 5] = '\0';
                }

                break;
            }
        }

        state->lowestAvailableMag = INT_MAX;
        state->highestAvailableMag = 1;
        //currMag = 1;

        /* iterate over all possible mags and test their availability */
        for(currMag = 1; currMag <= NUM_MAG_DATASETS; currMag *= 2) {

            /* compile the path to the currently tested directory */
            //if(i!=0) currMag *= 2;
    #ifdef Q_OS_UNIX
            sprintf(currPath,
                "%s%smag%d/",
                levelUpPath,
                datasetBaseDirName,
                currMag);
    #else
            sprintf(currPath,
                "%s%smag%d\\",
                levelUpPath,
                datasetBaseDirName,
                currMag);
    #endif
            FILE *testKconf;
            //sprintf(currKconfPath, "%s%s", currPath, "knossos.conf");

            /* try fopen() on knossos.conf of currently tested dataset */
            if ((testKconf = fopen(currKconfPath, "r"))) {

                if(state->lowestAvailableMag > currMag) {
                    state->lowestAvailableMag = currMag;
                }
                state->highestAvailableMag = currMag;

                fclose(testKconf);
                /* add dataset path to magPaths; magPaths is used by the loader */

                strcpy(state->magPaths[log2uint32(currMag)], currPath);

                /* the last 4 letters are "mag1" by convention; if not,
                 * K multires won't work */
                strncpy(datasetBaseExpName,
                        state->name,
                        strlen(state->name)-4);
                datasetBaseExpName[strlen(state->name)-4] = '\0';

                strncpy(state->datasetBaseExpName,
                        datasetBaseExpName,
                        strlen(datasetBaseExpName)-1);
                state->datasetBaseExpName[strlen(datasetBaseExpName)-1] = '\0';

                sprintf(state->magNames[log2uint32(currMag)], "%smag%d", datasetBaseExpName, currMag);
            } else break;
        }

        /* Do not enable multires by default, even if more than one dataset was found.
         * Multires might be confusing to untrained tracers! Experts can easily enable it..
         * The loaded gui config might lock K to the current mag later one, which is fine. */
        if(state->highestAvailableMag > 1) {
            state->viewerState->datasetMagLock = true;
        }

        if(state->highestAvailableMag > NUM_MAG_DATASETS) {
            state->highestAvailableMag = NUM_MAG_DATASETS;
            LOG("KNOSSOS currently supports only datasets downsampled by a factor of %d. This can easily be changed in the source.", NUM_MAG_DATASETS);
        }

        state->magnification = state->lowestAvailableMag;

        /*state->boundary.x *= state->magnification;
        state->boundary.y *= state->magnification;
        state->boundary.z *= state->magnification;

        state->scale.x /= (float)state->magnification;
        state->scale.y /= (float)state->magnification;
        state->scale.z /= (float)state->magnification;*/

    }
    /* no magstring found, take mag read from .conf file of dataset */
    else {
        /* state->magnification already contains the right mag! */

        pathLen = strlen(state->path);

        if((state->path[pathLen-1] == '\\')
           || (state->path[pathLen-1] == '/')) {
        #ifdef Q_OS_UNIX
            state->path[pathLen-1] = '/';
        #else
            state->path[pathLen-1] = '\\';
        #endif
        }
        else {
        #ifdef Q_OS_UNIX
            state->path[pathLen] = '/';
        #else
            state->path[pathLen] = '\\';
        #endif
            state->path[pathLen + 1] = '\0';
        }

        /* the loader uses only magNames and magPaths */
        strcpy(state->magNames[log2uint32(state->magnification)], state->name);
        strcpy(state->magPaths[log2uint32(state->magnification)], state->path);

        state->lowestAvailableMag = state->magnification;
        state->highestAvailableMag = state->magnification;

        state->boundary.x *= state->magnification;
        state->boundary.y *= state->magnification;
        strcpy(state->datasetBaseExpName, state->name);
        state->boundary.z *= state->magnification;

        state->scale.x /= (float)state->magnification;
        state->scale.y /= (float)state->magnification;
        state->scale.z /= (float)state->magnification;

        state->viewerState->datasetMagLock = true;


    }
    return true;

}

bool Knossos::cleanUpMain() {


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

bool Knossos::tempConfigDefaults() {

    if(tempConfig == NULL) {
        tempConfig = Knossos::emptyState();
        if(tempConfig == NULL)
            return false;
    }

    // General stuff
    tempConfig->boergens = false;
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
    tempConfig->overlay = false;

    // For the viewer
    tempConfig->viewerState->highlightVp = VIEWPORT_UNDEFINED;
    tempConfig->viewerState->vpKeyDirection[VIEWPORT_XY] = 1;
    tempConfig->viewerState->vpKeyDirection[VIEWPORT_XZ] = 1;
    tempConfig->viewerState->vpKeyDirection[VIEWPORT_YZ] = 1;
    tempConfig->viewerState->overlayVisible = false;
    tempConfig->viewerState->datasetColortableOn = false;
    tempConfig->viewerState->datasetAdjustmentOn = false;
    tempConfig->viewerState->treeColortableOn = false;
    tempConfig->viewerState->viewerReady = false;
    tempConfig->viewerState->drawVPCrosshairs = true;
    tempConfig->viewerState->showVPLabels = false;
    tempConfig->viewerState->stepsPerSec = 40;
    tempConfig->viewerState->numberViewports = 4;
    tempConfig->viewerState->inputmap = NULL;
    tempConfig->viewerState->dropFrames = 1;
    tempConfig->viewerState->userMove = false;
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
    tempConfig->viewerState->autoTracingEnabled = false;
    tempConfig->viewerState->autoTracingDelay = 50;
    tempConfig->viewerState->autoTracingSteps = 10;
    tempConfig->viewerState->recenteringTimeOrth = 500;
    tempConfig->viewerState->walkOrth = false;

    tempConfig->viewerState->vpConfigs = (vpConfig *) malloc(tempConfig->viewerState->numberViewports * sizeof(struct vpConfig));
    if(tempConfig->viewerState->vpConfigs == NULL) {
        LOG("Out of memory.");
        return false;
    }

    memset(tempConfig->viewerState->vpConfigs, '\0', tempConfig->viewerState->numberViewports * sizeof(struct vpConfig));

    /**
     * @todo the coordinates are likely wrong in our case
     */
    for(int i = 0; i < tempConfig->viewerState->numberViewports; i++) {
        switch(i) {
        case VIEWPORT_XY:
            SET_COORDINATE(tempConfig->viewerState->vpConfigs[i].upperLeftCorner, 5, 30, 0);
            tempConfig->viewerState->vpConfigs[i].type = VIEWPORT_XY;
            break;
        case VIEWPORT_XZ:
            SET_COORDINATE(tempConfig->viewerState->vpConfigs[i].upperLeftCorner, 5, 385, 0);
            tempConfig->viewerState->vpConfigs[i].type = VIEWPORT_XZ;
            break;
        case VIEWPORT_YZ:
            SET_COORDINATE(tempConfig->viewerState->vpConfigs[i].upperLeftCorner, 360, 30, 0);
            tempConfig->viewerState->vpConfigs[i].type = VIEWPORT_YZ;
            break;
        case VIEWPORT_SKELETON:
            SET_COORDINATE(tempConfig->viewerState->vpConfigs[i].upperLeftCorner, 360, 385, 0);
            tempConfig->viewerState->vpConfigs[i].type = VIEWPORT_SKELETON;
            break;
        }

        tempConfig->viewerState->vpConfigs[i].draggedNode = NULL;
        tempConfig->viewerState->vpConfigs[i].userMouseSlideX = 0.;
        tempConfig->viewerState->vpConfigs[i].userMouseSlideY = 0.;
        tempConfig->viewerState->vpConfigs[i].edgeLength = 350;
        tempConfig->viewerState->vpConfigs[i].texture.texUnitsPerDataPx = 1. / TEXTURE_EDGE_LEN;
        tempConfig->viewerState->vpConfigs[i].texture.edgeLengthPx = TEXTURE_EDGE_LEN;
        tempConfig->viewerState->vpConfigs[i].texture.edgeLengthDc = TEXTURE_EDGE_LEN / tempConfig->cubeEdgeLength;

        //This variable indicates the current zoom value for a viewport.
        //Zooming is continous, 1: max zoom out, 0.1: max zoom in (adjust values..)
        tempConfig->viewerState->vpConfigs[i].texture.zoomLevel = VPZOOMMIN;
    }

    // For the GUI
    snprintf(tempConfig->viewerState->gui->settingsFile, 2048, "defaultSettings.xml");

    // For the client
    tempConfig->clientState->connectAsap = false;
    tempConfig->clientState->connectionTimeout = 3000;
    tempConfig->clientState->remotePort = 7890;
    strncpy(tempConfig->clientState->serverAddress, "localhost", 1024);
    tempConfig->clientState->connected = false;
    tempConfig->clientState->synchronizeSkeleton = true;
    tempConfig->clientState->synchronizePosition = true;
    tempConfig->clientState->saveMaster = false;

    // For the remote
    tempConfig->remoteState->activeTrajectory = 0;
    tempConfig->remoteState->maxTrajectories = 16;
    tempConfig->remoteState->type = false;

    // For the skeletonizer
    tempConfig->skeletonState->lockRadius = 100;
    tempConfig->skeletonState->lockPositions = false;

    strncpy(tempConfig->skeletonState->onCommentLock, "seed", 1024);
    tempConfig->skeletonState->branchpointUnresolved = false;
    tempConfig->skeletonState->autoFilenameIncrementBool = true;
    tempConfig->skeletonState->autoSaveBool = true;
    tempConfig->skeletonState->autoSaveInterval = 5;
    tempConfig->skeletonState->skeletonTime = 0;
    tempConfig->skeletonState->skeletonTimeCorrection = state->time.elapsed();
    tempConfig->skeletonState->definedSkeletonVpView = 0;

    //This number is currently arbitrary, but high values ensure a good performance
    tempConfig->skeletonState->skeletonDCnumber = 8000;
    tempConfig->skeletonState->workMode = ON_CLICK_DRAG;

    return true;

}

bool Knossos::readDataConfAndLocalConf() {

    int32_t length;
    char configFile[1024];

    memset(configFile, '\0', 1024);
    length = strlen(tempConfig->path);

    if(length >= 1010) {
        // We need to append "/knossos.conf"
        LOG("Data path too long.");
        _Exit(false);
    }

    strcat(configFile, tempConfig->path);
    strcat(configFile, "/knossos.conf");

    LOG("Trying to read %s.", configFile);

    readConfigFile(configFile);

    readConfigFile("knossos.conf");

    return true;

}

bool Knossos::configFromCli(int argCount, char *arguments[]) {

 #define NUM_PARAMS 15

 char *lval = NULL, *rval = NULL;
 char *equals = NULL;
 int32_t i = 0, j = 0, llen = 0;
 const char *lvals[NUM_PARAMS] = {
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
                 _Exit(false);
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
         _Exit(false);
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
                     tempConfig->clientState->connectAsap = true;
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
                     Knossos::readConfigFile(rval);
                     break;
                 case 12:
                     tempConfig->magnification = (int32_t)atoi(rval);
                     break;
                 case 13:
                     tempConfig->overlay = true;
                     tempConfig->viewerState->overlayVisible = true;
                     break;
                 case 14:
                     strncpy(tempConfig->viewerState->gui->settingsFile, rval, 2000);
                     strcpy(tempConfig->viewerState->gui->settingsFile + strlen(rval), ".xml");
                     break;
             }
         }
     }
 }

 return true;
}

#ifdef LINUX
void Knossos::catchSegfault(int signum) {
    LOG("Oops, you found a bug. Tell the developers!");
    fflush(stdout);
    fflush(stderr);

    _Exit(false);

}
#endif


/**
  * This function shows how wired the code is at some places.
  * A structural update might be advantegous for the future.
  */

void loadDefaultTreeLUT() {

    if(Viewer::loadTreeColorTable("default.lut", &(state->viewerState->defaultTreeTable[0]), GL_RGB) == false) {
        Knossos::loadTreeLUTFallback();
        MainWindow::treeColorAdjustmentsChanged();
    }


}
