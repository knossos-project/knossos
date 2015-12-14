#include "segmentlistdecorator.h"

#include "skeleton/node.h"

SegmentListDecorator::SegmentListDecorator(QObject *parent) : QObject(parent) {}

nodeListElement * SegmentListDecorator::source(segmentListElement *self) {
    return &self->source;
}

nodeListElement * SegmentListDecorator::target(segmentListElement *self) {
    return &self->target;
}

int SegmentListDecorator::source_id(segmentListElement *self) {
    return self->source.nodeID;
}

int SegmentListDecorator::target_id(segmentListElement *self) {
    return self->target.nodeID;
}

QString SegmentListDecorator::static_Segment_help() {
    return QString("the read-only class representing a connection between tree nodes of the KNOSSOS skeleton. Access to attributes only via getter and setter." \
                   "source() : returns the source node of the segment" \
                   "source_id() : returns the source node id of the segment" \
                   "target() : returns the target node of the segment" \
                   "target_id() : returns the target node id of the segment");
}
