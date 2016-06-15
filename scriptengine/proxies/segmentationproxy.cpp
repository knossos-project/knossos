#include "segmentationproxy.h"

#include "segmentation/segmentation.h"

auto & objectFromId(const quint64 objId) {
    const auto it = Segmentation::singleton().objectIdToIndex.find(objId);
    if (it == std::end(Segmentation::singleton().objectIdToIndex)) {
        throw std::runtime_error("object does not exist");
    }
    return Segmentation::singleton().objects[it->second];
}

void SegmentationProxy::subobjectFromId(const quint64 subObjId, const QList<int> & coord) {
    Segmentation::singleton().subobjectFromId(subObjId, Coordinate(coord));
}

void SegmentationProxy::setRenderOnlySelectedObjs(const bool b) {
    Segmentation::singleton().setRenderOnlySelectedObjs(b);
}

bool SegmentationProxy::isRenderOnlySelecdedObjs() {
    return Segmentation::singleton().renderOnlySelectedObjs;
}

quint64 SegmentationProxy::largestObjectContainingSubobject(const quint64 subObjId, const QList<int> & coord) {
    const auto & subobject = Segmentation::singleton().subobjectFromId(subObjId, Coordinate(coord));
    const auto objIndex = Segmentation::singleton().largestObjectContainingSubobject(subobject);
    return Segmentation::singleton().objects[objIndex].id;
}

QList<quint64> SegmentationProxy::subobjectIdsOfObject(const quint64 objId) {
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

QList<quint64> SegmentationProxy::selectedObjects() {
    QList<quint64> selectedIds;
    for (const auto index : Segmentation::singleton().selectedObjectIndices) {
        selectedIds.append(Segmentation::singleton().objects[index].id);
    }
    return selectedIds;
}

void SegmentationProxy::changeComment(const quint64 objId, const QString & comment) {
    Segmentation::singleton().changeComment(objectFromId(objId), comment);
}

void SegmentationProxy::removeObject(const quint64 objId) {
    Segmentation::singleton().removeObject(objectFromId(objId));
}

void SegmentationProxy::selectObject(const quint64 objId) {
    Segmentation::singleton().selectObject(objectFromId(objId));
}

void SegmentationProxy::unselectObject(const quint64 objId) {
    Segmentation::singleton().unselectObject(objectFromId(objId));
}

void SegmentationProxy::jumpToObject(const quint64 objId) {
    Segmentation::singleton().jumpToObject(objectFromId(objId));
}

QList<int> SegmentationProxy::objectLocation(const quint64 objId) {
    return objectFromId(objId).location.list();
}
