/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
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
#include <QCollator>
#include <QDir>
#include <QFileInfoList>
#include <QIODevice>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

SignalRelay::SignalRelay() {
     state->signalRelay = this;
}

auto PythonQtInit = []() {
    PythonQt::init();
#ifdef QtAll
    PythonQt_QtAll::init();
#endif
    QObject::connect(PythonQt::self(), &PythonQt::pythonStdOut, [](const QString & outMsg){
        static QString text;
        text += outMsg;
        if (text.endsWith('\n')) {
            text.chop(1);
            qDebug() << text.toUtf8().constData();
            text.clear();
        }
    });
    QObject::connect(PythonQt::self(), &PythonQt::pythonStdErr, [](const QString & outMsg){
        static QString text;
        text += outMsg;
        if (text.endsWith('\n')) {
            text.chop(1);
            qWarning() << text.toUtf8().constData();
            text.clear();
        }
    });
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
}

void Scripting::initialize() {
    PythonQt::self()->registerClass(&EmitOnCtorDtor::staticMetaObject);

    evalScript("import sys");
    evalScript("sys.argv = ['']");  // <- this is needed to import the ipython module from the site-package
#ifdef Q_OS_OSX
    // as ipython does not export it's sys paths after the installation we refer to that site-package
    evalScript("sys.path.append('/Library/Python/2.7/site-packages')");
#endif
    evalScript("try: import site\nexcept ImportError: nosite = True\nelse:\n    nosite = False", Py_single_input);
    if (evalScript("nosite", Py_eval_input).toBool()) {
        QMessageBox box;
        box.setIcon(QMessageBox::Critical);
        box.setText("Python 2.7 installation misconfigured.");
        box.setInformativeText("Couldn’t »import site« which means that Python 2.7 couldn’t find its default library files \n"
                               "and using most Python libraries – and plugins depending on those – wont work. \n"
                               "\n"
                               "Un- and reinstallation of a system wide Python 2.7 is advised. \n"
                               "(Alternatively try to fix it manually: registry, environment variables)");
        box.setDetailedText(evalScript("sys.version", Py_eval_input).toString());
        box.addButton("Ignore", QMessageBox::AcceptRole);
        box.exec();
    }
    PythonQt::self()->createModuleFromScript(SCRIPTING_KNOSSOS_MODULE);
    evalScript(QString("import %1").arg(SCRIPTING_KNOSSOS_MODULE));

    evalScript(QString("%1.%2 = {}").arg(SCRIPTING_KNOSSOS_MODULE).arg(SCRIPTING_PLUGIN_CONTAINER));

    addObject("signal_relay", state->signalRelay);
    addObject("knossos", &pythonProxy);
    addObject("scripting", this);
    addObject("segmentation", &segmentationProxy);
    addObject("skeleton", &skeletonProxy);
    addObject("knossos_global_viewer", state->viewer);
    addObject("knossos_global_mainwindow", state->viewer->window);
    addObject("knossos_global_skeletonizer", &Skeletonizer::singleton());
    addObject("knossos_global_segmentation", &Segmentation::singleton());
    addObject("knossos_global_session", &Annotation::singleton());
    addObject("knossos_global_loader", &Loader::Controller::singleton());
    addVariable("Mode_Tracing", AnnotationMode::Mode_Tracing);
    addVariable("Mode_TracingAdvanced", AnnotationMode::Mode_TracingAdvanced);
    addVariable("Mode_Paint", AnnotationMode::Mode_Paint);
    addVariable("Mode_Merge", AnnotationMode::Mode_Merge);
    addVariable("Mode_MergeSimple", AnnotationMode::Mode_MergeSimple);
    addVariable("Mode_Selection", AnnotationMode::Mode_Selection);
    addWidgets();

    createDefaultPluginDir();
    addPythonPath(getPluginDir());
    addPresetCustomPythonPaths();

#ifdef Q_OS_LINUX //in linux there’s an explicit symlink to a python 2 binary
    _ctx.evalFile(QString("sys.path.append('%1')").arg("./python2"));
#else
    _ctx.evalFile(QString("sys.path.append('%1')").arg("./python"));
#endif

    executeResourceStartup();

    const auto * snapshotWidget = &state->viewer->window->widgetContainer.snapshotWidget;
    QObject::connect(&pythonProxy, &PythonProxy::viewport_snapshot_vp_size, snapshotWidget, &SnapshotWidget::snapshotVpSizeRequest);
    QObject::connect(&pythonProxy, &PythonProxy::viewport_snapshot_dataset_size, snapshotWidget, &SnapshotWidget::snapshotDatasetSizeRequest);
    QObject::connect(&pythonProxy, &PythonProxy::viewport_snapshot, snapshotWidget, &SnapshotWidget::snapshotRequest);
    QObject::connect(&pythonProxy, &PythonProxy::set_layer_visibility, state->viewer, &Viewer::setLayerVisibility);
    QObject::connect(&pythonProxy, &PythonProxy::set_mesh_3d_alpha_factor, state->viewer, &Viewer::setMesh3dAlphaFactor);
    QObject::connect(&pythonProxy, &PythonProxy::set_mesh_slicing_alpha_factor, state->viewer, &Viewer::setMeshSlicingAlphaFactor);
    QObject::connect(&pythonProxy, &PythonProxy::set_tree_visibility, &state->viewer->window->widgetContainer.preferencesWidget.treesTab, &TreesTab::setTreeVisibility);
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
    this->pluginDir = pluginDir;
    addPythonPath(pluginDir);
    emit pluginDirChanged(pluginDir);
}

QString Scripting::getPluginDir() {
    return pluginDir;
}

