/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "buildinfo.h"
#include "coordinate.h"
#include "dataset.h"
#include "loader.h"
#include "network.h"
#include "scriptengine/scripting.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/mainwindow.h"
#include "widgets/viewports/viewportbase.h"

#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QScreen>
#include <QSplashScreen>
#include <QSslSocket>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTextCodec>
#include <QtConcurrentRun>
#include <QTimer>

#include <boost/stacktrace.hpp>
#include <boost/leaf.hpp>

#include <exception>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

// obsolete with CMAKE_AUTOSTATICPLUGINS in msys2
//#if defined(Q_OS_WIN) && defined(QT_STATIC)
//#include <QtPlugin>
//Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
//#endif

class Splash : public QSplashScreen {
public:
    Splash(const QString & img_filename) : QSplashScreen(QPixmap(img_filename)) {
#ifdef Q_OS_WIN// http://lists.qt-project.org/pipermail/qt-interest-old/2010-May/023534.html
        // give splash a task bar icon, this is especially important if a message box blocks startup
        const int exstyle = GetWindowLong(reinterpret_cast<HWND>(winId()), GWL_EXSTYLE);
        SetWindowLong(reinterpret_cast<HWND>(winId()), GWL_EXSTYLE, exstyle & ~WS_EX_TOOLWINDOW);
#endif
        show();
    }
};

struct MessageHermit {
    const QString title, info, detail{};
    QString formattedOutput{};
    QDebug formatter{QDebug(&formattedOutput) << "qApp:" << qApp};
    int fake_argc{0};
    char * fake_argv[1]{};
    const QApplication & a{qApp ? *qApp : static_cast<const QApplication&>(QApplication{fake_argc, fake_argv})};
    QMessageBox box{nullptr};
    void run(bool exec = true) {
        std::cerr << title.toStdString() << std::endl;
        std::cerr << info.toStdString() << std::endl;
        std::cerr << detail.toStdString() << std::endl;
        formatter << "→ app:" << &a << "→ qApp:" << qApp;
        std::cerr << formattedOutput.toStdString() << std::endl;
        qDebug() << title;
        qDebug() << info;
        qDebug().noquote() << detail;
        qDebug().noquote() << formattedOutput;
        if (!exec) {
            return;
        }
        box.setIcon(QMessageBox::Critical);
        box.setText(title);
        box.setInformativeText(info);
        box.setDetailedText(detail);
        box.exec();
    }
};

void debugMessageHandler(QtMsgType type, const QMessageLogContext &
#ifdef QT_MESSAGELOGCONTEXT
    context
#endif
    , const QString & msg)
{
    QString intro;
    switch (type) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    case QtInfoMsg:     intro = QString("Info: ");     break;
#endif
    case QtDebugMsg:    intro = QString();    break;// no intro spam
    case QtWarningMsg:  intro = QString("Warning: ");  break;
    case QtCriticalMsg: intro = QString("Critical: "); break;
    case QtFatalMsg:    intro = QString("Fatal: ");    break;
    }
    auto txt = QString("%4%5").arg(intro).arg(msg);
#ifdef QT_MESSAGELOGCONTEXT
    if (context.file && context.line) {
        txt.prepend(QString("[%1:%2] \t").arg(QFileInfo(context.file).fileName()).arg(context.line));
    }
#endif
    // open the file once
    static std::ofstream outFile(QStandardPaths::writableLocation(QStandardPaths::DataLocation).toStdString()+"/log.txt", std::ios_base::app);
    static int64_t file_debug_output_limit = 5000;
    if (type != QtDebugMsg || --file_debug_output_limit > 0) {
        outFile << txt.toStdString() << std::endl;
    }
    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
        std::cerr << txt.toStdString() << std::endl;
    } else {
        std::cout << txt.toStdString() << std::endl;
    }

    if (type == QtFatalMsg) {
        MessageHermit{"EXTERMINATE!!!", "got a QtFatalMsg", txt}.run();
        std::abort();
    }
}

Q_DECLARE_METATYPE(std::string)

