#ifndef CLIENT_H
#define CLIENT_H

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

#include "knossos-global.h"
#include <QObject>
#include <QThread>

/**
  *
  * @class Client
  * @brief The client class is dedicated to open more than one knossos instance at wish.
  * This is based that users can work with datasets in different magnification at once!
  */
class QAbstractSocket;
class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);

    bool qconnectToServer();

    static bool broadcastPosition(uint32_t x, uint32_t y, uint32_t z);
    static bool skeletonSyncBroken();
    static int32_t bytesToInt(Byte *source);
    static bool integerToBytes(Byte *dest, int32_t source);
    static bool floatToBytes(Byte *dest, float source);
    static int Wrapper_SDLNet_TCP_Open(void *params);
    static bool IOBufferAppend(struct IOBuffer *iobuffer, Byte *data, uint32_t length, QMutex *mutex);
    static bool addPeer(uint32_t id, char *name, float xScale, float yScale, float zScale, int32_t xOffset, int32_t yOffset, int32_t zOffset);
    static bool delPeer(uint32_t id);
    static bool broadcastCoordinate(uint32_t x, uint32_t y, uint32_t z);
    static bool syncMessage(const char *fmt, ...);
    static int32_t parseInBufferByFmt(int32_t len, const char *fmt, float *f, Byte *s, int32_t *d, struct IOBuffer *buffer);
    static Coordinate *transNetCoordinate(unsigned int id, int x, unsigned int y, int z);
    
signals:
    void finished();
public slots:
    void start();
    void socketConnectionSucceeded();
    void socketConnectionFailed(QAbstractSocket::SocketError error);
    
};

#endif // CLIENT_H
