#ifndef CLIENT_H
#define CLIENT_H

#include "knossos-global.h"
#include <QObject>
#include <QThread>

class Client : public QThread
{
    Q_OBJECT
public:
    int32_t connectAsap;
    int32_t remotePort;
    int32_t connected;
    Byte synchronizePosition;
    Byte synchronizeSkeleton;
    int32_t connectionTimeout;
    int32_t connectionTried;
    char serverAddress[1024];
    IPaddress remoteServer;
    TCPsocket remoteSocket;
    SDLNet_SocketSet socketSet;

    uint32_t myId;
    uint32_t saveMaster;

    peerListElement *firstPeer;
    IOBuffer *inBuffer;
    IOBuffer *outBuffer;

    explicit Client(QObject *parent = 0);
    static int client();
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
    static bool syncMessage(char *fmt, ...);
    static int32_t parseInBufferByFmt(int32_t len, const char *fmt, float *f, Byte *s, int32_t *d, struct IOBuffer *buffer);
    static Coordinate *transNetCoordinate(unsigned int id, int x, unsigned int y, int z);
    
signals:
    
public slots:
    
};

#endif // CLIENT_H
