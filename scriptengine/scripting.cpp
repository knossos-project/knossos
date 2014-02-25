#include "scripting.h"
#include "decorators/colordecorator.h"
#include "decorators/skeletondecorator.h"
#include "decorators/treelistdecorator.h"
#include "decorators/nodelistdecorator.h"
#include "highlighter.h"
#include "knossos-global.h"

extern stateInfo *state;

Scripting::Scripting(QObject *parent) :
    QThread(parent)
{

}



void Scripting::run() {

    QFont font("Courier");
    font.setPixelSize(12);
    //font.setBold(true);

    PythonQt::init();

    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    //console = new NicePyConsole(0, ctx);

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
    ctx.addObject("Skeleton", state->skeletonState);

    //ctx.addObject("skeletonState");

    colorDecorator = new ColorDecorator();
    skeletonDecorator = new SkeletonDecorator();
    treeListDecorator = new TreeListDecorator();
    nodeListDecorator = new NodeListDecorator();

    PythonQt::self()->addDecorators(colorDecorator);
    PythonQt::self()->registerCPPClass("Color", "", "knossos");

    PythonQt::self()->addDecorators(skeletonDecorator);
    PythonQt::self()->registerCPPClass("Skeleton", "", "knossos");

    PythonQt::self()->addDecorators(treeListDecorator);
    PythonQt::self()->registerCPPClass("treeListElement", "", "knossos");

    PythonQt::self()->addDecorators(nodeListDecorator);
    PythonQt::self()->registerCPPClass("nodeListElement", "", "knossos");


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
