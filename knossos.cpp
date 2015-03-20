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
#include "task.h"//because curl wants to be first on win

#include "knossos.h"

#include "eventmodel.h"
#include "file_io.h"
#include "loader.h"
#include "network.h"
#include "remote.h"
#include "scriptengine/proxies/skeletonproxy.h"
#include "scriptengine/scripting.h"
#include "skeleton/skeletonizer.h"
#include "version.h"
#include "viewer.h"
#include "widgets/widgetcontainer.h"

#include <QApplication>
#include <QFileInfo>

#include <cmath>
#include <iostream>
#include <fstream>

#define GLUT_DISABLE_ATEXIT_HACK
#ifdef Q_OS_MAC
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#define NUMTHREADS 4

#if defined(Q_OS_WIN) && defined(QT_STATIC)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

std::unique_ptr<Knossos> knossos;

Knossos::Knossos(QObject *parent) : QObject(parent) {}

Splash::Splash(const QString & img_filename, const int timeout_msec) : screen(QPixmap(img_filename), Qt::WindowStaysOnTopHint) {
    screen.show();
    //the splashscreen is hidden after a timeout, it could also wait for the mainwindow
    QObject::connect(&timer, &QTimer::timeout, &screen, &QSplashScreen::close);
    timer.start(timeout_msec);
}

void debugMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & msg) {
    QString intro;
    switch (type) {
    case QtDebugMsg:    intro = QString("Debug: ");    break;
    case QtWarningMsg:  intro = QString("Warning: ");  break;
    case QtCriticalMsg: intro = QString("Critical: "); break;
    case QtFatalMsg:    intro = QString("Fatal: ");    break;
    }
    QString txt = QString("[%1:%2] \t%4%5").arg(QFileInfo(context.file).fileName()).arg(context.line).arg(intro).arg(msg);
    //open the file once
    static std::ofstream outFile(QStandardPaths::writableLocation(QStandardPaths::DataLocation).toStdString()+"/log.txt", std::ios_base::app);
    outFile   << txt.toStdString() << std::endl;
    std::cout << txt.toStdString() << std::endl;

    if (type == QtFatalMsg) {
        std::abort();
    }
}

int global_argc;
char **global_argv;

void Knossos::applyDefaultConfig() {
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
        qDebug() << "Error loading default parameters.";
        _Exit(false);
    }

    if(global_argc >= 2) {
        Knossos::configFromCli(global_argc, global_argv);
    }

    if(state->path[0] != '\0') {
        // Got a path from cli.
        Knossos::readDataConfAndLocalConf();
        // We need to read the specified config file again because it should
        // override all parameters from other config files.
        Knossos::configFromCli(global_argc, global_argv);
    }
    else {
        Knossos::readConfigFile("knossos.conf");
    }
    state->viewerState->voxelDimX = state->scale.x;
    state->viewerState->voxelDimY = state->scale.y;
    state->viewerState->voxelDimZ = state->scale.z;

    if(global_argc >= 2) {
        if(Knossos::configFromCli(global_argc, global_argv) == false) {
            qDebug() << "Error reading configuration from command line.";
        }
    }
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    QApplication a(argc, argv);
    qInstallMessageHandler(debugMessageHandler);
    /* On OSX there is the problem that the splashscreen nevers returns and it prevents the start of the application.
       I searched for the reason and found this here : https://bugreports.qt-project.org/browse/QTBUG-35169
       As I found out randomly that effect does not occur if the splash is invoked directly after the QApplication(argc, argv)
    */
#ifdef NDEBUG
    Splash splash(":/resources/splash.png", 1500);
