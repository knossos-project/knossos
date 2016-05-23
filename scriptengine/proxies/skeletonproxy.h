#ifndef SKELETONPROXY_H
#define SKELETONPROXY_H

#include "skeleton/node.h"
#include "skeleton/tree.h"
#include "widgets/viewport.h"

#include <QList>
#include <QObject>
#include <QString>

class SkeletonProxy : public QObject {
    Q_OBJECT
signals:
    void echo(QString message);

public slots:
    int annotation_time();
    QString skeleton_file();
    bool has_unsaved_changes();

    void delete_skeleton();
    treeListElement *find_tree_by_id(int tree_id);
    QList<treeListElement *> find_trees(const QString & comment);
    treeListElement *first_tree();
    treeListElement *tree_with_previous_id(int tree_id);
    treeListElement *tree_with_next_id(int tree_id);
    QList<treeListElement *> trees();
    treeListElement * add_tree();
    treeListElement * add_tree(int tree_id);
    bool set_tree_comment(int tree_id, const QString & comment);
    void set_tree_color(int tree_id, const QColor & color);
    bool set_active_tree(int tree_id);
    bool delete_tree(int tree_id);
    bool merge_trees(int tree_id, int other_tree_id);
    bool move_to_next_tree();
    bool move_to_previous_tree();

    nodeListElement *find_node_by_id(quint64 node_id);
    QList<nodeListElement *> find_nodes_in_tree(treeListElement & tree, const QString & comment);
    void move_node_to_tree(quint64 node_id, int tree_id);
    nodeListElement *find_nearby_node_from_tree(int tree_id, int x, int y, int z);
    nodeListElement *node_with_prev_id(quint64 node_id, bool same_tree);
    nodeListElement *node_with_next_id(quint64 node_id, bool same_tree);
    bool edit_node(quint64 node_id, float radius, int x, int y, int z, int in_mag);
    void jump_to_node(nodeListElement *node);
    bool delete_node(quint64 node_id);
    bool set_active_node(quint64 node_id);
    nodeListElement *active_node();
    bool add_node(quint64 node_id, int x, int y, int z, int parent_tree_id = 0,
                  float radius = 1.5, int inVp = ViewportType::VIEWPORT_UNDEFINED, int inMag = 1,
                  int time = 0);
    bool set_branch_node(quint64 node_id);
    bool add_segment(quint64 source_id, quint64 target_id);
    bool delete_segment(quint64 source_id, quint64 target_id);
    bool delete_comment(quint64 node_id);
    bool set_comment(quint64 node_id, char *comment);

    void export_converter(const QString &path);

    QList<nodeListElement *> *selectedNodes();
    void selectNodes(QList<nodeListElement *> nodes);

    static QString help();
};

#endif // SKELETONPROXY_H
