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

#pragma once

#include "skeleton/node.h"
#include "viewer.h"

#include <QObject>
#include <QList>
#include <QVector>

struct _object;
using PyObject = _object;

class PythonProxy : public QObject {
    Q_OBJECT
signals:
    void echo(QString message);
    void viewport_snapshot_vp_size(SnapshotOptions & options);
    void viewport_snapshot_dataset_size(SnapshotOptions & options);
    void viewport_snapshot(const SnapshotOptions & options);
    void set_layer_visibility(int layer, bool visibilty);
    void set_mesh_3d_alpha_factor(float alpha);
    void set_mesh_slicing_alpha_factor(float alpha);

    void set_tree_visibility(const bool show_in_3d, const bool only_selected_in_3d, const bool show_in_ortho, const bool only_selected_in_ortho);
    void set_mesh_visibility(const bool show_in_3d, const bool show_in_ortho);

public slots:
    ViewportType get_viewport_type(int i) {
        return static_cast<ViewportType>(i);
    }
    SnapshotOptions make_snapshot_options(const QString & path, const ViewportType vpType, int size = 2048, const bool withAxes = true, const bool withBox = true, const bool withOverlay = true, const bool withSkeleton = true, const bool withScale = true, const bool withVpPlanes = true) {
        return {path, vpType, size, withAxes, withBox, withOverlay, withSkeleton, withScale, withVpPlanes};
    }

    void annotation_load(const QString & filename, const bool merge = false);
    void annotation_save(const QString & filename);
    void annotation_add_file(const QString & name, const QByteArray & content);
    QString annotation_filename();
    QByteArray annotation_get_file(const QString & name);
    int annotation_time();
    void set_annotation_time(int ms);
    QString get_knossos_version();
    QString get_knossos_revision();
    QString get_knossos_revision_date() const;
    int get_cube_edge_length();
    QList<int> get_position();
    QList<float> get_scale();
    void set_position(QList<int> coord);

    quint64 read_overlay_voxel(QList<int> coord);
    bool write_overlay_voxel(QList<int> coord, quint64 val);
    QVector<int> process_region_by_strided_buf_proxy(QList<int> globalFirst, QList<int> size, quint64 dataPtr,
                                        QList<int> strides, bool isWrite, bool isMarkedChanged);
    void coord_cubes_mark_changed_proxy(QVector<int> cubeChangeSetList);
    void set_movement_area(QList<int> minCoord, QList<int> maxCoord);
    void set_work_mode(const int mode);
    void refocus_viewport3d(const int x = 0, const int y = 0, const int z = 0);
    void reset_movement_area();
    QList<int> get_movement_area();
    float get_outside_movement_area_factor();
    void oc_reslice_notify_all(QList<int> coord);
    int loader_loading_nr();
    bool loader_finished();
    bool load_style_sheet(const QString &path);
    void set_magnification_lock(const bool locked);
};
