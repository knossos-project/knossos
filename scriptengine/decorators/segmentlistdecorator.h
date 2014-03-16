#ifndef SEGMENTLISTDECORATOR_H
#define SEGMENTLISTDECORATOR_H

#include <QObject>

class segmentListElement;
class nodeListElement;
class SegmentListDecorator : public QObject
{
    Q_OBJECT
public:
    explicit SegmentListDecorator(QObject *parent = 0);
    
signals:
    
public slots:
    /*
    segmentListElement *new_segmentListElement();
    segmentListElement *new_segmentListElement(int sourceID, int targetID);
    segmentListElement *new_segmentListElement(nodeListElement *source, nodeListElement *target);
    */

    nodeListElement *source(segmentListElement *self);
    nodeListElement *target(segmentListElement *self);
    int source_id(segmentListElement *self);
    int target_id(segmentListElement *self);
    QString static_segmentListElement_help();

    /*
    void set_source(segmentListElement *self, nodeListElement *source);
    void set_target(segmentListElement *self, nodeListElement *target);
    void set_source_id(segmentListElement *self, int sourceID);
    void set_target_id(segmentListElement *self, int targetID);
    */
};

#endif // SEGMENTLISTDECORATOR_H
