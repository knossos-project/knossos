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

#ifndef SEGMENTATIONPROXY_H
#define SEGMENTATIONPROXY_H

#include "coordinate.h"

#include <QList>
#include <QObject>

class SegmentationProxy : public QObject {
    Q_OBJECT
public slots:
    void mergelist_clear();
    void mergelist_load(QString &mergelist);
    QString mergelist_save();

    void subobject_from_id(const quint64 subObjId, const QList<int> & coord);
    void set_render_only_selected_objs(const bool b);
    bool is_render_only_selected_objs();

    quint64 largest_object_containing_subobject(const quint64 subObjId, const QList<int> & coord);
    QList<quint64> subobject_ids_of_object(const quint64 objId);

    QList<quint64> objects();
    QList<quint64> selected_objects();

    void add_subobject(const quint64 objId, const quint64 subobjectId);
    void change_comment(const quint64 objId, const QString & comment);
    void change_color(const quint64 objId, const QColor & color);
    void create_object(const quint64 objId, const quint64 initialSubobjectId, const QList<int> & location = {0, 0, 0}, const bool todo = false, const bool immutable = false);
    void merge_selected_objects();
    void unmerge_selected_objects(const Coordinate & position);
    void remove_object(const quint64 objId);
    void select_object(const quint64 objId);
    quint64 subobject_at_location(const QList<int> &position);
    quint64 touched_subobject_id();
    void unselect_object(const quint64 objId);
    void jump_to_object(const quint64 objId);
    QList<int> object_location(const quint64 objId);
};

#endif // SEGMENTATIONPROXY_H
