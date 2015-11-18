#ifndef REMOTE_H
#define REMOTE_H

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

#include "coordinate.h"

#include <QElapsedTimer>
#include <QTimer>

#include <deque>

class Remote {
public:
    floatCoordinate recenteringOffset;
    bool rotate{false};
    uint activeVP{0};
    QTimer timer;
    QElapsedTimer elapsed;
    static const qint64 ms;
    static const float goodEnough;

    Remote();

    void process(const Coordinate & pos);
    std::deque<floatCoordinate> getLastNodes();
    void remoteWalk();
};

#endif // REMOTE_H
