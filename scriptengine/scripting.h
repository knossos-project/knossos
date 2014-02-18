#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>
#include <QThread>
#include <PythonQt/PythonQt.h>
#include <PythonQt/gui/PythonQtScriptingConsole.h>
#include <Python.h>

class ColorDecorator;
class SkeletonDecorator;
class TreeListDecorator;
class NodeListDecorator;

/** This class intializes the python qt engine */
class Scripting : public QThread
{
    Q_OBJECT
public:

    explicit Scripting(QObject *parent = 0);

    //NicePyConsole *console;
    PythonQtScriptingConsole *console;

    ColorDecorator *colorDecorator;
    SkeletonDecorator *skeletonDecorator;
    TreeListDecorator *treeListDecorator;
    NodeListDecorator *nodeListDecorator;

    void run();
signals:
    
public slots:
    void addScriptingObject(const QString &name, QObject *obj);
};

#endif // SCRIPTING_H
