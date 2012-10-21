#ifndef REMOTE_H
#define REMOTE_H

#include <QObject>

class Remote : public QObject
{
    Q_OBJECT
public:
    explicit Remote(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // REMOTE_H
