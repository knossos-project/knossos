/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "pythoninterpreterwidget.h"

#include "GuiConstants.h"
#include "scriptengine/scripting.h"
#include "stateInfo.h"
#include "viewer.h"

#include <PythonQt/gui/PythonQtScriptingConsole.h>

PythonInterpreterWidget::PythonInterpreterWidget(QWidget * parent) : DialogVisibilityNotify(PYTHON_TERMINAL_WIDGET, parent) {
    setWindowIcon(QIcon(":/resources/icons/python.png"));
    setWindowTitle("Python Interpreter");

    setLayout(&mainLayout);
    resize({800, 480});
}

void PythonInterpreterWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(PYTHON_TERMINAL_WIDGET);
    settings.setValue(VISIBLE, isVisible());
}

void PythonInterpreterWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(PYTHON_TERMINAL_WIDGET);
    restoreGeometry(settings.value(GEOMETRY).toByteArray());
}

void PythonInterpreterWidget::startConsole() {
#if !defined(WIN32) || defined(NDEBUG)// PythonQt bug: QWidget: Must construct a QApplication before a QWidget
    auto * console = new PythonQtScriptingConsole(this, PythonQt::self()->getMainModule());
    state->scripting->addObject("knossos_global_interpreter_console", console);
    mainLayout.addWidget(console);
#endif
}
