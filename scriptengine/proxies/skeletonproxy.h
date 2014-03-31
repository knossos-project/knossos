#ifndef SKELETONPROXY_H
#define SKELETONPROXY_H

#include <QObject>
#include <QList>
#include "knossos-global.h"


class SkeletonProxy : public QObject
{
    Q_OBJECT
public:
    explicit SkeletonProxy(QObject *parent = 0);

signals:
    bool loadSkeleton(QString file);
    bool saveSkeleton(QString file);
    bool addNodeSignal(Coordinate *coordinate, Byte VPtype);
    void treeAddedSignal(treeListElement *tree);
    void nodeAddedSignal();
    void updateToolsSignal();
    void clearSkeletonSignal();
    void userMoveSignal(int x, int y, int z, int serverMovement);
    void echo(QString message);
    void updateTreeViewSignal();
    bool sliceExtractSignal(Byte *datacube, Byte *slice, vpConfig *vpConfig);

public slots:
    int skeleton_time();
    QString skeleton_file();
    void to_xml(const QString &filename);
    void from_xml(const QString &filename);
    treeListElement *first_tree();
    bool has_unsaved_changes();
    void delete_tree(int tree_id);
    void delete_skeleton();
    void set_active_node(int node_id);
    void add_comment(int node_id, char *comment);
    nodeListElement *active_node();
    QList<treeListElement *> *trees();
    void add_tree(int tree_id, const QString &comment = 0, float r = -1, float g = -1, float b = -1, float a = 1);
    void add_node(int node_id, int x, int y, int z, int parent_tree_id = 0, float radius = 1.5, int inVp = 0, int inMag = 1, int time = 0);
    void add_segment(int source_id, int target_id);
    void add_branch_node(int node_id);
    QList<int> *cube_data_at(int x, int y, int z);
    void render_mesh(mesh *mesh);
    void export_converter(const QString &path);
    void save_sys_path(const QString &path);
    void save_working_directory(const QString &path);
    static QString help();

};

#endif // SKELETONPROXY_H
