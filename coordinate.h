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

#include <QList>
#include <QVector>
#include <QMetaType>

#include <cstddef>

#include <boost/functional/hash.hpp>

template<typename ComponentType, typename CoordinateDerived>
class CoordinateBase {
public:
    ComponentType x;
    ComponentType y;
    ComponentType z;

    constexpr CoordinateBase(ComponentType every = 0) : x(every), y(every), z(every) {}
    constexpr CoordinateBase(QList<ComponentType> l) : x(l[0]), y(l[1]), z(l[2]) {}
    constexpr CoordinateBase(QVector<ComponentType> l) : x(l[0]), y(l[1]), z(l[2]) {}
    constexpr CoordinateBase(ComponentType x, ComponentType y, ComponentType z) : x(x), y(y), z(z) {}

    constexpr ComponentType sum() const {
        return x+y+z;
    }
    constexpr QList<ComponentType> list() const {
        return {x, y, z};
    }
    constexpr QVector<ComponentType> vector() const {
        return {x, y, z};
    }
    constexpr bool operator==(const CoordinateDerived & rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    constexpr bool operator!=(const CoordinateDerived & rhs) const {
        return !(*this == rhs);
    }

    constexpr CoordinateDerived operator+(const CoordinateDerived & rhs) const {
        return CoordinateDerived(x + rhs.x, y + rhs.y, z  + rhs.z);
    }
    CoordinateDerived & operator+=(const CoordinateDerived & rhs) {
        return static_cast<CoordinateDerived&>(*this = *this + rhs);
    }

    constexpr CoordinateDerived operator-(const CoordinateDerived & rhs) const {
        return CoordinateDerived(x - rhs.x, y - rhs.y, z  - rhs.z);
    }
    CoordinateDerived & operator-=(const CoordinateDerived & rhs) {
        return static_cast<CoordinateDerived&>(*this = *this - rhs);
    }

    constexpr CoordinateDerived operator*(const CoordinateDerived & rhs) const {
        return CoordinateDerived(x * rhs.x, y * rhs.y, z  * rhs.z);
    }
    CoordinateDerived & operator*=(const CoordinateDerived & rhs) {
        return static_cast<CoordinateDerived&>(*this = *this * rhs);
    }

    constexpr CoordinateDerived operator/(const CoordinateDerived & rhs) const {
        return CoordinateDerived(x / rhs.x, y / rhs.y, z  / rhs.z);
    }
    CoordinateDerived & operator/=(const CoordinateDerived & rhs) {
        return static_cast<CoordinateDerived&>(*this = *this / rhs);
    }

    constexpr CoordinateDerived capped(const CoordinateDerived & min, const CoordinateDerived & max) const {
        return CoordinateDerived{
            std::max(min.x, std::min(x, max.x))
            , std::max(min.y, std::min(y, max.y))
            , std::max(min.z, std::min(z, max.z))
        };
    }
};

template<typename ComponentType = int, std::size_t tag = 0>
class Coord : public CoordinateBase<ComponentType, Coord<ComponentType, tag>> {
public:
    using CoordinateBase<ComponentType, Coord<ComponentType, tag>>::CoordinateBase;
    constexpr Coord() = default;
    template<typename T>
    constexpr Coord(const Coord<T, tag> & rhs) : CoordinateBase<ComponentType, Coord<ComponentType, tag>>(rhs.x, rhs.y, rhs.z) {}
    template<typename T>
    constexpr operator Coord<T, tag>() const {
        return Coord<T, tag>(this->x, this->y, this->z);
    }
};

using Coordinate = Coord<int, 0>;
using CoordOfCube = Coord<int, 1>;
using CoordInCube = Coord<int, 2>;
using floatCoordinate = Coord<float>;

template<>
class Coord<int, 0> : public CoordinateBase<int, Coordinate> {
public:
    using CoordinateBase<int, Coordinate>::CoordinateBase;
    constexpr CoordOfCube cube(const int size, const int mag) const;
    constexpr CoordInCube insideCube(const int size, const int mag) const;
};

template<>
class Coord<int, 1> : public CoordinateBase<int, CoordOfCube> {
public:
    using CoordinateBase<int, CoordOfCube>::CoordinateBase;
    constexpr Coordinate cube2Global(const int cubeEdgeLength, const int magnification) const;
};

template<>
class Coord<int, 2> : public CoordinateBase<int, CoordInCube> {
public:
    using CoordinateBase<int, CoordInCube>::CoordinateBase;
    constexpr Coordinate insideCube2Global(const CoordOfCube & cube, const int cubeEdgeLength, const int magnification) const;
};

constexpr CoordOfCube Coordinate::cube(const int size, const int mag) const {
    return {this->x / size / mag, this->y / size / mag, this->z / size / mag};
}
constexpr CoordInCube Coordinate::insideCube(const int size, const int mag) const {
    return {(this->x / mag) % size, (this->y / mag) % size, (this->z / mag) % size};
}

constexpr Coordinate CoordOfCube::cube2Global(const int cubeEdgeLength, const int magnification) const {
    return {this->x * cubeEdgeLength * magnification, this->y * cubeEdgeLength * magnification, this->z * cubeEdgeLength * magnification};
}

constexpr Coordinate CoordInCube::insideCube2Global(const CoordOfCube & cube, const int cubeEdgeLength, const int magnification) const {
    return cube.cube2Global(cubeEdgeLength, magnification) + Coord<int>{this->x * magnification, this->y * magnification, this->z * magnification};
}

Q_DECLARE_METATYPE(Coordinate)
Q_DECLARE_METATYPE(CoordOfCube)
Q_DECLARE_METATYPE(floatCoordinate)

namespace std {
template<typename T, std::size_t x>
struct hash<Coord<T, x>> {
    std::size_t operator()(const Coord<T, x> & cord) const {
        return boost::hash_value(std::make_tuple(cord.x, cord.y, cord.z));
    }
};
}

#endif
