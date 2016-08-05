/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "skeletonproxy.h"

#include "functions.h"
#include "stateInfo.h"
#include "skeleton/node.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFile>

treeListElement *SkeletonProxy::tree_with_previous_id(int tree_id) {
    treeListElement *tree = Skeletonizer::findTreeByTreeID(tree_id);
    return Skeletonizer::singleton().getTreeWithPrevID(tree);
}

treeListElement *SkeletonProxy::tree_with_next_id(int tree_id) {
    treeListElement *tree = Skeletonizer::findTreeByTreeID(tree_id);
    return Skeletonizer::singleton().getTreeWithNextID(tree);
}

bool SkeletonProxy::move_to_next_tree() {
    return Skeletonizer::singleton().moveToNextTree();
}

bool SkeletonProxy::move_to_previous_tree() {
    return Skeletonizer::singleton().moveToPrevTree();
}

treeListElement *SkeletonProxy::find_tree_by_id(int tree_id) {
    return Skeletonizer::findTreeByTreeID(tree_id);
}

QList<treeListElement *> SkeletonProxy::find_trees(const QString & comment) {
    return Skeletonizer::singleton().findTreesContainingComment(comment);
}

treeListElement *SkeletonProxy::first_tree() {
    return &state->skeletonState->trees.front();
}

bool SkeletonProxy::delete_tree(int tree_id) {
   if (!Skeletonizer::singleton().delTree(tree_id)) {
       emit echo(QString("could not delete the tree with id %1").arg(tree_id));
       return false;
   }
   return true;
}

bool SkeletonProxy::merge_trees(int tree_id, int other_tree_id) {
    if (!Skeletonizer::singleton().mergeTrees(tree_id, other_tree_id)) {
       emit echo (QString("Skeletonizer::mergeTrees failed!"));
       return false;
    }
    return true;
}

void SkeletonProxy::add_tree_pointcloud(int tree_id, const QVector<float> & vec) {
  qDebug() << vec;
  state->mainWindow->viewport3D->addTreePointcloud(tree_id);
}

nodeListElement *SkeletonProxy::find_node_by_id(quint64 node_id) {
    return Skeletonizer::findNodeByNodeID(node_id);
}

QList<nodeListElement *> SkeletonProxy::find_nodes_in_tree(treeListElement & tree, const QString & comment) {
    return Skeletonizer::findNodesInTree(tree, comment);
}

void SkeletonProxy::move_node_to_tree(quint64 node_id, int tree_id) {
    nodeListElement *node = Skeletonizer::findNodeByNodeID(node_id);
    Skeletonizer::singleton().selectNodes({node});
    Skeletonizer::singleton().moveSelectedNodesToTree(tree_id);
}

nodeListElement *SkeletonProxy::find_nearby_node_from_tree(int tree_id, int x, int y, int z) {
    treeListElement *tree = Skeletonizer::findTreeByTreeID(tree_id);
    Coordinate coord(x, y, z);
    return Skeletonizer::singleton().findNearbyNode(tree, coord);
}

nodeListElement *SkeletonProxy::node_with_prev_id(quint64 node_id, bool same_tree) {
    nodeListElement *node = Skeletonizer::findNodeByNodeID(node_id);
    return Skeletonizer::singleton().getNodeWithPrevID(node, same_tree);
}

nodeListElement *SkeletonProxy::node_with_next_id(quint64 node_id, bool same_tree) {
    nodeListElement *node = Skeletonizer::findNodeByNodeID(node_id);
    return Skeletonizer::singleton().getNodeWithNextID(node, same_tree);
}

bool SkeletonProxy::edit_node(quint64 node_id, float radius, int x, int y, int z, int in_mag) {
    if (!Skeletonizer::singleton().editNode(node_id, 0, radius, {x, y, z}, in_mag)) {
        emit echo (QString("Skeletonizer::editNode failed!"));
        return false;
    }
    return true;
}

int SkeletonProxy::annotation_time() {
    return Session::singleton().getAnnotationTime();
}

QString SkeletonProxy::skeleton_file() {
    return Session::singleton().annotationFilename;
}

// TEST LATER
void SkeletonProxy::export_converter(const QString &path) {
    QDir dir(path);
    if(!dir.exists()) {
        emit echo("path does not exist");
        return;
    }

    QFile targetFile(path + "converter.py");
    if(!targetFile.open(QIODevice::WriteOnly)) {
        emit echo("error creating a file in this path");
        return;
    }

    QTextStream stream(&targetFile);

    QFile converter(":/misc/python/converter.py");
    if(!converter.open(QIODevice::ReadOnly)) {
        emit echo("error while reading the converter from the resource directory");
        return;
    }

    stream << converter.readAll();

    converter.close();
    targetFile.close();

    emit echo("exported to (" + targetFile.fileName() + ")");

}

