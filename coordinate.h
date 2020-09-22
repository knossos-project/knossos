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

#include <QDebug>
#include <QList>
#include <QVector>
#include <QVector3D>
#include <QMetaType>

#include <cstddef>

#include <boost/functional/hash.hpp>

template<typename ComponentType, typename CoordinateDerived>
class CoordinateBase {
public:
    ComponentType x = 0;
    ComponentType y = 0;
    ComponentType z = 0;

    constexpr CoordinateBase() = default;
    constexpr CoordinateBase(QList<ComponentType> l) {
        if (l.size() != 3) {
            throw std::runtime_error("QList too short for coordinate");
        }
        std::tie(x, y, z) = std::make_tuple(l[0], l[1], l[2]);
    }
    constexpr CoordinateBase(QVector<ComponentType> l) {
        if (l.size() != 3) {
            throw std::runtime_error("QVector too short for coordinate");
        }
        std::tie(x, y, z) = std::make_tuple(l[0], l[1], l[2]);}
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

    template<typename T>
    using CoordinateDerived_if_valid_t = typename std::enable_if<std::is_convertible<T, CoordinateDerived>::value || std::is_convertible<T, ComponentType>::value, CoordinateDerived>::type;

    constexpr CoordinateDerived operator+(const CoordinateDerived & rhs) const {
        return CoordinateDerived(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    constexpr CoordinateDerived operator+(const ComponentType scalar) const {
        return CoordinateDerived(x + scalar, y + scalar, z + scalar);
    }
    template<typename T>
    CoordinateDerived_if_valid_t<T> & operator+=(const T & rhs) {
        return static_cast<CoordinateDerived&>(*this = *this + rhs);
    }

    constexpr CoordinateDerived operator-(const CoordinateDerived & rhs) const {
        return CoordinateDerived(x - rhs.x, y - rhs.y, z - rhs.z);
    }
    constexpr CoordinateDerived operator-(const ComponentType scalar) const {
        return CoordinateDerived(x - scalar, y - scalar, z - scalar);
    }
    template<typename T>
    CoordinateDerived_if_valid_t<T> & operator-=(const T & rhs) {
        return static_cast<CoordinateDerived&>(*this = *this - rhs);
    }

    template<typename T>
    constexpr CoordinateDerived componentMul(const T & rhs) const {
        return CoordinateDerived(x * rhs.x, y * rhs.y, z * rhs.z);
    }

    constexpr ComponentType dot(const CoordinateDerived & rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    constexpr CoordinateDerived cross(const CoordinateDerived & rhs) const {
        return CoordinateDerived(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
    }

    constexpr CoordinateDerived operator*(const ComponentType scalar) const {
        return CoordinateDerived(x * scalar, y * scalar, z * scalar);
    }
    template<typename T>
    CoordinateDerived_if_valid_t<T> & operator*=(const T & rhs) {
        return static_cast<CoordinateDerived&>(*this = *this * rhs);
    }

    constexpr CoordinateDerived operator/(const CoordinateDerived & rhs) const {
        return CoordinateDerived(x / rhs.x, y / rhs.y, z / rhs.z);
    }
    constexpr CoordinateDerived operator/(const ComponentType scalar) const {
        return CoordinateDerived(x / scalar, y / scalar, z / scalar);
    }
    template<typename T>
    CoordinateDerived_if_valid_t<T> & operator/=(const T & rhs) {
        return static_cast<CoordinateDerived&>(*this = *this / rhs);
    }

    constexpr float length() const {
        return std::sqrt(x*x + y*y + z*z);
    }

    constexpr float angleRad(CoordinateDerived & other) const {
        return std::acos(static_cast<float>(this->dot(other)) / (this->length() * other.length()));
    }

    bool normalize() {
        const ComponentType norm = length();
        if (norm != 0) {
            *this /= norm;
            return true;
        }
        return false;
    }

    constexpr CoordinateDerived capped(const CoordinateDerived & min, const CoordinateDerived & max) const {
        return CoordinateDerived{
            std::max(min.x, std::min(x, max.x - 1))
            , std::max(min.y, std::min(y, max.y - 1))
            , std::max(min.z, std::min(z, max.z - 1))
        };
    }

    constexpr CoordinateDerived toWorldFrom(const CoordinateDerived & v1, const CoordinateDerived & v2, const CoordinateDerived & n) const {
        return x * v1 + y * v2 + z * n;
    }

    constexpr CoordinateDerived toLocal(const CoordinateDerived & v1, const CoordinateDerived & v2, const CoordinateDerived & n) const {
        return CoordinateDerived(v1.dot({x, y, z}), v2.dot({x, y, z}), n.dot({x, y, z}));
    }
};

template<typename ComponentType, typename CoordinateDerived>
constexpr CoordinateDerived operator*(const ComponentType scalar, const CoordinateBase<ComponentType, CoordinateDerived> & coord) {
    return static_cast<CoordinateDerived>(coord * scalar);
}

template<typename ComponentType, typename CoordinateDerived>
QDebug operator<<(QDebug stream, const CoordinateBase<ComponentType, CoordinateDerived> & coord) {
    return stream << coord.x << coord.y << coord.z;
}

template<typename ComponentType = int, std::size_t tag = 0>
class Coord : public CoordinateBase<ComponentType, Coord<ComponentType, tag>> {
public:
    using CoordinateBase<ComponentType, Coord<ComponentType, tag>>::CoordinateBase;
    constexpr Coord() = default;
    template<typename T>
    constexpr Coord(const Coord<T, tag> & rhs) : CoordinateBase<ComponentType, Coord<ComponentType, tag>>(rhs.x, rhs.y, rhs.z) {}
    template<typename T>
    constexpr operator Coord<T, tag>() const {
        return Coord<T, tag>(std::round(this->x), std::round(this->y), std::round(this->z));
    }
    constexpr operator QVector3D() const {
        return QVector3D{this->x, this->y, this->z};
    }
};

using Coordinate = Coord<int, 0>;
using CoordOfCube = Coord<int, 1>;
using CoordInCube = Coord<int, 2>;
using CoordOfGPUCube = Coord<int, 3>;
using floatCoordinate = Coord<float>;

template<>
class Coord<int, 0> : public CoordinateBase<int, Coordinate> {
public:
    using CoordinateBase<int, Coordinate>::CoordinateBase;
    constexpr CoordOfCube cube(const int size, const floatCoordinate scale) const;
    constexpr CoordInCube insideCube(const int size, const floatCoordinate scale) const;
};

template<>
class Coord<int, 1> : public CoordinateBase<int, CoordOfCube> {
public:
    using CoordinateBase<int, CoordOfCube>::CoordinateBase;
    constexpr Coordinate cube2Global(const int cubeEdgeLength, const floatCoordinate scale) const;
};

template<>
class Coord<int, 2> : public CoordinateBase<int, CoordInCube> {
public:
    using CoordinateBase<int, CoordInCube>::CoordinateBase;
    constexpr Coordinate insideCube2Global(const CoordOfCube & cube, const int cubeEdgeLength, const floatCoordinate scale) const;
};

template<>
class Coord<int, 3> : public CoordinateBase<int, CoordOfGPUCube> {
public:
    using CoordinateBase<int, CoordOfGPUCube>::CoordinateBase;
    constexpr Coordinate cube2Global(const int cubeEdgeLength, const floatCoordinate scale) const;
};

constexpr CoordOfCube Coordinate::cube(const int size, const floatCoordinate scale) const {
    return CoordOfCube(x / size / scale.x, y / size / scale.y, z / size / scale.z);
}
constexpr CoordInCube Coordinate::insideCube(const int size, const floatCoordinate scale) const {
    return CoordInCube(static_cast<int>(x / scale.x) % size, static_cast<int>(y / scale.y) % size, static_cast<int>(z / scale.z) % size);
}

constexpr Coordinate CoordOfCube::cube2Global(const int size, const floatCoordinate scale) const {
    return Coordinate(x * size * scale.x, y * size * scale.y, z * size * scale.z);
}

constexpr Coordinate CoordOfGPUCube::cube2Global(const int size, const floatCoordinate scale) const {
    return Coordinate(x * size * scale.x, y * size * scale.y, z * size * scale.z);
}

constexpr Coordinate CoordInCube::insideCube2Global(const CoordOfCube & cube, const int size, const floatCoordinate scale) const {
    return cube.cube2Global(size, scale) + scale.componentMul(Coordinate{x, y, z});
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
namespace std {
template<>
struct hash<CoordOfGPUCube> {
    std::size_t operator()(const CoordOfGPUCube & cord) const {
        return boost::hash_value(std::make_tuple(cord.x, cord.y, cord.z));
    }
};
}
