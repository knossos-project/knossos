#include "segmentationproxy.h"
#include "segmentation/segmentation.h"

SegmentationProxy::SegmentationProxy(QObject *parent) :
    QObject(parent)
{

}

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
