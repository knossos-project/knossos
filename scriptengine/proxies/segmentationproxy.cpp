#include "segmentationproxy.h"

#include "segmentation/segmentation.h"

void SegmentationProxy::subobjectFromId(const quint64 subObjId, const QList<int> & coord) {
    Segmentation::singleton().subobjectFromId(subObjId, Coordinate(coord));
}

quint64 SegmentationProxy::largestObjectContainingSubobject(const quint64 subObjId, const QList<int> & coord) {
    const auto & subobject = Segmentation::singleton().subobjectFromId(subObjId, Coordinate(coord));
    const auto objIndex = Segmentation::singleton().largestObjectContainingSubobject(subobject);
    return Segmentation::singleton().objects[objIndex].id;
}

void SegmentationProxy::changeComment(const quint64 objIndex, const QString & comment) {
    Segmentation::singleton().changeComment(Segmentation::singleton().objects[objIndex], comment);
}

void SegmentationProxy::removeObject(const quint64 objIndex) {
    Segmentation::singleton().removeObject(Segmentation::singleton().objects[objIndex]);
}

void SegmentationProxy::setRenderAllObjs(const bool b) {
    Segmentation::singleton().setRenderAllObjs(b);
}

bool SegmentationProxy::isRenderAllObjs() {
    return Segmentation::singleton().renderAllObjs;
}

QList<quint64> SegmentationProxy::objectIds() {
    QList<quint64> objectIds;
    for (const auto & elem : Segmentation::singleton().objects) {
        objectIds.append(elem.id);
    }
    return objectIds;
}

quint64 SegmentationProxy::objectIdxFromId(const quint64 objId) {
    const auto it = Segmentation::singleton().objectIdToIndex.find(objId);
    if (it == std::end(Segmentation::singleton().objectIdToIndex)) {
        return {};
    }
    const auto objIndex = it->second;

    return objIndex;
}

QList<quint64> SegmentationProxy::subobjectIdsOfObjectByIndex(const quint64 objIndex) {
    QList<quint64> subobjectIds;
    const auto & obj = Segmentation::singleton().objects[objIndex];
    for (const auto & elem : obj.subobjects) {
        subobjectIds.append(elem.get().id);
    }
    return subobjectIds;
}

QList<quint64> SegmentationProxy::subobjectIdsOfObject(const quint64 objId) {
    const auto it = Segmentation::singleton().objectIdToIndex.find(objId);
    if (it == std::end(Segmentation::singleton().objectIdToIndex)) {
        return {};
    }
    QList<quint64> subobjectIds;
    const auto objIndex = it->second;
    const auto & obj = Segmentation::singleton().objects[objIndex];
    for (const auto & elem : obj.subobjects) {
        subobjectIds.append(elem.get().id);
    }
    return subobjectIds;
}

QList<quint64> SegmentationProxy::getAllObjectIdx() {
    QList<quint64> allObjIdx;
    for (const auto & elem : Segmentation::singleton().objects) {
        allObjIdx.append(elem.index);
    }
    return allObjIdx;
}

QList<quint64> SegmentationProxy::getSelectedObjectIndices() {
    QList<quint64> selected;
    for (const auto index : Segmentation::singleton().selectedObjectIndices) {
        selected.append(index);
    }
    return selected;
}

void SegmentationProxy::selectObject(const quint64 objIdx) {
    Segmentation::singleton().selectObject(objIdx);
}

void SegmentationProxy::unselectObject(const quint64 objectIndex) {
    Segmentation::singleton().unselectObject(objectIndex);
}

void SegmentationProxy::jumpToObject(const quint64 objIdx) {
    Segmentation::singleton().jumpToObject(objIdx);
}

QList<int> SegmentationProxy::getObjectLocation(const quint64 objectIndex) {
    if (objectIndex < Segmentation::singleton().objects.size()) {
        return Segmentation::singleton().objects[objectIndex].location.list();
    }
    return {};
}
