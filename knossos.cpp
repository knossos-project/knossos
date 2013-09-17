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
#include <QTest>
#include <QMutex>
#include <QWaitCondition>
#include <QSettings>
#include <QFile>
#include "sleeper.h"
#include "knossos-global.h"
#include "knossos.h"
#include "client.h"
#include "remote.h"
#include "loader.h"
#include "viewer.h"
#include "widgets/mainwindow.h"
#include "skeletonizer.h"
#include "eventmodel.h"
#include "widgets/viewport.h"
#include "widgets/widgetcontainer.h"
#include "widgets/tracingtimewidget.h"
#include "scripting.h"
#include "ftp.h"

#include "test/testcommentswidget.h"

#ifdef Q_OS_WIN
    #include "windows.h"
#endif

#define NUMTHREADS 4

struct stateInfo *state = NULL;
char logFilename[MAX_PATH] = {0};

Knossos::Knossos(QObject *parent) : QObject(parent) {}

int main(int argc, char *argv[])
{

    char tempPath[MAX_PATH] = {0};
    char *file = "/Users/amos/log.txt";
    strcpy(logFilename, file);

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationDomain("MPI");
    QCoreApplication::setOrganizationName("Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.");
    QCoreApplication::setApplicationName("Knossos QT");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    Knossos *knossos = new Knossos();

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

    if(Knossos::configDefaults() != true) {
        LOG("Error loading default parameters.")
        _Exit(false);
    }

    if(argc >= 2) {
        Knossos::configFromCli(argc, argv);
    }

    if(state->path[0] != '\0') {
        // Got a path from cli.
        Knossos::readDataConfAndLocalConf();
        // We need to read the specified config file again because it should
        // override all parameters from other config files.
        Knossos::configFromCli(argc, argv);
    }
    else {
        Knossos::readConfigFile("knossos.conf");
    }
    state->viewerState->voxelDimX = state->scale.x;
    state->viewerState->voxelDimY = state->scale.y;
    state->viewerState->voxelDimZ = state->scale.z;

    if(argc >= 2) {
        if(Knossos::configFromCli(argc, argv) == false) {
            LOG("Error reading configuration from command line.")
        }
    }

    if(knossos->initStates() != true) {
       LOG("Error during initialization of the state struct.")
        _Exit(false);
    }

    Knossos::printConfigValues();

    Viewer *viewer = new Viewer();
    Loader *loader = new Loader();
    Remote *remote = new Remote();
    Client *client = new Client();
    Scripting *scripts = new Scripting();
    scripts->reference = viewer->skeletonizer;

    QObject::connect(knossos, SIGNAL(treeColorAdjustmentChangedSignal()), viewer->window, SLOT(treeColorAdjustmentsChanged()));
    QObject::connect(knossos, SIGNAL(loadTreeColorTableSignal(const char*,float*,int)), viewer, SLOT(loadTreeColorTable(const char*,float*,int)));

    QObject::connect(viewer, SIGNAL(broadcastPosition(uint,uint,uint)), client, SLOT(broadcastPosition(uint,uint,uint)));
    QObject::connect(viewer, SIGNAL(loadSignal()), loader, SLOT(load()));
    QObject::connect(client, SIGNAL(updateSkeletonFileNameSignal(int,int,char*)), viewer->skeletonizer, SLOT(updateSkeletonFileName(int, int, char *)));
    QObject::connect(client, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), viewer->skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    QObject::connect(client, SIGNAL(addTreeCommentSignal(int,int,char*)), viewer->skeletonizer, SLOT(addTreeComment(int,int,char*)));

    QObject::connect(viewer->vp->delegate, SIGNAL(setRemoteStateTypeSignal(int)), remote, SLOT(setRemoteStateType(int)));
    QObject::connect(viewer->vp2->delegate, SIGNAL(setRemoteStateTypeSignal(int)), remote, SLOT(setRemoteStateType(int)));
    QObject::connect(viewer->vp3->delegate, SIGNAL(setRemoteStateTypeSignal(int)), remote, SLOT(setRemoteStateType(int)));
    QObject::connect(viewer->vp4->delegate, SIGNAL(setRemoteStateTypeSignal(int)), remote, SLOT(setRemoteStateType(int)));

    QObject::connect(viewer->vp->delegate, SIGNAL(setRecenteringPositionSignal(int,int,int)), remote, SLOT(setRecenteringPosition(int,int,int)));
    QObject::connect(viewer->vp2->delegate, SIGNAL(setRecenteringPositionSignal(int,int,int)), remote, SLOT(setRecenteringPosition(int,int,int)));
    QObject::connect(viewer->vp3->delegate, SIGNAL(setRecenteringPositionSignal(int,int,int)), remote, SLOT(setRecenteringPosition(int,int,int)));
    QObject::connect(viewer->vp4->delegate, SIGNAL(setRecenteringPositionSignal(int,int,int)), remote, SLOT(setRecenteringPosition(int,int,int)));

    QObject::connect(viewer->window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(setRecenteringPositionSignal(int,int,int)),remote, SLOT(setRecenteringPosition(int,int,int)));
    QObject::connect(viewer->window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(setRemoteStateTypeSignal(int)), remote, SLOT(setRemoteStateType(int)));

    QObject::connect(viewer->window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(setRecenteringPositionSignal(int,int,int)),remote, SLOT(setRecenteringPosition(int,int,int)));
    QObject::connect(viewer->window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(setRemoteStateTypeSignal(int)), remote, SLOT(setRemoteStateType(int)));
    QObject::connect(viewer->window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(setRecenteringPositionSignal(int,int,int)),remote, SLOT(setRecenteringPosition(int,int,int)));
    QObject::connect(viewer->window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(setRemoteStateTypeSignal(int)), remote, SLOT(setRemoteStateType(int)));

    QObject::connect(client, SIGNAL(remoteJumpSignal(int,int,int)), remote, SLOT(remoteJump(int,int,int)));
    QObject::connect(client, SIGNAL(skeletonWorkModeSignal(int,uint)), viewer->skeletonizer, SLOT(setSkeletonWorkMode(int,uint)));
    QObject::connect(client, SIGNAL(clearSkeletonSignal(int,int)), viewer->skeletonizer, SLOT(clearSkeleton(int,int)));
    QObject::connect(client, SIGNAL(delSegmentSignal(int,int,int,segmentListElement*)), viewer->skeletonizer, SLOT(delSegment(int,int,int,segmentListElement*)));
    QObject::connect(client, SIGNAL(editNodeSignal(int,int,nodeListElement*,float,int,int,int,int)), viewer->skeletonizer, SLOT(editNode(int,int,nodeListElement*,float,int,int,int,int)));
    QObject::connect(client, SIGNAL(delNodeSignal(int,int,nodeListElement*)), viewer->skeletonizer, SLOT(delNode(int,int,nodeListElement*)));
    QObject::connect(client, SIGNAL(delTreeSignal(int,int)), viewer->skeletonizer, SLOT(delTree(int,int)));
    QObject::connect(client, SIGNAL(addCommentSignal(int,const char*,nodeListElement*,int)), viewer->skeletonizer, SLOT(addComment(int,const char*,nodeListElement*,int)));
    QObject::connect(client, SIGNAL(editCommentSignal(int,commentListElement*,int,char*,nodeListElement*,int)), viewer->skeletonizer, SLOT(editComment(int,commentListElement*,int,char*,nodeListElement*,int)));
    QObject::connect(client, SIGNAL(delCommentSignal(int,commentListElement*,int)), viewer->skeletonizer, SLOT(delComment(int,commentListElement*,int)));
    QObject::connect(client, SIGNAL(popBranchNodeSignal(int)), viewer->skeletonizer, SLOT(popBranchNode(int)));
    QObject::connect(client, SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int)), viewer->skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int)));
    QObject::connect(client, SIGNAL(sendConnectedState()), viewer->window->widgetContainer->synchronizationWidget, SLOT(updateConnectionInfo()));
    QObject::connect(client, SIGNAL(sendDisconnectedState()), viewer->window->widgetContainer->synchronizationWidget, SLOT(updateDisconnectionInfo()));

    QObject::connect(remote, SIGNAL(updatePositionSignal(int)), viewer, SLOT(updatePosition(int)));
    QObject::connect(remote, SIGNAL(userMoveSignal(int,int,int,int)), viewer, SLOT(userMove(int,int,int,int)));
    QObject::connect(remote, SIGNAL(updateViewerStateSignal()), viewer, SLOT(updateViewerState()));

    QObject::connect(remote, SIGNAL(idleTimeSignal()), viewer->window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));    
    QObject::connect(viewer->window, SIGNAL(loadTreeLUTFallback()), knossos, SLOT(loadTreeLUTFallback()));

    loader->start();
    viewer->run();
    remote->start();
    client->start();
    scripts->run();

    /* TEST */
    /*
    TestCommentsWidget test;
    test.reference = viewer;
    QTest::qExec(&test);
    */

    return a.exec();
}


