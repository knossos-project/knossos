#include "client.h"

static int32_t clientRun() {}
static int32_t connectToServer() {}
static int32_t closeConnection() {}
static float bytesToFloat(Byte *source) {}
static uint32_t parseInBuffer() {}
static uint32_t flushOutBuffer() {}
static int32_t cleanUpClient() {}

Client::Client(QObject *parent) :
    QThread(parent)
{
}

int Client::client() {}
uint32_t Client::broadcastPosition(uint32_t x, uint32_t y, uint32_t z) {}
int32_t Client::skeletonSyncBroken() {}
int32_t Client::bytesToInt(Byte *source) {}
int32_t Client::integerToBytes(Byte *dest, int32_t source) {}
int32_t Client::floatToBytes(Byte *dest, float source) {}
int Client::Wrapper_SDLNet_TCP_Open(void *params) {}
uint32_t Client::IOBufferAppend(struct IOBuffer *iobuffer, Byte *data, uint32_t length, SDL_mutex *mutex) {}
uint32_t Client::addPeer(uint32_t id, char *name, float xScale, float yScale, float zScale, int32_t xOffset, int32_t yOffset, int32_t zOffset) {}
uint32_t Client::delPeer(uint32_t id) {}
uint32_t Client::broadcastCoordinate(uint32_t x, uint32_t y, uint32_t z) {}
int32_t Client::syncMessage(char *fmt, ...) {}
int32_t Client::clientSyncBroken() {}
int32_t Client::parseInBufferByFmt(int32_t len, const char *fmt, float *f, Byte *s, int32_t *d, struct IOBuffer *buffer) {}
