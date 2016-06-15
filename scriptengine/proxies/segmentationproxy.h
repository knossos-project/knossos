#ifndef SEGMENTATIONPROXY_H
#define SEGMENTATIONPROXY_H

#include <QList>
#include <QObject>

class SegmentationProxy : public QObject {
    Q_OBJECT
public slots:
    void subobjectFromId(const quint64 subObjId, const QList<int> & coord);
    void setRenderOnlySelectedObjs(const bool b);
    bool isRenderOnlySelecdedObjs();

    quint64 largestObjectContainingSubobject(const quint64 subObjId, const QList<int> & coord);
    QList<quint64> subobjectIdsOfObject(const quint64 objId);

    QList<quint64> objects();
    QList<quint64> selectedObjects();

    void changeComment(const quint64 objId, const QString & comment);
    void changeColor(const quint64 objId, const QColor & color);
    void removeObject(const quint64 objId);
    void selectObject(const quint64 objId);
    void unselectObject(const quint64 objId);
    void jumpToObject(const quint64 objId);
    QList<int> objectLocation(const quint64 objId);
};

#endif // SEGMENTATIONPROXY_H
