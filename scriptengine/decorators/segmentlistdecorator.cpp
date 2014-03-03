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

nodeListElement *SegmentListDecorator::getSource(segmentListElement *self) {
    return self->getSource();
}

void SegmentListDecorator::setSource(segmentListElement *self, nodeListElement *source) {
    self->setSource(source);
}

nodeListElement *SegmentListDecorator::getTarget(segmentListElement *self) {
    return self->getTarget();
}

void SegmentListDecorator::setTarget(segmentListElement *self, nodeListElement *target) {
    self->setTarget(target);
}

void SegmentListDecorator::setSource(segmentListElement *self, int sourceID) {
    self->setSource(sourceID);
}

void SegmentListDecorator::setTarget(segmentListElement *self, int targetID) {
    self->setTarget(targetID);
}
