#ifndef LOADER_H
#define LOADER_H

#include <QObject>
#include <QThread>

class Loader : public QThread
{
    Q_OBJECT
public:
    explicit Loader(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // LOADER_H
