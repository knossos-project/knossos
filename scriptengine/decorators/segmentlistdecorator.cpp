#include "segmentlistdecorator.h"

SegmentListDecorator::SegmentListDecorator(QObject *parent) :
    QObject(parent)
{
}

segmentListElement *SegmentListDecorator::new_Segment() {
    return new segmentListElement();
}
