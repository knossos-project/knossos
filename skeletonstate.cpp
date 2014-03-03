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

PyObject *skeletonState::addNewSkeleton(PyObject *args) {
    PyObject *newSkeleton;

    if(!PyArg_Parse(args, "O", &newSkeleton)) {
        qDebug() << "No object";
        Py_RETURN_NONE;
    }


    PyObject *annotations;

    if(PyObject_HasAttrString(newSkeleton, "annotations")) {
        annotations = PyObject_GetAttrString(newSkeleton, "annotations");
        qDebug() << "found attribute annotations";

        if(!PySet_Check(annotations)) {
            qDebug() << "attribute annotations is not from type set";
            Py_RETURN_NONE;
        } else {
            qDebug() << "attribute annotations is from type set";
        }

        int size = PySet_Size(annotations);
        qDebug() << "there are " << size << " entries in the set";

        PyObject *iterator, *item;

        iterator = PyObject_GetIter(annotations);
        if(!iterator) {
            qDebug() << "error getting iterator from annotations";
            Py_RETURN_NONE;
        }

        PyObject *skeletonAnnotation;
        PyObject *nodes;
        while(item = PyIter_Next(iterator)) {
            if(!PyArg_Parse(item , "O", &skeletonAnnotation)) {
                qDebug() << "no object type skeletonAnnotation";
                Py_RETURN_NONE;
            }

            nodes = PyObject_GetAttrString(skeletonAnnotation, "nodes");
            if(!PySet_Check(nodes)) {
                qDebug() << "attribute nodes in not from type set";
                Py_RETURN_NONE;
            } else {
                qDebug() << "attributes nodes is from type set";
            }

            size = PySet_Size(nodes);
            qDebug() << "there are" << size << "entries in the set";

            iterator = PyObject_GetIter(nodes);
            if(!iterator) {
                qDebug() << "error getting iterator from nodes";
            }

            PyObject *skeletonNode;
            PyObject *data;
            while(item = PyIter_Next(iterator)) {
                if(!PyArg_Parse(item, "O", &skeletonNode)) {
                    qDebug() << "no object type SkeletonNode";
                    Py_RETURN_NONE;
                }

                data = PyObject_GetAttrString(skeletonNode, "data");

            }

        }

    } else {
       qDebug() << "no attribute annotations";
    }

}

void skeletonState::setIdleTime(uint idleTime) {
    state->skeletonState->idleTime = idleTime;
}

void skeletonState::setSkeletonTime(uint skeletonTime) {
    state->skeletonState->skeletonTime = skeletonTime;
}

void skeletonState::setEditPosition(int x, int y, int z) {
    emit userMoveSignal(x, y, z, TELL_COORDINATE_CHANGE);
}

void skeletonState::setActiveNode(int id) {
    Skeletonizer::setActiveNode(CHANGE_MANUAL, 0, id);
}

void skeletonState::addComment(int nodeID, char *comment) {
    nodeListElement *node = Skeletonizer::findNodeByNodeID(nodeID);
    if(node) {
        Skeletonizer::addComment(CHANGE_MANUAL, QString(comment), node, 0, false);
    }
}

void skeletonState::addBranchNode(int nodeID) {
    nodeListElement *currentNode = Skeletonizer::findNodeByNodeID(nodeID);
    if(currentNode)
        Skeletonizer::pushBranchNode(CHANGE_MANUAL, true, false, currentNode, 0, false);
}

/*
void skeletonState::parseTree(PyObject *skeletonAnnotation) {
    PyObject *nodes = PyObject_GetAttrString(skeletonAnnotation, "nodes");
    if(!nodes) {
        qDebug() << "no attribute nodes";
    }

    PyObject *set;
    if(!PyArg_Parse(nodes, "O", &nodes)) {
        qDebug() << "attribute node is no object";
    }

    if(!PySet_Check(set)) {
        qDebug() << "attribute data is not from type set";
    }

    PyObject *iterator, *item;

    iterator = PyObject_GetIter(set);
    if(!iterator) {
        qDebug() << "could not get the set iterator";
    }

    PyObject *skeletonNode;
    while(item = PyIter_Next(iterator)) {
        if(!PyArg_Parse(item, "O", &skeletonNode)) {
            qDebug() << "no object type";
        }

        parseNode(skeletonNode);
    }

    Py_RETURN_NONE;

}

void skeletonState::parseNode(PyObject *skeletonNode) {
    PyObject *data = PyObject_GetAttrString(skeletonNode, "data");

    if(!data) {
        qDebug() << "no attribute data";
    }

    PyObject *dictionary;
    if(!PyArg_Parse(data, "O", &dictionary)) {
        qDebug() << "attribute data is no object";
    }

    if(!PyDict_Check(dictionary)) {
        qDebug() << "attribute data is not from type object dictionary";
    }

    float radius;
    int x, y, z, id, inVp, inMag, time;

    PyObject *result;
    result = PyDict_GetItemString(dictionary, "id");
    if(!PyArg_Parse(result, "i", &id)) {
        qDebug() << "no value for x";
    }

    result = PyDict_GetItemString(dictionary, "radius");
    if(!PyArg_Parse(result, "f", &radius)) {
        qDebug() << "no value for radius";
    }

    result = PyDict_GetItemString(dictionary, "x");
    if(!PyArg_Parse(result, "i", &x)) {
        qDebug() << "no value for x";
    }

    result = PyDict_GetItemString(dictionary, "y");
    if(!PyArg_Parse(result, "i", &y)) {
        qDebug() << "no value for y";
    }

    result = PyDict_GetItemString(dictionary, "z");
    if(!PyArg_Parse(result, "i", &z)) {
        qDebug() << "no value for z";
    }

    result = PyDict_GetItemString(dictionary, "inMag");
    if(!PyArg_Parse(result, "i", &inMag)) {
        qDebug() << "no value for inMag";
    }

    result = PyDict_GetItemString(dictionary, "inVp");
    if(!PyArg_Parse(result, "i", &inVp)) {
        qDebug() << "no value for inVp";
    }

    result = PyDict_GetItemString(dictionary, "time");
    if(!PyArg_Parse(result, "i", &time)) {
        qDebug() << "no value for time";
    }

    qDebug() << "new node object with attributes: id=" << id << " radius=" << radius << " x=" << x << " y=" << y << " z=" << z;


}

void skeletonState::parseNewSkeleton(PyObject *args) {
    PyObject *newSkeleton;

    if(!PyArg_Parse(args, "O", &newSkeleton)) {
        qDebug() << "No object";
        Py_RETURN_NONE;
    }

    PyObject *annotations;

    if(PyObject_HasAttrString(newSkeleton, "annotations")) {
        annotations = PyObject_GetAttrString(newSkeleton, "annotations");
        qDebug() << "found attribute annotations";

        if(!PySet_Check(annotations)) {
            qDebug() << "attribute annotations is not from type set";
            Py_RETURN_NONE;
        } else {
            qDebug() << "attribute annotations is from type set";
        }

        parseNewSkeleton(annotations);

     }

}
*/
