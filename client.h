#ifndef CLIENT_H
#define CLIENT_H

#include "knossos-global.h"
#include <QObject>
#include <QThread>

class Client : public QThread
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);
    static int client();
    static uint32_t broadcastPosition(uint32_t x, uint32_t y, uint32_t z);
    static int32_t skeletonSyncBroken();
    static int32_t bytesToInt(Byte *source);
    static int32_t integerToBytes(Byte *dest, int32_t source);
    static int32_t floatToBytes(Byte *dest, float source);
    static int Wrapper_SDLNet_TCP_Open(void *params);
    static uint32_t IOBufferAppend(struct IOBuffer *iobuffer, Byte *data, uint32_t length, SDL_mutex *mutex);
    static uint32_t addPeer(uint32_t id, char *name, float xScale, float yScale, float zScale, int32_t xOffset, int32_t yOffset, int32_t zOffset);
    static uint32_t delPeer(uint32_t id);
    static uint32_t broadcastCoordinate(uint32_t x, uint32_t y, uint32_t z);
    static int32_t syncMessage(char *fmt, ...);
    static int32_t clientSyncBroken();
    static int32_t parseInBufferByFmt(int32_t len, const char *fmt, float *f, Byte *s, int32_t *d, struct IOBuffer *buffer);
    
signals:
    
public slots:
    
};

#endif // CLIENT_H
