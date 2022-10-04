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

#include "dataset.h"
#include "functions.h"

#include <boost/math/constants/constants.hpp>

#include <cmath>

/** this file contains function which are not dependent from any state */

constexpr bool inRange(const int value, const int min, const int max) {
    return value >= min && value < max;
}

bool insideCurrentSupercube(const Coordinate & coord, const Coordinate & center, const int & cubesPerDimension, const Coordinate & cubeSize) {
    const auto halfSupercube = cubeSize.componentMul(cubesPerDimension-1) / 2.;
    const auto c = coord.cube(cubeSize.x, {1.f,1.f,1.f});
    const auto tl = (center - halfSupercube).cube(cubeSize.x, {1.f,1.f,1.f});
    const auto br = (center + halfSupercube).cube(cubeSize.x, {1.f,1.f,1.f}) + 1;
    bool valid = true;
    valid &= inRange(c.x, tl.x, br.x);
    valid &= inRange(c.y, tl.y, br.y);
    valid &= inRange(c.z, tl.z, br.z);
    return valid;
}

bool currentlyVisible(const Coordinate & coord, const Coordinate & center, const int & cubesPerDimension, const Coordinate & cubeSize) {
    const bool valid = insideCurrentSupercube(coord, center, cubesPerDimension, cubeSize);
    const auto c = coord.cube(cubeSize.x, {1.f,1.f,1.f}).cube2Global(cubeSize.x, {1.f,1.f,1.f});
    const bool xvalid = valid & inRange(coord.x, c.x, c.x + cubeSize.x);
    const bool yvalid = valid & inRange(coord.y, c.y, c.y + cubeSize.y);
    const bool zvalid = valid & inRange(coord.z, c.z, c.z + cubeSize.z);
    return xvalid || yvalid || zvalid;
}

int roundFloat(float number) {
    if(number >= 0) return (int)(number + 0.5);
    else return (int)(number - 0.5);
}

int sgn(float number) {
    if(number > 0.) return 1;
    else if(number == 0.) return 0;
    else return -1;
}


//Some math helper functions
float radToDeg(float rad) {
    return ((180. * rad) / boost::math::constants::pi<float>());
}

float degToRad(float deg) {
    return ((deg / 180.) * boost::math::constants::pi<float>());
}

bool intersectLineAndPlane(const floatCoordinate planeNormal, const floatCoordinate planeUpVec,
                           const floatCoordinate lineUpVec, const floatCoordinate lineDirectionVec,
                           floatCoordinate & intersectionPoint) {
    if (std::abs(lineDirectionVec.dot(planeNormal)) > 0.0001) {
        const float lambda = (planeNormal.dot(planeUpVec) - planeNormal.dot(lineUpVec)) / lineDirectionVec.dot(planeNormal);
        intersectionPoint = lineUpVec + lineDirectionVec * lambda;
        return true;
    }
    return false;
}