void SkeletonProxy::jump_to_node(nodeListElement *node) {
    if(node) {
        Skeletonizer::singleton().jumpToNode(*node);
    }
}

bool SkeletonProxy::has_unsaved_changes() {
    return Session::singleton().unsavedChanges;
}

void SkeletonProxy::delete_skeleton() {
    Skeletonizer::singleton().clearSkeleton();
}

bool SkeletonProxy::delete_segment(quint64 source_id, quint64 target_id) {
    auto * sourceNode = Skeletonizer::singleton().findNodeByNodeID(source_id);
    auto * targetNode = Skeletonizer::singleton().findNodeByNodeID(target_id);
    if (sourceNode && targetNode) {
        auto segmentIt = Skeletonizer::findSegmentBetween(*sourceNode, *targetNode);
        if (segmentIt != std::end(sourceNode->segments)) {
            Skeletonizer::singleton().delSegment(segmentIt);
            return true;
        }
    }
    return false;
}

bool SkeletonProxy::delete_node(quint64 node_id) {
    if (!Skeletonizer::singleton().delNode(node_id, NULL)) {
        emit echo(QString("could not delete the node with id %1").arg(node_id));
        return false;
    }
    return true;
}

bool SkeletonProxy::set_active_node(quint64 node_id) {
    if (!Skeletonizer::singleton().setActiveNode(find_node_by_id(node_id))) {
        emit echo(QString("could not set the node with id %1 to active node").arg(node_id));
        return false;
    }
    return true;
}

nodeListElement *SkeletonProxy::active_node() {
    return state->skeletonState->activeNode;
}

auto & addNode(boost::optional<decltype(nodeListElement::nodeID)> nodeId, const QList<int> & coordinate, const treeListElement & parent_tree, const QVariantHash & properties) {
    auto nodeIt = Skeletonizer::singleton().addNode(nodeId, {coordinate}, parent_tree, properties);
    if (!nodeIt) {
        throw std::runtime_error((QObject::tr("could not add node") + (nodeId ? QObject::tr(" with nodeid %1").arg(nodeId.get()) : QObject::tr(""))).toStdString());
    }
    return nodeIt.get();
}
nodeListElement * SkeletonProxy::add_node(const QList<int> & coordinate, const treeListElement & parent_tree, const QVariantHash & properties) {
    return &addNode(boost::none, coordinate, parent_tree, properties);
}
nodeListElement * SkeletonProxy::add_node(quint64 node_id, const QList<int> & coordinate, const treeListElement & parent_tree, const QVariantHash & properties) {
    return &addNode(boost::make_optional(static_cast<decltype(nodeListElement::nodeID)>(node_id)), coordinate, parent_tree, properties);
}

QList<treeListElement*> SkeletonProxy::trees() {
    QList<treeListElement*> trees;
    for (auto & tree : state->skeletonState->trees) {
        trees.append(&tree);
    }
    return trees;
}

treeListElement * SkeletonProxy::add_tree(const QVariantHash & properties) {
    return &Skeletonizer::singleton().addTree(boost::none, boost::none, properties);
}
treeListElement * SkeletonProxy::add_tree(int tree_id, const QVariantHash &properties) {
    return &Skeletonizer::singleton().addTree(tree_id, boost::none, properties);
}

treeListElement & treeFromId(int treeId) {
    if (auto * tree = Skeletonizer::findTreeByTreeID(treeId)) {
        return *tree;
    }
    const auto errorText = QObject::tr("skeletonproxy treeFromId: not tree with id %1").arg(treeId);
    qWarning() << errorText;
    throw std::runtime_error(errorText.toStdString());
}

void SkeletonProxy::set_tree_comment(int tree_id, const QString & comment) {
    Skeletonizer::singleton().setComment(treeFromId(tree_id), comment);
}

void SkeletonProxy::set_tree_color(int tree_id, const QColor & color) {
    Skeletonizer::singleton().setColor(treeFromId(tree_id), color);
}

bool SkeletonProxy::set_active_tree(int tree_id) {
    if (!Skeletonizer::singleton().setActiveTreeByID(tree_id)) {
        emit echo(QString("could not set active tree (id %1)").arg(tree_id));
        return false;
    }
    return true;
}

bool SkeletonProxy::set_comment(quint64 node_id, char *comment) {
    auto node = state->skeletonState->nodesByNodeID[node_id];
    if (node) {
        Skeletonizer::singleton().setComment(*node, QString(comment));
        return true;
    }
    return false;
}

