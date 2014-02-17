#ifndef MAINWINDOWDECORATOR_H
#define MAINWINDOWDECORATOR_H

#include <QObject>
#include "knossos-global.h"
#include "widgets/mainwindow.h"

class MainWindowDecorator : public QObject
{
    Q_OBJECT
public:
    explicit MainWindowDecorator(QObject *parent = 0);

signals:

public slots:

};

#endif // MAINWINDOWDECORATOR_H
