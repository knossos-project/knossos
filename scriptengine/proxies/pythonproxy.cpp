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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "pythonproxy.h"

#include "annotation/annotation.h"
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

#include <QApplication>
#include <QFile>

void PythonProxy::annotation_load(const QString & filename, const bool merge) {
    state->mainWindow->openFileDispatch({filename}, merge, true);
}

void PythonProxy::annotation_save(const QString & filename) {
    state->mainWindow->save(filename, true);
}

void PythonProxy::annotation_add_file(const QString & name, const QByteArray & content) {
    Annotation::singleton().extraFiles[name] = content;
    Annotation::singleton().setUnsavedChanges();
}

QByteArray PythonProxy::annotation_get_file(const QString & name) {
    if (auto it = Annotation::singleton().extraFiles.find(name); it != std::end(Annotation::singleton().extraFiles)) {
        return *it;
    } else {
        throw std::runtime_error("annotation_get_file not available " + name.toUtf8());
    }
}

QString PythonProxy::annotation_filename() {
    return Annotation::singleton().annotationFilename;
}

QString PythonProxy::get_knossos_version() {
    return KREVISION;
}

QString PythonProxy::get_knossos_revision() {
    return KREVISION;
}

QString PythonProxy::get_knossos_revision_date() const {
    return KREVISIONDATE;
}

int PythonProxy::get_cube_edge_length() {
    return Dataset::current().cubeShape.x;
}

QList<int> PythonProxy::get_position() {
    return state->viewerState->currentPosition.list();
}

QList<float> PythonProxy::get_scale() {
    return Dataset::current().scale.list();
}

QVector<int> PythonProxy::process_region_by_strided_buf_proxy(QList<int> globalFirst, QList<int> size,
                             quint64 dataPtr, QList<int> strides, bool isWrite, bool isMarkChanged) {
    auto cubeChangeSet = processRegionByStridedBuf(Coordinate(globalFirst), Coordinate(globalFirst) + Coordinate(size) - 1, (char*)dataPtr, Coordinate(strides), isWrite, isMarkChanged);
    QVector<int> cubeChangeSetVector;
    for (auto &elem : cubeChangeSet) {
        cubeChangeSetVector += elem.vector();
    }
    return cubeChangeSetVector;
}

void PythonProxy::coord_cubes_mark_changed_proxy(QVector<int> cubeChangeSetList) {
    CubeCoordSet cubeChangeSet;
    auto elemNum = cubeChangeSetList.length();
    for (int i = 0; i < elemNum; i += 3) {
        cubeChangeSet.emplace(CoordOfCube(cubeChangeSetList[i],cubeChangeSetList[i+1],cubeChangeSetList[i+2]));
    }
    coordCubesMarkChanged(cubeChangeSet);
}

quint64 PythonProxy::read_overlay_voxel(QList<int> coord) {
    return readVoxel(coord);
}

bool PythonProxy::write_overlay_voxel(QList<int> coord, quint64 val) {
    return writeVoxel(coord, val);
}

void PythonProxy::set_position(QList<int> coord) {
    state->viewer->setPosition({static_cast<float>(coord[0]), static_cast<float>(coord[1]), static_cast<float>(coord[2])});
}

void PythonProxy::refocus_viewport3d(const int x, const int y, const int z) {
    state->viewer->mainWindow.viewport3D->refocus((x > 0 && y > 0 && z > 0) ? Coordinate(x, y, z) : boost::optional<Coordinate>());
}

void PythonProxy::reset_movement_area() {
    Annotation::singleton().resetMovementArea();
}

void PythonProxy::set_movement_area(QList<int> minCoord, QList<int> maxCoord) {
    Annotation::singleton().updateMovementArea(minCoord,maxCoord);
}

QList<int> PythonProxy::get_movement_area() {
    return  Annotation::singleton().movementAreaMin.list() + Annotation::singleton().movementAreaMax.list();
}

float PythonProxy::get_outside_movement_area_factor() {
    return state->viewerState->outsideMovementAreaFactor;
}

void PythonProxy::set_work_mode(const int mode) {
    state->mainWindow->setWorkMode(static_cast<AnnotationMode>(mode));
}

void PythonProxy::oc_reslice_notify_all(QList<int> coord) {
    state->viewer->reslice_notify_all(Segmentation::singleton().layerId, Dataset::datasets[Segmentation::singleton().layerId].global2cube(coord));
}

int PythonProxy::loader_loading_nr() {
    return Loader::Controller::singleton().loadingNr;
}

bool PythonProxy::loader_finished() {
    return Loader::Controller::singleton().isFinished();
}

void PythonProxy::set_magnification_lock(const bool locked) {
    state->viewer->setMagnificationLock(locked);
}

int PythonProxy::annotation_time() {
    return Annotation::singleton().getAnnotationTime();
}

void PythonProxy::set_annotation_time(int ms) {
    Annotation::singleton().setAnnotationTime(ms);
}

// UNTESTED
bool PythonProxy::load_style_sheet(const QString &filename) {
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
