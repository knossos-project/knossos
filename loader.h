#ifndef LOADER_H
#define LOADER_H

#include <QObject>
#include <QThread>

class Loader : public QObject
{
    Q_OBJECT
public:
    explicit Loader(QObject *parent = 0);

signals:
    void finished();
public slots:
    void load();
    void start();



    
};

#endif // LOADER_H
