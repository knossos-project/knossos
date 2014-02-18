#ifndef NODE_H
#define NODE_H

#include <QObject>
#include "knossos-global.h"

class Node : public QObject, public nodeListElement
{
    Q_OBJECT
public:
    explicit Node(QObject *parent = 0);

    
signals:
    
public slots:
    
};

#endif // NODE_H