#endif
    QCoreApplication::setOrganizationDomain("knossostool.org");
    QCoreApplication::setOrganizationName("MPIMF");
    QCoreApplication::setApplicationName(QString("Knossos %1").arg(KVERSION));
    QSettings::setDefaultFormat(QSettings::IniFormat);

    knossos.reset(new Knossos);

    global_argc = argc;
    global_argv = argv;

    knossos->applyDefaultConfig();

    knossos->initStates();

    Viewer viewer;
    Remote remote;

    QObject::connect(knossos.get(), &Knossos::treeColorAdjustmentChangedSignal, viewer.window, &MainWindow::treeColorAdjustmentsChanged);
    QObject::connect(knossos.get(), &Knossos::loadTreeColorTableSignal, &viewer, &Viewer::loadTreeColorTable);

    QObject::connect(viewer.window, &MainWindow::loadTreeLUTFallback, knossos.get(), &Knossos::loadTreeLUTFallback);
    QObject::connect(viewer.window->widgetContainer->datasetLoadWidget, &DatasetLoadWidget::changeDatasetMagSignal, &viewer, &Viewer::changeDatasetMag, Qt::DirectConnection);
    QObject::connect(viewer.skeletonizer, &Skeletonizer::setRecenteringPositionSignal, &remote, &Remote::setRecenteringPosition);

    QObject::connect(viewer.eventModel, &EventModel::setRecenteringPositionSignal, &remote, &Remote::setRecenteringPosition);
    QObject::connect(viewer.eventModel, &EventModel::setRecenteringPositionWithRotationSignal, &remote, &Remote::setRecenteringPositionWithRotation);

    QObject::connect(&Segmentation::singleton(), &Segmentation::setRecenteringPositionSignal, &remote, &Remote::setRecenteringPosition);

    qRegisterMetaType<UserMoveType>();
    qRegisterMetaType<ViewportType>();
    QObject::connect(&remote, &Remote::userMoveSignal, &viewer, &Viewer::userMove);
    QObject::connect(&remote, &Remote::updateViewerStateSignal, &viewer, &Viewer::updateViewerState);
    QObject::connect(&remote, &Remote::rotationSignal, &viewer, &Viewer::setRotation);

    knossos->loadDefaultTreeLUT();

    viewer.run();
    Scripting scripts;
    remote.start();

    viewer.window->widgetContainer->datasetLoadWidget->loadDataset();

    viewer.window->widgetContainer->datasetOptionsWidget->updateCompressionRatioDisplay();
    Knossos::printConfigValues();

    QObject::connect(viewer.window->widgetContainer->pythonPropertyWidget, &PythonPropertyWidget::changeWorkingDirectory, &scripts, &Scripting::changeWorkingDirectory);

    QObject::connect(signalDelegate, &SkeletonProxySignalDelegate::userMoveSignal, &remote, &Remote::remoteJump);
    QObject::connect(signalDelegate, &SkeletonProxySignalDelegate::loadSkeleton, &annotationFileLoad);
    QObject::connect(signalDelegate, &SkeletonProxySignalDelegate::saveSkeleton, &annotationFileSave);
    QObject::connect(signalDelegate, &SkeletonProxySignalDelegate::clearSkeletonSignal, viewer.window, &MainWindow::clearSkeletonSlotNoGUI);

    return a.exec();
}

/**
 * This function is common to initStates, which is called when knossos starts,
 * and to switching to another dataset. Therefore it includes *only* those
 * actions that are dataset-specific, or that are required for reseting dataset-
 * derived content, e.g. the actual cube contents
 */
bool Knossos::commonInitStates() {
    // the voxel dim stuff needs an cleanup. this is such a mess. fuck
    state->viewerState->voxelDimX = state->scale.x;
    state->viewerState->voxelDimY = state->scale.y;
    state->viewerState->voxelDimZ = state->scale.z;
    state->viewerState->voxelXYRatio = state->scale.x / state->scale.y;
    state->viewerState->voxelXYtoZRatio = state->scale.x / state->scale.z;

    //reset viewerState texture properties
    for(uint i = 0; i < Viewport::numberViewports; i++) {
        state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx = 1. / TEXTURE_EDGE_LEN;
        state->viewerState->vpConfigs[i].texture.edgeLengthPx = TEXTURE_EDGE_LEN;
        state->viewerState->vpConfigs[i].texture.edgeLengthDc = TEXTURE_EDGE_LEN / state->cubeEdgeLength;

        //This variable indicates the current zoom value for a viewport.
        //Zooming is continous, 1: max zoom out, 0.1: max zoom in (adjust values..)
        state->viewerState->vpConfigs[i].texture.zoomLevel = VPZOOMMIN;
    }

    // searches for multiple mag datasets and enables multires if more
    //  than one was found
    if(state->path[0] == '\0') {
        return false;//no dataset loaded
    }

    return findAndRegisterAvailableDatasets();
}

/**
 * This function initializes the values of state with the value of tempConfig
 * Beyond it allocates the dynamic data structures
 */
