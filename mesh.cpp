#include "knossos-global.h"
#include "renderer.h"

mesh::mesh() {
    Renderer::initMesh(this, 1024);
}

mesh::mesh(int mode) {
    this->mode = mode;
}
