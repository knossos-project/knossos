#include "knossos-global.h"
#include "skeletonizer.h"
#include "functions.h"
#include <PythonQt/PythonQt.h>
extern stateInfo *state;


skeletonState::skeletonState()
{
}

int skeletonState::getSkeletonTime() {
    return state->skeletonState->skeletonTime;
}

bool skeletonState::hasUnsavedChanges() {
    return unsavedChanges;
}

treeListElement *skeletonState::getFirstTree() {
    return state->skeletonState->firstTree;
}

QString skeletonState::getSkeletonFile() {
    return skeletonFileAsQString;
}

bool skeletonState::fromXml(QString file) {
   return loadSkeleton(file);
}

bool skeletonState::toXml(QString file) {
    return saveSkeleton(file);
}

void skeletonState::addTree(treeListElement *tree) {
    if(!tree) {

        qDebug() << "tree is NULL";
        return;
    }

    if(!checkTreeParameter(tree->treeID, tree->color.r, tree->color.g, tree->color.b, tree->color.a)) {
        qDebug() << "one of the arguments is in negative range";
        return;
    }

    treeListElement *theTree = Skeletonizer::addTreeListElement(true, CHANGE_MANUAL, tree->treeID, tree->color, false);
    if(tree->comment) {
        Skeletonizer::addTreeComment(CHANGE_MANUAL, tree->treeID, tree->comment);
    }

    Skeletonizer::setActiveTreeByID(theTree->getTreeID());
    emit updateToolsSignal();
    emit treeAddedSignal(theTree);

    nodeListElement *currentNode = theTree->firstNode;
    while(currentNode) {
        addNode(currentNode);
        currentNode = currentNode->next;
    }
 }

void skeletonState::addTree(int treeID, Color color, QString comment) {
    if(!checkTreeParameter(treeID, color.r, color.g, color.b, color.a)) {
        qDebug() << "one of the arguments is in negative range";
        return;
    }


    color4F c4f;
    c4f.r = color.r;
    c4f.g = color.g;
    c4f.b = color.b;
    c4f.a = color.a;


    treeListElement *theTree = Skeletonizer::addTreeListElement(true, CHANGE_MANUAL, treeID, c4f, false);
    if(comment.isNull() == false) {
        Skeletonizer::addTreeComment(CHANGE_MANUAL, treeID, comment.toLocal8Bit().data());
    }

    Skeletonizer::setActiveTreeByID(treeID);
    emit updateToolsSignal();
    emit treeAddedSignal(theTree);
}

void skeletonState::addTree(int treeID, float r, float g, float b, float a, QString comment) {
    if(!checkTreeParameter(treeID, r, g, b, a)) {
        qDebug() << "one of the arguments is in negative range";
        return;
    }

    color4F color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;

    treeListElement *theTree = Skeletonizer::addTreeListElement(true, CHANGE_MANUAL, treeID, color, false);
    if(comment.isNull() == false) {
        Skeletonizer::addTreeComment(CHANGE_MANUAL, treeID, comment.toLocal8Bit().data());
    }

    Skeletonizer::setActiveTreeByID(treeID);
    emit updateToolsSignal();
    emit treeAddedSignal(theTree);

    nodeListElement *currentNode = theTree->firstNode;
    while(currentNode) {
        addNode(currentNode);
        currentNode = currentNode->next;
    }
}

void skeletonState::addNode(nodeListElement *node) {
    if(!node or !checkNodeParameter(node->nodeID, node->position.x, node->position.y, node->position.z)) {
        qDebug() << "one of the arguments is in negative range";
        return;
    }

    if(state->skeletonState->activeTree) {
        node->setParent(state->skeletonState->activeTree);
    } else {
        emit treeAddedSignal(state->skeletonState->firstTree);
    }

    if(Skeletonizer::addNode(CHANGE_MANUAL, node->nodeID, node->radius, node->getParentID(), &node->position, node->getViewport(), node->getMagnification(), node->getTime(), false, false)) {
        Skeletonizer::setActiveNode(CHANGE_MANUAL, 0, node->nodeID);
        emit nodeAddedSignal();
    }

}