bool Knossos::initStates() {

   // For the viewer
   state->viewerState->autoTracingMode = navigationMode::recenter;
   state->viewerState->autoTracingDelay = 50;
   state->viewerState->autoTracingSteps = 10;

   state->viewerState->depthCutOff = state->viewerState->depthCutOff;
   state->viewerState->cumDistRenderThres = 7.f; //in screen pixels
   Knossos::loadNeutralDatasetLUT(&(state->viewerState->neutralDatasetTable[0][0]));
   loadDefaultTreeLUT();

   state->viewerState->treeLutSet = false;

   /* @todo todo emitting signals out of class seems to be problematic
   emit knossos->calcDisplayedEdgeLengthSignal();
   */

   // For the skeletonizer
   strcpy(state->skeletonState->skeletonCreatedInVersion, "3.2");

   state->time.start();

   // Those values can be calculated from given parameters
   state->magnification = 1;
   state->lowestAvailableMag = 1;
   state->highestAvailableMag = 1;

   memset(state->currentDirections, 0, LL_CURRENT_DIRECTIONS_SIZE*sizeof(state->currentDirections[0]));
   state->currentDirectionsIndex = 0;
   SET_COORDINATE(state->previousPositionX, 0, 0, 0);
   SET_COORDINATE(state->currentPositionX, 0, 0, 0);

   curl_global_init(CURL_GLOBAL_DEFAULT);
   state->loadFtpCachePath = (char*)malloc(CSTRING_SIZE);

#ifdef Q_OS_UNIX
   const char *tmp = "/tmp/";
   strcpy(state->loadFtpCachePath, tmp);
#endif
#ifdef Q_OS_WIN32
    GetTempPathA(CSTRING_SIZE, state->loadFtpCachePath);
#endif

   return commonInitStates();

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
    QApplication::processEvents(); //ensure everything’s done
    Loader::Controller::singleton().waitForWorkerThread();//suspend loader
    Knossos::sendRemoteSignal();

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

    state->loadMode = LM_LOCAL;
    state->compressionRatio = 0;
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
            state->lowestAvailableMag = state->magnification;
            state->highestAvailableMag = state->magnification;
        } else if(token == "cube_edge_length") {
            state->cubeEdgeLength = tokenList.at(1).toInt();
        } else if(token == "ftp_mode") {
            state->loadMode = LM_FTP;
            int ti;
            std::string stdString;
            const int FTP_PARAMS_NUM = 4;
            char *ftp_conf_strings[FTP_PARAMS_NUM] = { state->ftpHostName, state->ftpBasePath, state->ftpUsername, state->ftpPassword };
            for (ti = 0; ti < FTP_PARAMS_NUM; ti++) {
                token = tokenList.at(ti + 1);
                stdString = token.toStdString();
                strncpy(ftp_conf_strings[ti], stdString.c_str(), CSTRING_SIZE);
            }
            state->ftpFileTimeout = tokenList.at(ti + 1).toInt();

        } else if (token == "compression_ratio") {
            state->compressionRatio = tokenList.at(1).toInt();
        } else {
            qDebug() << "Skipping unknown parameter";
        }
    }

    return true;
}

