#include <QSettings>
#include <QDir>
#include <QFileInfoList>
#include <QToolBar>
#include <QMenu>

#include "scripting.h"
#include "widgets/GuiConstants.h"
#include "decorators/floatcoordinatedecorator.h"
#include "decorators/coordinatedecorator.h"
#include "decorators/colordecorator.h"
#include "decorators/treelistdecorator.h"
#include "decorators/nodelistdecorator.h"
#include "decorators/nodecommentdecorator.h"
#include "decorators/segmentlistdecorator.h"
#include "decorators/meshdecorator.h"
#include "proxies/pythonproxy.h"
#include "proxies/segmentationproxy.h"
#include "proxies/skeletonproxy.h"

#include "eventmodel.h"
#include "highlighter.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

void PythonQtInit() {
    PythonQt::init(PythonQt::RedirectStdOut);
#ifdef QtAll
    PythonQt_QtAll::init();
#endif
}

Scripting::Scripting() : _ctx(NULL) {
    state->scripting = this;

    PythonQtInit();
    _ctx = PythonQt::self()->getMainModule();

    skeletonProxy = new SkeletonProxy();
    segmentationProxy = new SegmentationProxy();
    pythonProxy = new PythonProxy();
    PythonQt::self()->registerClass(&EmitOnCtorDtor::staticMetaObject);

    colorDecorator = new ColorDecorator();
    coordinateDecorator = new CoordinateDecorator();
    floatCoordinateDecorator = new FloatCoordinateDecorator();
    meshDecorator = new MeshDecorator();
    nodeListDecorator = new NodeListDecorator();
    nodeCommentDecorator = new NodeCommentDecorator();
    segmentListDecorator = new SegmentListDecorator();
    treeListDecorator = new TreeListDecorator();

    _ctx.evalScript("import sys");
    _ctx.evalScript("sys.argv = ['']");  // <- this is needed to import the ipython module from the site-package
#ifdef Q_OS_OSX
    // as ipython does not export it's sys paths after the installation we refer to that site-package
    _ctx.evalScript("sys.path.append('/Library/Python/2.7/site-packages')");
#endif
    _ctx.evalScript("plugin_container = []");

    _ctx.addObject("signalRelay", state->signalRelay);
    _ctx.addObject("knossos", pythonProxy);
    _ctx.addObject("segmentation", segmentationProxy);
    _ctx.addObject("skeleton", skeletonProxy);
    _ctx.addObject("knossos_global_viewer", state->viewer);
    _ctx.addObject("knossos_global_mainwindow", state->viewer->window);
    _ctx.addObject("knossos_global_eventmodel", state->viewer->eventModel);
    _ctx.addObject("knossos_global_skeletonizer", &Skeletonizer::singleton());
    _ctx.addObject("knossos_global_segmentation", &Segmentation::singleton());
    _ctx.addObject("knossos_global_loader", &Loader::Controller::singleton());
    _ctx.addVariable("GL_POINTS", GL_POINTS);
    _ctx.addVariable("GL_LINES", GL_LINES);
    _ctx.addVariable("GL_LINE_STRIP", GL_LINE_STRIP);
    _ctx.addVariable("GL_LINE_LOOP", GL_LINE_LOOP);
    _ctx.addVariable("GL_TRIANGLES", GL_TRIANGLES);
    _ctx.addVariable("GL_TRIANGLES_STRIP", GL_TRIANGLE_STRIP);
    _ctx.addVariable("GL_TRIANGLE_FAN", GL_TRIANGLE_FAN);
    _ctx.addVariable("GL_QUADS", GL_QUADS);
    _ctx.addVariable("GL_QUAD_STRIP", GL_QUAD_STRIP);
    _ctx.addVariable("GL_POLYGON", GL_POLYGON);
    addWidgets();

    QString module("internal");

    PythonQt::self()->addDecorators(colorDecorator);
    PythonQt::self()->registerCPPClass("color4F", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(coordinateDecorator);
    PythonQt::self()->registerCPPClass("Coordinate", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(floatCoordinateDecorator);
    PythonQt::self()->registerCPPClass("floatCoordinate", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(meshDecorator);
    PythonQt::self()->registerCPPClass("mesh", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(nodeListDecorator);
    PythonQt::self()->registerCPPClass("Node", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(nodeCommentDecorator);
    PythonQt::self()->registerCPPClass("NodeComment", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(segmentListDecorator);
    PythonQt::self()->registerCPPClass("Segment", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(treeListDecorator);
    PythonQt::self()->registerCPPClass("Tree", "", module.toLocal8Bit().data());

    changeWorkingDirectory();
    executeFromUserDirectory();
    addCustomPythonPath();

#ifdef Q_OS_LINUX //in linux there’s an explicit symlink to a python 2 binary
    _ctx.evalFile(QString("sys.path.append('%1')").arg("./python2"));
#else
    _ctx.evalFile(QString("sys.path.append('%1')").arg("./python"));
#endif

    autoStartTerminal();
}

QVariant getSettingsValue(const QString &key) {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    auto value = settings.value(key);
    settings.endGroup();
    return value;
}

void Scripting::autoStartTerminal() {
    auto value = getSettingsValue(PYTHON_AUTOSTART_TERMINAL);
    if (value.isNull()) { return; }
    auto autoStartFolder = value.toBool();
    if (autoStartFolder) {
        qDebug() << "TRUE!";
        state->viewer->window->widgetContainer->pythonPropertyWidget->openTerminal();
    }
}

void Scripting::changeWorkingDirectory() {
    auto value = getSettingsValue(PYTHON_WORKING_DIRECTORY);
    if (value.isNull()) { return; }
    auto workingDir = value.toString();
    if (workingDir.isEmpty()) { return; }

    _ctx.evalScript("import os");
    _ctx.evalScript(QString("os.chdir('%1')").arg(workingDir));
}

void Scripting::addCustomPythonPath() {
    auto value = getSettingsValue(PYTHON_CUSTOM_PATHS);
    if (value.isNull()) { return; }
    auto customPaths = value.toStringList();
    for (const auto & customPath : customPaths) {
        _ctx.evalScript(QString("sys.path.append('%1')").arg(customPath));
    }
}

void Scripting::executeFromUserDirectory() {
    auto value = getSettingsValue(PYTHON_AUTOSTART_FOLDER);
    if (value.isNull()) { return; }
    auto autoStartFolder = value.toString();
    if (autoStartFolder.isEmpty()) { return; }

    QDir scriptDir(autoStartFolder);
    QStringList endings;
    endings << "*.py";
    scriptDir.setNameFilters(endings);
    QFileInfoList entries = scriptDir.entryInfoList();
    foreach(const QFileInfo &script, entries) {
        QFile file(script.canonicalFilePath());
        if(!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
            continue;
        }
        _ctx.evalFile(script.canonicalFilePath());
    }
}

/** This methods create a pep8-style object name for all knossos widget and
 *  adds them to the python-context. Widgets with a leading Q are ignored
*/
void Scripting::addWidgets() {
    QWidgetList list = QApplication::allWidgets();
    foreach(QWidget *widget, list) {
        QString name = widget->metaObject()->className();
        QByteArray array;

        for(int i = 0; i < name.size(); i++) {
            if(name.at(i).isLower()) {
                array.append(name.at(i));
            } else if(name.at(i).isUpper()) {
                if(i == 0 && name.at(i) == 'Q') {
                    continue;
                } else if(i == 0 && name.at(i) != 'Q') {
                    array.append(name.at(i).toLower());
                } else {
                    array.append(QString("_%1").arg(name.at(i).toLower()));
                }
            }
        }

        _ctx.addObject("widget_" + QString(array), widget);
    }
}
