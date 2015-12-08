#include "pythoninterpreterwidget.h"
#include "viewer.h"

#include <PythonQt/PythonQt.h>
#include <QVBoxLayout>

PythonInterpreterWidget::PythonInterpreterWidget(QWidget *parent) : QDialog(parent), console(NULL)
{
    setWindowIcon(QIcon(":/resources/icons/python.png"));
    setWindowTitle("Python Interpreter");

    setLayout(&mainLayout);
    resize(QSize(500,300));
}

void PythonInterpreterWidget::startConsole() {
    console = new PythonQtScriptingConsole((QWidget*)this->parent(), PythonQt::self()->getMainModule());
    state->scripting->addObject("knossos_global_interpreter_console", console);
    mainLayout.addWidget(console);
}
