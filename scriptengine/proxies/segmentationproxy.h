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
    void subobjectFromId(quint64 subObjId, QList<int> coord);
    quint64 largestObjectContainingSubobject(quint64 subObjId, QList<int> coord);
    void changeComment(quint64 objIndex, QString comment);
    void removeObject(quint64 objIndex);
    void setRenderAllObjs(bool b);
    bool isRenderAllObjs();
    QList<quint64> objectIndices();
    QList<quint64> subobjectIdsOfObject(quint64 objIndex);
};

#endif // SEGMENTATIONPROXY_H
