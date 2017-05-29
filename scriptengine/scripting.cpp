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

#include "scripting.h"

#include "decorators/coordinatedecorator.h"
#include "decorators/floatcoordinatedecorator.h"
#include "decorators/nodelistdecorator.h"
#include "decorators/treelistdecorator.h"
#include "decorators/segmentlistdecorator.h"
#include "loader.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/GuiConstants.h"
#include "widgets/mainwindow.h"

#include <PythonQt/PythonQt.h>
#ifdef QtAll
#include <PythonQt/PythonQt_QtAll.h>
#endif

#include <QApplication>
#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

SignalRelay::SignalRelay() {
     state->signalRelay = this;
}

auto PythonQtInit = []() {
    PythonQt::init(PythonQt::RedirectStdOut);
#ifdef QtAll
    PythonQt_QtAll::init();
#endif
    const auto redirect = [](const QString & outMsg){ qDebug() << outMsg; };
    QObject::connect(PythonQt::self(), &PythonQt::pythonStdOut, redirect);
    QObject::connect(PythonQt::self(), &PythonQt::pythonStdErr, redirect);
    return PythonQt::self()->getMainModule();
};

QVariant Scripting::evalScript(const QString& script, int start) {
    return _ctx.evalScript(script, start);
}

const QString SCRIPTING_KNOSSOS_MODULE = "KnossosModule";
const QString SCRIPTING_PLUGIN_CONTAINER = "plugin_container";
const QString SCRIPTING_IMPORT_KEY = "import";
const QString SCRIPTING_INSTANCE_KEY = "instance";

Scripting::Scripting() : _ctx{PythonQtInit()} {
    state->scripting = this;

    PythonQt::self()->registerClass(&EmitOnCtorDtor::staticMetaObject);

    evalScript("import sys");
    evalScript("sys.argv = ['']");  // <- this is needed to import the ipython module from the site-package
#ifdef Q_OS_OSX
    // as ipython does not export it's sys paths after the installation we refer to that site-package
    evalScript("sys.path.append('/Library/Python/2.7/site-packages')");
#endif
    PythonQt::self()->createModuleFromScript(SCRIPTING_KNOSSOS_MODULE);
    evalScript(QString("import %1").arg(SCRIPTING_KNOSSOS_MODULE));

    evalScript(QString("%1.%2 = {}").arg(SCRIPTING_KNOSSOS_MODULE).arg(SCRIPTING_PLUGIN_CONTAINER));

    addObject("signalRelay", state->signalRelay);
    addObject("knossos", &pythonProxy);
    addObject("scripting", this);
    addObject("segmentation", &segmentationProxy);
    addObject("skeleton", &skeletonProxy);
    addObject("knossos_global_viewer", state->viewer);
    addObject("knossos_global_mainwindow", state->viewer->window);
    addObject("knossos_global_skeletonizer", &Skeletonizer::singleton());
    addObject("knossos_global_segmentation", &Segmentation::singleton());
    addObject("knossos_global_loader", &Loader::Controller::singleton());
    addVariable("GL_POINTS", GL_POINTS);
    addVariable("GL_LINES", GL_LINES);
    addVariable("GL_LINE_STRIP", GL_LINE_STRIP);
    addVariable("GL_LINE_LOOP", GL_LINE_LOOP);
    addVariable("GL_TRIANGLES", GL_TRIANGLES);
    addVariable("GL_TRIANGLES_STRIP", GL_TRIANGLE_STRIP);
    addVariable("GL_TRIANGLE_FAN", GL_TRIANGLE_FAN);
    addVariable("GL_QUADS", GL_QUADS);
    addVariable("GL_QUAD_STRIP", GL_QUAD_STRIP);
    addVariable("GL_POLYGON", GL_POLYGON);
    addWidgets();

    auto makeDecorator = [](QObject * decorator, const char * typeName){
        // PythonQt tries to reparent the decorators, we do their missing work of pushing it into their thread first
        // due to the QObject handling they also get deleted by PythonQt then
        decorator->moveToThread(PythonQt::self()->thread());
        PythonQt::self()->addDecorators(decorator);
        PythonQt::self()->registerCPPClass(typeName, "", "internal");
    };
    makeDecorator(new CoordinateDecorator, "Coordinate");
    makeDecorator(new FloatCoordinateDecorator, "floatCoordinate");
    makeDecorator(new NodeListDecorator, "Node");
    makeDecorator(new SegmentListDecorator, "Segment");
    makeDecorator(new TreeListDecorator, "Tree");

    createDefaultPluginDir();
    addPythonPath(getPluginDir());
    addPresetCustomPythonPaths();

#ifdef Q_OS_LINUX //in linux there’s an explicit symlink to a python 2 binary
    _ctx.evalFile(QString("sys.path.append('%1')").arg("./python2"));
#else
    _ctx.evalFile(QString("sys.path.append('%1')").arg("./python"));
#endif

    changeWorkingDirectory();
    executeResourceStartup();
    executeFromUserDirectory();
    const auto * snapshotWidget = &state->viewer->window->widgetContainer.snapshotWidget;
    QObject::connect(&state->scripting->pythonProxy, &PythonProxy::viewport_snapshot_vp_size, snapshotWidget, &SnapshotWidget::snapshotVpSizeRequest);
    QObject::connect(&state->scripting->pythonProxy, &PythonProxy::viewport_snapshot_dataset_size, snapshotWidget, &SnapshotWidget::snapshotDatasetSizeRequest);
    QObject::connect(&state->scripting->pythonProxy, &PythonProxy::viewport_snapshot, snapshotWidget, &SnapshotWidget::snapshotRequest);
    QObject::connect(&state->scripting->pythonProxy, &PythonProxy::set_layer_visibility, state->viewer, &Viewer::setLayerVisibility);
    state->viewer->window->widgetContainer.pythonInterpreterWidget.startConsole();
}

