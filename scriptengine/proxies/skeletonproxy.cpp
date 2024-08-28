/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
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
#include <QMessageBox>

treeListElement & treeFromId(decltype(treeListElement::treeID) treeId) {
    if (auto * tree = Skeletonizer::singleton().findTreeByTreeID(treeId)) {
        return *tree;
    }
    const auto errorText = QObject::tr("skeletonproxy treeFromId: not tree with id %1").arg(treeId);
    qWarning() << errorText;
    throw std::runtime_error(errorText.toStdString());
}

treeListElement *SkeletonProxy::tree_with_previous_id(quint64 tree_id) {
    treeListElement *tree = Skeletonizer::singleton().findTreeByTreeID(tree_id);
    return Skeletonizer::singleton().getTreeWithPrevID(tree);
}

treeListElement *SkeletonProxy::tree_with_next_id(quint64 tree_id) {
    treeListElement *tree = Skeletonizer::singleton().findTreeByTreeID(tree_id);
    return Skeletonizer::singleton().getTreeWithNextID(tree);
}

void SkeletonProxy::move_to_next_tree() {
    state->viewer->mainWindow.widgetContainer.annotationWidget.skeletonTab.jumpToNextTree(true);
}

void SkeletonProxy::move_to_previous_tree() {
    state->viewer->mainWindow.widgetContainer.annotationWidget.skeletonTab.jumpToNextTree(false);
}

bool SkeletonProxy::extract_connected_component(quint64 node_id) {
    return Skeletonizer::singleton().extractConnectedComponent(node_id);
}

treeListElement *SkeletonProxy::find_tree_by_id(quint64 tree_id) {
    return Skeletonizer::singleton().findTreeByTreeID(tree_id);
}

QList<treeListElement *> SkeletonProxy::find_trees(const QString & comment) {
    return Skeletonizer::singleton().findTreesContainingComment(comment);
}

treeListElement *SkeletonProxy::first_tree() {
    return &state->skeletonState->trees.front();
}

bool SkeletonProxy::delete_tree(quint64 tree_id) {
    if (!Skeletonizer::singleton().delTree(tree_id)) {
        emit echo(QString("could not delete the tree with id %1").arg(tree_id));
        return false;
    }
    return true;
}

bool SkeletonProxy::merge_trees(quint64 tree_id, quint64 other_tree_id) {
    if (!Skeletonizer::singleton().mergeTrees(tree_id, other_tree_id)) {
       emit echo (QString("Skeletonizer::mergeTrees failed!"));
       return false;
    }
    return true;
}

void SkeletonProxy::add_tree_mesh(quint64 tree_id, QVector<float> & verts, QVector<float> & normals, QVector<unsigned int> & indices, const QVector<float> & color, int draw_mode, bool swap_xy) {
    if (verts.size() % 3 != 0) {
        throw std::runtime_error(QObject::tr("SkeletonProxy::add_tree_mesh failed: vertex coordinates not divisible by 3, got %1 vert coords.").arg(verts.size()).toStdString());
    }
    if (draw_mode == GL_TRIANGLES && indices.size() % 3 != 0) {
        throw std::runtime_error(QObject::tr("SkeletonProxy::add_tree_mesh failed: triangles coordinates not divisible by 3, got %1 indices.").arg(indices.size()).toStdString());
    }
    if (!normals.empty() && normals.size() != verts.size()) {
        throw std::runtime_error(QObject::tr("SkeletonProxy::add_tree_mesh failed: vertex to normal count mismatch (should be equal), got %1 vert coords, %2 normal coords.").arg(verts.size()).arg(normals.size()).toStdString());
    }
    if (indices.empty()) {
        throw std::runtime_error(QObject::tr("SkeletonProxy::add_tree_mesh failed: indices required").toStdString());
    }
    const auto correctColorSize = verts.size()/3 * 4;
    if (!color.empty() && color.size() != correctColorSize) {
        throw std::runtime_error(QObject::tr("SkeletonProxy::add_tree_mesh failed: number of color components not 0 or 4 × vertices (%1), got %2 color components.").arg(correctColorSize).arg(color.size()).toStdString());
    }
    QVector<std::uint8_t> uintColors(color.size());
    for (int i = 0 ; i < color.size(); ++i) {
        uintColors[i] = color[i] * 255;
    }
    Skeletonizer::singleton().addMeshToTree(tree_id, verts, normals, indices, uintColors, draw_mode, swap_xy);
}

void SkeletonProxy::delete_tree_mesh(quint64 tree_id) {
    Skeletonizer::singleton().deleteMeshOfTree(treeFromId(tree_id));
}

nodeListElement *SkeletonProxy::find_node_by_id(quint64 node_id) {
    return Skeletonizer::singleton().findNodeByNodeID(node_id);
}

QList<nodeListElement *> SkeletonProxy::find_nodes_in_tree(treeListElement & tree, const QString & comment) {
    return Skeletonizer::findNodesInTree(tree, comment);
}

void SkeletonProxy::move_node_to_tree(quint64 node_id, quint64 tree_id) {
    nodeListElement *node = Skeletonizer::singleton().findNodeByNodeID(node_id);
    Skeletonizer::singleton().select(QSet{node});
    Skeletonizer::singleton().moveSelectedNodesToTree(tree_id);
}

