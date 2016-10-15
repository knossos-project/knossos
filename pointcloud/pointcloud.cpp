#include "pointcloud.h"

PointCloud::PointCloud(treeListElement * tree, bool useTreeColor, GLenum render_mode) : correspondingTree(tree), useTreeColor(useTreeColor), render_mode(render_mode) {
    position_buf.create();
    normal_buf.create();
    color_buf.create();
    index_buf.create();
}

PointCloud::~PointCloud() {
    position_buf.destroy();
    normal_buf.destroy();
    color_buf.destroy();
    index_buf.destroy();
}
