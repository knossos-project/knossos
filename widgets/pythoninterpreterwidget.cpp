#include "pythoninterpreterwidget.h"

#include "scriptengine/scripting.h"
#include "viewer.h"

#include <PythonQt/gui/PythonQtScriptingConsole.h>

PythonInterpreterWidget::PythonInterpreterWidget(QWidget * parent) : QDialog(parent) {
    setWindowIcon(QIcon(":/resources/icons/python.png"));
    setWindowTitle("Python Interpreter");

    setLayout(&mainLayout);
    resize({500, 300});
}

void PythonInterpreterWidget::startConsole() {
#if !defined(WIN32) || defined(NDEBUG)// PythonQt bug: QWidget: Must construct a QApplication before a QWidget
    auto * console = new PythonQtScriptingConsole(this, PythonQt::self()->getMainModule());
    state->scripting->addObject("knossos_global_interpreter_console", console);
    mainLayout.addWidget(console);
#endif
}
