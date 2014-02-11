#ifndef NODELISTDECORATOR_H
#define NODELISTDECORATOR_H

#include <QObject>
#include "knossos-global.h"

extern stateInfo *state;

class nodeListDecorator : public QObject
{
    Q_OBJECT
public:
    explicit nodeListDecorator(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // NODELISTDECORATOR_H
