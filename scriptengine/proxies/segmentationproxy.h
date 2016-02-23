#ifndef SEGMENTATIONPROXY_H
#define SEGMENTATIONPROXY_H

#include <QList>
#include <QObject>

class SegmentationProxy : public QObject {
    Q_OBJECT
public slots:
    void subobjectFromId(quint64 subObjId, QList<int> coord);
    quint64 largestObjectContainingSubobject(quint64 subObjId, QList<int> coord);
    void changeComment(quint64 objIndex, QString comment);
    void removeObject(quint64 objIndex);
    void setRenderAllObjs(bool b);
    bool isRenderAllObjs();
    QList<quint64> objectIds();
    QList<quint64> subobjectIdsOfObject(quint64 objId);
    void jumpToObject(quint64 objIdx);
    QList<quint64> getAllObjectIdx();
    void selectObject(quint64 objIdx);
    void unselectObject(const quint64 objectIndex);
    QList<quint64> getSelectedObjectIndices();
    QList<quint64> * getObjectLocation(const quint64 objectIndex);
};

#endif // SEGMENTATIONPROXY_H
