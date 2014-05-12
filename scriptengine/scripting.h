#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>
#include <QThread>

#include <PythonQt/PythonQt.h>
#include <PythonQt/PythonQtClassInfo.h>
#include <PythonQt/gui/PythonQtScriptingConsole.h>
#include <PythonQt/PythonQtPythonInclude.h>
#include <PythonQt/PythonQtStdIn.h>
#include <PythonQt/PythonQt_QtAll.h>


class ColorDecorator;
class FloatCoordinateDecorator;
class CoordinateDecorator;
class TreeListDecorator;
class NodeListDecorator;
class SegmentListDecorator;
class MeshDecorator;
class SkeletonProxy;
class TransformDecorator;
class PointDecorator;
class Highlighter;
class QSettings;
class PythonQtObjectPtr;


class KernelStarter : public QThread {
 Q_OBJECT
public:
    explicit KernelStarter(QObject *parent = 0) { }


    void run() {
        PythonQt::init();
        PythonQtObjectPtr ctx = PythonQt::self()->getMainModule();
        ctx.evalScript("import IPython");
        ctx.evalScript("IPython.embed_kernel()");
    }
};

/** This class intializes the python qt engine */
class Scripting : public QThread
{
    Q_OBJECT
public:
    explicit Scripting(QObject *parent = 0);   
    CoordinateDecorator *coordinateDecorator;
    PythonQtScriptingConsole *console;
    FloatCoordinateDecorator *floatCoordinateDecorator;
    ColorDecorator *colorDecorator;
    TreeListDecorator *treeListDecorator;
    NodeListDecorator *nodeListDecorator;
    SegmentListDecorator *segmentListDecorator;
    MeshDecorator *meshDecorator;
    SkeletonProxy *skeletonProxy;
    TransformDecorator *transformDecorator;
    PointDecorator *pointDecorator;
    Highlighter *highlighter;

    void run();

    PythonQtObjectPtr ctx;
signals:

public slots:
    static void addScriptingObject(const QString &name, QObject *obj);
    void saveSettings(const QString &key, const QVariant &value);
    void addDoc();   
    void executeFromUserDirectory();
    void out(const QString &out);
    void err(const QString &err);



protected:
    QSettings *settings;   
};

#endif // SCRIPTING_H
