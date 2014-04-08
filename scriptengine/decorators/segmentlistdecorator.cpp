#include "segmentlistdecorator.h"
#include "knossos-global.h"

SegmentListDecorator::SegmentListDecorator(QObject *parent) :
    QObject(parent)
{
}

/*
SegmentListElement *SegmentListDecorator::new_SegmentListElement() {
    return new SegmentListElement();
}

SegmentListElement *SegmentListDecorator::new_SegmentListElement(int sourceID, int targetID) {
    return new SegmentListElement(sourceID, targetID);
}

SegmentListElement *SegmentListDecorator::new_SegmentListElement(NodeListElement *source, NodeListElement *target) {
    return new SegmentListElement(source, target);
}
*/


NodeListElement *SegmentListDecorator::source(SegmentListElement *self) {
    return self->source;
}

NodeListElement *SegmentListDecorator::target(SegmentListElement *self) {
    return self->target;
}

int SegmentListDecorator::source_id(SegmentListElement *self) {
    if(self->source)
        return self->source->nodeID;

    return 0;
}

int SegmentListDecorator::target_id(SegmentListElement *self) {
    if(self->target)
        return self->target->nodeID;

    return 0;
}

QString SegmentListDecorator::static_SegmentListElement_help() {
    return QString("the read-only class representing a connection between tree nodes of the KNOSSOS skeleton. Access to attributes only via getter and setter." \
                   "source() : returns the source node of the segment" \
                   "source_id() : returns the source node id of the segment" \
                   "target() : returns the target node of the segment" \
                   "target_id() : returns the target node id of the segment");
}


/*
void SegmentListDecorator::set_source(SegmentListElement *self, NodeListElement *source) {
    self->setSource(source);
}

void SegmentListDecorator::set_target(SegmentListElement *self, NodeListElement *target) {
    self->setTarget(target);
}

void SegmentListDecorator::set_source_id(SegmentListElement *self, int sourceID) {
    self->setSource(sourceID);
}

void SegmentListDecorator::set_target_id(SegmentListElement *self, int targetID) {
    self->setTarget(targetID);
}*/

