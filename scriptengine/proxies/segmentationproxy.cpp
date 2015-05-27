#include "segmentationproxy.h"
#include "segmentation/segmentation.h"

SegmentationProxy::SegmentationProxy(QObject *parent) :
    QObject(parent)
{

}

void SegmentationProxy::clickSubObj(quint64 subObjId, QList<int> coord) {
    Segmentation::singleton().subobjectFromId(subObjId, Coordinate(coord));
}
