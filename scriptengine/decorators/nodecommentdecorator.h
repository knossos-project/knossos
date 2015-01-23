#ifndef NODECOMMENTDECORATOR_H
#define NODECOMMENTDECORATOR_H

#include "knossos-global.h"

#include <QObject>

class NodeCommentDecorator : public QObject
{
    Q_OBJECT
public:
    explicit NodeCommentDecorator(QObject *parent = 0);
    
signals:
    
public slots:

    char *content(commentListElement *self);
    nodeListElement *node(commentListElement *self);
    QString static_NodeComment_help();
};

#endif // NODECOMMENTDECORATOR_H
