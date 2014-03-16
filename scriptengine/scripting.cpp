#include "scripting.h"
#include "decorators/coordinatedecorator.h"
#include "decorators/colordecorator.h"
#include "decorators/treelistdecorator.h"
#include "decorators/nodelistdecorator.h"
#include "decorators/segmentlistdecorator.h"
#include "highlighter.h"
#include "knossos-global.h"

extern stateInfo *state;

Q_DECLARE_METATYPE(Coordinate)

Scripting::Scripting(QObject *parent) :
    QThread(parent)
{

}


void Scripting::addDoc() {
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    ctx.evalScript("import knossos_python_api");
    ctx.evalScript("knossos_python_api.__doc__ = 'this is the root module of the knossos python api.'");

    ctx.evalScript("from knossos_python_api import *");
    ctx.evalScript("internal.__doc__ = 'This module contains internal data structures which are used in knossos. They are constrained to read-only access in python'");

    ctx.evalScript("import sys");
    ctx.evalScript("sys.path.append('/home/amos/PyFi')");
    ctx.evalScript("execfile('/home/amos/PyFi/small_test.py')");

}

void Scripting::run() {

    QFont font("Courier");
    font.setPixelSize(12);    

    PythonQt::init(PythonQt::RedirectStdOut, QString("knossos_python_api").toLocal8Bit());
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    console = new PythonQtScriptingConsole(NULL, ctx);
    console->setWindowTitle("Knossos Scripting Console");
    highlighter = new Highlighter(console->document());


    ctx.addObject("knossos", state->skeletonState);


    coordinateDecorator = new CoordinateDecorator();
    colorDecorator = new ColorDecorator();    
    treeListDecorator = new TreeListDecorator();
    nodeListDecorator = new NodeListDecorator();
    segmentListDecorator = new SegmentListDecorator();

    qRegisterMetaType<Coordinate>();

    QString module("internal");

    PythonQt::init(PythonQt::RedirectStdOut, module.toLocal8Bit());

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
    addDoc();

    connect(state->skeletonState, SIGNAL(echo(QString)), console, SLOT(consoleMessage(QString)));

    console->resize(800, 300);
    console->setFont(font);
    console->show();

    exec();
}

void Scripting::addScriptingObject(const QString &name, QObject *obj) {
    //PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
    //ctx.addObject(name, obj);
}

void Scripting::reflect(QObject *obj) {
    const QMetaObject *meta = obj->metaObject();

    for(int i = 0; i < meta->methodCount(); i++) {
        QMetaMethod method = meta->method(i);

       qDebug() << method.methodSignature();
    }


}