QVariant getSettingsValue(const QString &key) {
    QSettings settings;
    settings.beginGroup(PYTHON_PROPERTY_WIDGET);
    auto value = settings.value(key);
    settings.endGroup();
    return value;
}

void Scripting::setPluginDir(const QString &pluginDir) {
    QSettings settings;
    settings.beginGroup(PLUGIN_SETTINGS_PREFIX + PLUGIN_MGR_NAME);
    settings.setValue(PLUGIN_DIR_VAL_NAME,pluginDir);
    settings.endGroup();
    addPythonPath(pluginDir);
}

QString Scripting::getPluginDir() {
    QSettings settings;
    settings.beginGroup(PLUGIN_SETTINGS_PREFIX + PLUGIN_MGR_NAME);
    auto pluginDirVal = settings.value(PLUGIN_DIR_VAL_NAME);
    return pluginDirVal.isNull() ? QString() : pluginDirVal.toString();
}

QString Scripting::getPluginNames() {
    QSettings settings;
    settings.beginGroup(PLUGIN_SETTINGS_PREFIX + PLUGIN_MGR_NAME);
    auto pluginNamesVal = settings.value(PLUGIN_NAMES_VAL_NAME);
    return pluginNamesVal.isNull() ? "" : pluginNamesVal.toString();
}

void Scripting::setPluginNames(const QString &pluginNamesStr) {
    QSettings settings;
    settings.beginGroup(PLUGIN_SETTINGS_PREFIX + PLUGIN_MGR_NAME);
    settings.setValue(PLUGIN_NAMES_VAL_NAME, pluginNamesStr);
    settings.endGroup();
    state->viewer->window->refreshPluginMenu();
}

QString Scripting::getDefaultPluginDir() {
    return QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(PLUGINS_DIRECTORY);
}

