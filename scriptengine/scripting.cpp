#include <QSettings>
#include "scripting.h"
#include "decorators/floatcoordinatedecorator.h"
#include "decorators/coordinatedecorator.h"
#include "decorators/colordecorator.h"
#include "decorators/treelistdecorator.h"
#include "decorators/nodelistdecorator.h"
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


extern stateInfo *state;

Scripting::Scripting(QObject *parent) :
    QThread(parent)
{
    settings = new QSettings("python_api", "Knossos QT");

    floatCoordinateDecorator = new FloatCoordinateDecorator();
    coordinateDecorator = new CoordinateDecorator();
    colorDecorator = new ColorDecorator();
    treeListDecorator = new TreeListDecorator();
    nodeListDecorator = new NodeListDecorator();
    segmentListDecorator = new SegmentListDecorator();
    meshDecorator = new MeshDecorator();


    transformDecorator = new TransformDecorator();
    pointDecorator = new PointDecorator();
    skeletonProxy = new SkeletonProxy();

}


void Scripting::addDoc() {
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    ctx.evalScript("import knossos_python_api");
    ctx.evalScript("knossos_python_api.__doc__ = 'this is the root module of the knossos python api.'");

    ctx.evalScript("from knossos_python_api import *");
    ctx.evalScript("internal.__doc__ = 'This module contains internal data structures which are used in knossos. They are constrained to read-only access in python'");

    ctx.evalScript("import sys");
    ctx.evalScript("sys.path.append('/home/knossos/Schreibtisch')");
    ctx.evalScript("execfile('/home/knossos/Schreibtisch/render_mesh.py')");

    ctx.evalScript("import os");



    // here is simply the convention : Those keys are first available after pathes were saved.
    if(!settings->value("sys_path").toString().isNull()) {
        console->consoleMessage("loaded the following sys_pathes:");
        QStringList pathList;
        pathList = settings->value("sys_path").toStringList();
        foreach(const QString &path, pathList) {
           PythonQt::self()->addSysPath(settings->value("sys_path").toString());
           console->consoleMessage(path);
        }

    }

    if(!settings->value("working_directory").isNull()) {
        ctx.evalScript(QString("os.chdir(%1)").arg(settings->value("working_dir").toString()));
        console->consoleMessage(QString("changed working directory according to the saved path: %").arg(settings->value("working_dir").toString()));
    }

}

void Scripting::run() {

    QFont font("Courier");
    font.setPixelSize(12);    

    PythonQt::init(PythonQt::RedirectStdOut, QString("knossos_python_api").toLocal8Bit());
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    console = new PythonQtScriptingConsole(NULL, ctx);
    console->setWindowTitle("Knossos Scripting Console");
    highlighter = new Highlighter(console->document());

    Render render;


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


    QString module("internal");
    PythonQt::init(PythonQt::RedirectStdOut, module.toLocal8Bit());

    PythonQt::self()->addDecorators(floatCoordinateDecorator);
    PythonQt::self()->registerCPPClass("floatCoordinate", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(coordinateDecorator);
    PythonQt::self()->registerCPPClass("Coordinate", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(colorDecorator);
    PythonQt::self()->registerCPPClass("color4F", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(segmentListDecorator);
    PythonQt::self()->registerCPPClass("segmentListElement", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(treeListDecorator);
    PythonQt::self()->registerCPPClass("treeListElement", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(nodeListDecorator);
    PythonQt::self()->registerCPPClass("nodeListElement", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(meshDecorator);
    PythonQt::self()->registerCPPClass("mesh", "",  module.toLocal8Bit().data());

    QString renderModule("rendering");

    PythonQt::self()->addDecorators(pointDecorator);
    PythonQt::self()->registerCPPClass("Point", "", renderModule.toLocal8Bit().data());

    PythonQt::self()->addDecorators(transformDecorator);
    PythonQt::self()->registerCPPClass("Transform", "", renderModule.toLocal8Bit().data());

    addDoc();

    connect(signalDelegate, SIGNAL(echo(QString)), console, SLOT(consoleMessage(QString)));
    connect(signalDelegate, SIGNAL(saveSettingsSignal(QString,QVariant)), this, SLOT(saveSettings(QString,QVariant)));

    console->resize(800, 300);
    console->setFont(font);
    console->show();

    exec();
}

void Scripting::addScriptingObject(const QString &name, QObject *obj) {
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
    ctx.addObject(name, obj);
}

void Scripting::reflect(QObject *obj) {
    const QMetaObject *meta = obj->metaObject();

    for(int i = 0; i < meta->methodCount(); i++) {
        QMetaMethod method = meta->method(i);
        qDebug() << method.methodSignature();
    }
}

void Scripting::saveSettings(const QString &key, const QVariant &value) {       
    settings->setValue(key, value);
}

