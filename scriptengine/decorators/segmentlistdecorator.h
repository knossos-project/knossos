#ifndef SEGMENTLISTDECORATOR_H
#define SEGMENTLISTDECORATOR_H

#include <QObject>

class SegmentListElement;
class NodeListElement;
class SegmentListDecorator : public QObject
{
    Q_OBJECT
public:
    explicit SegmentListDecorator(QObject *parent = 0);
    
signals:
    
public slots:
    /*
    SegmentListElement *new_SegmentListElement();
    SegmentListElement *new_SegmentListElement(int sourceID, int targetID);
    SegmentListElement *new_SegmentListElement(NodeListElement *source, NodeListElement *target);
    */

    NodeListElement *source(SegmentListElement *self);
    NodeListElement *target(SegmentListElement *self);
    int source_id(SegmentListElement *self);
    int target_id(SegmentListElement *self);
    QString static_SegmentListElement_help();

    /*
    void set_source(SegmentListElement *self, NodeListElement *source);
    void set_target(SegmentListElement *self, NodeListElement *target);
    void set_source_id(SegmentListElement *self, int sourceID);
    void set_target_id(SegmentListElement *self, int targetID);
    */
};

#endif // SEGMENTLISTDECORATOR_H
