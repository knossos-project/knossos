#ifndef EXAMPLEOBJECT_H
#define EXAMPLEOBJECT_H

#include <QObject>
#include <Python.h>

class ExampleObject : public QObject
{
    Q_OBJECT
public:
    explicit ExampleObject(QObject *parent = 0);
    
signals:
    
public slots:
    void giveMeYourPythonObject(PyObject *obj);
};

#endif // EXAMPLEOBJECT_H
