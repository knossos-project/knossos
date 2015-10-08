#ifndef GPUCUBER_H
#define GPUCUBER_H

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QVector3D>

#include <boost/functional/hash.hpp>
#include <boost/multi_array/multi_array_ref.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace std {
template<>
struct hash<QVector3D> {
    std::size_t operator()(const QVector3D & cord) const {
        return boost::hash_value(std::make_tuple(cord.x(), cord.y(), cord.z()));
    }
};
}

class gpu_raw_cube {
public:
    QOpenGLTexture cube{QOpenGLTexture::Target3D};
    gpu_raw_cube(const int gpucubeedge, const bool index = false);
    void generate(boost::multi_array_ref<std::uint8_t, 3>::const_array_view<3>::type view);
};

class gpu_lut_cube : public gpu_raw_cube {
public:
    using gpu_index = std::uint16_t;
private:
    std::unordered_map<std::uint64_t, gpu_index> id_to_lut_index;
    gpu_index highest_index = 0;
    std::vector<std::array<std::uint8_t, 4>> colors;
public:
    QOpenGLTexture lut{QOpenGLTexture::Target1D};
    gpu_lut_cube(const int gpucubeedge);
    void generate(boost::multi_array_ref<std::uint64_t, 3>::const_array_view<3>::type view);
};

class TextureLayer {
public:
    QOffscreenSurface surface;
    QOpenGLContext ctx;//ctx has to live past textures
    std::unordered_map<QVector3D, std::unique_ptr<gpu_raw_cube>> textures;
    std::unique_ptr<gpu_raw_cube> bogusCube;
    float opacity = 1.0f;
    bool enabled = true;
    bool isOverlayData = false;
    TextureLayer(QOpenGLContext & sharectx);
    ~TextureLayer();
    template<typename cube_type, typename elem_type>
    void createBogusCube(const int cpucubeedge, const int gpucubeedge);
    void createBogusCube(const int cpucubeedge, const int gpucubeedge);
    template<typename cube_type, typename elem_type>
    void cubeSubArray(const boost::const_multi_array_ref<elem_type, 3> cube, const int gpucubeedge, const int xi, const int yi, const int zi, const int x_offset, const int y_offset, const int z_offset);
    void cubeSubArray(const char * data, const int cpucubeedge, const int gpucubeedge, const int x, const int y, const int z, const int x_offset, const int y_offset, const int z_offset);
};

#endif//GPUCUBER_H
