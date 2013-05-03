#ifndef LOADER_H
#define LOADER_H

/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#include <QObject>
#include <QThread>
#include "knossos-global.h"

class Loader : public QThread
{
    Q_OBJECT
public:
    explicit Loader(QObject *parent = 0);
    void run();

    Hashtable *Dcoi;

    CubeSlotList *freeDcSlots;
    CubeSlotList *freeOcSlots;
    Byte *DcSetChunk;
    Byte *OcSetChunk;
    Byte *bogusDc;
    Byte *bogusOc;
signals:
    void finished();
public slots:
    void load();
protected:
    bool initialized;
    bool addCubicDcSet(int32_t xBase, int32_t yBase, int32_t zBase, int32_t edgeLen, Hashtable *target);
    bool DcoiFromPos(Hashtable *Dcoi);
    CubeSlot *slotListGetElement(CubeSlotList *slotList);
    bool loadCube(Coordinate coordinate, Byte *freeDcSlot, Byte *freeOcSlot);
    int32_t slotListDelElement(CubeSlotList *slotList, CubeSlot *element);
    bool slotListDel(CubeSlotList *delList);
    int32_t slotListAddElement(CubeSlotList *slotList, Byte *datacube);
    CubeSlotList *slotListNew();
    bool initLoader();
    bool removeLoadedCubes(int32_t magChange);
    bool loadCubes();
};

#endif // LOADER_H
