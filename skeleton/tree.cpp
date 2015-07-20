#include "tree.h"

#include "node.h"

#include <QString>

QList<nodeListElement *> *treeListElement::getNodes() {
    QList<nodeListElement *> *nodes = new QList<nodeListElement *>();

    nodeListElement *currentNode = firstNode.get();
    while(currentNode) {
        nodes->append(currentNode);
        currentNode = currentNode->next.get();
    }

    return nodes;
}

QList<segmentListElement *> treeListElement::getSegments() {
    QSet<segmentListElement *> *complete_segments = new QSet<segmentListElement *>();

    QList<nodeListElement *> *nodes = this->getNodes();
    for(int i = 0; i < nodes->size(); i++) {
        QList<segmentListElement *> *segments = nodes->at(i)->getSegments();
        for(int j = 0; j < segments->size(); j++) {
            segmentListElement *segment = segments->at(i);
            if(!complete_segments->contains(segment)) {
                complete_segments->insert(segment);
            }
        }
    }

    return complete_segments->toList();
}


