#include "remote.h"

Remote::Remote(QObject *parent) :
    QThread(parent)
{
}

void Remote::checkIdleTime() { }

int32_t Remote::remoteJump(int32_t x, int32_t y, int32_t z) { return 0;}
