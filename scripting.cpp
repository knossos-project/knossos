#include "scripting.h"
#include "knossos-global.h"

extern stateInfo *state;

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


    //PythonQt::self()->addInstanceDecorators(reference);
    //PythonQt::self()->registerClass(reference->metaObject(), "skeleton");
    ctx.addObject("skeleton", reference);

    console->setFont(font);
    console->appendCommandPrompt(true);
    console->show();

    exec();

}

void Scripting::addScriptingObject(const QString &name, QObject *obj) {
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
    ctx.addObject(name, obj);
}
