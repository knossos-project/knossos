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

#include "gpucuber.h"

#include "segmentation/segmentation.h"

#include <boost/multi_array.hpp>

gpu_raw_cube::gpu_raw_cube(const int gpucubeedge, const bool index) {
    cube.setAutoMipMapGenerationEnabled(false);
    cube.setSize(gpucubeedge, gpucubeedge, gpucubeedge);
    cube.setMipLevels(1);
    cube.setMinificationFilter(index ? QOpenGLTexture::Nearest : QOpenGLTexture::Linear);
    cube.setMagnificationFilter(index ? QOpenGLTexture::Nearest : QOpenGLTexture::Linear);
    cube.setFormat(index ? QOpenGLTexture::R16_UNorm : QOpenGLTexture::R8_UNorm);
    cube.setWrapMode(QOpenGLTexture::ClampToEdge);
    cube.allocateStorage();
}

std::vector<char> gpu_raw_cube::prepare(boost::multi_array_ref<uint8_t, 3>::const_array_view<3>::type view) {
    std::vector<char> data;
    for (const auto & d2 : view)
    for (const auto & d1 : d2)
    for (const auto & elem : d1) {
        data.emplace_back(elem);
    }
    return data;
}

void gpu_raw_cube::upload(const std::vector<char> & data) {
    cube.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, data.data());
}

void gpu_raw_cube::generate(boost::multi_array_ref<uint8_t, 3>::const_array_view<3>::type view) {
    upload(prepare(view));
}

gpu_lut_cube::gpu_lut_cube(const int gpucubeedge) : gpu_raw_cube(gpucubeedge, true) {
    lut.setAutoMipMapGenerationEnabled(false);
    lut.setMipLevels(1);
    lut.setMinificationFilter(QOpenGLTexture::Nearest);
    lut.setMagnificationFilter(QOpenGLTexture::Nearest);
    lut.setFormat(QOpenGLTexture::RGBA8_UNorm);
}

std::vector<gpu_lut_cube::gpu_index> gpu_lut_cube::prepare(boost::multi_array_ref<uint64_t, 3>::const_array_view<3>::type view) {
    bool lastValid{false};
    uint64_t lastElem{0};
    uint64_t lastIndex{0};
    std::array<std::uint8_t, 4> lastColor{{}};

    std::vector<gpu_index> data;
    for (const auto & d2 : view)
    for (const auto & d1 : d2)
    for (const auto & elem : d1) {
        if (lastValid && elem == lastElem) {
            data.emplace_back(lastIndex);
        } else {
            const auto it = id_to_lut_index.find(elem);
            const auto existing = it != std::end(id_to_lut_index);
            const auto index = existing ? it->second : (id_to_lut_index[elem] = highest_index++);//increment after assignment
            data.emplace_back(index);
            if (!existing) {
                const auto color = Segmentation::singleton().colorObjectFromSubobjectId(elem);
                lastColor = {{std::get<0>(color), std::get<1>(color), std::get<2>(color), std::get<3>(color)}};
                colors.push_back(lastColor);
            }
            lastElem = elem;
            lastIndex = index;
            lastColor = colors[index];
            lastValid = true;
        }
    }
    const auto lutSize = std::pow(2, std::ceil(std::log2(colors.size())));
    colors.resize(lutSize);
    return data;
}

void gpu_lut_cube::upload(const std::vector<gpu_index> & data) {
    lut.setSize(colors.size());
    lut.allocateStorage();

    cube.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, data.data());
    const auto & colors = this->colors;
    lut.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt32_RGBA8_Rev, colors.data());
}

void gpu_lut_cube::generate(boost::multi_array_ref<uint64_t, 3>::const_array_view<3>::type view) {
    upload(prepare(view));
}

TextureLayer::TextureLayer(QOpenGLContext & sharectx) {
    surface.create();
    ctx.setFormat(surface.format());
    ctx.setShareContext(&sharectx);
    ctx.create();
}
TextureLayer::~TextureLayer() {
    ctx.makeCurrent(&surface);//QOpenGLTexture dtor needs a current ctx
}

template<typename cube_type, typename elem_type>
void TextureLayer::createBogusCube(const Coordinate & cpucubeedge, const int gpucubeedge) {
    ctx.makeCurrent(&surface);
    std::vector<char> data;
    data.resize(cpucubeedge.z * cpucubeedge.y * cpucubeedge.x * sizeof(elem_type));
    std::fill(std::begin(data), std::end(data), 0);
    bogusCube.reset(new cube_type(gpucubeedge));
    boost::multi_array_ref<elem_type, 3> cube(reinterpret_cast<elem_type*>(data.data()), boost::extents[cpucubeedge.z][cpucubeedge.y][cpucubeedge.x]);
    using range = boost::multi_array_types::index_range;
    static_cast<cube_type*>(bogusCube.get())->generate(cube[boost::indices[range(0,gpucubeedge)][range(cpucubeedge.y-gpucubeedge,cpucubeedge.y-0)][range(0,gpucubeedge)]]);
}

void TextureLayer::createBogusCube(const Coordinate & cpucubeedge, const int gpucubeedge) {
    if (isOverlayData) {
        createBogusCube<gpu_lut_cube, std::uint64_t>(cpucubeedge, gpucubeedge);
    } else {
        createBogusCube<gpu_raw_cube, std::uint8_t>(cpucubeedge, gpucubeedge);
    }
}

template<typename cube_type, typename elem_type>
void TextureLayer::cubeSubArray(const boost::const_multi_array_ref<elem_type, 3> cube, const int gpucubeedge, const CoordOfGPUCube gpuCoord, const Coordinate offset) {
    ctx.makeCurrent(&surface);
    using range = boost::multi_array_types::index_range;
    const auto view = cube[boost::indices[range(0+offset.z,gpucubeedge+offset.z)][range(0+offset.y,gpucubeedge+offset.y)][range(0+offset.x,gpucubeedge+offset.x)]];
    textures[gpuCoord].reset(new cube_type(gpucubeedge));
    static_cast<cube_type*>(textures[gpuCoord].get())->generate(view);
}

void TextureLayer::cubeSubArray(const void * data, const Coordinate & cpucubeedge, const int gpucubeedge, const CoordOfGPUCube gpuCoord, const Coordinate & offset) {
    if (isOverlayData) {
        boost::const_multi_array_ref<std::uint64_t, 3> cube(reinterpret_cast<const std::uint64_t *>(data), boost::extents[cpucubeedge.z][cpucubeedge.y][cpucubeedge.x]);
        cubeSubArray<gpu_lut_cube>(cube, gpucubeedge, gpuCoord, offset);
    } else {
        boost::const_multi_array_ref<std::uint8_t, 3> cube(reinterpret_cast<const std::uint8_t *>(data), boost::extents[cpucubeedge.z][cpucubeedge.y][cpucubeedge.x]);
        cubeSubArray<gpu_raw_cube>(cube, gpucubeedge, gpuCoord, offset);
    }
}
