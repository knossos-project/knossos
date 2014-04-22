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
#include "sleeper.h"
#include "widgets/mainwindow.h"
#include <PythonQt/PythonQtStdIn.h>

extern stateInfo *state;
static QString message;

static QString f(void *data) {
    qDebug() << "from stdin";


    QFile file;
    if(file.open(stdout, QIODevice::ReadWrite | QIODevice::Unbuffered)) {
        qDebug() << file.readAll();

        uint l = file.write("unger");
        if(l == -1) {
            qDebug() << "error";
        }
        file.close();
        return QString("unger");
    }

    return message;
}



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

void Scripting::executeFromUserDirectory(PythonQtObjectPtr &ctx) {

    QDir scriptDir("./python/user");
    QStringList endings;
    endings << "*.py";
    scriptDir.setNameFilters(endings);
    QFileInfoList entries = scriptDir.entryInfoList();



    foreach(const QFileInfo &script, entries) {
        QString path = script.absolutePath();
        QFile file(script.canonicalFilePath());

        qDebug() << script.canonicalFilePath();

        if(!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
            continue;
        }

        QTextStream stream(&file);
        QString content =  stream.readAll();
        ctx.evalScript(content);
    }

}


void Scripting::addDoc() {
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    ctx.evalScript("import knossos_python_api");
    ctx.evalScript("knossos_python_api.__doc__ = 'this is the root module of the knossos python api.'");

    ctx.evalScript("from knossos_python_api import *");
    ctx.evalScript("internal.__doc__ = 'This module contains internal data structures which are used in knossos. They are constrained to read-only access in python'");

    ctx.evalScript("import sys");    
    ctx.evalScript("import os");

    // here is simply the convention : Those keys are first available after pathes were saved.
    if(!settings->value("sys_path").isNull()) {
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

    PythonQt::init();
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
    PythonQt_QtAll::init();


    ctx.addObject("app", qApp);
    ctx.evalScript("import sys");
    ctx.evalScript("sys.argv = ['']");
    ctx.evalScript("from PythonQt import *");
    ctx.evalScript("execfile('includes.py')");

    ctx.evalScript("sys.stdout = open('/home/amos/log.txt', 'w')");
    ctx.evalScript("sys.stdout.write('created')");
    ctx.evalScript("sys.stderr = open('/home/amos/error.txt', 'w')");

    ctx.evalScript("import IPython");
    ctx.evalScript("IPython.embed_kernel()");
    ctx.evalScript("python/terminal.py");

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
    PythonQt::self()->registerCPPClass("FloatCoordinate", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(coordinateDecorator);
    PythonQt::self()->registerCPPClass("Coordinate", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(colorDecorator);
    PythonQt::self()->registerCPPClass("Color4F", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(segmentListDecorator);
    PythonQt::self()->registerCPPClass("SegmentListElement", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(treeListDecorator);
    PythonQt::self()->registerCPPClass("TreeListElement", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(nodeListDecorator);
    PythonQt::self()->registerCPPClass("NodeListElement", "", module.toLocal8Bit().data());

    PythonQt::self()->addDecorators(meshDecorator);
    PythonQt::self()->registerCPPClass("Mesh", "",  module.toLocal8Bit().data());

    /*

    connect(signalDelegate, SIGNAL(echo(QString)), console, SLOT(consoleMessage(QString)));
    connect(signalDelegate, SIGNAL(saveSettingsSignal(QString,QVariant)), this, SLOT(saveSettings(QString,QVariant)));

    connect(PythonQt::self(), SIGNAL(pythonStdOut(QString)), this, SLOT(out(QString)));
    connect(PythonQt::self(), SIGNAL(pythonStdErr(QString)), this, SLOT(err(QString)));

    */



    //PythonQt::self()->setRedirectStdInCallback(f, &message);




    //addDoc();
   // executeFromUserDirectory(ctx);


    ctx.evalScript("sys.stdout.close()");
    ctx.evalScript("sys.stderr.close()");
}

void Scripting::addScriptingObject(const QString &name, QObject *obj) {
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
    ctx.addObject(name, obj);
}

void Scripting::saveSettings(const QString &key, const QVariant &value) {       
    settings->setValue(key, value);
}

void Scripting::out(const QString &out) {
    qDebug() << "from stdout";
    qDebug() << out;

    QFile file;
    if(file.open(stdout, QIODevice::ReadOnly)) {
        qDebug() << file.readAll();
    }

}

void Scripting::err(const QString &err) {
    qDebug() << "from stderr";
    qDebug() << err;

}

void Scripting::read() {
   console->consoleMessage(QString(process->readAll()));
}
