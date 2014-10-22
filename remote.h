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

class Remote : public QThread {
    Q_OBJECT
private:
    bool rotate;
    uint activeVP;

    std::deque<floatCoordinate> lastRecenterings;
public:
    explicit Remote(QObject *parent = 0);
    void msleep(unsigned long msec);

    bool remoteWalk(int x, int y, int z);
    void run();

    floatCoordinate recenteringPosition;

signals:
    void finished();
    void updateViewerStateSignal();
    void userMoveSignal(int x, int y, int z, Byte userMoveType, Byte viewportType);
    void rotationSignal(float x, float y, float z, float angle);
public slots:
    void setRecenteringPosition(float x, float y, float z);
    void setRecenteringPositionWithRotation(float x, float y, float z, uint vp);
    bool remoteJump(int x, int y, int z);
};

#endif // REMOTE_H
