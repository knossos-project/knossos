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
#include "knossos.h"

#include "eventmodel.h"
#include "file_io.h"
#include "loader.h"
#include "network.h"
#include "remote.h"
#include "scriptengine/proxies/skeletonproxy.h"
#include "scriptengine/proxies/pythonproxy.h"
#include "scriptengine/scripting.h"
#include "skeleton/skeletonizer.h"
#include "version.h"
#include "viewer.h"
#include "widgets/mainwindow.h"
#include "widgets/widgetcontainer.h"

#include <QApplication>
#include <QFileInfo>

#include <cmath>
#include <iostream>
#include <fstream>

#define NUMTHREADS 4

#if defined(Q_OS_WIN) && defined(QT_STATIC)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

class Splash {
    QSplashScreen screen;
public:
    Splash(const QString & img_filename, const int timeout_msec) : screen(QPixmap(img_filename), Qt::WindowStaysOnTopHint) {
        screen.show();
        //the splashscreen is hidden after a timeout, it could also wait for the mainwindow
        QTimer::singleShot(timeout_msec, &screen, &QSplashScreen::close);
    }
};

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
    static int64_t file_debug_output_limit = 5000;
    if (type != QtDebugMsg || --file_debug_output_limit > 0) {
        outFile   << txt.toStdString() << std::endl;
    }
    std::cout << txt.toStdString() << std::endl;

    if (type == QtFatalMsg) {
        std::abort();
    }
}

Q_DECLARE_METATYPE(std::string)

int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);//explicitly enable sharing for undocked viewports
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
    QCoreApplication::setApplicationName(QString("KNOSSOS %1").arg(KVERSION));
    QSettings::setDefaultFormat(QSettings::IniFormat);

    Knossos::configDefaults();

    SignalRelay signalRelay;
    Viewer viewer;
    Remote remote;

    qRegisterMetaType<std::string>();
    qRegisterMetaType<Coordinate>();
    qRegisterMetaType<CoordOfCube>();
    qRegisterMetaType<floatCoordinate>();

    QObject::connect(viewer.skeletonizer, &Skeletonizer::setRecenteringPositionSignal, &remote, &Remote::setRecenteringPosition);

    QObject::connect(viewer.eventModel, &EventModel::setRecenteringPositionSignal, &remote, &Remote::setRecenteringPosition);
    QObject::connect(viewer.eventModel, &EventModel::setRecenteringPositionWithRotationSignal, &remote, &Remote::setRecenteringPositionWithRotation);

    QObject::connect(&Segmentation::singleton(), &Segmentation::setRecenteringPositionSignal, &remote, &Remote::setRecenteringPosition);

    qRegisterMetaType<UserMoveType>();
    qRegisterMetaType<ViewportType>();
    QObject::connect(&remote, &Remote::userMoveSignal, &viewer, &Viewer::userMove);
    QObject::connect(&remote, &Remote::updateViewerStateSignal, &viewer, &Viewer::updateViewerState);
    QObject::connect(&remote, &Remote::rotationSignal, &viewer, &Viewer::setRotation);

    Knossos::loadDefaultTreeLUT();

    Scripting scripts;
    viewer.run();
    remote.start();

    viewer.window->widgetContainer->datasetLoadWidget->loadDataset();

    viewer.window->widgetContainer->datasetOptionsWidget->updateCompressionRatioDisplay();

    QObject::connect(pythonProxySignalDelegate, &PythonProxySignalDelegate::userMoveSignal, &remote, &Remote::remoteJump);
    QObject::connect(skeletonProxySignalDelegate, &SkeletonProxySignalDelegate::loadSkeleton, &annotationFileLoad);
    QObject::connect(skeletonProxySignalDelegate, &SkeletonProxySignalDelegate::saveSkeleton, &annotationFileSave);
    QObject::connect(skeletonProxySignalDelegate, &SkeletonProxySignalDelegate::clearSkeletonSignal, viewer.window, &MainWindow::clearSkeletonSlotNoGUI);

    return a.exec();
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
                currentPath = QString("http://%1:%2@%3/%4/mag%5/knossos.conf").arg(state->ftpUsername).arg(state->ftpPassword).arg(state->ftpHostName).arg(state->ftpBasePath).arg(currMag);
                if (Network::singleton().refresh(currentPath).first) {
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
    }
    state->boundary.x *= state->magnification;
    state->boundary.y *= state->magnification;
    state->boundary.z *= state->magnification;

    state->scale.x /= (float)state->magnification;
    state->scale.y /= (float)state->magnification;
    state->scale.z /= (float)state->magnification;
    // update the volume boundary
    if((state->boundary.x >= state->boundary.y) && (state->boundary.x >= state->boundary.z)) {
        state->skeletonState->volBoundary = state->boundary.x * 2;
    }
    if((state->boundary.y >= state->boundary.x) && (state->boundary.y >= state->boundary.z)) {
        state->skeletonState->volBoundary = state->boundary.y * 2;
    }
    if((state->boundary.z >= state->boundary.x) && (state->boundary.z >= state->boundary.y)) {
        state->skeletonState->volBoundary = state->boundary.z * 2;
    }

    return true;
}

stateInfo * emptyState() {
    stateInfo *state = new stateInfo();
    state->viewerState = new viewerState();
    state->skeletonState = new skeletonState();
    return state;
}

bool Knossos::configDefaults() {
    bool firstRun = false;
    if (nullptr == state) {
        firstRun = true;
    }

    if (firstRun) {
        state = emptyState();

        state->loaderUserMoveType = USERMOVE_NEUTRAL;
        state->loaderUserMoveViewportDirection = {};
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
    state->viewerState->showScalebar = false;
    state->viewerState->stepsPerSec = 40;
    state->viewerState->dropFrames = 1;
    state->viewerState->walkFrames = 10;
    state->viewerState->nodeSelectSquareVpId = -1;
    state->viewerState->nodeSelectionSquare.first = {};
    state->viewerState->nodeSelectionSquare.second = {};

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

    // For the skeletonizer
    state->skeletonState->lockRadius = 100;
    state->skeletonState->lockPositions = false;

    strncpy(state->skeletonState->onCommentLock, "seed", 1024);
    state->skeletonState->branchpointUnresolved = false;
    state->skeletonState->definedSkeletonVpView = -1;

    state->loadMode = LM_LOCAL;
    state->compressionRatio = 0;

    state->keyD = state->keyF = false;
    state->repeatDirection = {{}};
    state->viewerKeyRepeat = false;

    return true;

}

void Knossos::loadDefaultTreeLUT() {
    if (!state->viewer->loadTreeColorTable("default.lut", &(state->viewerState->defaultTreeTable[0]), GL_RGB)) {
        state->viewer->loadTreeColorTable(":/resources/color_palette/default.json", &(state->viewerState->defaultTreeTable[0]), GL_RGB);
        state->viewer->window->treeColorAdjustmentsChanged();
    }
}
