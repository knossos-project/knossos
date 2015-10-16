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

#include <stateInfo.h>
#include <widgets/viewport.h>

#include <QObject>
#include <QThread>

#include <deque>

class Remote : public QThread {
    Q_OBJECT
private:
    bool rotate;
    uint activeVP;

public:
    explicit Remote(QObject *parent = 0);

    void remoteWalk(int x, int y, int z);
    void run();

    floatCoordinate recenteringPosition;
    std::deque<floatCoordinate> getLastNodes();

signals:
    void finished();
    void userMoveSignal(const floatCoordinate & floatStep, UserMoveType userMoveType, const Coordinate & viewportNormal = {0, 0, 0});
    void rotationSignal(const floatCoordinate & axis, const float angle);
public slots:
    void setRecenteringPosition(const floatCoordinate &newPos);
    void setRecenteringPositionWithRotation(const floatCoordinate & newPos, const uint vp);
    bool remoteJump(const Coordinate & jumpVec);
};

#endif // REMOTE_H
