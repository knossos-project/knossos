#include "scripting.h"
#include "decorators/colordecorator.h"
#include "decorators/skeletondecorator.h"
#include "decorators/treelistdecorator.h"
#include "decorators/nodelistdecorator.h"
#include "decorators/segmentlistdecorator.h"
#include "highlighter.h"
#include "knossos-global.h"

extern stateInfo *state;

Scripting::Scripting(QObject *parent) :
    QThread(parent)
{

}

void Scripting::addDoc() {
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    ctx.evalScript("import knossos_python_api");
    ctx.evalScript("knossos_python_api.__doc__ = 'this is the root module which has to be included for interacting between python and knossos'");

    ctx.evalScript("from knossos_python_api import knossos");
    ctx.evalScript("knossos.__doc__ = 'This module contains data structures'");
    ctx.evalScript("knossos.treeListElement.__doc__ = 'A tree class which '");

    //ctx.evalScript("execfile('/home/amos/Schreibtisch/NewSkeletonTest.py')");
    ctx.evalScript("import sys");
    ctx.evalScript("sys.path.append('/home/amos/PyFi')");
    //ctx.evalScript("execfile('/home/amos/Projects/skeleton-analysis/NewSkeleton.py')");
    //ctx.evalScript("execfile('/home/amos/Projects/skeleton-analysis/adapter.py')");
    //ctx.evalScript("execfile('/home/amos/Projects/skeleton-analysis/small_test.py')");

    //ctx.evalFile("/home/amos/PyFi/small_test.py");


    PythonQtObjectPtr obj = ctx.getVariable("skel");
    PyObject *pobj = obj.object();
    if(!pobj) {
        qDebug() << "obj is null";
    }

    //if(!PyArg_Parse())




}


void Scripting::run() {

    QFont font("Courier");
    font.setPixelSize(12);
    //font.setBold(true);

    PythonQt::init(PythonQt::RedirectStdOut, QString("knossos_python_api").toLocal8Bit());
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    console = new PythonQtScriptingConsole(NULL, ctx);
    console->setWindowTitle("Knossos Scripting Console");
    highlighter = new Highlighter(console->document());

    //coordinateDecorator = new CoordinateDecorator();
    //PythonQt::self()->addDecorators(coordinateDecorator);
    //PythonQt::self()->registerCPPClass("Coordinate", "", "knossos");
    //PythonQt::self()->addDecorators(coordinateDecorator);
    //PythonQt::self()->registerClass(coordinateDecorator->metaObject(), "Coordinate");

    //ctx.addObject("skeleton", skeletonReference);
    //ctx.addObject("state", state);
    ctx.addObject("skeleton", state->skeletonState);

    //ctx.addObject("skeletonState");

    colorDecorator = new ColorDecorator();
    skeletonDecorator = new SkeletonDecorator();
    treeListDecorator = new TreeListDecorator();
    nodeListDecorator = new NodeListDecorator();
    segmentListDecorator = new SegmentListDecorator();

    PythonQtClassInfo info;
    info.setMetaObject(treeListDecorator->metaObject());

    PythonQt::init(PythonQt::RedirectStdOut, QString("knossos").toLocal8Bit());

    PythonQt::self()->addDecorators(colorDecorator);
    PythonQt::self()->registerCPPClass("Color", "", "knossos");


    PythonQt::self()->addDecorators(segmentListDecorator);
    PythonQt::self()->registerCPPClass("segmentListElement", "", "knossos");

    PythonQt::self()->addDecorators(treeListDecorator);
    PythonQt::self()->registerCPPClass("treeListElement", "", "knossos");



    PythonQt::self()->addDecorators(nodeListDecorator);
    PythonQt::self()->registerCPPClass("nodeListElement", "", "knossos");
    addDoc();



    reflect(treeListDecorator);


    //

    //ctx.add
    //ctx.addObject("CoordinateInstance", coordinateDecorator);
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
