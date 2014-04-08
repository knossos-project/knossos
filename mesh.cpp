#include "knossos-global.h"
#include "renderer.h"

Mesh::Mesh() {
    Renderer::initMesh(this, 1024);
}

Mesh::Mesh(int mode) {
    this->mode = mode;
}
