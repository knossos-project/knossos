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
class Client : public QThread
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);

    bool connectToServer(QTcpSocket *remoteSocket);
    bool closeConnection(QTcpSocket *remoteSocket);
    bool flushOutBuffer();

    static bool broadcastPosition(uint x, uint y, uint z);
    static bool skeletonSyncBroken();


    static int bytesToInt(Byte *source);
    static bool integerToBytes(Byte *dest, int source);
    static bool floatToBytes(Byte *dest, float source);
    static int Wrapper_SDLNet_TCP_Open(void *params);
    static bool IOBufferAppend(struct IOBuffer *iobuffer, Byte *data, uint length, QMutex *mutex);
    static bool addPeer(uint id, char *name, float xScale, float yScale, float zScale, int xOffset, int yOffset, int zOffset);
    static bool delPeer(uint id);
    static bool broadcastCoordinate(uint x, uint y, uint z);
    static bool syncMessage(const char *fmt, ...);
    int parseInBufferByFmt(int len, const char *fmt, float *f, Byte *s, int *d, struct IOBuffer *buffer);
    static Coordinate *transNetCoordinate(unsigned int id, int x, unsigned int y, int z);

    uint parseInBuffer();
    bool clientRun(QTcpSocket *remoteSocket);

    bool connectAsap;
    int remotePort;
    bool connected;
    Byte synchronizePosition;
    Byte synchronizeSkeleton;
    int connectionTimeout;
    bool connectionTried;
    char serverAddress[1024];

    QHostAddress *remoteServer;
    QTcpSocket *remoteSocket;
    QSet<QTcpSocket *> *socketSet;
    uint myId;
    bool saveMaster;

    struct peerListElement *firstPeer;
    struct IOBuffer *inBuffer;
    struct IOBuffer *outBuffer;


    void run();
signals:
    void finished();
    void updateSkeletonFileNameSignal(int targetRevision, int increment, char *filename);
    void setActiveNodeSignal(int targetRevision, nodeListElement *node, int nodeID);
    void addTreeCommentSignal(int targetRevision, int treeID, char *comment);
    void remoteJumpSignal(int x, int y, int z);
    void skeletonWorkModeSignal(int targetRevision, uint workMode);
    void clearSkeletonSignal(int targetRevision, int loadingSkeleton);
    void delSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel);
    void editNodeSignal(int targetRevision, int nodeID, nodeListElement *node, float newRadius, int newXPos, int newYPos, int newZPos, int inMag);
    void delNodeSignal(int targetRevision, int nodeID, nodeListElement *nodeToDel);
    void delTreeSignal(int targetRevision, int treeID);
    void addCommentSignal(int targetRevision, const char *content, nodeListElement *node, int nodeID);
    bool editCommentSignal(int targetRevision, commentListElement *currentComment, int nodeID, char *newContent, nodeListElement *newNode, int newNodeID);
    bool delCommentSignal(int targetRevision, commentListElement *currentComment, int commentNodeID);
    void popBranchNodeSignal(int targetRevision);
    void pushBranchNodeSignal(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, int branchNodeID);
    void sendConnectedState();
    void sendDisconnectedState();
public slots:

    void socketConnectionSucceeded();
    void socketConnectionFailed(QAbstractSocket::SocketError error);
    
};

#endif // CLIENT_H