nodeListElement *SkeletonProxy::find_nearby_node_from_tree(quint64 tree_id, int x, int y, int z) {
    treeListElement *tree = Skeletonizer::singleton().findTreeByTreeID(tree_id);
    Coordinate coord(x, y, z);
    return Skeletonizer::singleton().findNearbyNode(tree, coord);
}

nodeListElement *SkeletonProxy::node_with_prev_id(quint64 node_id, bool same_tree) {
    nodeListElement *node = Skeletonizer::singleton().findNodeByNodeID(node_id);
    return Skeletonizer::singleton().getNodeWithPrevID(node, same_tree);
}

nodeListElement *SkeletonProxy::node_with_next_id(quint64 node_id, bool same_tree) {
    nodeListElement *node = Skeletonizer::singleton().findNodeByNodeID(node_id);
    return Skeletonizer::singleton().getNodeWithNextID(node, same_tree);
}

bool SkeletonProxy::set_radius(const quint64 node_id, const float radius) {
    auto * node = Skeletonizer::singleton().findNodeByNodeID(node_id);
    if (node != nullptr) {
        Skeletonizer::singleton().setRadius(*node, radius);
        return true;
    }
    return false;
}

bool SkeletonProxy::set_position(const quint64 node_id, const QVector3D & position) {
    auto * node = Skeletonizer::singleton().findNodeByNodeID(node_id);
    if (node != nullptr) {
        Skeletonizer::singleton().setPosition(*node, Coordinate(position.x(), position.y(), position.z()));
        return true;
    }
    return false;
}

void SkeletonProxy::skeleton_filename() {
    QMessageBox box;
    box.setIcon(QMessageBox::Warning);
    box.setText("skeleton.skeleton_filename() has been removed. Use knossos.annotation_filename() instead.");
    box.exec();
}

void SkeletonProxy::clear_skeleton() {
    Skeletonizer::singleton().clearSkeleton();
}

QString SkeletonProxy::save_skeleton() {
    QString save_string;
    QXmlStreamWriter xml(&save_string);
    Skeletonizer::singleton().saveXmlSkeleton(xml);
    return save_string;
}

void SkeletonProxy::load_skeleton(QString & xml_string, const bool merge, const QString & treeCmtOnMultiLoad) {
    QXmlStreamReader xml(xml_string);
    Skeletonizer::singleton().loadXmlSkeleton(xml, merge, treeCmtOnMultiLoad);
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
    return Annotation::singleton().unsavedChanges;
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
    if (!Skeletonizer::singleton().delNode(node_id, nullptr)) {
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
    return &Skeletonizer::singleton().addTreeAndActivate(boost::none, boost::none, properties);
}
treeListElement * SkeletonProxy::add_tree(quint64 tree_id, const QVariantHash &properties) {
    return &Skeletonizer::singleton().addTreeAndActivate(tree_id, boost::none, properties);
}

void SkeletonProxy::set_tree_comment(quint64 tree_id, const QString & comment) {
    Skeletonizer::singleton().setComment(treeFromId(tree_id), comment);
}

void SkeletonProxy::set_tree_color(quint64 tree_id, const QColor & color) {
    Skeletonizer::singleton().setColor(treeFromId(tree_id), color);
}

bool SkeletonProxy::get_tree_render(quint64 tree_id) {
    return treeFromId(tree_id).render;
}

void SkeletonProxy::set_tree_render(quint64 tree_id, const bool render) {
    Skeletonizer::singleton().setRender(treeFromId(tree_id), render);
}

QColor SkeletonProxy::get_tree_color(quint64 tree_id) {
    return treeFromId(tree_id).color;
}

bool SkeletonProxy::set_active_tree(quint64 tree_id) {
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
    auto * sourceNode = Skeletonizer::singleton().findNodeByNodeID(source_id);
    auto * targetNode = Skeletonizer::singleton().findNodeByNodeID(target_id);
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
    nodeListElement *currentNode = Skeletonizer::singleton().findNodeByNodeID(node_id);
    if(nullptr == currentNode) {
        emit echo(QString("no node with id %1 found").arg(node_id));
        return false;
    }
    Skeletonizer::singleton().pushBranchNode(*currentNode);
    return true;
}

QList<nodeListElement *> SkeletonProxy::selected_nodes() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return {std::cbegin(state->skeletonState->selectedNodes), std::cend(state->skeletonState->selectedNodes)};
#else
    return QVector<nodeListElement *>::fromStdVector(state->skeletonState->selectedNodes).toList();
#endif
}

void SkeletonProxy::select_nodes(QList<nodeListElement *> nodes) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    Skeletonizer::singleton().select(QSet(std::cbegin(nodes), std::cend(nodes)));
#else
    Skeletonizer::singleton().select(nodes.toSet());
#endif
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

quint64 SkeletonProxy::last_clicked_mesh_id() {
    if (state->skeletonState->meshLastClickInformation) {
        return state->skeletonState->meshLastClickInformation.get().treeId;
    } else {
        throw std::runtime_error{"none available"};
    }
}
