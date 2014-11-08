#include "pythonproxy.h"
#include "functions.h"
#include "skeletonizer.h"
#include <QApplication>
#include <QFile>

#include "knossos-global.h"

PythonProxy::PythonProxy(QObject *parent) :
    QObject(parent)
{

}

QList<int> PythonProxy::getOcPixel(QList<int> Dc, QList<int> pxInDc) {
    char *cube = (char*)Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[int_log(state->magnification)], Coordinate(Dc[0], Dc[1], Dc[2]));
    if (NULL == cube) {
        return QList<int>();
    }
    int index = (pxInDc[2] * state->cubeSliceArea) + (pxInDc[1] * state->cubeEdgeLength) + pxInDc[0];
    int byte_index = index * OBJID_BYTES;
    QList<int> charList;
    for (int i = 0; i < 3; i++) {
        charList.append((int)cube[byte_index + i]);
    }
    return charList;
}

QList<int> PythonProxy::getPosition() {
    QList<int> l;
    Coordinate c = state->currentPositionX;
    l.append(c.x);
    l.append(c.y);
    l.append(c.z);
    return l;
}