void Scripting::createDefaultPluginDir() {
    auto pluginDir = getPluginDir();
    if (!pluginDir.isEmpty() && QDir(pluginDir).exists()) {
        return;
    }
    pluginDir = getDefaultPluginDir();
    if (!QDir().mkpath(pluginDir)) {
        QMessageBox errorBox(QMessageBox::Warning, "Python Plugin Manager: Error",
                             QString("Cannot create plugin directory:\n%1").arg(pluginDir),
                             QMessageBox::Ok, NULL);
        errorBox.exec();
        return;
    }
    setPluginDir(pluginDir);
}

void Scripting::changeWorkingDirectory() {
    auto value = getSettingsValue(PYTHON_WORKING_DIRECTORY);
    if (value.isNull()) { return; }
    auto workingDir = value.toString();
    if (workingDir.isEmpty()) { return; }

    evalScript("import os");
    evalScript(QString("os.chdir('%1')").arg(workingDir));
}

void Scripting::addPythonPath(const QString &path) {
    if (_customPathDirs.contains(path)) {
        return;
    }
    _customPathDirs.append(path);
    evalScript(QString("sys.path.append('%1')").arg(path));
}

void Scripting::addPresetCustomPythonPaths() {
    auto value = getSettingsValue(PYTHON_CUSTOM_PATHS);
    if (value.isNull()) { return; }
    auto customPaths = value.toStringList();
    for (const auto & customPath : customPaths) {
        addPythonPath(customPath);
    }
}

void Scripting::runFile(const QString &filename) {
    QFile pyFile(filename);
    pyFile.open(QIODevice::ReadOnly);
    QString s;
    QTextStream textStream(&pyFile);
    s.append(textStream.readAll());
    pyFile.close();

    evalScript(s, Py_file_input);
}

void Scripting::moveSymbolIntoKnossosModule(const QString& name) {
    evalScript(QString("%1.%2 = %2; del %2").arg(SCRIPTING_KNOSSOS_MODULE).arg(name));
}

QString Scripting::getContainerStr() {
    return QString("%1.%2").arg(SCRIPTING_KNOSSOS_MODULE).arg(SCRIPTING_PLUGIN_CONTAINER);
}

QString Scripting::getPluginInContainerStr(const QString &pluginName) {
    return QString("%1['%2']").arg(getContainerStr()).arg(pluginName);
}

QString Scripting::getImportInContainerStr(const QString &pluginName) {
    return QString("%1['%2']").arg(getPluginInContainerStr(pluginName)).arg(SCRIPTING_IMPORT_KEY);
}

QString Scripting::getInstanceInContainerStr(const QString &pluginName) {
    return QString("%1['%2']").arg(getPluginInContainerStr(pluginName)).arg(SCRIPTING_INSTANCE_KEY);
}

bool Scripting::pluginActionError(const QString &actionStr, const QString &pluginName, const QString &errorStr, bool isQuiet) {
    if (!isQuiet) {
        QMessageBox errorBox(QMessageBox::Warning, "Plugin action error",
                             QString("Failed '%1' for plugin '%2':\n%3").arg(actionStr).arg(pluginName).arg(errorStr),
                             QMessageBox::Ok, NULL);
        errorBox.exec();
    }
    return false;
}

bool Scripting::isPluginImported(const QString &pluginName) {
    return      (
                evalScript(QString("'%1' in %2").arg(pluginName).arg(getContainerStr()), Py_eval_input).toBool()
                &&
                evalScript(QString("'%1' in %2").arg(SCRIPTING_IMPORT_KEY).arg(getPluginInContainerStr(pluginName)), Py_eval_input).toBool()
                &&
                evalScript(QString("%1 != None").arg(getImportInContainerStr(pluginName)), Py_eval_input).toBool()
                );
}

bool Scripting::importPlugin(const QString &pluginName, bool isQuiet) {
    auto actionStr = "import";
    if (isPluginImported(pluginName)) {
        return true;
    }
    evalScript(QString("%1 = {}").arg(getPluginInContainerStr(pluginName)));
    evalScript(QString("import %1").arg(pluginName));
    if (!evalScript(QString("'%1' in sys.modules").arg(pluginName), Py_eval_input).toBool()) {
        return pluginActionError(actionStr, pluginName, "import error", isQuiet);
    }
    evalScript(QString("%1 = %2; del %2").arg(getImportInContainerStr(pluginName)).arg(pluginName));
    return true;
}

