#include "remote.h"

Remote::Remote(QObject *parent) :
    QThread(parent)
{
}

void Remote::checkIdleTime() { }
