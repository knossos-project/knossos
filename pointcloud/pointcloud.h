#ifndef POINTCLOUD_H
#define POINTCLOUD_H

#include "coordinate.h"

#include <QObject>
#include <QOpenGLBuffer>
#include <QVector>

#include <boost/optional.hpp>
#include <unordered_map>

struct BufferSelection;
class PointCloud {
public:
    boost::optional<BufferSelection> pointCloudTriangleIDtoInformation(const uint32_t triangleID) const;

    PointCloud(GLenum render_mode = GL_POINTS);
    ~PointCloud();

    std::size_t vertex_count{0};
    std::size_t index_count{0};
    QOpenGLBuffer position_buf{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer normal_buf{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer color_buf{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer index_buf{QOpenGLBuffer::IndexBuffer};
    GLenum render_mode{GL_POINTS};

    QVector<float> vertex_coords;
    QVector<unsigned int> indices;
};

struct BufferSelection {
    std::reference_wrapper<const std::uint64_t> it;
    floatCoordinate coord;
};

#endif // POINTCLOUD_H
