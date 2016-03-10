#ifndef SEGMENTATIONPROXY_H
#define SEGMENTATIONPROXY_H

#include <QList>
#include <QObject>

class SegmentationProxy : public QObject {
    Q_OBJECT
public slots:
    void subobjectFromId(const quint64 subObjId, const QList<int> & coord);
    quint64 largestObjectContainingSubobject(const quint64 subObjId, const QList<int> & coord);
    void changeComment(const quint64 objIndex, const QString & comment);
    void removeObject(const quint64 objIndex);
    void setRenderAllObjs(const bool b);
    bool isRenderAllObjs();
    QList<quint64> objectIds();
    QList<quint64> subobjectIdsOfObject(const quint64 objId);
    QList<quint64> getAllObjectIdx();
    void selectObject(const quint64 objIdx);
    void unselectObject(const quint64 objectIndex);
    void jumpToObject(const quint64 objIdx);
    QList<quint64> getSelectedObjectIndices();
    QList<int> getObjectLocation(const quint64 objectIndex);
};

#endif // SEGMENTATIONPROXY_H
