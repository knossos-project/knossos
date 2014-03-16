#include "knossos-global.h"
#include "skeletonizer.h"
#include "functions.h"


skeletonState::skeletonState()
{
}

int skeletonState::skeleton_time() {
    return skeletonTime;
}

QString skeletonState::skeleton_file() {
    return skeletonFileAsQString;
}

void skeletonState::from_xml(const QString &filename) {
   if(!loadSkeleton(filename)) {
       emit echo(QString("could not load from %1").arg(filename));
   }
}

void skeletonState::to_xml(const QString &filename) {
    if(!saveSkeleton(filename)) {
        emit echo(QString("could not save to %1").arg(filename));
    }
}

treeListElement *skeletonState::first_tree() {
    return firstTree;
}

bool skeletonState::has_unsaved_changes() {
    return unsavedChanges;
}

/** @todo update treeview */
void skeletonState::delete_tree(int tree_id) {
   if(!Skeletonizer::delTree(CHANGE_MANUAL, tree_id, true)) {
       emit echo(QString("could not delete the tree with id %1").arg(tree_id));
   }
}

void skeletonState::delete_skeleton() {
    emit clearSkeletonSignal();
}

void skeletonState::set_active_node(int node_id) {
    if(!Skeletonizer::setActiveNode(CHANGE_MANUAL, 0, node_id)) {
        emit echo(QString("could not set the node with id %1 to active node").arg(node_id));
    }
}

nodeListElement *skeletonState::active_node() {
    return this->activeNode;
}

void skeletonState::add_node(int node_id, int x, int y, int z, int parent_tree_id, float radius, int inVp, int inMag, int time) {
    if(!checkNodeParameter(node_id, x, y, z)) {
        emit echo(QString("one of the first four arguments is in negative range. node is rejected"));
        return;
    }

    if(inVp < VIEWPORT_XY or inVp > VIEWPORT_ARBITRARY) {
        emit echo(QString("viewport argument is out of range. node is rejected"));
        return;
    }

    Coordinate coordinate(x, y, z);
    if(Skeletonizer::addNode(CHANGE_MANUAL, node_id, radius, parent_tree_id, &coordinate, inVp, inMag, time, false, false)) {
        Skeletonizer::setActiveNode(CHANGE_MANUAL, activeNode, node_id);
        emit nodeAddedSignal();
    } else {
        emit echo(QString("could not add the node with node id %1").arg(node_id));
    }
}

QList<treeListElement *> *skeletonState::trees() {
    QList<treeListElement *> *trees = new QList<treeListElement *>();
    treeListElement *currentTree = firstTree;
    while(currentTree) {
        trees->append(currentTree);
        currentTree = currentTree->next;
    }
    return trees;
}

void skeletonState::add_tree(int tree_id, const QString &comment, float r, float g, float b, float a) {
    color4F color(r, g, b, a);
    treeListElement *theTree = Skeletonizer::addTreeListElement(true, CHANGE_MANUAL, tree_id, color, false);
    if(!theTree) {
        emit echo(QString("could not add the tree with tree id %1").arg(tree_id));
        return;
    }

    if(comment.isEmpty() == false) {
        Skeletonizer::addTreeComment(CHANGE_MANUAL, tree_id, comment.toLocal8Bit().data());
    }

    Skeletonizer::setActiveTreeByID(tree_id);
    emit updateToolsSignal();
    emit treeAddedSignal(theTree);

}

void skeletonState::add_comment(int node_id, char *comment) {
    nodeListElement *node = Skeletonizer::findNodeByNodeID(node_id);
    if(node) {
        if(!Skeletonizer::addComment(CHANGE_MANUAL, QString(comment), node, 0, false)) {
            emit echo(QString("An unexpected error occured while adding a comment for node id %1").arg(node_id));
        }
    } else {
        emit echo(QString("no node id id %1 found").arg(node_id));
    }
}

void skeletonState::add_segment(int source_id, int target_id) {
    if(Skeletonizer::addSegment(CHANGE_MANUAL, source_id, target_id, false)) {

    } else {
       emit echo(QString("could not add a segment with source id %1 and target id %2").arg(source_id).arg(target_id));
    }
}

void skeletonState::add_branch_node(int node_id) {
    nodeListElement *currentNode = Skeletonizer::findNodeByNodeID(node_id);
    if(currentNode) {
        if(Skeletonizer::pushBranchNode(CHANGE_MANUAL, true, false, currentNode, 0, false)) {
            emit updateToolsSignal();
        } else {
            emit echo(QString("An unexpected error occured while adding a branch node"));
        }

    } else {
        emit echo(QString("no node with id %1 found").arg(node_id));
    }

}

QString skeletonState::help() {
    return QString("The python representation of knossos. You gain access to the following methods:" \
                   "\n\n GETTER:" \
                   "\n trees() : returns a list of trees" \
                   "\n active_node() : returns the active node object" \
                   "\n skeleton_file() : returns the file from which the current skeleton is loaded" \
                   "\n first_tree() : returns the first tree of the knossos skeleton" \
                   "\n\n SETTER:" \
                   "\n add_branch_node(node_id) : sets the node with node_id to branch_node" \
                   "\n add_segment(source_id, target_id) : adds a segment for the nodes. Both nodes must be added before"
                   "\n add_comment(node_id) : adds a comment for the node. Must be added before" \
                   "\n add_tree(tree_id, comment, r (opt), g (opt), b (opt), a (opt)) : adds a new tree" \
                   "\n\t If no color is set then the knossos lookup table sets the color." \
                   "\n\n add_node(node_id, x, y, z, parent_id (opt), radius (opt), viewport (opt), mag (opt), time (opt))" \
                   "\n\t adds a node for a parent tree. " \
                   "\n\t if no parent_id is set then the current active node will be chosen." \
                   "\n delete_tree(tree_id) : deletes the tree"
                   "\n delete_skeleton() : deletes the entire skeleton" \
                   "\n from_xml(filename) : loads a skeleton from a .nml file" \
                   "\n to_xml(filename) : saves a skeleton to a .nml file");

}
