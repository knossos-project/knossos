#include "scripting.h"
#include "decorators/skeletondecorator.h"
#include "decorators/treelistdecorator.h"

Scripting::Scripting(QObject *parent) :
    QThread(parent)
{
}

void Scripting::run() {

    QFont font("Courier");
    font.setPixelSize(12);
    font.setBold(true);

    PythonQt::init();
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();


    console = new PythonQtScriptingConsole(NULL, ctx);
    console->setWindowTitle("Knossos Scripting Console");

    //coordinateDecorator = new CoordinateDecorator();
    //PythonQt::self()->addDecorators(coordinateDecorator);
    //PythonQt::self()->registerCPPClass("Coordinate", "", "knossos");
    //PythonQt::self()->addDecorators(coordinateDecorator);
    //PythonQt::self()->registerClass(coordinateDecorator->metaObject(), "Coordinate");

    //ctx.addObject("skeleton", skeletonReference);
    ctx.addObject("state", state);


    TreeListDecorator *treeDecorator = new TreeListDecorator();
    NodeListElementDecorator *nodeDecorator = new NodeListElementDecorator();
    SkeletonDecorator *skeletonDecorator = new SkeletonDecorator();

    PythonQt::self()->addDecorators(treeDecorator);
    PythonQt::self()->registerCPPClass("treeListElement", "", "knossos");

    PythonQt::self()->addDecorators(nodeDecorator);
    PythonQt::self()->registerCPPClass("nodeListElement", "", "knossos");

    PythonQt::self()->addDecorators(skeletonDecorator);
    PythonQt::self()->registerCPPClass("Skeleton", "", "knossos");


    //ctx.add
    //ctx.addObject("CoordinateInstance", coordinateDecorator);

    console->setFont(font);
    console->appendCommandPrompt(true);
    console->show();



    exec();

}

void Scripting::addScriptingObject(const QString &name, QObject *obj) {
    //PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
    //ctx.addObject(name, obj);
}