int main(int argc, char * argv[]) { boost::leaf::try_handle_all([argc, &argv]() mutable -> boost::leaf::result<int> {
#ifdef _GLIBCXX_DEBUG
    std::cerr << "_GLIBCXX_DEBUG set → debug stdlib in use, which might result in crashes from mismatching ABI" << std::endl;
#endif
#ifdef _GLIBCXX_DEBUG_PEDANTIC
    std::cerr << "_GLIBCXX_DEBUG_PEDANTIC set" << std::endl;
#endif
    QCoreApplication::setOrganizationDomain("knossos.app");
    QCoreApplication::setOrganizationName("MPIN");
    QCoreApplication::setApplicationName("KNOSSOS");
    QCoreApplication::setApplicationVersion(KREVISION);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QFile::copy(QSettings{QSettings::IniFormat, QSettings::UserScope, "MPIN", "KNOSSOS 5.0"}.fileName(), QSettings{}.fileName());// doesn’t overwrite

    qInstallMessageHandler(debugMessageHandler);// needs application to be set for log path
    qDebug() << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toUtf8().constData();
    qDebug() << KREVISION << " " << KREVISIONDATE;
    qDebug() << QSysInfo::prettyProductName() << QSysInfo::productType() << QSysInfo::productVersion();
    qDebug() << QSysInfo::kernelType() << QSysInfo::kernelVersion() << QSysInfo::buildAbi() << QSysInfo::buildCpuArchitecture() << QSysInfo::currentCpuArchitecture();
    qDebug() << QSysInfo::machineHostName();
    QtConcurrent::run([](){
        qDebug() << "QSslSocket::supportsSsl()" << QSslSocket::supportsSsl();// workaround until https://bugreports.qt.io/browse/QTBUG-59750
        qDebug() << "sslLibrary   BuildVersion" << QSslSocket::sslLibraryBuildVersionString() << QSslSocket::sslLibraryBuildVersionNumber();
        qDebug() << "sslLibrary RuntimeVersion" << QSslSocket::sslLibraryVersionString() << QSslSocket::sslLibraryVersionNumber();
    });
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);// explicitly enable sharing for undocked viewports

    QSurfaceFormat format{QSurfaceFormat::defaultFormat()};
    format.setVersion(2, 0);
    format.setDepthBufferSize(24);
    format.setSamples(8);// set it here to the most common value (the default) so it doesn’t complain unnecessarily later
//    format.setSwapInterval(0);
//    format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setOption(QSurfaceFormat::DeprecatedFunctions);
    if (ViewportBase::oglDebug) {
        format.setOption(QSurfaceFormat::DebugContext);
    }
    QSurfaceFormat::setDefaultFormat(format);
#ifdef Q_OS_OSX
    const auto end = std::next(argv, argc);
    bool styleOverwrite = std::find_if(argv, end, [](const auto & argvv){
        return QString::fromUtf8(argvv).contains("-style");
    }) != end;
#endif
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QApplication app(argc, argv);
    if (argc == 2 && std::string(argv[1]) == "exit") {
        QTimer::singleShot(5000, &app, [](){
            if (::state->scripting) {
                qDebug() << QDir::currentPath();
                for (const auto script : {"import ssl", "from PythonQt import QtGui", "from PythonQt.QtGui import QPushButton"}) {
                    qDebug() << "testing" << script;
                    ::state->scripting->evalScript(script, 257);
                }
            }
            Loader::Controller::singleton().suspendLoader();
            Loader::Controller::singleton().worker.reset();// have to make really sure loader is done
            QCoreApplication::quit();
        });
    }

    QFile file(":/resources/style.qss");
    file.open(QFile::ReadOnly);
    app.setStyleSheet(file.readAll());
    file.close();
#ifdef Q_OS_OSX
    if (!styleOverwrite) {// default to Fusion style on OSX if nothing contrary was specified (because the default theme looks bad)
        QApplication::setStyle(QStyleFactory::create("Fusion"));
    }
#endif
#ifdef NDEBUG
    Splash splash(qApp->primaryScreen()->devicePixelRatio() == 1.0 ? ":/resources/splash.png" : ":/resources/splash@2x.png");
#endif
    qRegisterMetaType<std::string>();
    qRegisterMetaType<Coordinate>();
    qRegisterMetaType<CoordOfCube>();
    qRegisterMetaType<Dataset>("Dataset");
    qRegisterMetaType<Dataset::list_t>("Dataset::list_t");
    qRegisterMetaType<floatCoordinate>();
    qRegisterMetaType<SnapshotOptions>();
    qRegisterMetaType<UserMoveType>();
    qRegisterMetaType<ViewportType>();

    stateInfo state;
    ::state = &state;

    SignalRelay signalRelay;
    Scripting scripts;
    Viewer viewer;
    QTimer::singleShot(0, [&](){// get into the event loop first
        state.mainWindow->loadSettings();// load settings after viewer and window are accessible through state and viewer
        state.mainWindow->widgetContainer.datasetLoadWidget.loadDataset();// load last used dataset or show
        viewer.timer.start(0);
        if (!(argc == 2 && std::string(argv[1]) == "exit")) {
#ifdef NDEBUG
            splash.finish(state.mainWindow);
#endif
        }
    });
    // ensure killed QNAM’s before QNetwork deinitializes
    std::unique_ptr<Loader::Controller> loader_deleter{&Loader::Controller::singleton()};
    std::unique_ptr<Network> network_deleter{&Network::singleton()};
    return app.exec();
},
    [argc, &argv](const boost::leaf::verbose_diagnostic_info & info) -> int {
        const auto text = QObject::tr("KNOSSOS will terminate due to a problem.    ");// a little padding to increase the mbox width
        const auto details = (std::ostringstream{} << boost::stacktrace::stacktrace{}).str();
        MessageHermit{text, QString::fromStdString((std::ostringstream{} << info).str()), QString::fromStdString(details)}
            .run(argc != 2 || std::string(argv[1]) != "exit");
        throw;
    }
);
}
