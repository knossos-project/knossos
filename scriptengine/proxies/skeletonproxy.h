#ifndef SKELETONPROXY_H
#define SKELETONPROXY_H

#include <QObject>
#include <QList>
#include <QVBoxLayout>
#include "knossos-global.h"

class QMenuBar;
class QToolBar;

/** Actually this class it not really needed. It only hides the SIGNALS from the SkeletonProxy */
class SkeletonProxySignalDelegate : public QObject  {
    Q_OBJECT
signals:
    bool moveToNextTreeSignal();
    bool moveToPreviousTreeSignal();
    bool loadSkeleton(QString file);
    bool saveSkeleton(QString file);
    bool addNodeSignal(Coordinate *Coordinate, Byte VPtype);
    void treeAddedSignal(treeListElement *tree);
    void nodeAddedSignal();
    void updateToolsSignal();
    bool jumpToActiveNodeSignal();
    void clearSkeletonSignal();
    void userMoveSignal(int x, int y, int z);    
    void updateTreeViewSignal();
    bool sliceExtractSignal(Byte *datacube, Byte *slice, vpConfig *vpConfig);
    void saveSettingsSignal(const QString &key, const QVariant &value);
    uint renderTextSignal(Coordinate *pos, char *string, uint currentVP, uint viewportType);
    QToolBar *toolBarSignal();
    QMenuBar *menuBarSignal();

};

extern SkeletonProxySignalDelegate *signalDelegate;

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
    void to_xml(const QString &filename);
    void from_xml(const QString &filename);
    bool has_unsaved_changes();

    void delete_skeleton();
    treeListElement *find_tree_by_id(int tree_id);
    treeListElement *first_tree();
    treeListElement *tree_with_previous_id(int tree_id);
    treeListElement *tree_with_next_id(int tree_id);
    QList<treeListElement *> *trees();
    void add_tree(int tree_id = 0, const QString &comment = 0, float r = -1, float g = -1, float b = -1, float a = 1);
    void delete_tree(int tree_id);
    void delete_active_tree();
    void add_tree_comment(int tree_id, const QString &comment);
    bool merge_trees(int tree_id, int other_tree_id);
    bool move_to_next_tree();
    bool move_to_previous_tree();
    // edit_tree_comment ?

    nodeListElement *find_node_by_id(int node_id);
    nodeListElement *find_node_in_radius(int x, int y, int z);
    bool move_node_to_tree(int node_id, int tree_id);
    nodeListElement *find_nearby_node_from_tree(int tree_id, int x, int y, int z);
    nodeListElement *node_with_prev_id(int node_id, bool same_tree);
    nodeListElement *node_with_next_id(int node_id, bool same_tree);
    bool edit_node(int node_id, float radius, int x, int y, int z, int in_mag);
    bool jump_to_active_node();
    void delete_node(int node_id);
    void delete_active_node();
    void set_active_node(int node_id);
    nodeListElement *active_node();
    void add_node(int node_id, int x, int y, int z, int parent_tree_id = 0, float radius = 1.5, int inVp = 0, int inMag = 1, int time = 0);
    void set_branch_node(int node_id);

    void add_segment(int source_id, int target_id);
    void delete_segment(int source_id, int target_id);
    segmentListElement *find_segment(int source_id, int target_id);


    void add_comment(int node_id, char *comment);


    QList<int> *cube_data_at(int x, int y, int z);
    void render_mesh(mesh *mesh);

    void export_converter(const QString &path);
    void save_sys_path(const QString &path);
    void save_working_directory(const QString &path);

    void loadStyleSheet(const QString &path);
    QList<mesh *> *user_geom_list();
    void move_to(int x, int y, int z);
    void render_text(const QString &path, int x, int y, int z);    
    static QString help();
};

#endif // SKELETONPROXY_H