void skeletonState::addNode(int nodeID, float radius, int x, int y, int z, int inVp, int inMag, int time) {
    if(!checkNodeParameter(nodeID, x, y, z)) {
        qDebug() << "one of the arguments is in negative range";
        return;
    }

    int activeID = 0;
    if(state->skeletonState->activeTree) {
        activeID = activeTree->treeID;
    }

    Coordinate coordinate;
    coordinate.x = x;
    coordinate.y = y;
    coordinate.z = z;

    if(Skeletonizer::addNode(CHANGE_MANUAL, nodeID, radius, activeID, &coordinate, inVp, inMag, time, false, false)) {
        Skeletonizer::setActiveNode(CHANGE_MANUAL, activeNode, nodeID);
        emit nodeAddedSignal();
    }

}

void skeletonState::addNode(int nodeID, int radius, int parentID, int x, int y, int z, int inVp, int inMag, int time) {
    if(!checkNodeParameter(nodeID, x, y, z)) {
        qDebug() << "one of the arguments is in negative range";
        return;
    }

    Coordinate coordinate;
    coordinate.x = x;
    coordinate.y = y;
    coordinate.z = z;

    if(Skeletonizer::addNode(CHANGE_MANUAL, nodeID, radius, parentID, &coordinate, inVp, inMag, time, false, false)) {
        Skeletonizer::setActiveNode(CHANGE_MANUAL, activeNode, nodeID);
        emit nodeAddedSignal();
    }
}

void skeletonState::addNode(int x, int y, int z, int viewport) {
    if(!checkNodeParameter(0, x, y, z)) {
        qDebug() << "one of the arguments is in negative range";
        return;
    }

    Coordinate coordinate;
    coordinate.x = x;
    coordinate.y = y;
    coordinate.z = z;

    if(viewport < VIEWPORT_XY | viewport > VIEWPORT_ARBITRARY) {
        qDebug() << "viewport is out of range";
        return;
    }

    //if(Skeletonizer::addNode(CHANGE_MANUAL)) {
        emit addNodeSignal(&coordinate, viewport);
        emit nodeAddedSignal();
    //}
}

QList<treeListElement *> *skeletonState::getTrees() {
    QList<treeListElement *> *trees = new QList<treeListElement *>();
    treeListElement *currentTree = state->skeletonState->firstTree;
    while(currentTree) {
        trees->append(currentTree);
        currentTree = currentTree->next;
    }
    return trees;
}

 // maybe a special case
void skeletonState::addTrees(QList<treeListElement *> *list)  {
    if(!list) {
        qDebug() << "list object is NULL";
        return;
    }

    for(int i = 0; i < list->size(); i++) {
        treeListElement *currentTree = list->at(i);
        if(currentTree and !checkTreeParameter(currentTree->treeID, currentTree->color.r, currentTree->color.g, currentTree->color.b, currentTree->color.a)) {
            return;
        }

        treeListElement *theTree = Skeletonizer::addTreeListElement(true, CHANGE_MANUAL, currentTree->treeID, currentTree->color, false);
        emit treeAddedSignal(currentTree);
        if(currentTree->comment) {
            Skeletonizer::addTreeComment(CHANGE_MANUAL, currentTree->treeID, currentTree->comment);
        }

        nodeListElement *currentNode = theTree->firstNode;
        while(currentNode) {            
           addNode(currentNode);

           QList<segmentListElement *> *segments = currentNode->getSegments();
           for(int i = 0; i < segments->size(); i++) {
                segmentListElement *segment = segments->at(i);
                addSegment(segment->source, segment->target);
           }


            currentNode = currentNode->next;
        }

    }
}

bool  skeletonState::deleteTree(int id) {
   return Skeletonizer::delTree(CHANGE_MANUAL, id, true);
}

void skeletonState::deleteSkeleton() {
    emit clearSkeletonSignal();
}

void skeletonState::addSegment(nodeListElement *source, nodeListElement *target) {
    if(source and target) {
        Skeletonizer::addSegment(CHANGE_MANUAL, source->nodeID, target->nodeID, false);
    } else {
        qDebug() << "source and target may not be NULL";
    }
}
