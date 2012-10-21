#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QThread>

class Client : public QThread
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // CLIENT_H
