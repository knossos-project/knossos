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
    static int32_t remoteJump(int32_t x, int32_t y, int32_t z);
    
signals:
    
public slots:
    
};

#endif // REMOTE_H
