/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef PYTHONPROXY_H
#define PYTHONPROXY_H

#include "skeleton/node.h"

#include <QObject>
#include <QList>
#include <QVector>

struct _object;
using PyObject = _object;

class PythonProxy : public QObject {
    Q_OBJECT
signals:
    void echo(QString message);
    void viewport_snapshot_vp_size(SnapshotOptions & options);
    void viewport_snapshot_dataset_size(SnapshotOptions & options);
    void viewport_snapshot(const SnapshotOptions & options);

public slots:
    ViewportType get_viewport_type(int i) {
        return static_cast<ViewportType>(i);
    }
    SnapshotOptions make_snapshot_options(const QString & path, const ViewportType vpType, int size = 2048, const bool withAxes = true, const bool withBox = true, const bool withOverlay = true, const bool withSkeleton = true, const bool withScale = true, const bool withVpPlanes = true) {
        return {path, vpType, size, withAxes, withBox, withOverlay, withSkeleton, withScale, withVpPlanes};
    }

    void annotationLoad(const QString & filename, const bool merge = false);
    void annotationSave(const QString & filename);
    int annotation_time();
    void set_annotation_time(int ms);
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
    bool loaderFinished();
    bool loadStyleSheet(const QString &path);
    void setMagnificationLock(const bool locked);

    void set_layer_visibility(int layer, bool visibilty);
};

#endif // PYTHONPROXY_H
