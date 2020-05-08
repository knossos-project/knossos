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

#include <QObject>

#include <unordered_set>

class brush_t {
public:
    enum class mode_t {
        two_dim, three_dim
    };
    enum class view_t {
        xy, xz, zy, arb
    };
    enum class shape_t {
        angular, round
    };

    double radius = 100;
    using radius_t = decltype(radius);
    bool inverse = false;
    mode_t mode = mode_t::two_dim;
    view_t view = view_t::xy;
    shape_t shape = shape_t::round;
    floatCoordinate v1{1, 0, 0};
    floatCoordinate v2{0, 1, 0};
    floatCoordinate n{0, 0, 1};
};

class brush_subject : public QObject, private brush_t {
    Q_OBJECT
public:
    brush_t value() const {
        return static_cast<brush_t>(*this);
    }
    void setInverse(const bool newInverse) {
        inverse = newInverse;
        emit inverseChanged(inverse);
    }
    bool isInverse() const {
        return inverse;
    }
    void setMode(const mode_t newMode) {
        mode = newMode;
        emit modeChanged(mode);
    }
    mode_t getMode() const {
        return mode;
    }
    void setRadius(const radius_t newRadius);
    auto getRadius() const {
        return radius;
    }
    void setView(const view_t newView, const floatCoordinate & newV1, const floatCoordinate & newV2, const floatCoordinate & newN) {
        view = newView;
        v1 = newV1;
        v2 = newV2;
        n = newN;
    }
    view_t getView() const {
        return view;
    }
    void setShape(const shape_t newShape) {
        shape = newShape;
        emit shapeChanged(shape);
    }
    shape_t getShape() const {
        return shape;
    }
signals:
    void inverseChanged(const bool);
    void modeChanged(const mode_t);
    void radiusChanged(const radius_t);
    void shapeChanged(const shape_t);
};

void subobjectBucketFill(const Coordinate & seed, const uint64_t fillsoid, const brush_t & brush, const Coordinate & areaMin, const Coordinate & areaMax);
void connectedComponent(const Coordinate & seed);
void verticalSplittingPlane(const Coordinate & seed);