bool Scripting::isPluginOpen(const QString &pluginName) {
    return      (
                isPluginImported(pluginName)
                &&
                evalScript(QString("'%1' in %2").arg(SCRIPTING_INSTANCE_KEY).arg(getPluginInContainerStr(pluginName)), Py_eval_input).toBool()
                &&
                evalScript(QString("%1 != None").arg(getInstanceInContainerStr(pluginName)), Py_eval_input).toBool()
                );
}

bool Scripting::instantiatePlugin(const QString &pluginName, bool isQuiet) {
    auto actionStr = "instanstiate";
    if (!isPluginImported(pluginName)) {
        return pluginActionError(actionStr, pluginName, "not imported", isQuiet);
    }
    if (isPluginOpen(pluginName)) {
        return true;
    }
    evalScript(QString("%1.main_class()").arg(getImportInContainerStr(pluginName)));
    if (!isPluginOpen(pluginName)) {
        return pluginActionError(actionStr, pluginName, "still not open", isQuiet);
    }
    return true;
}

bool Scripting::removePluginInstance(const QString &pluginName, bool isQuiet) {
    auto actionStr = "remove instance";
    if (!isPluginOpen(pluginName)) {
        return true;
    }
    evalScript(QString("%1 = None").arg(getInstanceInContainerStr(pluginName)));
    if (isPluginOpen(pluginName)) {
        return pluginActionError(actionStr, pluginName, "still open", isQuiet);
    }
    return true;
}

bool Scripting::removePluginImport(const QString &pluginName, bool isQuiet) {
    auto actionStr = "remove import";
    if (!isPluginImported(pluginName)) {
        return true;
    }
    evalScript(QString("%1 = None").arg(getImportInContainerStr(pluginName)));
    if (isPluginImported(pluginName)) {
        return pluginActionError(actionStr, pluginName, "still imported", isQuiet);
    }
    return true;
}

bool Scripting::closePlugin(const QString &pluginName, bool isQuiet) {
    auto actionStr = "close";
    if (!isPluginOpen(pluginName)) {
        return true;
    }
    if (!evalScript(QString("%1.close()").arg(getInstanceInContainerStr(pluginName)), Py_eval_input).toBool()) {
        return pluginActionError(actionStr, pluginName, "close failed", isQuiet);
    }
    if (!removePluginInstance(pluginName, isQuiet)) {
        return pluginActionError(actionStr, pluginName, "instance not removed", isQuiet);
    }
    return true;
}

bool Scripting::reloadPlugin(const QString &pluginName, bool isQuiet) {
    auto actionStr = "reload";
    if (!closePlugin(pluginName, isQuiet)) {
        return pluginActionError(actionStr, pluginName, "close failed", isQuiet);
    }
    if (isPluginImported(pluginName))  {
        if (!evalScript(QString("reload(%1) != None").arg(getImportInContainerStr(pluginName)), Py_eval_input).toBool()) {
            return pluginActionError(actionStr, pluginName, "reload failed", isQuiet);
        }
    }
    if (!openPlugin(pluginName, isQuiet)) {
        return pluginActionError(actionStr, pluginName, "open failed", isQuiet);
    }
    return true;
}

bool Scripting::isPluginVisible(const QString &pluginName) {
    if (!isPluginOpen(pluginName)) {
        return false;
    }
    return evalScript(QString("%1.isVisible()").arg(getInstanceInContainerStr(pluginName)), Py_eval_input).toBool();
}

bool Scripting::isPluginActive(const QString &pluginName) {
    return evalScript(QString("%1.isActiveWindow").arg(getInstanceInContainerStr(pluginName)), Py_eval_input).toBool();
}

