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
    segmentListElement *new_segmentListElement();
    segmentListElement *new_segmentListElement(int sourceID, int targetID);
    segmentListElement *new_segmentListElement(nodeListElement *source, nodeListElement *target);
};

#endif // SEGMENTLISTDECORATOR_H
