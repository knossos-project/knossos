#ifndef SEGMENTATIONPROXY_H
#define SEGMENTATIONPROXY_H

#include <QObject>
#include <QList>

class SegmentationProxy : public QObject
{
    Q_OBJECT
public:
    explicit SegmentationProxy(QObject *parent = 0);

public slots:
    void clickSubObj(quint64 subObjId, QList<int> coord);
};

#endif // SEGMENTATIONPROXY_H
