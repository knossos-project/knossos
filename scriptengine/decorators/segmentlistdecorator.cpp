#include "segmentlistdecorator.h"
#include "knossos-global.h"

SegmentListDecorator::SegmentListDecorator(QObject *parent) :
    QObject(parent)
{
}

segmentListElement *SegmentListDecorator::new_segmentListElement() {
    return new segmentListElement();
}

segmentListElement *SegmentListDecorator::new_segmentListElement(int sourceID, int targetID) {
    return new segmentListElement(sourceID, targetID);
}

segmentListElement *SegmentListDecorator::new_segmentListElement(nodeListElement *source, nodeListElement *target) {
    return new segmentListElement(source, target);
}
