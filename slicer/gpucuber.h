/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include "coordinate.h"

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
    std::vector<floatCoordinate> vertices;
    gpu_raw_cube(const int gpucubeedge, const bool index = false);
    virtual ~gpu_raw_cube() = default;
    std::vector<char> prepare(boost::multi_array_ref<std::uint8_t, 3>::const_array_view<3>::type view);
    void upload(const std::vector<char> & data);
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
    std::vector<gpu_index> prepare(boost::multi_array_ref<uint64_t, 3>::const_array_view<3>::type view);
    void upload(const std::vector<gpu_index> & data);
    void generate(boost::multi_array_ref<std::uint64_t, 3>::const_array_view<3>::type view);
};

class TextureLayer {
public:
    QOffscreenSurface surface;
    QOpenGLContext ctx;//ctx has to live past textures
    std::unordered_map<CoordOfGPUCube, std::unique_ptr<gpu_raw_cube>> textures;
    std::unique_ptr<gpu_raw_cube> bogusCube;
    float opacity = 1.0f;
    bool enabled = true;
    bool isOverlayData = false;
    std::vector<std::pair<CoordOfGPUCube, Coordinate>> pendingOrthoCubes;
    std::vector<std::pair<CoordOfGPUCube, Coordinate>> pendingArbCubes;
    TextureLayer(QOpenGLContext & sharectx);
    ~TextureLayer();
    template<typename cube_type, typename elem_type>
    void createBogusCube(const Coordinate & cpucubeedge, const int gpucubeedge);
    void createBogusCube(const Coordinate & cpucubeedge, const int gpucubeedge);
    template<typename cube_type, typename elem_type>
    void cubeSubArray(const boost::const_multi_array_ref<elem_type, 3> cube, const int gpucubeedge, const CoordOfGPUCube gpuCoord, const Coordinate offset);
    void cubeSubArray(const void * data, const Coordinate &  cpucubeedge, const int gpucubeedge, const CoordOfGPUCube gpuCoord, const Coordinate & offset);
};
