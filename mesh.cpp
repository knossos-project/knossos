#include "knossos-global.h"
#include "renderer.h"

mesh::mesh() {
    //Renderer::initmesh(this, 1024);
}

mesh::mesh(int mode) {
    this->mode = mode;
}
