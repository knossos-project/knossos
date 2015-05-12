#include "pythonproxy.h"

#include "buildinfo.h"
#include "functions.h"
#include "segmentation/cubeloader.h"
#include "stateInfo.h"
#include "skeleton/node.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "version.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <QApplication>
#include <QFile>

PythonProxySignalDelegate *pythonProxySignalDelegate = new PythonProxySignalDelegate();

PythonProxy::PythonProxy(QObject *parent) : QObject(parent) {}

QString PythonProxy::getKnossosVersion() {
    return KVERSION;
}

QString PythonProxy::getKnossosRevision() {
    return KREVISION;
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

QList<int> CoordToList(const Coordinate &c) {
    QList<int> l;
    l.append(c.x);
    l.append(c.y);
    l.append(c.z);
    return l;
}

QList<int> PythonProxy::getPosition() {
    return CoordToList(state->viewerState->currentPosition);
}

QList<float> PythonProxy::getScale() {
    QList<float> l;
    auto scale = state->scale;
    l.append(scale.x);
    l.append(scale.y);
    l.append(scale.z);
    return l;
}


QByteArray PythonProxy::readDc2Pointer(int x, int y, int z) {
    Coordinate position(x, y, z);
    char *data = Coordinate2BytePtr_hash_get_or_fail(state->Dc2Pointer[(int)std::log2(state->magnification)], position);
    if(!data) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(x).arg(y).arg(z));
        return QByteArray();
    }

    return QByteArray::fromRawData((const char*)data, state->cubeBytes);
}

char *PythonProxy::addrDcOc2Pointer(int x, int y, int z, bool isOc) {
    coord2bytep_map_t *PointerMap = isOc ? state->Oc2Pointer : state->Dc2Pointer;
    return Coordinate2BytePtr_hash_get_or_fail(PointerMap[(int)std::log2(state->magnification)], Coordinate(x,y,z));
}

PyObject* PythonProxy::PyBufferAddrDcOc2Pointer(int x, int y, int z, bool isOc) {
    void *data = addrDcOc2Pointer(x,y,z,isOc);
    return PyBuffer_FromReadWriteMemory(data, state->cubeBytes*(isOc ? OBJID_BYTES : 1));
}

int PythonProxy::readDc2PointerPos(int x, int y, int z, int pos) {
    char *data = addrDcOc2Pointer(x,y,z, false);
    if(!data) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(x).arg(y).arg(z));
        return -1;
    }

    return data[pos];
}

bool PythonProxy::writeDc2Pointer(int x, int y, int z, char *bytes) {
    char *data = addrDcOc2Pointer(x,y,z,false);
    if(!data) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(x).arg(y).arg(z));
        return false;
    }

    memcpy(data, bytes, state->cubeBytes);
    return true;
}

bool PythonProxy::writeDc2PointerPos(int x, int y, int z, int pos, int val) {
    char *data = addrDcOc2Pointer(x,y,z,false);
    if(!data) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(x).arg(y).arg(z));
        return false;
    }

    data[pos] = val;
    return true;
}

QByteArray PythonProxy::readOc2Pointer(int x, int y, int z) {
    char *data = addrDcOc2Pointer(x,y,z,true);
    if(!data) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(x).arg(y).arg(z));
        return QByteArray();
    }

    return QByteArray::fromRawData((const char*)data, state->cubeBytes * OBJID_BYTES);
}

quint64 PythonProxy::readOc2PointerPos(int x, int y, int z, int pos) {
    quint64 *data = (quint64*)addrDcOc2Pointer(x,y,z,true);
    if(!data) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(x).arg(y).arg(z));
        return -1;
    }

    return data[pos];
}

bool PythonProxy::writeOc2Pointer(int x, int y, int z, char *bytes) {
    char *data = addrDcOc2Pointer(x,y,z,true);
    if(!data) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(x).arg(y).arg(z));
        return false;
    }

    memcpy(data, bytes, state->cubeBytes * OBJID_BYTES);
    return true;
}

bool PythonProxy::writeOc2PointerPos(int x, int y, int z, int pos, quint64 val) {
    quint64 *data = (quint64*)addrDcOc2Pointer(x,y,z,true);
    if(!data) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(x).arg(y).arg(z));
        return false;
    }

    data[pos] = val;
    return true;
}

quint64 PythonProxy::readOverlayVoxel(int x, int y, int z) {
    return readVoxel(Coordinate(x,y,z));
}

bool PythonProxy::writeOverlayVoxel(int x, int y, int z, quint64 val) {
    return writeVoxel(Coordinate(x,y,z), val);
}

void PythonProxy::setPosition(int x, int y, int z) {
    emit pythonProxySignalDelegate->userMoveSignal(x, y, z, USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
}

void PythonProxy::resetMovementArea() {
    Session::singleton().resetMovementArea();
}

void PythonProxy::setMovementArea(int minx, int miny, int minz, int maxx, int maxy, int maxz) {
    Session::singleton().updateMovementArea(Coordinate(minx, miny, minz),Coordinate(maxx, maxy, maxz));
}

QList<int> PythonProxy::getMovementArea() {
    Coordinate min = Session::singleton().movementAreaMin;
    Coordinate max = Session::singleton().movementAreaMax;
    QList<int> l;
    l.append(min.x);
    l.append(min.y);
    l.append(min.z);
    l.append(max.x);
    l.append(max.y);
    l.append(max.z);
    return l;
}

float PythonProxy::getMovementAreaFactor() {
    return state->viewerState->movementAreaFactor;
}

// UNTESTED
bool PythonProxy::loadStyleSheet(const QString &filename) {
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        emit echo("Error reading the style sheet file");
        return false;
    }

    QString design(file.readAll());

    qApp->setStyleSheet(design);
    file.close();
    return true;
}