bool Scripting::showPlugin(const QString &pluginName, bool isQuiet) {
    auto actionStr = "show";
    auto instanceInContainer = getInstanceInContainerStr(pluginName);
    if (!isPluginOpen(pluginName)) {
        return pluginActionError(actionStr, pluginName, "not open", isQuiet);
    }
    if (!isPluginVisible(pluginName)) {
        evalScript(QString("%1.show()").arg(instanceInContainer));
        if (!isPluginVisible(pluginName)) {
            return pluginActionError(actionStr, pluginName, "still not visible", isQuiet);
        }
    }
    if (!isPluginActive(pluginName)) {
        evalScript(QString("%1.activateWindow()").arg(instanceInContainer));
        if (!isPluginActive(pluginName)) {
            return pluginActionError(actionStr, pluginName, "still not active", isQuiet);
        }
    }
    return true;
}

bool Scripting::hidePlugin(const QString &pluginName, bool isQuiet) {
    auto actionStr = "hide";
    if (!isPluginOpen(pluginName)) {
        return pluginActionError(actionStr, pluginName, "not open", isQuiet);
    }
    if (!isPluginVisible(pluginName)) {
        return true;
    }
    evalScript(QString("%1.hide()").arg(getInstanceInContainerStr(pluginName)));
    if (isPluginVisible(pluginName)) {
        return pluginActionError(actionStr, pluginName, "still visible", isQuiet);
    }
    return true;
}

bool Scripting::openPlugin(const QString &pluginName, bool isQuiet) {
    auto actionStr = "open";
    if (!importPlugin(pluginName, isQuiet)) {
        return pluginActionError(actionStr, pluginName, "import failed", isQuiet);
    }
    if (!instantiatePlugin(pluginName, isQuiet)) {
        return pluginActionError(actionStr, pluginName, "instantiation failed", isQuiet);
    }
    if (!showPlugin(pluginName, isQuiet)) {
        return pluginActionError(actionStr, pluginName, "show failed", isQuiet);
    }
    return true;
}

void Scripting::addObject(const QString& name, QObject* object) {
    _ctx.addObject(name, object);
    moveSymbolIntoKnossosModule(name);
}

void Scripting::addVariable(const QString& name, const QVariant& v) {
    _ctx.addVariable(name, v);
    moveSymbolIntoKnossosModule(name);
}

void Scripting::executeFromUserDirectory() {
    auto value = getSettingsValue(PYTHON_AUTOSTART_FOLDER);
    if (value.isNull()) { return; }
    auto autoStartFolder = value.toString();
    if (autoStartFolder.isEmpty()) { return; }

    QDir scriptDir(autoStartFolder);
    QStringList endings;
    endings << "*.py";
    scriptDir.setNameFilters(endings);
    QFileInfoList entries = scriptDir.entryInfoList();
    foreach(const QFileInfo &script, entries) {
        QFile file(script.canonicalFilePath());
        if(!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
            continue;
        }
        _ctx.evalFile(script.canonicalFilePath());
    }
}

void Scripting::executeResourceStartup() {
    auto startupDir = QDir(":/resources/plugins/startup");
    for (const auto & plugin : startupDir.entryInfoList()) {
        runFile(plugin.absoluteFilePath());
    }
}

/** This methods create a pep8-style object name for all knossos widget and
 *  adds them to the python-context. Widgets with a leading Q are ignored
*/
void Scripting::addWidgets() {
    QWidgetList list = QApplication::allWidgets();
    foreach(QWidget *widget, list) {
        QString name = widget->metaObject()->className();
        QByteArray array;

        for(int i = 0; i < name.size(); i++) {
            if(name.at(i).isLower()) {
                array.append(name.at(i));
            } else if(name.at(i).isUpper()) {
                if(i == 0 && name.at(i) == 'Q') {
                    continue;
                } else if(i == 0 && name.at(i) != 'Q') {
                    array.append(name.at(i).toLower());
                } else {
                    array.append(QString("_%1").arg(name.at(i).toLower()));
                }
            }
        }

        addObject("knossos_global_widget_" + QString(array), widget);
    }
}
