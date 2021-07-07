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

constexpr bool inRange(const int value, const int min, const int max);
bool insideCurrentSupercube(const Coordinate & coord, const Coordinate & center, const Coordinate & cubesPerDimension, const Coordinate & cubeSize);
bool insideCurrentSupercube(const Coordinate & coord, const Coordinate & center, const int & cubesPerDimension, const Coordinate & cubeSize);
bool currentlyVisible(const Coordinate & coord, const Coordinate & center, const Coordinate & cubesPerDimension, const Coordinate & cubeSize);
bool currentlyVisible(const Coordinate & coord, const Coordinate & center, const int & cubesPerDimension, const Coordinate & cubeSize);

class Rotation {
public:
    floatCoordinate axis;
    float alpha{};

    Rotation() = default;
    Rotation(const floatCoordinate & axis, const float alpha) : axis(axis), alpha(alpha) {}
};

int roundFloat(float number);
int sgn(float number);

float radToDeg(float rad);
float degToRad(float deg);

bool intersectLineAndPlane(const floatCoordinate planeNormal, const floatCoordinate planeUpVec,
                           const floatCoordinate lineUpVec, const floatCoordinate lineDirectionVec,
                           floatCoordinate & intersectionPoint);
