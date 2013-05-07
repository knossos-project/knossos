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

#include <QObject>
#include <QThread>
#include <knossos-global.h>

class Remote : public QThread
{
    Q_OBJECT
public:
    explicit Remote(QObject *parent = 0);
    static void checkIdleTime();

    bool remoteTrajectory(int32_t trajNumber);
    bool newTrajectory(char *trajName, char *trajectory);
    bool remoteWalkTo(int32_t x, int32_t y, int32_t z);
    bool remoteWalk(int32_t x, int32_t y, int32_t z);
    void run();
    bool updateRemoteState();

    int32_t type; // type: REMOTE_TRAJECTORY, REMOTE_RECENTERING
    int32_t maxTrajectories;
    int32_t activeTrajectory;
    Coordinate recenteringPosition;

signals:
    void finished();
    void updateViewerStateSignal();
    void userMoveSignal(int32_t x, int32_t y, int32_t z, int32_t serverMovement);
public slots:
    void setRemoteStateType(int32_t type);
    void setRecenteringPosition(int32_t x, int32_t y, int32_t z);
    bool remoteJump(int32_t x, int32_t y, int32_t z);
};

#endif // REMOTE_H
