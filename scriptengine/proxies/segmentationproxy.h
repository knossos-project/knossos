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

    void addSubobject(const quint64 objId, const quint64 subobjectId);
    void changeComment(const quint64 objId, const QString & comment);
    void changeColor(const quint64 objId, const QColor & color);
    void createObject(const quint64 objId, const quint64 initialSubobjectId, const QList<int> & location = {0, 0, 0}, const bool todo = false, const bool immutable = false);
    void removeObject(const quint64 objId);
    void selectObject(const quint64 objId);
    void unselectObject(const quint64 objId);
    void jumpToObject(const quint64 objId);
    QList<int> objectLocation(const quint64 objId);
};

#endif // SEGMENTATIONPROXY_H
