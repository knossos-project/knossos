#ifndef PYTHONPROXY_H
#define PYTHONPROXY_H

#include "skeleton/node.h"

#include <QObject>
#include <QList>
#include <QVector>

/** Actually this class it not really needed. It only hides the SIGNALS from the PythonProxy */
class PythonProxySignalDelegate : public QObject  {
    Q_OBJECT
signals:
    void userMoveSignal(const floatCoordinate & floatStep, UserMoveType userMoveType, const floatCoordinate & viewportNormal = {0, 0, 0});
};

extern PythonProxySignalDelegate *pythonProxySignalDelegate;

struct _object;
using PyObject = _object;

class PythonProxy : public QObject {
    Q_OBJECT
public:
    explicit PythonProxy(QObject *parent = 0);

signals:
    void echo(QString message);

public slots:
    void annotationLoad(const QString & filename, const bool merge = false);
    void annotationSave(const QString & filename);

    QString getKnossosVersion();
    QString getKnossosRevision();
    int getCubeEdgeLength();
    QList<int> getOcPixel(QList<int> Dc, QList<int> pxInDc);
    QList<int> getPosition();
    QList<float> getScale();
    void setPosition(QList<int> coord);
    quint64 readOverlayVoxel(QList<int> coord);
    bool writeOverlayVoxel(QList<int> coord, quint64 val);
    char *addrDcOc2Pointer(QList<int> coord, bool isOc);
    PyObject *PyBufferAddrDcOc2Pointer(QList<int> coord, bool isOc);
    QByteArray readDc2Pointer(QList<int> coord);
    int readDc2PointerPos(QList<int> coord, int pos);
    bool writeDc2Pointer(QList<int> coord, char *bytes);
    bool writeDc2PointerPos(QList<int> coord, int pos, int val);
    QByteArray readOc2Pointer(QList<int> coord);
    quint64 readOc2PointerPos(QList<int> coord, int pos);
    bool writeOc2Pointer(QList<int> coord, char *bytes);
    bool writeOc2PointerPos(QList<int> coord, int pos, quint64 val);
    QVector<int> processRegionByStridedBufProxy(QList<int> globalFirst, QList<int> size, quint64 dataPtr,
                                        QList<int> strides, bool isWrite, bool isMarkedChanged);
    void coordCubesMarkChangedProxy(QVector<int> cubeChangeSetList);
    void setMovementArea(QList<int> minCoord, QList<int> maxCoord);
    void resetMovementArea();
    QList<int> getMovementArea();
    float getMovementAreaFactor();
    void oc_reslice_notify_all(QList<int> coord);
    int loaderLoadingNr();
    bool loadStyleSheet(const QString &path);
    void setDatasetLocking(const bool locked);
};

#endif // PYTHONPROXY_H
