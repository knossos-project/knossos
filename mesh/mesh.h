#pragma once

#include <QObject>
#include <QOpenGLBuffer>
#include <QVector>

#include <boost/optional.hpp>

#include <cstdint>
#include <optional>
#include <vector>

class treeListElement;
struct BufferSelection;
class Mesh {
public:
    boost::optional<BufferSelection> pointCloudTriangleIDtoInformation(const uint32_t triangleID) const;

    explicit Mesh(treeListElement * tree, bool useTreeColor = true, GLenum render_mode = GL_POINTS);

    treeListElement * correspondingTree{nullptr};

    bool useTreeColor{true};

    std::size_t vertex_count{};
    std::size_t index_count{};
    static inline QOpenGLBuffer unibuf, uniindexbuf{QOpenGLBuffer::IndexBuffer};
    static inline std::size_t unibufoffset{}, unibufindexoffset{};
    std::optional<std::size_t> position_buf, normal_buf, color_buf, picking_color_buf, index_buf;
    GLenum render_mode{GL_POINTS};

    static inline std::size_t pickingIdCount{1};
    std::size_t pickingIdOffset{};

    QVector<float> verts;
    QVector<float> normals;
    QVector<std::uint8_t> colors;
    std::vector<std::array<GLubyte, 4>> picking_colors;
    QVector<unsigned int> indices;
};
