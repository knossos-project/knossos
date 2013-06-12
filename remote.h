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

    bool remoteTrajectory(int trajNumber);
    bool newTrajectory(char *trajName, char *trajectory);
    bool remoteWalkTo(int x, int y, int z);
    bool remoteWalk(int x, int y, int z);
    void run();
    bool updateRemoteState();
    bool remoteDelay(int s);

    int type; // type: REMOTE_TRAJECTORY, REMOTE_RECENTERING
    int maxTrajectories;
    int activeTrajectory;
    Coordinate recenteringPosition;

signals:
    void finished();
    void updateViewerStateSignal();
    void userMoveSignal(int x, int y, int z, int serverMovement);
    void updatePositionSignal(int serverMovement);
    void idleTimeSignal();
public slots:
    void setRemoteStateType(int type);
    void setRecenteringPosition(int x, int y, int z);
    bool remoteJump(int x, int y, int z);
};

#endif // REMOTE_H
