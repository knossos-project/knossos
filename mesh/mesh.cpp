#include "mesh.h"

Mesh::Mesh(treeListElement * tree, bool useTreeColor, GLenum render_mode) : correspondingTree(tree), useTreeColor(useTreeColor), render_mode(render_mode) {}

#include "stateInfo.h"
#include "widgets/mainwindow.h"

Mesh::~Mesh() {
    // avoids (apparently inefficient) postponed deletion in the driver
    state->mainWindow->viewport3D->makeCurrent();
}
