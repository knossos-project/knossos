/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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

void PythonProxy::annotation_add_file(const QString & name, const QByteArray & content) {
    Session::singleton().extraFiles[name] = content;
    Session::singleton().unsavedChanges = true;
}

QByteArray PythonProxy::annotation_get_file(const QString & name) {
    if (Session::singleton().extraFiles.find(name) == std::end(Session::singleton().extraFiles)) {
        return QByteArray();
    }
    return Session::singleton().extraFiles[name];
}

QString PythonProxy::getKnossosVersion() {
    return KREVISION;
}

QString PythonProxy::getKnossosRevision() {
    return KREVISION;
}

int PythonProxy::getCubeEdgeLength() {
    return Dataset::current().cubeEdgeLength;
}

QList<int> PythonProxy::getPosition() {
    return state->viewerState->currentPosition.list();
}

QList<float> PythonProxy::getScale() {
    return Dataset::current().scale.list();
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
    state->viewer->reslice_notify_all(Segmentation::singleton().layerId, Coordinate(coord));
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
