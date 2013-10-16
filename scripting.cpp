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

    ctx.addObject("Skeleton", reference);

    console->setFont(font);
    console->appendCommandPrompt(true);
    console->show();

    exec();

}
