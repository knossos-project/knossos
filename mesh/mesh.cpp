#include "mesh.h"

Mesh::Mesh(treeListElement * tree, bool useTreeColor, GLenum render_mode) : correspondingTree(tree), useTreeColor(useTreeColor), render_mode(render_mode) {
    position_buf.create();
    normal_buf.create();
    color_buf.create();
    index_buf.create();
    picking_color_buf.create();
}
