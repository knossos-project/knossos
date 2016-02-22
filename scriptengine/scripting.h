#ifndef SCRIPTING_H
#define SCRIPTING_H

#include "proxies/pythonproxy.h"
#include "proxies/segmentationproxy.h"
#include "proxies/skeletonproxy.h"

#include <functional>
#include <QMouseEvent>
#include <QObject>
#include <QThread>
#include <PythonQt/PythonQt.h>
#ifdef QtAll
#include <PythonQt/PythonQt_QtAll.h>
#endif
#include <PythonQt/PythonQtClassInfo.h>
#include <PythonQt/PythonQtPythonInclude.h>
#include <PythonQt/PythonQtStdIn.h>

#include "stateInfo.h"
#include "widgets/viewport.h"

class ColorDecorator;
class FloatCoordinateDecorator;
class CoordinateDecorator;
class TreeListDecorator;
class NodeListDecorator;
class NodeCommentDecorator;
class SegmentListDecorator;
class MeshDecorator;
class TransformDecorator;
class PointDecorator;
class Highlighter;
class QSettings;
class PythonQtObjectPtr;

void PythonQtInit();

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
    ~EmitOnCtorDtor() {
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
    explicit SignalRelay() {state->signalRelay = this;}
signals:
    void Signal_EventModel_handleMouseHover(EmitOnCtorDtor*,Coordinate,quint64,int, const QMouseEvent*);
    void Signal_Viewort_mouseReleaseEvent(EmitOnCtorDtor*,class ViewportBase*, const QMouseEvent*);
    void Signal_EventModel_handleMouseReleaseMiddle(EmitOnCtorDtor*,Coordinate,int,const QMouseEvent*);
    void Signal_MainWindow_closeEvent(EmitOnCtorDtor*,QCloseEvent*);
};

const QString PLUGIN_MGR_NAME = "pluginMgr";
const QString PLUGIN_SETTINGS_PREFIX = "Plugin_";
const QString PLUGIN_DIR_VAL_NAME = "PluginDir";
const QString PLUGIN_NAMES_VAL_NAME = "PluginNames";

/** This class intializes the python qt engine */
class Scripting : public QObject
{
    Q_OBJECT
public:
    explicit Scripting();
    void runFile(const QString &filename);
    void addObject(const QString& name, QObject* object);
    void addVariable(const QString& name, const QVariant& v);
    CoordinateDecorator *coordinateDecorator;
    FloatCoordinateDecorator *floatCoordinateDecorator;
    ColorDecorator *colorDecorator;
    TreeListDecorator *treeListDecorator;
    NodeListDecorator *nodeListDecorator;
    NodeCommentDecorator *nodeCommentDecorator;
    SegmentListDecorator *segmentListDecorator;
    MeshDecorator *meshDecorator;
    SkeletonProxy skeletonProxy;
    SegmentationProxy segmentationProxy;
    PythonProxy pythonProxy;
    TransformDecorator *transformDecorator;
    PointDecorator *pointDecorator;
    Highlighter *highlighter;
protected:
    QSettings *settings;
private:
    PythonQtObjectPtr _ctx;
    QStringList _customPathDirs;
    QVariant evalScript(const QString& script, int start = Py_file_input);
    void moveSymbolIntoKnossosModule(const QString& name);
    void executeFromUserDirectory();
    void executeResourceStartup();
    void changeWorkingDirectory();
    void createDefaultPluginDir();
    void addPresetCustomPythonPaths();
    void addPythonPath(const QString &customPath);
    void addWidgets();
    void autoStartTerminal();
    bool pluginActionError(const QString &actionStr, const QString &pluginName, const QString &errorStr, bool isQuiet);
public slots:
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
    QString getPluginNames();
    void setPluginNames(const QString &pluginNamesStr);
    QString getPluginDir();
    void setPluginDir(const QString &pluginDir);
    QString getDefaultPluginDir();
};

#endif // SCRIPTING_H
