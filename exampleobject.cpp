#include "exampleobject.h"
#include <QDebug>

ExampleObject::ExampleObject(QObject *parent) :
    QObject(parent)
{
}

void ExampleObject::giveMeYourPythonObject(PyObject *obj) {
    //obj->
    qDebug() << obj;
}
