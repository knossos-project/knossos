#ifndef TREE_H
#define TREE_H

#include <QObject>
#include "knossos-global.h"

class Tree : public QObject, public treeListElement
{
    Q_OBJECT
public:
    explicit Tree(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // TREE_H
