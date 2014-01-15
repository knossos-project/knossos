#include "scripting.h"


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



    //PythonQt::self()->addInstanceDecorators(stateReference);
    //PythonQt::self()->registerCPPClass(reference->metaObject(), "skeleton");
    ctx.addObject("skeleton", skeletonReference);
    ctx.addObject("state", state);

    console->setFont(font);
    console->appendCommandPrompt(true);
    console->show();

    exec();

}

void Scripting::addScriptingObject(const QString &name, QObject *obj) {
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
    ctx.addObject(name, obj);
}
