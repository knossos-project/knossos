/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "pythonproxy.h"

#include "buildinfo.h"
#include "functions.h"
#include "loader.h"
#include "segmentation/cubeloader.h"
#include "skeleton/node.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "stateInfo.h"
#include "version.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <Python.h>

#include <QApplication>
#include <QFile>

void PythonProxy::annotationLoad(const QString & filename, const bool merge) {
    state->mainWindow->openFileDispatch({filename}, merge, true);
}

void PythonProxy::annotationSave(const QString & filename) {
    state->mainWindow->save(filename, true);
}

QString PythonProxy::getKnossosVersion() {
    return KVERSION;
}

QString PythonProxy::getKnossosRevision() {
    return KREVISION;
}

int PythonProxy::getCubeEdgeLength() {
    return state->cubeEdgeLength;
}

QList<int> PythonProxy::getOcPixel(QList<int> Dc, QList<int> pxInDc) {
    char *cube = (char*)Coordinate2BytePtr_hash_get_or_fail(state->Oc2Pointer[int_log(state->magnification)], CoordOfCube(Dc[0], Dc[1], Dc[2]));
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
    return state->viewerState->currentPosition.list();
}

QList<float> PythonProxy::getScale() {
    return state->scale.list();
}


char *PythonProxy::addrDcOc2Pointer(QList<int> coord, bool isOc) {
    coord2bytep_map_t *PointerMap = isOc ? state->Oc2Pointer : state->Dc2Pointer;
    char *data = Coordinate2BytePtr_hash_get_or_fail(PointerMap[(int)std::log2(state->magnification)], coord);
    if (data == NULL) {
        emit echo(QString("no cube data found at Coordinate (%1, %2, %3)").arg(coord[0]).arg(coord[1]).arg(coord[2]));
    }
    return data;
}

QByteArray PythonProxy::readDc2Pointer(QList<int> coord) {
    char *data = addrDcOc2Pointer(coord, false);
    if(!data) {
        return QByteArray();
    }

    return QByteArray::fromRawData((const char*)data, state->cubeBytes);
}

PyObject* PythonProxy::PyBufferAddrDcOc2Pointer(QList<int> coord, bool isOc) {
    void *data = addrDcOc2Pointer(coord,isOc);
    return PyBuffer_FromReadWriteMemory(data, state->cubeBytes*(isOc ? OBJID_BYTES : 1));
}

int PythonProxy::readDc2PointerPos(QList<int> coord, int pos) {
    char *data = addrDcOc2Pointer(coord, false);
    if(!data) {
        return -1;
    }

    return data[pos];
}

bool PythonProxy::writeDc2Pointer(QList<int> coord, char *bytes) {
    char *data = addrDcOc2Pointer(coord,false);
    if(!data) {
        return false;
    }

    memcpy(data, bytes, state->cubeBytes);
    return true;
}

bool PythonProxy::writeDc2PointerPos(QList<int> coord, int pos, int val) {
    char *data = addrDcOc2Pointer(coord,false);
    if(!data) {
        return false;
    }

    data[pos] = val;
    return true;
}

QByteArray PythonProxy::readOc2Pointer(QList<int> coord) {
    char *data = addrDcOc2Pointer(coord,true);
    if(!data) {
        return QByteArray();
    }

    return QByteArray::fromRawData((const char*)data, state->cubeBytes * OBJID_BYTES);
}

quint64 PythonProxy::readOc2PointerPos(QList<int> coord, int pos) {
    quint64 *data = (quint64*)addrDcOc2Pointer(coord,true);
    if(!data) {
        return -1;
    }

    return data[pos];
}

bool PythonProxy::writeOc2Pointer(QList<int> coord, char *bytes) {
    char *data = addrDcOc2Pointer(coord,true);
    if(!data) {
        return false;
    }

    memcpy(data, bytes, state->cubeBytes * OBJID_BYTES);
    return true;
}

bool PythonProxy::writeOc2PointerPos(QList<int> coord, int pos, quint64 val) {
    quint64 *data = (quint64*)addrDcOc2Pointer(coord,true);
    if(!data) {
        return false;
    }

    data[pos] = val;
    return true;
}

QVector<int> PythonProxy::processRegionByStridedBufProxy(QList<int> globalFirst, QList<int> size,
                             quint64 dataPtr, QList<int> strides, bool isWrite, bool isMarkChanged) {
    auto cubeChangeSet = processRegionByStridedBuf(Coordinate(globalFirst), Coordinate(globalFirst) + Coordinate(size) - 1, (char*)dataPtr, Coordinate(strides), isWrite, isMarkChanged);
    QVector<int> cubeChangeSetVector;
    for (auto &elem : cubeChangeSet) {
        cubeChangeSetVector += elem.vector();
    }
    return cubeChangeSetVector;
}

void PythonProxy::coordCubesMarkChangedProxy(QVector<int> cubeChangeSetList) {
    CubeCoordSet cubeChangeSet;
    auto elemNum = cubeChangeSetList.length();
    for (int i = 0; i < elemNum; i += 3) {
        cubeChangeSet.emplace(CoordOfCube(cubeChangeSetList[i],cubeChangeSetList[i+1],cubeChangeSetList[i+2]));
    }
    coordCubesMarkChanged(cubeChangeSet);
}

quint64 PythonProxy::readOverlayVoxel(QList<int> coord) {
    return readVoxel(coord);
}

bool PythonProxy::writeOverlayVoxel(QList<int> coord, quint64 val) {
    return writeVoxel(coord, val);
}

void PythonProxy::setPosition(QList<int> coord) {
    state->viewer->setPosition({static_cast<float>(coord[0]), static_cast<float>(coord[1]), static_cast<float>(coord[2])});
}

void PythonProxy::resetMovementArea() {
    Session::singleton().resetMovementArea();
}

void PythonProxy::setMovementArea(QList<int> minCoord, QList<int> maxCoord) {
    Session::singleton().updateMovementArea(minCoord,maxCoord);
}

QList<int> PythonProxy::getMovementArea() {
    return  Session::singleton().movementAreaMin.list() + Session::singleton().movementAreaMax.list();
}

float PythonProxy::getMovementAreaFactor() {
    return state->viewerState->outsideMovementAreaFactor;
}

void PythonProxy::oc_reslice_notify_all(QList<int> coord) {
    state->viewer->oc_reslice_notify_all(Coordinate(coord));
}

int PythonProxy::loaderLoadingNr() {
    return Loader::Controller::singleton().loadingNr;
}

bool PythonProxy::loaderFinished() {
    return Loader::Controller::singleton().isFinished();
}

void PythonProxy::setMagnificationLock(const bool locked) {
    state->viewer->setMagnificationLock(locked);
}

int PythonProxy::annotation_time() {
    return Session::singleton().getAnnotationTime();
}

void PythonProxy::set_annotation_time(int ms) {
    Session::singleton().setAnnotationTime(ms);
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