/**
 * This function initializes the values of state with the value of tempConfig
 * Beyond it allocates the dynamic data structures
 */
bool Knossos::initStates() {
   state->time.start();

   //General stuff   
   if(state->M % 2 == 0) {
       state->M = state->M - 1;
   }

   // For the viewer  
   state->viewerState->autoTracingMode = 0;
   state->viewerState->autoTracingDelay = 50;
   state->viewerState->autoTracingSteps = 10;
   state->skeletonState->idleTimeSession = 0;

   // the voxel dim stuff needs an cleanup. this is such a mess. fuck
   state->viewerState->voxelDimX = state->scale.x;
   state->viewerState->voxelDimY = state->scale.y;
   state->viewerState->voxelDimZ = state->scale.z;
   state->viewerState->voxelXYRatio = state->scale.x / state->scale.y;
   state->viewerState->voxelXYtoZRatio = state->scale.x / state->scale.z;
   state->viewerState->depthCutOff = state->viewerState->depthCutOff;
   state->viewerState->cumDistRenderThres = 7.f; //in screen pixels
   Knossos::loadNeutralDatasetLUT(&(state->viewerState->neutralDatasetTable[0][0]));
   loadDefaultTreeLUT();

   state->viewerState->treeLutSet = false;

   /* @arb */
   state->alpha = 0;
   state->beta = 0;
   floatCoordinate v1, v2, v3;
   Viewer::getDirectionalVectors(state->alpha, state->beta, &v1, &v2, &v3);

   CPY_COORDINATE(state->viewerState->vpConfigs[0].v1 , v1);
   CPY_COORDINATE(state->viewerState->vpConfigs[0].v2 , v2);
   CPY_COORDINATE(state->viewerState->vpConfigs[0].n , v3);
   CPY_COORDINATE(state->viewerState->vpConfigs[1].v1 , v1);
   CPY_COORDINATE(state->viewerState->vpConfigs[1].v2 , v3);
   CPY_COORDINATE(state->viewerState->vpConfigs[1].n , v2);
   CPY_COORDINATE(state->viewerState->vpConfigs[2].v1 , v3);
   CPY_COORDINATE(state->viewerState->vpConfigs[2].v2 , v2);
   CPY_COORDINATE(state->viewerState->vpConfigs[2].n , v1);

   /**/

   for(uint i = 0; i < state->viewerState->numberViewports; i++) {

       state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx =
           state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
           / (float)state->magnification;       
       state->viewerState->vpConfigs[i].texture.usedTexLengthDc = state->M;

   }

   if(state->M * state->cubeEdgeLength >= TEXTURE_EDGE_LEN) {
       LOG("Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!")
       return false;
   }

   /* @todo todo emitting signals out of class seems to be problematic
   emit knossos->calcDisplayedEdgeLengthSignal();
   */


   // For the client

   state->clientState->inBuffer = (IOBuffer *)malloc(sizeof(struct IOBuffer));
   if(state->clientState->inBuffer == NULL) {
       LOG("Out of memory.")
       return false;
   }
   memset(state->clientState->inBuffer, '\0', sizeof(struct IOBuffer));

   state->clientState->inBuffer->data = (Byte *)malloc(128);
   if(state->clientState->inBuffer->data == NULL) {
       LOG("Out of memory.")
       return false;
   }
   memset(state->clientState->inBuffer->data, '\0', 128);

   state->clientState->inBuffer->size = 128;
   state->clientState->inBuffer->length = 0;

   state->clientState->outBuffer = (IOBuffer *)malloc(sizeof(struct IOBuffer));
   if(state->clientState->outBuffer == NULL) {
       LOG("Out of memory.")
       return false;
   }
   memset(state->clientState->outBuffer, '\0', sizeof(struct IOBuffer));

   state->clientState->outBuffer->data = (Byte *) malloc(128);
   if(state->clientState->outBuffer->data == NULL) {
       LOG("Out of memory.")
       return false;
   }
   memset(state->clientState->outBuffer->data, '\0', 128);

   state->clientState->outBuffer->size = 128;
   state->clientState->outBuffer->length = 0;

   // For the skeletonizer   
   strcpy(state->skeletonState->skeletonCreatedInVersion, "3.2");
   state->skeletonState->idleTime = 0;
   state->skeletonState->idleTimeNow = 0;
   state->skeletonState->idleTimeLast = 0;

   // Those values can be calculated from given parameters
   state->cubeSliceArea = state->cubeEdgeLength * state->cubeEdgeLength;
   state->cubeBytes = state->cubeEdgeLength * state->cubeEdgeLength * state->cubeEdgeLength;
   state->cubeSetElements = state->M * state->M * state->M;
   state->cubeSetBytes = state->cubeSetElements * state->cubeBytes;

   memset(state->currentDirections, 0, LL_CURRENT_DIRECTIONS_SIZE*sizeof(state->currentDirections[0]));
   state->currentDirectionsIndex = 0;
   SET_COORDINATE(state->previousPositionX, 0, 0, 0);
   SET_COORDINATE(state->currentPositionX, 0, 0, 0);

   state->loadLocalSystem = LS_UNIX;
   if (LM_FTP == state->loadMode) {

       state->loadFtpCachePath = (char*)malloc(MAX_PATH);

       char *tmp = "/Users/amos/temp/";
       strcpy(state->loadFtpCachePath, tmp);

       curl_global_init(CURL_GLOBAL_DEFAULT);
   }

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


   if (false == Knossos::findAndRegisterAvailableDatasets()) {
       return false;
   }

   return true;

}


