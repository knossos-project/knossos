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

#include "dataset.h"
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
#include "widgets/viewport.h"

#include <QApplication>
#include <QFileInfo>
#include <QSplashScreen>
#include <QStandardPaths>


#include <iostream>
#include <fstream>

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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    case QtInfoMsg:     intro = QString("Info: ");     break;
#endif
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

    stateInfo state;
    ::state = &state;
    Dataset::dummyDataset().applyToState();

    SignalRelay signalRelay;
    Viewer viewer;
    Remote remote;

    qRegisterMetaType<std::string>();
    qRegisterMetaType<Coordinate>();
    qRegisterMetaType<CoordOfCube>();
    qRegisterMetaType<floatCoordinate>();

    QObject::connect(viewer.skeletonizer, &Skeletonizer::setRecenteringPositionSignal, &remote, &Remote::setRecenteringPosition);
    viewer.window->forEachVPDo( [&remote](ViewportBase & vp) {
        QObject::connect(&vp, &ViewportBase::setRecenteringPositionSignal, &remote, &Remote::setRecenteringPosition);
        QObject::connect(&vp, &ViewportBase::setRecenteringPositionWithRotationSignal, &remote, &Remote::setRecenteringPositionWithRotation);
    });

    QObject::connect(&Segmentation::singleton(), &Segmentation::setRecenteringPositionSignal, &remote, &Remote::setRecenteringPosition);

    qRegisterMetaType<UserMoveType>();
    qRegisterMetaType<ViewportType>();
    QObject::connect(&remote, &Remote::userMoveSignal, &viewer, &Viewer::userMove);
    QObject::connect(&remote, &Remote::rotationSignal, &viewer, &Viewer::setRotation);

    Scripting scripts;
    viewer.run();
    remote.start();

    viewer.window->widgetContainer.datasetLoadWidget.loadDataset();

    viewer.window->widgetContainer.datasetOptionsWidget.updateCompressionRatioDisplay();

    QObject::connect(pythonProxySignalDelegate, &PythonProxySignalDelegate::userMoveSignal, &remote, &Remote::remoteJump);
    QObject::connect(skeletonProxySignalDelegate, &SkeletonProxySignalDelegate::loadSkeleton, &annotationFileLoad);
    QObject::connect(skeletonProxySignalDelegate, &SkeletonProxySignalDelegate::saveSkeleton, &annotationFileSave);
    QObject::connect(skeletonProxySignalDelegate, &SkeletonProxySignalDelegate::clearSkeletonSignal, viewer.window, &MainWindow::clearSkeletonSlotNoGUI);

    return a.exec();
}

void Knossos::sendRemoteSignal() {
    state->protectRemoteSignal.lock();
    state->remoteSignal = true;
    state->protectRemoteSignal.unlock();

    state->conditionRemoteSignal.wakeOne();
}

void Knossos::sendQuitSignal() {
    state->quitSignal = true;
    QApplication::processEvents(); //ensure everything’s done
    Loader::Controller::singleton().suspendLoader();
    Knossos::sendRemoteSignal();
}
