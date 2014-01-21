#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>
#include <QThread>

#include <PythonQt/PythonQt.h>
#include <PythonQt/gui/PythonQtScriptingConsole.h>
#include <Python.h>
#include "skeletonizer.h"
#include "knossos-global.h"

extern stateInfo *state;
/** This class intializes the python qt engine in a seperate thread */
class Scripting : public QThread
{
    Q_OBJECT
public:
    explicit Scripting(QObject *parent = 0);    
    PythonQtScriptingConsole *console;
    Skeletonizer *skeletonReference;
    CoordinateDecorator *coordinateDecorator;
    PyObject *aObject;
    void run();
signals:
    
public slots:
    void addScriptingObject(const QString &name, QObject *obj);
};

#endif // SCRIPTING_H
