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

#include <QElapsedTimer>
#include <QTimer>

#include <boost/optional.hpp>

#include <deque>

class Remote {
    Coordinate targetPos;
    floatCoordinate recenteringOffset;
    bool rotate{false};
    QTimer timer;
    QElapsedTimer elapsed;
    static constexpr qint64 ms{10};
    static constexpr float goodEnough{0.01f};

    std::deque<floatCoordinate> getLastNodes();
    void remoteWalk();

public:
    Remote();
    void process(const Coordinate & pos, bool rotate = false);
};
