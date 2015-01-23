#include <QSettings>
#include <QDir>
#include <QFileInfoList>
#include <QToolBar>
#include <QMenu>

#include "widgets/GuiConstants.h"
#include "scripting.h"
#include "decorators/floatcoordinatedecorator.h"
#include "decorators/coordinatedecorator.h"
#include "decorators/colordecorator.h"
#include "decorators/treelistdecorator.h"
#include "decorators/nodelistdecorator.h"
#include "decorators/nodecommentdecorator.h"
#include "decorators/segmentlistdecorator.h"
#include "decorators/meshdecorator.h"
#include "decorators/transformdecorator.h"
#include "decorators/pointdecorator.h"
#include "proxies/skeletonproxy.h"

#include "geometry/render.h"
#include "geometry/point.h"
#include "geometry/transform.h"
#include "geometry/shape.h"

#include "highlighter.h"
#include "knossos-global.h"
#include "widgets/mainwindow.h"

void PythonQtInit() {
    PythonQt::init(PythonQt::RedirectStdOut);
    PythonQt_QtAll::init();
}

Scripting::Scripting() {
    PythonQtInit();
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    skeletonProxy = new SkeletonProxy();

    colorDecorator = new ColorDecorator();
    coordinateDecorator = new CoordinateDecorator();
    floatCoordinateDecorator = new FloatCoordinateDecorator();
    meshDecorator = new MeshDecorator();
    nodeListDecorator = new NodeListDecorator();
    nodeCommentDecorator = new NodeCommentDecorator();
    segmentListDecorator = new SegmentListDecorator();
    treeListDecorator = new TreeListDecorator();
//    transformDecorator = new TransformDecorator();
//    pointDecorator = new PointDecorator();

    ctx.evalScript("import sys");
    ctx.evalScript("sys.argv = ['']");  // <- this is needed to import the ipython module from the site-package
#ifdef Q_OS_OSX
    // as ipython does not export it's sys paths after the installation we refer to that site-package
    ctx.evalScript("sys.path.append('/Library/Python/2.7/site-packages')");
#endif
    ctx.evalScript("plugin_container = []");

    ctx.addObject("knossos", skeletonProxy);
    ctx.addVariable("GL_POINTS", GL_POINTS);
    ctx.addVariable("GL_LINES", GL_LINES);
    ctx.addVariable("GL_LINE_STRIP", GL_LINE_STRIP);
    ctx.addVariable("GL_LINE_LOOP", GL_LINE_LOOP);
    ctx.addVariable("GL_TRIANGLES", GL_TRIANGLES);
    ctx.addVariable("GL_TRIANGLES_STRIP", GL_TRIANGLE_STRIP);
    ctx.addVariable("GL_TRIANGLE_FAN", GL_TRIANGLE_FAN);
    ctx.addVariable("GL_QUADS", GL_QUADS);
    ctx.addVariable("GL_QUAD_STRIP", GL_QUAD_STRIP);
    ctx.addVariable("GL_POLYGON", GL_POLYGON);
    addWidgets(ctx);

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

#ifdef Q_OS_LINUX //in linux there’s an explicit symlink to a python 2 binary
    ctx.evalFile(QString("sys.path.append('%1')").arg("./python2"));
#else
    ctx.evalFile(QString("sys.path.append('%1')").arg("./python"));
#endif
//    ctx.evalScript("import IPython");
//    ctx.evalScript("IPython.embed_kernel()");
}

void Scripting::addScriptingObject(const QString &name, QObject *obj) {
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
    ctx.addObject(name, obj);
}

void Scripting::saveSettings(const QString &key, const QVariant &value) {
    settings->setValue(key, value);
}

void Scripting::changeWorkingDirectory() {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    QString path = settings.value(PYTHON_WORKING_DIRECTORY).toString();
    settings.endGroup();

    if(!path.isEmpty()) {
        PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
        ctx.evalScript("import os");
        ctx.evalScript(QString("os.chdir('%1')").arg(path));
    }
}


void Scripting::executeFromUserDirectory() {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    QString path = settings.value(PYTHON_AUTOSTART_FOLDER).toString();
    settings.endGroup();

    if(!path.isEmpty()) {
        qDebug() << path;
        QDir scriptDir(path);
        QStringList endings;
        endings << "*.py";
        scriptDir.setNameFilters(endings);
        QFileInfoList entries = scriptDir.entryInfoList();

        PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
        foreach(const QFileInfo &script, entries) {
            QFile file(script.canonicalFilePath());

            if(!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
                continue;
            }

            ctx.evalFile(script.canonicalFilePath());

        }
    }
}

/** This methods create a pep8-style object name for all knossos widget and
 *  adds them to the python-context. Widgets with a leading Q are ignored
*/
void Scripting::addWidgets(PythonQtObjectPtr &context) {
    QWidgetList list = QApplication::allWidgets();
    foreach(QWidget *widget, list) {
        QString name = widget->metaObject()->className();
        QByteArray array;

        for(int i = 0; i < name.size(); i++) {
            if(name.at(i).isLower()) {
                array.append(name.at(i));
            } else if(name.at(i).isUpper()) {
                if(i == 0 and name.at(i) == 'Q') {
                    continue;
                } else if(i == 0 and name.at(i) != 'Q') {
                    array.append(name.at(i).toLower());
                } else {
                    array.append(QString("_%1").arg(name.at(i).toLower()));
                }
            }
        }

        context.addObject(QString(array), widget);
    }
}
