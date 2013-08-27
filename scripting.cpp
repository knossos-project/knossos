#include "scripting.h"
#include "knossos-global.h"

extern stateInfo *state;

Scripting::Scripting(QObject *parent) :
    QThread(parent)
{
    QFont font("Courier");
    font.setPixelSize(12);
    font.setBold(true);

    PythonQt::init();
    PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();

    console = new PythonQtScriptingConsole(NULL, ctx);
    console->setWindowTitle("Knossos Scripting Console");
    console->setFont(font);
    console->appendCommandPrompt(true);
    console->show();

}

void Scripting::run() {
    exec();
}
