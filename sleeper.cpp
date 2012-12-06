#include "sleeper.h"
#include <QThread>

Sleeper::Sleeper(QThread *parent) :
    QThread(parent)
{
}

void Sleeper::msleep(int ms) {
    QThread::msleep(ms);
}
