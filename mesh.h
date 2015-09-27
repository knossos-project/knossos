#ifndef MESH_H
#define MESH_H

#include "color4F.h"
#include "coordinate.h"

class mesh {
public:
    mesh() {}
    mesh(int mode) { this->mode = mode; } /* GL_POINTS, GL_TRIANGLES, etc. */
    floatCoordinate *vertices;
    floatCoordinate *normals;
    color4F *colors;

    /* useful for flexible mesh manipulations */
    uint vertsBuffSize;
    uint normsBuffSize;
    uint colsBuffSize;
    /* indicates last used element in corresponding buffer */
    uint vertsIndex;
    uint normsIndex;
    uint colsIndex;
    uint mode;
    uint size;
};

#endif//MESH_H