/* http://aggregate.org/MAGIC/#Log2%20of%20an%20Integer */
uint Knossos::ones32(register uint x) {
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
uint Knossos::log2uint32(register uint x) {

    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);

    return(ones32(x >> 1));
}

bool Knossos::lockSkeleton(uint targetRevision) {
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
bool Knossos::unlockSkeleton(int increment) {
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
    qDebug() << "sending the client signal";
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
    int i = 0;

    for(i = 0; string[i] != '\0'; i++) {
        if(string[i] == '\n') // ? os specific ?
            string[i] = ' ';
        }

    return true;

}

bool Knossos::readConfigFile(const char *path) {

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
            qDebug("Error reading config file at path:%s", path);
            return false;
    }

    QTextStream stream(&file);
    while(!stream.atEnd()) {
        QString line = stream.readLine();
        if(line.isEmpty())
            continue;

        QStringList tokenList = line.split(
                    QRegExp("[ ;]"),
                    QString::SkipEmptyParts);

        QString token = tokenList.at(0);

        if(token == "experiment") {
            token = tokenList.at(2);
            QStringList experimentTokenList = token.split(
                        QRegExp("[\"]"),
                        QString::SkipEmptyParts);
            QString experimentToken = experimentTokenList.at(0);
            std::string stdString = experimentToken.toStdString();
            strncpy(state->name, stdString.c_str(), 1024);

        } else if(token == "scale") {
            token = tokenList.at(1);
            if(token == "x") {
                state->scale.x = tokenList.at(2).toFloat();
            } else if(token == "y") {
                state->scale.y = tokenList.at(2).toFloat();
            } else if(token == "z") {
                state->scale.z = tokenList.at(2).toFloat();
            }
        } else if(token == "boundary") {
            token = tokenList.at(1);
            if(token == "x") {
                state->boundary.x = tokenList.at(2).toFloat();
            } else if(token == "y") {
                state->boundary.y = tokenList.at(2).toFloat();
            } else if(token == "z") {
                state->boundary.z = tokenList.at(2).toFloat();
            }
        } else if(token == "magnification") {
            state->magnification = tokenList.at(1).toInt();
        } else if(token == "ftp_mode") {
            state->loadMode = LM_FTP;
            int ti;
            std::string stdString;
            const int FTP_PARAMS_NUM = 4;
            char *ftp_conf_strings[FTP_PARAMS_NUM] = { state->ftpHostName, state->ftpBasePath, state->ftpUsername, state->ftpPassword };
            for (ti = 0; ti < FTP_PARAMS_NUM; ti++) {
                token = tokenList.at(ti + 1);
                stdString = token.toStdString();
                strncpy(ftp_conf_strings[ti], stdString.c_str(), MAX_PATH);
            }
            state->ftpFileTimeout = tokenList.at(ti + 1).toInt();

        } else if (token == "compression_ratio") {
            state->compressionRatio = tokenList.at(1).toInt();
        } else {
            LOG("Skipping unknown parameter");
        }

    }

    if(file.isOpen())
        file.close();


        return true;
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

    int i;

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
    state->clientState = new clientState();

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
    uint currMag, i;
    uint isMultiresCompatible = false;
    char currPath[1024];
    char levelUpPath[1024];
    char currKconfPath[1024];
    char datasetBaseDirName[1024];
    char datasetBaseExpName[1024];
    int isPathSepTerminated = false;
    uint pathLen;

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

    if(state->path[strlen(state->path) - 1] == '/'
       || state->path[strlen(state->path) - 1] == '\\') {
        isPathSepTerminated = true;
    }

    if (LM_FTP == state->loadMode) {
        isMultiresCompatible = true;
    }
    else {
        if(isPathSepTerminated) {
            if(strncmp(&state->path[strlen(state->path) - 5], "mag1", 4) == 0) {
                isMultiresCompatible = true;
            }
        }
        else {
            if(strncmp(&state->path[strlen(state->path) - 4], "mag1", 4) == 0) {
                isMultiresCompatible = true;
            }
        }
    }

    if(isMultiresCompatible && (state->magnification == 1)) {
        if (LM_FTP != state->loadMode) {
            /* take base path and go one level up */
            pathLen = strlen(state->path);

            for(i = 1; i < pathLen; i++) {
                if((state->path[pathLen-i] == '\\')
                    || (state->path[pathLen-i] == '/')) {
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
        }

        state->lowestAvailableMag = INT_MAX;
        state->highestAvailableMag = 1;
        //currMag = 1;

        /* iterate over all possible mags and test their availability */
        for(currMag = 1; currMag <= NUM_MAG_DATASETS; currMag *= 2) {
            int currMagExists = false;
            if (LM_FTP == state->loadMode) {
                char *ftpDirDelim = "/";
                int confSize = 0;
                sprintf(currPath, "%smag%d%sknossos.conf", state->ftpBasePath, currMag, ftpDirDelim);
                /* if (1 == FtpSize(currPath, &confSize, FTPLIB_TEXT, state->ftpConn)) { */
                if (EXIT_SUCCESS == downloadFile(currPath, NULL)) {
                    currMagExists = true;
                }
            }
            else {
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
                sprintf(currKconfPath, "%s%s", currPath, "knossos.conf");

                /* try fopen() on knossos.conf of currently tested dataset */
                if ((testKconf = fopen(currKconfPath, "r"))) {
                    fclose(testKconf);
                    currMagExists = true;
                }
            }
            if (currMagExists) {

                if(state->lowestAvailableMag > currMag) {
                    state->lowestAvailableMag = currMag;
                }
                state->highestAvailableMag = currMag;

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
        LOG("Highest Mag: %d", state->highestAvailableMag);

        if(state->lowestAvailableMag == INT_MAX) {
            /* This can happen if a bug in the string parsing above causes knossos to
             * search the wrong directories. We exit here to prevent guaranteed
             * subsequent crashes. */

            LOG("Unsupported data path format.");
            _Exit(false);
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
        if(!pathLen) {
            LOG("No valid dataset specified.\n");
            _Exit(false);
        }

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

bool Knossos::configDefaults() {
    state = Knossos::emptyState();
    state->loadSignal = false;
    state->remoteSignal = false;
    state->quitSignal = false;
    state->clientSignal = false;
    state->conditionLoadSignal = new QWaitCondition();
    state->conditionRemoteSignal = new QWaitCondition();
    state->conditionClientSignal = new QWaitCondition();
    state->protectSkeleton = new QMutex(QMutex::Recursive);
    state->protectLoadSignal = new QMutex();
    state->protectLoaderSlots = new QMutex();
    state->protectRemoteSignal = new QMutex();
    state->protectClientSignal = new QMutex();
    state->protectCube2Pointer = new QMutex();
    state->protectPeerList = new QMutex();
    state->protectOutBuffer = new QMutex();

    // General stuff
    state->boergens = false;
    state->boundary.x = 1000;
    state->boundary.y = 1000;
    state->boundary.z = 1000;
    state->scale.x = 1.;
    state->scale.y = 1.;
    state->scale.z = 1.;
    state->offset.x = 0;
    state->offset.y = 0;
    state->offset.z = 0;
    state->cubeEdgeLength = 128;
    state->M = 3;
    state->magnification = 1;
    state->overlay = false;

    // For the viewer
    state->viewerState->highlightVp = VIEWPORT_UNDEFINED;
    state->viewerState->vpKeyDirection[VIEWPORT_XY] = 1;
    state->viewerState->vpKeyDirection[VIEWPORT_XZ] = 1;
    state->viewerState->vpKeyDirection[VIEWPORT_YZ] = 1;
    state->viewerState->overlayVisible = false;
    state->viewerState->datasetColortableOn = false;
    state->viewerState->datasetAdjustmentOn = false;
    state->viewerState->treeColortableOn = false;
    state->viewerState->viewerReady = false;
    state->viewerState->drawVPCrosshairs = true;
    state->viewerState->showVPLabels = false;
    state->viewerState->stepsPerSec = 40;
    state->viewerState->numberViewports = 4;
    state->viewerState->dropFrames = 1;
    state->viewerState->walkFrames = 10;
    state->viewerState->userMove = false;

    /* @arb */
    state->viewerState->moveCache.x = 0.0;
    state->viewerState->moveCache.y = 0.0;
    state->viewerState->moveCache.z = 0.0;
    state->viewerState->alphaCache = 0;
    state->viewerState->betaCache = 0;
    state->viewerState->modeArbitrary = 0;
      /* */

    state->viewerState->screenSizeX = 1024;
    state->viewerState->screenSizeY = 740;
    state->viewerState->filterType = GL_LINEAR;
    state->viewerState->currentPosition.x = 0;
    state->viewerState->currentPosition.y = 0;
    state->viewerState->currentPosition.z = 0;
    state->viewerState->voxelDimX = state->scale.x;
    state->viewerState->voxelDimY = state->scale.y;
    state->viewerState->voxelDimZ = state->scale.z;
    state->viewerState->voxelXYRatio = state->viewerState->voxelDimX / state->viewerState->voxelDimY;
    state->viewerState->voxelXYtoZRatio = state->viewerState->voxelDimX / state->viewerState->voxelDimZ;
    state->viewerState->depthCutOff = 5.;
    state->viewerState->luminanceBias = 0;
    state->viewerState->luminanceRangeDelta = 255;
    state->viewerState->autoTracingDelay = 50;
    state->viewerState->autoTracingSteps = 10;
    state->viewerState->recenteringTimeOrth = 500;
    state->viewerState->walkOrth = false;
    state->viewerState->moveVP = 10;
    state->viewerState->resizeVP = 10;
    state->viewerState->showPosSizButtons = true;

    state->viewerState->vpConfigs = (vpConfig *) malloc(state->viewerState->numberViewports * sizeof(struct vpConfig));
    if(state->viewerState->vpConfigs == NULL) {
        LOG("Out of memory.")
        return false;
    }
    memset(state->viewerState->vpConfigs, '\0', state->viewerState->numberViewports * sizeof(struct vpConfig));

    for(uint i = 0; i < state->viewerState->numberViewports; i++) {
        switch(i) {
        case VIEWPORT_XY:
            SET_COORDINATE(state->viewerState->vpConfigs[i].upperLeftCorner, 5, 30, 0);
            state->viewerState->vpConfigs[i].type = VIEWPORT_XY;
            break;
        case VIEWPORT_XZ:
            SET_COORDINATE(state->viewerState->vpConfigs[i].upperLeftCorner, 5, 385, 0);
            state->viewerState->vpConfigs[i].type = VIEWPORT_XZ;
            break;
        case VIEWPORT_YZ:
            SET_COORDINATE(state->viewerState->vpConfigs[i].upperLeftCorner, 360, 30, 0);
            state->viewerState->vpConfigs[i].type = VIEWPORT_YZ;
            break;
        case VIEWPORT_SKELETON:
            SET_COORDINATE(state->viewerState->vpConfigs[i].upperLeftCorner, 360, 385, 0);
            state->viewerState->vpConfigs[i].type = VIEWPORT_SKELETON;
            break;
        }

        state->viewerState->vpConfigs[i].draggedNode = NULL;
        state->viewerState->vpConfigs[i].userMouseSlideX = 0.;
        state->viewerState->vpConfigs[i].userMouseSlideY = 0.;
        state->viewerState->vpConfigs[i].edgeLength = 350;
        state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx = 1. / TEXTURE_EDGE_LEN;
        state->viewerState->vpConfigs[i].texture.edgeLengthPx = TEXTURE_EDGE_LEN;
        state->viewerState->vpConfigs[i].texture.edgeLengthDc = TEXTURE_EDGE_LEN / state->cubeEdgeLength;

        //This variable indicates the current zoom value for a viewport.
        //Zooming is continous, 1: max zoom out, 0.1: max zoom in (adjust values..)
        state->viewerState->vpConfigs[i].texture.zoomLevel = VPZOOMMIN;
    }

    // For the GUI
    snprintf(state->viewerState->gui->settingsFile, 2048, "defaultSettings.xml");

    // For the client
    state->clientState->connectAsap = false;
    state->clientState->connectionTimeout = 3000;
    state->clientState->remotePort = 7890;
    strncpy(state->clientState->serverAddress, "localhost", 1024);
    state->clientState->connected = false;
    state->clientState->synchronizeSkeleton = true;
    state->clientState->synchronizePosition = true;
    state->clientState->saveMaster = false;


    // For the skeletonizer
    state->skeletonState->lockRadius = 100;
    state->skeletonState->lockPositions = false;

    strncpy(state->skeletonState->onCommentLock, "seed", 1024);
    state->skeletonState->branchpointUnresolved = false;
    state->skeletonState->autoFilenameIncrementBool = true;
    state->skeletonState->autoSaveBool = true;
    state->skeletonState->autoSaveInterval = 5;
    state->skeletonState->skeletonTime = 0;
    state->skeletonState->skeletonTimeCorrection = state->time.elapsed();
    state->skeletonState->definedSkeletonVpView = 0;

    //This number is currently arbitrary, but high values ensure a good performance
    state->skeletonState->skeletonDCnumber = 8000;
    state->skeletonState->workMode = ON_CLICK_DRAG;

    state->loadMode = LM_LOCAL;
    state->compressionRatio = 0;

    return true;

}

bool Knossos::readDataConfAndLocalConf() {

    int length;
    char configFile[1024];

    memset(configFile, '\0', 1024);
    length = strlen(state->path);

    if(length >= 1010) {
        // We need to append "/knossos.conf"
        LOG("Data path too long.")
        _Exit(false);
    }

    strcat(configFile, state->path);
    strcat(configFile, "/knossos.conf");


    LOG("Trying to read %s.", configFile)

    readConfigFile(configFile);

    readConfigFile("knossos.conf");

    return true;

}

bool Knossos::configFromCli(int argCount, char *arguments[]) {

 #define NUM_PARAMS 15

 char *lval = NULL, *rval = NULL;
 char *equals = NULL;
 int i = 0, j = 0, llen = 0;
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
                 LOG("Out of memory.")
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
         LOG("Out of memory.")
         _Exit(false);
     }
     memset(lval, '\0', llen + 1);

     strncpy(lval, arguments[i], llen);

     for(j = 0; j < NUM_PARAMS; j++) {
         if(strcmp(lval, lvals[j]) == 0) {
             switch(j) {
                 case 0:
                     strncpy(state->path, rval, 1023);
                     break;
                 case 1:
                     state->clientState->connectAsap = true;
                     break;
                 case 2:
                     state->scale.x = (float)strtod(rval, NULL);
                     break;
                 case 3:
                     state->scale.y = (float)strtod(rval, NULL);
                     break;
                 case 4:
                     state->scale.z = (float)strtod(rval, NULL);
                     break;
                 case 5:
                     state->boundary.x = (int)atoi(rval);
                     break;
                 case 6:
                     state->boundary.y = (int)atoi(rval);
                     break;
                 case 7:
                     state->boundary.z = (int)atoi(rval);
                     break;
                 case 8:
                     strncpy(state->name, rval, 1023);
                     break;
                 case 9:
                     state->M = (int)atoi(rval);
                     break;
                 case 10:
                     Viewer::loadDatasetColorTable(rval, &(state->viewerState->datasetColortable[0][0]), GL_RGB);
                     break;
                 case 11:
                     Knossos::readConfigFile(rval);
                     break;
                 case 12:
                     state->magnification = (int)atoi(rval);
                     break;
                 case 13:
                     state->overlay = true;
                     state->viewerState->overlayVisible = true;
                     break;
                 case 14:
                     strncpy(state->viewerState->gui->settingsFile, rval, 2000);
                     strcpy(state->viewerState->gui->settingsFile + strlen(rval), ".xml");
                     break;
             }
         }
     }     
 }

 return true;
}


/**
  * This function shows how wired the code is at some places.
  * A structural update might be advantegous for the future.
  */

void Knossos::loadDefaultTreeLUT() {

    if(loadTreeColorTableSignal("default.lut", &(state->viewerState->defaultTreeTable[0]), GL_RGB) == false) {
        Knossos::loadTreeLUTFallback();
        emit treeColorAdjustmentChangedSignal();
    }
}

void rewire() {

}
