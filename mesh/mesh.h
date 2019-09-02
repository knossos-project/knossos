#ifndef MESH_H
#define MESH_H

#include "coordinate.h"

#include <QObject>
#include <QOpenGLBuffer>
#include <QVector>

#include <boost/optional.hpp>
#include <unordered_map>

class treeListElement;
struct BufferSelection;
class Mesh {
public:
    boost::optional<BufferSelection> pointCloudTriangleIDtoInformation(const uint32_t triangleID) const;

    explicit Mesh(treeListElement * tree, bool useTreeColor = true, GLenum render_mode = GL_POINTS);

    treeListElement * correspondingTree{nullptr};

    bool useTreeColor{true};

    std::size_t vertex_count{0};
    std::size_t index_count{0};
    QOpenGLBuffer position_buf{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer normal_buf{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer color_buf{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer index_buf{QOpenGLBuffer::IndexBuffer};
    GLenum render_mode{GL_POINTS};

    boost::optional<std::size_t> pickingIdOffset;
    QOpenGLBuffer picking_color_buf{QOpenGLBuffer::VertexBuffer};
};

#endif// MESH_H
