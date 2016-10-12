#include "pointcloud.h"

PointCloud::PointCloud(GLenum render_mode) : render_mode(render_mode) {
    position_buf.create();
    normal_buf.create();
    color_buf.create();
    index_buf.create();
}
