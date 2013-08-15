#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>
#include <QThread>
#include <PythonQT/PythonQt.h>
#include <PythonQT/gui/PythonQtScriptingConsole.h>

class Scripting : public QThread
{
    Q_OBJECT
public:
    explicit Scripting(QObject *parent = 0);    
    PythonQtScriptingConsole *console;
    void run();
signals:
    
public slots:
    
};

#endif // SCRIPTING_H
