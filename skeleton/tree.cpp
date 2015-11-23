#include "tree.h"

#include "node.h"

#include <QString>

treeListElement::treeListElement(const int id) : treeID{id} {
    memset(comment, '\0', sizeof(comment));
}

QList<nodeListElement *> *treeListElement::getNodes() {
    QList<nodeListElement *> * nodes = new QList<nodeListElement *>();

    for (auto & node : this->nodes) {
        nodes->append(&node);
    }

    return nodes;
}

QList<segmentListElement *> treeListElement::getSegments() {
    QSet<segmentListElement *> *complete_segments = new QSet<segmentListElement *>();

    QList<nodeListElement *> *nodes = this->getNodes();
    for(int i = 0; i < nodes->size(); i++) {
        QList<segmentListElement *> *segments = nodes->at(i)->getSegments();
        for(int j = 0; j < segments->size(); j++) {
            segmentListElement *segment = segments->at(j);
            if(!complete_segments->contains(segment)) {
                complete_segments->insert(segment);
            }
        }
    }

    return complete_segments->toList();
}
