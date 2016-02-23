#include "segmentationproxy.h"

#include "segmentation/segmentation.h"

void SegmentationProxy::subobjectFromId(quint64 subObjId, QList<int> coord) {
    Segmentation::singleton().subobjectFromId(subObjId, Coordinate(coord));
}

quint64 SegmentationProxy::largestObjectContainingSubobject(quint64 subObjId, QList<int> coord) {
    return Segmentation::singleton().largestObjectContainingSubobject(
                Segmentation::singleton().subobjectFromId(subObjId, Coordinate(coord)));
}

void SegmentationProxy::changeComment(quint64 objIndex, QString comment) {
    Segmentation::singleton().changeComment(Segmentation::singleton().objects[objIndex], comment);
}

void SegmentationProxy::removeObject(quint64 objIndex) {
    Segmentation::singleton().removeObject(Segmentation::singleton().objects[objIndex]);
}

void SegmentationProxy::setRenderAllObjs(bool b) {
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

QList<quint64> SegmentationProxy::subobjectIdsOfObject(quint64 objId) {
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

void SegmentationProxy::jumpToObject(quint64 objIdx) {
    Segmentation::singleton().jumpToObject(objIdx);
}

void SegmentationProxy::selectObject(quint64 objIdx) {
    Segmentation::singleton().selectObject(objIdx);
}

QList<quint64> SegmentationProxy::getAllObjectIdx() {
    QList<quint64> allObjIdx;

    for(auto & elem : Segmentation::singleton().objects) {
        allObjIdx.append(elem.index);
    }

    return allObjIdx;
}

QList<quint64> SegmentationProxy::getSelectedObjectIndices() {
    QList<quint64> selected;

    for(auto elem : Segmentation::singleton().selectedObjectIndices) {
        selected.append(elem);
    }

    return selected;
}

void SegmentationProxy::unselectObject(const quint64 objectIndex) {
    Segmentation::singleton().unselectObject(objectIndex);
}

QList<quint64> * SegmentationProxy::getObjectLocation(const quint64 objectIndex) {
    if(objectIndex < Segmentation::singleton().objects.size()) {
        auto & obj = Segmentation::singleton().objects[objectIndex];
        QList<quint64> * location = new QList<quint64>();
        location->append(obj.location.x);
        location->append(obj.location.y);
        location->append(obj.location.z);
        return location;
    }

    return nullptr;
}
