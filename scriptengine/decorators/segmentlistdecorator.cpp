#include "segmentlistdecorator.h"
#include "knossos-global.h"

SegmentListDecorator::SegmentListDecorator(QObject *parent) :
    QObject(parent)
{
}

/*
segmentListElement *SegmentListDecorator::new_segmentListElement() {
    return new segmentListElement();
}

segmentListElement *SegmentListDecorator::new_segmentListElement(int sourceID, int targetID) {
    return new segmentListElement(sourceID, targetID);
}

segmentListElement *SegmentListDecorator::new_segmentListElement(nodeListElement *source, nodeListElement *target) {
    return new segmentListElement(source, target);
}
*/


nodeListElement *SegmentListDecorator::source(segmentListElement *self) {
    return self->source;
}

nodeListElement *SegmentListDecorator::target(segmentListElement *self) {
    return self->target;
}

int SegmentListDecorator::source_id(segmentListElement *self) {
    if(self->source)
        return self->source->nodeID;

    return 0;
}

int SegmentListDecorator::target_id(segmentListElement *self) {
    if(self->target)
        return self->target->nodeID;

    return 0;
}

QString SegmentListDecorator::static_segmentListElement_help() {
    return QString("");
}


/*
void SegmentListDecorator::set_source(segmentListElement *self, nodeListElement *source) {
    self->setSource(source);
}

void SegmentListDecorator::set_target(segmentListElement *self, nodeListElement *target) {
    self->setTarget(target);
}

void SegmentListDecorator::set_source_id(segmentListElement *self, int sourceID) {
    self->setSource(sourceID);
}

void SegmentListDecorator::set_target_id(segmentListElement *self, int targetID) {
    self->setTarget(targetID);
}*/

