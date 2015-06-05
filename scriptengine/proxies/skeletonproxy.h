#ifndef SKELETONPROXY_H
#define SKELETONPROXY_H

#include "skeleton/node.h"

#include <QList>
#include <QObject>
#include <QVBoxLayout>

class QMenuBar;
class QToolBar;

/** Actually this class it not really needed. It only hides the SIGNALS from the SkeletonProxy */
class SkeletonProxySignalDelegate : public QObject  {
    Q_OBJECT
signals:
    void loadSkeleton(const QString &filename, const QString &treeCmtOnMultiLoad, bool *isSuccess = NULL);
    void saveSkeleton(const QString & filename, bool *isSuccess = NULL);
    void clearSkeletonSignal();
};

extern SkeletonProxySignalDelegate *skeletonProxySignalDelegate;

class SkeletonProxy : public QObject
{
    Q_OBJECT
public:
    explicit SkeletonProxy(QObject *parent = 0);

signals:
    void echo(QString message);


public slots:
    int skeleton_time();
    QString skeleton_file();
    bool annotation_save(const QString &filename);
    bool annotation_load(const QString &filename, bool isMerge = false);
    bool has_unsaved_changes();

    void delete_skeleton();
    treeListElement *find_tree_by_id(int tree_id);
    treeListElement *first_tree();
    treeListElement *tree_with_previous_id(int tree_id);
    treeListElement *tree_with_next_id(int tree_id);
    QList<treeListElement *> *trees();
    int findAvailableTreeID();
    bool add_tree(int tree_id = 0, float r = -1, float g = -1, float b = -1, float a = 1);
    bool set_tree_comment(int tree_id, const QString &comment);
    bool set_active_tree(int tree_id);
    bool delete_tree(int tree_id);
    bool merge_trees(int tree_id, int other_tree_id);
    bool move_to_next_tree();
    bool move_to_previous_tree();

    nodeListElement *find_node_by_id(int node_id);
    nodeListElement *find_node_in_radius(int x, int y, int z);
    bool move_node_to_tree(int node_id, int tree_id);
    nodeListElement *find_nearby_node_from_tree(int tree_id, int x, int y, int z);
    nodeListElement *node_with_prev_id(int node_id, bool same_tree);
    nodeListElement *node_with_next_id(int node_id, bool same_tree);
    bool edit_node(int node_id, float radius, int x, int y, int z, int in_mag);
    bool jump_to_active_node();
    bool delete_node(int node_id);
    bool set_active_node(int node_id);
    nodeListElement *active_node();
    int findAvailableNodeID();
    bool add_node(int node_id, int x, int y, int z, int parent_tree_id = 0,
                  float radius = 1.5, int inVp = ViewportType::VIEWPORT_UNDEFINED, int inMag = 1,
                  int time = 0);
    bool set_branch_node(int node_id);
    bool add_segment(int source_id, int target_id);
    bool delete_segment(int source_id, int target_id);
    segmentListElement *find_segment(int source_id, int target_id);
    bool delete_comment(int node_id);
    bool set_comment(int node_id, char *comment);

    void export_converter(const QString &path);

    QList<nodeListElement *> *selectedNodes();
    void selectNodes(QList<nodeListElement *> nodes);

    static QString help();
};

#endif // SKELETONPROXY_H
