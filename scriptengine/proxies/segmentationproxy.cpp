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

#include "segmentationproxy.h"

#include "segmentation/cubeloader.h"
#include "segmentation/segmentation.h"

auto & objectFromId(const quint64 objId) {
    const auto it = Segmentation::singleton().objectIdToIndex.find(objId);
    if (it == std::end(Segmentation::singleton().objectIdToIndex)) {
        throw std::runtime_error(QObject::tr("object with id %1 does not exist").arg(objId).toStdString());
    }
    return Segmentation::singleton().objects[it->second];
}

void SegmentationProxy::mergelist_clear() {
    Segmentation::singleton().mergelistClear();
}

void SegmentationProxy::mergelist_load(QString & mergelist) {
    QTextStream stream(&mergelist);
    Segmentation::singleton().mergelistLoad(stream);
}

QString SegmentationProxy::mergelist_save() {
    QString mergelist;
    QTextStream stream(&mergelist);
    Segmentation::singleton().mergelistSave(stream);
    return mergelist;
}

void SegmentationProxy::subobject_from_id(const quint64 subObjId, const QList<int> & coord) {
    Segmentation::singleton().subobjectFromId(subObjId, Coordinate(coord));
}

void SegmentationProxy::set_render_only_selected_objs(const bool b) {
    Segmentation::singleton().setRenderOnlySelectedObjs(b);
}

bool SegmentationProxy::is_render_only_selected_objs() {
    return Segmentation::singleton().renderOnlySelectedObjs;
}

quint64 SegmentationProxy::largest_object_containing_subobject(const quint64 subObjId, const QList<int> & coord) {
    const auto & subobject = Segmentation::singleton().subobjectFromId(subObjId, Coordinate(coord));
    const auto objIndex = Segmentation::singleton().largestObjectContainingSubobject(subobject);
    return Segmentation::singleton().objects[objIndex].id;
}

QList<quint64> SegmentationProxy::subobject_ids_of_object(const quint64 objId) {
    QList<quint64> subobjectIds;
    const auto & obj = objectFromId(objId);
    for (const auto & elem : obj.subobjects) {
        subobjectIds.append(elem.get().id);
    }
    return subobjectIds;
}

QList<quint64> SegmentationProxy::objects() {
    QList<quint64> objectIds;
    for (const auto & elem : Segmentation::singleton().objects) {
        objectIds.append(elem.id);
    }
    return objectIds;
}

QList<quint64> SegmentationProxy::selected_objects() {
    QList<quint64> selectedIds;
    for (const auto index : Segmentation::singleton().selectedObjectIndices) {
        selectedIds.append(Segmentation::singleton().objects[index].id);
    }
    return selectedIds;
}

void SegmentationProxy::add_subobject(const quint64 objId, const quint64 subobjectId) {
    Segmentation::singleton().newSubObject(objectFromId(objId), subobjectId);
}

void SegmentationProxy::change_comment(const quint64 objId, const QString & comment) {
    Segmentation::singleton().changeComment(objectFromId(objId), comment);
}

void SegmentationProxy::change_color(const quint64 objId, const QColor & color) {
    Segmentation::singleton().changeColor(objectFromId(objId), std::make_tuple(color.red(), color.green(), color.blue()));
}

void SegmentationProxy::create_object(const quint64 objId, const quint64 initialSubobjectId, const QList<int> & location, const bool todo, const bool immutable) {
    Segmentation::singleton().createObjectFromSubobjectId(initialSubobjectId, Coordinate(location), objId, todo, immutable);
}

void SegmentationProxy::merge_selected_objects() {
    Segmentation::singleton().mergeSelectedObjects();
}

void SegmentationProxy::unmerge_selected_objects(const Coordinate & position) {
    Segmentation::singleton().unmergeSelectedObjects(position);
}

void SegmentationProxy::remove_object(const quint64 objId) {
    Segmentation::singleton().removeObject(objectFromId(objId));
}

void SegmentationProxy::select_object(const quint64 objId) {
    Segmentation::singleton().selectObject(objectFromId(objId));
}

quint64 SegmentationProxy::touched_subobject_id() {
    return Segmentation::singleton().touched_subobject_id;
}

quint64 SegmentationProxy::subobject_at_location(const QList<int> & position) {
    return readVoxel({position});
}

void SegmentationProxy::unselect_object(const quint64 objId) {
    Segmentation::singleton().unselectObject(objectFromId(objId));
}

void SegmentationProxy::jump_to_object(const quint64 objId) {
    Segmentation::singleton().jumpToObject(objectFromId(objId));
}

QList<int> SegmentationProxy::object_location(const quint64 objId) {
    return objectFromId(objId).location.list();
}
