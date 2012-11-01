#ifndef REMOTE_H
#define REMOTE_H

#include <QObject>
#include <QThread>

class Remote : public QThread
{
    Q_OBJECT
public:
    explicit Remote(QObject *parent = 0);
    static void checkIdleTime();
    
signals:
    
public slots:
    
};

#endif // REMOTE_H