bool Knossos::printConfigValues() {
    qDebug() << QString("Configuration:\n\tExperiment:\n\t\tPath: %0\n\t\tName: %1\n\t\tBoundary (x): %2\n\t\tBoundary (y): %3\n\t\tBoundary (z): %4\n\t\tScale (x): %5\n\t\tScale (y): %6\n\t\tScale (z): %7\n\n\tData:\n\t\tCube bytes: %8\n\t\tCube edge length: %9\n\t\tCube slice area: %10\n\t\tM (cube set edge length): %11\n\t\tCube set elements: %12\n\t\tCube set bytes: %13\n\t\tZ-first cube order: %14\n")
               .arg(state->path)
               .arg(state->name)
               .arg(state->boundary.x)
               .arg(state->boundary.y)
               .arg(state->boundary.z)
               .arg(state->scale.x)
               .arg(state->scale.y)
               .arg(state->scale.z)
               .arg(state->cubeBytes)
               .arg(state->cubeEdgeLength)
               .arg(state->cubeSliceArea)
               .arg(state->M)
               .arg(state->cubeSetElements)
               .arg(state->cubeSetBytes);
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

stateInfo *Knossos::emptyState() {
    stateInfo *state = new stateInfo();

    state->viewerState = new viewerState();

    state->skeletonState = new skeletonState();
    state->taskState = new taskState();
    return state;
}

bool Knossos::findAndRegisterAvailableDatasets() {
    QDir datasetDir;
    QString baseMagDirName;
    bool isMultiresCompatible = false;
    if (LM_FTP == state->loadMode) {
        isMultiresCompatible = true;
    }
    else {
        // dataset is multires-compatible if a mag subfolder exists.
        datasetDir.setPath(state->path);
        QStringList dirContent = datasetDir.entryList(QStringList("*mag*"), QDir::Dirs);
        isMultiresCompatible = dirContent.empty() == false;
    }

    if(isMultiresCompatible) {
        if(LM_LOCAL == state->loadMode) {
            // retrieve potential basename at the beginning of the mag subfolders,
            // e.g. folder "xxx_mag1" with "xxx" being the basename
            QStringList dirContent = datasetDir.entryList(QStringList("*mag*"), QDir::Dirs);
            baseMagDirName = dirContent.front();
            int magPart = baseMagDirName.lastIndexOf(QRegExp("mag.*"));
            baseMagDirName.remove(magPart, baseMagDirName.length() - magPart);
        }

        // iterate over all possible mags and test their availability
        // (available if corresponding .conf file exists)
        for(uint currMag = 1; currMag <= NUM_MAG_DATASETS; currMag *= 2) {
            int currMagExists = false;
            QString currentPath;
            if(LM_FTP == state->loadMode) {
                currentPath = QString("%1mag%2/knossos.conf").arg(state->ftpBasePath).arg(currMag);
                if (EXIT_SUCCESS == downloadFile(currentPath.toStdString().c_str(), NULL)) {
                    currMagExists = true;
                }
            }
            else {
                currentPath = QString("%1/%2%3%4/").arg(state->path).arg(baseMagDirName).arg("mag").arg(currMag);
                QDir currentMagDir(currentPath);
                if(currentMagDir.entryList(QStringList("*.conf")).empty() == false) {
                    currMagExists = true;
                }
            }

            if(currMagExists) {
                if(state->lowestAvailableMag > currMag) {
                    state->lowestAvailableMag = currMag;
                }
                state->highestAvailableMag = currMag;
            }
        }
        qDebug("lowest Mag: %d, Highest Mag: %d", state->lowestAvailableMag, state->highestAvailableMag);

        if(state->highestAvailableMag > NUM_MAG_DATASETS) {
            state->highestAvailableMag = NUM_MAG_DATASETS;
            qDebug("KNOSSOS currently supports only datasets downsampled by a factor of %d.\
                   This can easily be changed in the source.", NUM_MAG_DATASETS);
        }
    }
    // no magstring found, take mag read from .conf file of dataset
    if(isMultiresCompatible == false) {
        int pathLen = strlen(state->path);
        if(!pathLen) {
            qDebug() << "No valid dataset specified.\n";
            return false;
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

        state->lowestAvailableMag = state->magnification;
        state->highestAvailableMag = state->magnification;

        state->boundary.x *= state->magnification;
        state->boundary.y *= state->magnification;
        state->boundary.z *= state->magnification;

        state->scale.x /= (float)state->magnification;
        state->scale.y /= (float)state->magnification;
        state->scale.z /= (float)state->magnification;
    }

    return true;
}

bool Knossos::configDefaults() {
    bool firstRun = false;
    if (nullptr == state) {
        firstRun = true;
    }

    if (firstRun) {
        state = Knossos::emptyState();

        state->loaderUserMoveType = USERMOVE_NEUTRAL;
        SET_COORDINATE(state->loaderUserMoveViewportDirection, 0, 0, 0);
        state->remoteSignal = false;
        state->quitSignal = false;
        state->conditionRemoteSignal = new QWaitCondition();
        state->protectRemoteSignal = new QMutex();
        state->protectCube2Pointer = new QMutex();
    }

    state->path[0] = '\0';
    state->name[0] = '\0';

    // General stuff
    state->boundary.x = 1000;
    state->boundary.y = 1000;
    state->boundary.z = 1000;
    state->scale.x = 1.;
    state->scale.y = 1.;
    state->scale.z = 1.;
    state->cubeEdgeLength = 128;
    state->M = 0;//invalid M, so the datasetLoadWidget can tell if M was provided by cmdline
    state->magnification = 1;
    state->lowestAvailableMag = 1;
    state->highestAvailableMag = 1;
    state->overlay = true;

    // For the viewer
    state->viewerState->highlightVp = VIEWPORT_UNDEFINED;
    state->viewerState->vpKeyDirection[VIEWPORT_XY] = 1;
    state->viewerState->vpKeyDirection[VIEWPORT_XZ] = 1;
    state->viewerState->vpKeyDirection[VIEWPORT_YZ] = 1;
    state->viewerState->datasetColortableOn = false;
    state->viewerState->datasetAdjustmentOn = false;
    state->viewerState->treeColortableOn = false;
    state->viewerState->viewerReady = false;
    state->viewerState->drawVPCrosshairs = true;
    state->viewerState->showVPLabels = false;
    state->viewerState->stepsPerSec = 40;
    state->viewerState->dropFrames = 1;
    state->viewerState->walkFrames = 10;
    state->viewerState->nodeSelectSquareVpId = -1;
    SET_COORDINATE(state->viewerState->nodeSelectionSquare.first, 0, 0, 0);
    SET_COORDINATE(state->viewerState->nodeSelectionSquare.second, 0, 0, 0);

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

    if (firstRun) {
        state->viewerState->vpConfigs = (vpConfig *) malloc(Viewport::numberViewports * sizeof(struct vpConfig));
        if(state->viewerState->vpConfigs == NULL) {
            qDebug() << "Out of memory.";
            return false;
        }
        memset(state->viewerState->vpConfigs, '\0', Viewport::numberViewports * sizeof(struct vpConfig));
        for(uint i = 0; i < Viewport::numberViewports; i++) {
            switch(i) {
            case VP_UPPERLEFT:
                state->viewerState->vpConfigs[i].type = VIEWPORT_XY;
                SET_COORDINATE(state->viewerState->vpConfigs[i].upperLeftCorner, 5, 30, 0);
                state->viewerState->vpConfigs[i].id = VP_UPPERLEFT;
                break;
            case VP_LOWERLEFT:
                state->viewerState->vpConfigs[i].type = VIEWPORT_XZ;
                SET_COORDINATE(state->viewerState->vpConfigs[i].upperLeftCorner, 5, 385, 0);
                state->viewerState->vpConfigs[i].id = VP_LOWERLEFT;
                break;
            case VP_UPPERRIGHT:
                state->viewerState->vpConfigs[i].type = VIEWPORT_YZ;
                SET_COORDINATE(state->viewerState->vpConfigs[i].upperLeftCorner, 360, 30, 0);
                state->viewerState->vpConfigs[i].id = VP_UPPERRIGHT;
                break;
            case VP_LOWERRIGHT:
                state->viewerState->vpConfigs[i].type = VIEWPORT_SKELETON;
                SET_COORDINATE(state->viewerState->vpConfigs[i].upperLeftCorner, 360, 385, 0);
                state->viewerState->vpConfigs[i].id = VP_LOWERRIGHT;
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
    }


    // For the skeletonizer
    state->skeletonState->lockRadius = 100;
    state->skeletonState->lockPositions = false;

    strncpy(state->skeletonState->onCommentLock, "seed", 1024);
    state->skeletonState->branchpointUnresolved = false;
    state->skeletonState->autoFilenameIncrementBool = true;
    state->skeletonState->autoSaveBool = true;
    state->skeletonState->autoSaveInterval = 5;
    state->skeletonState->skeletonTime = 0;
    state->skeletonState->definedSkeletonVpView = -1;

    state->loadMode = LM_LOCAL;
    state->compressionRatio = 0;

    state->keyD = state->keyF = false;
    state->repeatDirection = {{}};
    state->viewerKeyRepeat = false;

    return true;

}

bool Knossos::readDataConfAndLocalConf() {

    int length;
    char configFile[1024];

    memset(configFile, '\0', 1024);
    length = strlen(state->path);

    if(length >= 1010) {
        // We need to append "/knossos.conf"
        qDebug() << "Data path too long.";
        _Exit(false);
    }

    strcat(configFile, state->path);
    strcat(configFile, "/knossos.conf");


    qDebug("Trying to read %s.", configFile);
    readConfigFile(configFile);

    readConfigFile("knossos.conf");

    return true;
}

bool Knossos::configFromCli(int argCount, char *arguments[]) {

 #define NUM_PARAMS 14

 char *lval = NULL, *rval = NULL;
 char *equals = NULL;
 int i = 0, j = 0, llen = 0;
 const char *lvals[NUM_PARAMS] = {
                             "--data-path",       // 0  Do not forget
                             "--connect-asap",    // 1  to also modify, client is no more
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
                 qDebug() << "Out of memory.";
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
         qDebug() << "Out of memory.";
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
                    qDebug() << "switch not applicable anymore";
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
                     state->overlay = std::stoi(rval);
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