bool SkeletonProxy::delete_comment(quint64 node_id) {
    auto node = state->skeletonState->nodesByNodeID[node_id];
    if (node) {
        Skeletonizer::singleton().setComment(*node, "");
        return true;
    }
    return false;
}

bool SkeletonProxy::add_segment(quint64 source_id, quint64 target_id) {
    auto * sourceNode = Skeletonizer::findNodeByNodeID(source_id);
    auto * targetNode = Skeletonizer::findNodeByNodeID(target_id);
    if(sourceNode != nullptr && targetNode != nullptr) {
        if (!Skeletonizer::singleton().addSegment(*sourceNode, *targetNode)) {
            emit echo(QString("could not add a segment with source id %1 and target id %2").arg(source_id).arg(target_id));
            return false;
        }
        return true;
    }
    return false;
}

bool SkeletonProxy::set_branch_node(quint64 node_id) {
    nodeListElement *currentNode = Skeletonizer::findNodeByNodeID(node_id);
    if(nullptr == currentNode) {
        emit echo(QString("no node with id %1 found").arg(node_id));
        return false;
    }
    Skeletonizer::singleton().pushBranchNode(*currentNode);
    return true;
}

QList<nodeListElement *> *SkeletonProxy::selectedNodes() {
    return new QList<nodeListElement *>(QVector<nodeListElement *>::fromStdVector(state->skeletonState->selectedNodes).toList());
}

void SkeletonProxy::selectNodes(QList<nodeListElement *> nodes) {
    Skeletonizer::singleton().selectNodes(nodes.toSet());
}

QString SkeletonProxy::help() {
    return QString("This is the unique main interface between python and knossos. You can't create a separate instance of this object:" \
                   "\n\n skeleton_time() : return the skeleton time" \
                   "\n\n skeleton_file() : returns the file from which the current skeleton is loaded" \
                   "\n from_xml(filename) : loads a skeleton from a .nml file" \
                   "\n to_xml(filename) : saves a skeleton to a .nml file" \
                   "\n has_unsaved_changes() : self-explaining" \
                   "\n active_node() : returns the active node object" \
                   "\n delete_skeleton() : deletes the entire skeleton." \
                   "\n delete_tree(tree_id) : deletes the tree with the passed id. Returns a message if no such tree exists." \
                   "\n delete_active_tree() : deletes the active tree or informs about that no active tree could be deleted." \
                   "\n find_tree_by_id(tree_id) : returns the searched tree or returns None" \
                   "\n first_tree() : returns the first tree of the knossos skeleton" \
                   "\n tree_with_previous_id(tree_id) : returns the tree with a previous id or None" \
                   "\n tree_with_next_id(tree_id) : returns the tree with the next id or None" \
                   "\n trees() : returns a list of trees" \
                   "\n add_tree(tree_id, comment, r, g, b, a) : adds a new tree. All parameter are optional." \
                   "\n knossos will give the next free tree id and uses the color lookup table to determine a color" \
                   "\n merge_trees(id1, id2) : merges two different trees to a single one. " \
                   "\n move_to_next_tree() : moves the viewer to the next tree" \
                   "\n move_to_prev_tree() : moves the viewer to the previosu tree" \
                   "\n export_converter(path) : creates a python class in the path which can be used to convert between the NewSkeleton class and KNOSSOS." \
                   "\n set_branch_node(node_id) : sets the node with node_id to branch_node" \
                   "\n add_segment(source_id, target_id) : adds a segment for the nodes. Both nodes must be added before" \
                   "\n delete_active_node() : deletes the active node or informs about that no active node could be deleted" \
                   "\n delete_segment(source_id, target_id) : deletes a segment with source" \
                   "\n add_comment(node_id) : adds a comment for the node. Must be added before" \

                   "\n\t If does not mind if no color is specified. The lookup table sets this automatically." \
                   "\n\n add_node(node_id, x, y, z, parent_id (opt), radius (opt), viewport (opt), mag (opt), time (opt))" \
                   "\n\t adds a node for a parent tree where a couple of parameter are optional. " \
                   "\n\t if no parent_id is set then the current active node will be chosen." \

                   "\n cube_data_at(x, y, z) : returns the data cube at the viewport position (x, y, z) as a string containing 128 * 128 * 128 bytes (2MB) of grayscale values. " \
                   "\n move_to(x, y, z) : recenters the viewport coordinates to (x, y, z)" \
                   "\n save_working_directory(path) : saves the working directory from the console");
}
