#include "client.h"

static int32_t clientRun() { return 0;}
static int32_t connectToServer() { return 0;}
static int32_t closeConnection() { return 0;}
static float bytesToFloat(Byte *source) { return 0;}
static uint32_t parseInBuffer() { return 0;}
static uint32_t flushOutBuffer() { return 0;}
static int32_t cleanUpClient() { return 0;}

Client::Client(QObject *parent) :
    QThread(parent)
{
}

int Client::client() { return 0;}
uint32_t Client::broadcastPosition(uint32_t x, uint32_t y, uint32_t z) { return 0;}
int32_t Client::skeletonSyncBroken() { return 0;}
int32_t Client::bytesToInt(Byte *source) { return 0;}
int32_t Client::integerToBytes(Byte *dest, int32_t source) { return 0;}
int32_t Client::floatToBytes(Byte *dest, float source) { return 0;}
int Client::Wrapper_SDLNet_TCP_Open(void *params) { return 0;}
uint32_t Client::IOBufferAppend(struct IOBuffer *iobuffer, Byte *data, uint32_t length, SDL_mutex *mutex) { return 0;}
uint32_t Client::addPeer(uint32_t id, char *name, float xScale, float yScale, float zScale, int32_t xOffset, int32_t yOffset, int32_t zOffset) { return 0;}
uint32_t Client::delPeer(uint32_t id) { return 0;}
uint32_t Client::broadcastCoordinate(uint32_t x, uint32_t y, uint32_t z) { return 0;}
int32_t Client::syncMessage(char *fmt, ...) { return 0;}
int32_t Client::clientSyncBroken() { return 0;}
int32_t Client::parseInBufferByFmt(int32_t len, const char *fmt, float *f, Byte *s, int32_t *d, struct IOBuffer *buffer) { return 0;}
