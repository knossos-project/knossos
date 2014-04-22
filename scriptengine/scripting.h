#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>
#include <QThread>
#include <PythonQt/PythonQt.h>
#include <PythonQt_QtAll/PythonQt_QtAll.h>
#include <PythonQt/PythonQtClassInfo.h>
#include <PythonQt/gui/PythonQtScriptingConsole.h>
#include <QProcess>
#include <PythonQt/PythonQtPythonInclude.h>
#include <PythonQt/PythonQtStdIn.h>
#include <QSocketNotifier>


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

/** This class intializes the python qt engine */
class Scripting : public QThread
{
    Q_OBJECT
public:
    explicit Scripting(QObject *parent = 0);



    PythonQtScriptingConsole *console;
    CoordinateDecorator *coordinateDecorator;
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
    QTextStream *stream;
    QProcess *process;
    bool first;



signals:

public slots:
    static void addScriptingObject(const QString &name, QObject *obj);
    void saveSettings(const QString &key, const QVariant &value);
    void addDoc();   
    void executeFromUserDirectory(PythonQtObjectPtr &ctx);
    void out(const QString &out);
    void err(const QString &err);    
    void read();


protected:
    QSettings *settings;   

};

#endif // SCRIPTING_H
