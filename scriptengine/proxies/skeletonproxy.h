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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef SKELETONPROXY_H
#define SKELETONPROXY_H

#include "skeleton/node.h"
#include "skeleton/tree.h"
#include "widgets/viewports/viewportbase.h"

#include <QList>
#include <QObject>
#include <QString>

class SkeletonProxy : public QObject {
    Q_OBJECT
signals:
    void echo(QString message);

public slots:

    void skeleton_filename();
    bool has_unsaved_changes();
    void clear_skeleton();
    QString save_skeleton();
    void load_skeleton(QString & xml_string, const bool merge, const QString & treeCmtOnMultiLoad = "");

    void delete_skeleton();
    bool extract_connected_component(quint64 node_id);
    treeListElement *find_tree_by_id(quint64 tree_id);
    QList<treeListElement *> find_trees(const QString & comment);
    treeListElement *first_tree();
    treeListElement *tree_with_previous_id(quint64 tree_id);
    treeListElement *tree_with_next_id(quint64 tree_id);
    QList<treeListElement *> trees();
    treeListElement * add_tree(const QVariantHash & properties = {});
    treeListElement * add_tree(quint64 tree_id, const QVariantHash & properties = {});
    void set_tree_comment(quint64 tree_id, const QString & comment);
    void set_tree_color(quint64 tree_id, const QColor & color);
    void set_tree_render(quint64 tree_id, const bool render);
    QColor get_tree_color(quint64 tree_id);
    bool set_active_tree(quint64 tree_id);
    bool delete_tree(quint64 tree_id);
    bool merge_trees(quint64 tree_id, quint64 other_tree_id);
    void add_tree_mesh(quint64 tree_id, QVector<float> & verts, QVector<float> & normals, QVector<unsigned int> & indices, const QVector<float> & color, int draw_mode = 0, bool swap_xy = false);
    void delete_tree_mesh(quint64 tree_id);
    void move_to_next_tree();
    void move_to_previous_tree();

    nodeListElement *find_node_by_id(quint64 node_id);
    QList<nodeListElement *> find_nodes_in_tree(treeListElement & tree, const QString & comment);
    void move_node_to_tree(quint64 node_id, quint64 tree_id);
    nodeListElement *find_nearby_node_from_tree(quint64 tree_id, int x, int y, int z);
    nodeListElement *node_with_prev_id(quint64 node_id, bool same_tree);
    nodeListElement *node_with_next_id(quint64 node_id, bool same_tree);
    bool set_radius(const quint64 node_id, const float radius);
    bool set_position(const quint64 node_id, const QVector3D & position);
    void jump_to_node(nodeListElement *node);
    bool delete_node(quint64 node_id);
    bool set_active_node(quint64 node_id);
    nodeListElement *active_node();
    nodeListElement * add_node(const QList<int> & coordinate, const treeListElement & parent_tree, const QVariantHash & properties = {});
    nodeListElement * add_node(quint64 node_id, const QList<int> & coordinate, const treeListElement & parent_tree, const QVariantHash & properties = {});
    bool set_branch_node(quint64 node_id);
    bool add_segment(quint64 source_id, quint64 target_id);
    bool delete_segment(quint64 source_id, quint64 target_id);
    bool delete_comment(quint64 node_id);
    bool set_comment(quint64 node_id, char *comment);

    quint64 last_clicked_mesh_id();

    void export_converter(const QString &path);

    QList<nodeListElement *> *selected_nodes();
    void select_nodes(QList<nodeListElement *> nodes);

    static QString help();
};

#endif // SKELETONPROXY_H
