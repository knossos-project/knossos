#ifndef VIEWER_H
#define VIEWER_H

#include <QObject>
#include <QThread>

class Viewer : public QThread
{
    Q_OBJECT
public:
    explicit Viewer(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // VIEWER_H
