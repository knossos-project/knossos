#ifndef NODECOMMENTDECORATOR_H
#define NODECOMMENTDECORATOR_H

#include <QObject>

class commentListElement;
class nodeListElement;

class NodeCommentDecorator : public QObject {
    Q_OBJECT
public:
    explicit NodeCommentDecorator(QObject *parent = 0);

public slots:
    char *content(commentListElement *self);
    nodeListElement *node(commentListElement *self);
    QString static_NodeComment_help();
};

#endif // NODECOMMENTDECORATOR_H
