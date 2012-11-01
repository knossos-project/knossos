/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2012
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

class Knossos : public QObject {
public:
    bool unlockSkeleton(int32_t increment);
    bool lockSkeleton(int32_t targetRevision);
    bool sendLoadSignal(uint32_t x, uint32_t y, uint32_t z);
    bool sendRemoteSignal();
    bool sendClientSignal();
    bool sendQuitSignal();
    bool sendServerSignal();
    void sendDatasetChangeSignal(uint32_t upOrDownFlag);
    uint32_t log2uint32(register uint32_t x);
    uint32_t ones32(register uint32_t x);

};


/*
2. Funktion zum durch-switchen von nodes ohne comment, auch branchpoints ohne comment sollen dabei gefunden werden (fuer offizielle Version nicht noetig)
1. Menu schlieﬂen bei klick irgendwo rein
2. cursor wechsel resize
3. allg. cursor Fadenkreuz

*/
