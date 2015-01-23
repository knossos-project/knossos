#include "nodecommentdecorator.h"
#include "knossos-global.h"


NodeCommentDecorator::NodeCommentDecorator(QObject *parent) :
    QObject(parent)
{

}

char *NodeCommentDecorator::content(commentListElement *self) {
    return self->content;
}

nodeListElement *NodeCommentDecorator::node(commentListElement *self) {
    return self->node;
}

QString NodeCommentDecorator::static_NodeComment_help() {
    return QString("A read-only class representing a comment in the node of the KNOSSOS skeleton. Access to attributes only via getter." \
            "\n TODO() : TODO" \
            "\n TODO() : TODO");
}