std::set<QString> Scripting::getPluginNames() {
    QSettings settings;
    settings.beginGroup(PLUGIN_SETTINGS_PREFIX + PLUGIN_MGR_NAME);
    auto pluginNamesVal = settings.value(PLUGIN_NAMES_VAL_NAME);
    std::set<QString> plugins;
    for (const auto & elem : pluginNamesVal.toString().split(";")) {
        plugins.emplace(elem);
    }
    for (auto elem : QDir{pluginDir}.entryList(QStringList() << "*.py")) {
        if (plugins.find(elem.chopped(3)) == std::end(plugins)) {
            plugins.emplace(elem);
        }
    }
    return plugins;
}

void Scripting::setPluginNames(const QString &pluginNamesStr) {
    QSettings settings;
    settings.beginGroup(PLUGIN_SETTINGS_PREFIX + PLUGIN_MGR_NAME);
    settings.setValue(PLUGIN_NAMES_VAL_NAME, pluginNamesStr);
    settings.endGroup();
    state->viewer->window->refreshScriptingMenu();
}

QString Scripting::getDefaultPluginDir() {
    return QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(PLUGINS_DIRECTORY);
}

void Scripting::createDefaultPluginDir() {
    if (!getPluginDir().isEmpty() && QDir(getPluginDir()).exists()) {
        return;
    }
    const auto defaultPluginDir = getDefaultPluginDir();
    if (!QDir().mkpath(defaultPluginDir)) {
        QMessageBox errorBox{QApplication::activeWindow()};
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setText(QObject::tr("Python Plugin Manager: Error"));
        errorBox.setInformativeText(QObject::tr("Cannot create plugin directory:\n%1").arg(defaultPluginDir));
        errorBox.exec();
        return;
    }
    setPluginDir(defaultPluginDir);
}

void Scripting::changeWorkingDirectory(const QString & newDir) {
    workingDir = newDir;
    if (!workingDir.isEmpty()) {
        evalScript("import os");
        evalScript(QString("os.chdir('%1')").arg(workingDir));
    }
    emit workingDirChanged(workingDir);
}

void Scripting::addPythonPath(const QString &path) {
    if (_customPathDirs.contains(path)) {
        return;
    }
    _customPathDirs.append(path);
    evalScript(QString("import sys; sys.path.append('%1')").arg(path));
}

void Scripting::addPresetCustomPythonPaths() {
    auto value = getSettingsValue(PYTHON_CUSTOM_PATHS);
    if (value.isNull()) { return; }
    auto customPaths = value.toStringList();
    for (const auto & customPath : customPaths) {
        addPythonPath(customPath);
    }
}

bool isNewer(const QString & v, const QString & otherV) {
    const auto & parts = v.split(".");
    const auto & otherParts = otherV.split(".");
    QCollator collator;
    collator.setNumericMode(true);
    for (int i{0}; i != std::min(parts.size(), otherParts.size()); ++i) {
        if (parts[i] != otherParts[i]) {
            return collator.compare(parts[i], otherParts[i]) > 0;
        }
    }
    return parts.size() > otherParts.size();
}

void Scripting::registerPlugin(PyObject * plugin, const QString & version) {
    PyObject * sibling = nullptr;
    for (auto & elem : runningPlugins.keys()) {
        if (Py_TYPE(plugin)->tp_name == Py_TYPE(elem)->tp_name) {
            sibling = elem;
            break;
        }
    }
    if (sibling == nullptr || isNewer(version, runningPlugins[sibling])) {
        if (sibling != nullptr) {
            PyObject_CallMethod(sibling, const_cast<char*>("delete"), const_cast<char*>(""));
            runningPlugins.remove(sibling);
        }
        runningPlugins[plugin] = version;
        if (!loadedPlugin) {
            throw std::runtime_error("no loadedPlugin in registerPlugin");
        }
        registeredPlugins[QFileInfo{loadedPlugin.get()}.fileName()] = loadedPlugin.get();
    } else {
        PyObject_CallMethod(plugin, const_cast<char*>("delete"), const_cast<char*>(""));
    }
}

void Scripting::runFile(const QString &filename, bool runExistingFirst) {
    if (filename.isNull()) {
        return;
    }
    QFile pyFile(filename);
    runFile(pyFile, QFileInfo{filename}.fileName(), runExistingFirst);
}

void Scripting::runFile(QIODevice & pyFile, const QString & filename, bool runExistingFirst) {
    if(!pyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("runFile open failed");
    }
    if (runExistingFirst && registeredPlugins.contains(filename)) {
        qDebug() << "Running existing plugin first:" << filename;
        try {
            runFile(registeredPlugins[filename]);
        } catch(const std::runtime_error & e) {
            qDebug() << "Failed to load existing plugin:" << e.what();
        }
    }
    QTextStream textStream(&pyFile);
    auto destinationPath = getPluginDir() + '/' + filename;
    loadedPlugin = destinationPath;
    auto pluginContent = textStream.readAll();
    evalScript(pluginContent, Py_file_input);
    if (loadedPlugin) {
        QFile destinationFile{destinationPath};
        destinationFile.remove();
        if(destinationFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            destinationFile.write(pluginContent.toUtf8());
            state->mainWindow->refreshScriptingMenu();
        } else {
            registeredPlugins.remove(filename);
            qDebug() << "Failed to save plugin at" << destinationPath << ".";
        }
    }
    pyFile.close();
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
        QMessageBox errorBox{QApplication::activeWindow()};
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setText(QObject::tr("Plugin action error"));
        errorBox.setInformativeText(QObject::tr("Failed '%1' for plugin '%2':\n%3").arg(actionStr).arg(pluginName).arg(errorStr));
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
    QDir scriptDir(pluginDir);
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
