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

#ifndef SCRIPTING_H
#define SCRIPTING_H

#include "proxies/pythonproxy.h"
#include "proxies/segmentationproxy.h"
#include "proxies/skeletonproxy.h"
#include "widgets/viewports/viewportbase.h"

#include <PythonQt/PythonQtObjectPtr.h>

#include <QMap>
#include <QMouseEvent>
#include <QObject>

#include <functional>
#include <set>
#include <utility>

/*
 * This class emits on ctor a signal of arbitrary parameters that is passed to the ctor,
 * and on dtor a dtorSignal with a context parameter that may be set by dtorSetCtx.
 * Instantiate within a C++ scope to allow PythonQt python code to be notified when the
 * scope begins, and then when the scope terminates (upon completion/exception/break/return/etc.).
 *
 * Typical PythonQt usage:
 *
 * def mySignal_dtor_func(ctx):
 *      print "dtor: %s\n" % ctx
 *      return
 *
 * def mySignal_ctor_func(eocd, a, b, c):
 *      eocd.connect(mySignal_dtor_func)
 *      description = "%s: %d, %d" % (a, b, c)
 *      eocd.dtorSetCtx(description)
 *      print "mySignal_ctor: %s\n" % description
 *      return
 *
 * signalRelay.mySignal.connect(mySignal_ctor_func)
 *
*/
class EmitOnCtorDtor : public QObject
{
    Q_OBJECT
public:
    template<typename SignalType, typename Class, typename... Args>
    EmitOnCtorDtor(SignalType signalName, Class instance, Args && ...args) {
        emit std::mem_fn(signalName)(instance, this, std::forward<Args>(args)...);
    }
    virtual ~EmitOnCtorDtor() override {
        emit dtorSignal(dtorCtx);
    }
public slots:
    void dtorSetCtx(QVariant ctx) { dtorCtx = ctx;}
signals:
    void dtorSignal(QVariant);
private:
    QVariant dtorCtx;
};

class SignalRelay : public QObject
{
    Q_OBJECT
public:
    explicit SignalRelay();
signals:
    void Signal_EventModel_handleMouseHover(EmitOnCtorDtor*,Coordinate,quint64,int, const QMouseEvent*);
    void Signal_Viewort_mouseReleaseEvent(EmitOnCtorDtor*,class ViewportBase*, const QMouseEvent*);
    void Signal_EventModel_handleMouseReleaseMiddle(EmitOnCtorDtor*,Coordinate,int,const QMouseEvent*);
    void Signal_MainWindow_closeEvent(EmitOnCtorDtor*,QCloseEvent*);
};

const QString PLUGIN_MGR_NAME = "pluginMgr";
const QString PLUGIN_SETTINGS_PREFIX = "Plugin_";
const QString PLUGIN_NAMES_VAL_NAME = "PluginNames";

const QString PLUGINS_DIRECTORY = "knossos_plugins";

/** This class intializes the python qt engine */
class Scripting : public QObject {
    Q_OBJECT
    friend class PythonPropertyWidget;
public:
    explicit Scripting();
    void runFile(const QString &filename, bool runExistingFirst = false);
    void runFile(QIODevice & pyFile, const QString &filename, bool runExistingFirst = false);
    void addObject(const QString& name, QObject* object);
    void addVariable(const QString& name, const QVariant& v);
    SkeletonProxy skeletonProxy;
    SegmentationProxy segmentationProxy;
    PythonProxy pythonProxy;
    QMap<PyObject *, QString> runningPlugins;
    QMap<QString, QString> registeredPlugins;
private:
    boost::optional<QString> loadedPlugin;
    PythonQtObjectPtr _ctx;
    QStringList _customPathDirs;
    QVariant evalScript(const QString& script, int start = Py_file_input);
    QString pluginDir;
    QString workingDir;
    void initialize();
    void moveSymbolIntoKnossosModule(const QString& name);
    void executeFromUserDirectory();
    void executeResourceStartup();
    void changeWorkingDirectory(const QString &newDir);
    void createDefaultPluginDir();
    void addPresetCustomPythonPaths();
    void addPythonPath(const QString &customPath);
    void addWidgets();
    bool pluginActionError(const QString &actionStr, const QString &pluginName, const QString &errorStr, bool isQuiet);
signals:
    void pluginDirChanged(const QString & newDir);
    void workingDirChanged(const QString & newDir);
public slots:
    void registerPlugin(PyObject * plugin, const QString & version);
    QString getPluginInContainerStr(const QString &pluginName);
    QString getImportInContainerStr(const QString &pluginName);
    QString getInstanceInContainerStr(const QString &pluginName);
    QString getContainerStr();
    bool isPluginImported(const QString &pluginName);
    bool importPlugin(const QString &pluginName, bool isQuiet);
    bool isPluginOpen(const QString &pluginName);
    bool instantiatePlugin(const QString &pluginName, bool isQuiet);
    bool closePlugin(const QString &pluginName, bool isQuiet);
    bool removePluginInstance(const QString &pluginName, bool isQuiet);
    bool removePluginImport(const QString &pluginName, bool isQuiet);
    bool reloadPlugin(const QString &pluginName, bool isQuiet);
    bool isPluginVisible(const QString &pluginName);
    bool isPluginActive(const QString &pluginName);
    bool showPlugin(const QString &pluginName, bool isQuiet);
    bool hidePlugin(const QString &pluginName, bool isQuiet);
    bool openPlugin(const QString &pluginName, bool isQuiet);
    std::set<QString> getPluginNames();
    void setPluginNames(const QString &pluginNamesStr);
    QString getPluginDir();
    void setPluginDir(const QString &pluginDir);
    QString getDefaultPluginDir();
};

#endif // SCRIPTING_H
