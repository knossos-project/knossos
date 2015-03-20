/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#ifndef COORDINATE_H
#define COORDINATE_H

#include <QMetaType>

#include <cstddef>

class floatCoordinate {
public:
    float x;
    float y;
    float z;
};

#include <boost/functional/hash.hpp>

template<std::size_t = 0>//tag
class Coord {
public:
    using Coordinate = Coord<>;
    using CoordOfCube = Coord<1>;
    using CoordInCube = Coord<2>;
    int x;
    int y;
    int z;

    constexpr Coord(int x = 0, int y = 0, int z = 0) : x(x), y(y), z(z) {}

    bool operator==(const Coord & rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    /**
     * This function calculates the coordinates of the datacube from pixel coordinates
     */
    static Coordinate Px2DcCoord(Coordinate pxCoordinate, int cubeEdgeLength) {
        const auto cubeCoord = pxCoordinate.cube(cubeEdgeLength);
        return {cubeCoord.x, cubeCoord.y, cubeCoord.z};
    }

    constexpr Coordinate global2Legacy(int cubeEdgeLength) const {
        return Px2DcCoord(*this, cubeEdgeLength);
    }

    constexpr Coordinate legacy2Global(const int cubeEdgeLength) const {
        return {x * cubeEdgeLength, y * cubeEdgeLength, z * cubeEdgeLength};
    }

    constexpr Coordinate cube2Legacy() const {
        return {x, y, z};
    }

    CoordOfCube cube(const int size) const {
        return {x / size, y / size, z / size};
    }

    CoordInCube insideCube(const int size) const {
        return {x % size, y % size, z % size};
    }

    Coordinate cap(const Coordinate & min, const Coordinate & max) const {
        Coordinate copy{*this};
        copy.x = std::max(min.x, std::min(copy.x, max.x));
        copy.y = std::max(min.y, std::min(copy.y, max.y));
        copy.z = std::max(min.z, std::min(copy.z, max.z));
        return copy;
    }
};
using Coordinate = Coord<>;
using CoordOfCube = Coord<1>;
using CoordInCube = Coord<2>;

Q_DECLARE_METATYPE(Coordinate)
Q_DECLARE_METATYPE(CoordOfCube)
Q_DECLARE_METATYPE(floatCoordinate)

namespace std {
template<std::size_t x>
struct hash<Coord<x>> {
    std::size_t operator()(const Coord<x> & cord) const {
        return boost::hash_value(std::make_tuple(cord.x, cord.y, cord.z));
    }
};
}

#define SET_COORDINATE(coordinate, a, b, c) \
        { \
        (coordinate).x = (a); \
        (coordinate).y = (b); \
        (coordinate).z = (c); \
        }

#define SET_COORDINATE_FROM_ORIGIN_OFFSETS(coordinate, ox, oy, oz, offset_array) \
    { \
        SET_COORDINATE((coordinate), \
                       (ox) + (offset_array)[0], \
                       (oy) + (offset_array)[1], \
                       (oz) + (offset_array)[2]); \
    }

#define COMPARE_COORDINATE(c1, c2) \
    ( \
        (c1).x == (c2).x    \
        && (c1).y == (c2).y \
        && (c1).z == (c2).z \
    )

#define CONTAINS_COORDINATE(c1, c2, c3) \
    ( \
        (c2).x <= (c1).x    \
        && (c1).x <= (c3).x \
        && (c2).y <= (c1).y \
        && (c1).y <= (c3).y \
        && (c2).z <= (c1).z \
        && (c1).z <= (c3).z \
    )

#define ADD_COORDINATE(c1, c2) \
    { \
            (c1).x += (c2).x; \
            (c1).y += (c2).y; \
            (c1).z += (c2).z; \
    }

#define MUL_COORDINATE(c1, f) \
    {\
            (c1).x *= f;\
            (c1).y *= f;\
            (c1).z *= f;\
    }

#define SUB_COORDINATE(c1, c2) \
    { \
            (c1).x -= (c2).x; \
            (c1).y -= (c2).y; \
            (c1).z -= (c2).z; \
    }

#define DIV_COORDINATE(c1, d) \
    { \
            (c1).x /= (d); \
            (c1).y /= (d); \
            (c1).z /= (d); \
    }

#define CPY_COORDINATE(c1, c2) \
    { \
            (c1).x = (c2).x; \
            (c1).y = (c2).y; \
            (c1).z = (c2).z; \
    }

#endif
