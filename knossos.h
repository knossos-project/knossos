#ifndef KNOSSOS_H
#define KNOSSOS_H
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
    Q_OBJECT
public:
    explicit Knossos(QObject *parent = 0);
    bool stripNewlines(char *string);
    static bool readConfigFile(const char *path);
    static bool printConfigValues();
    static bool loadNeutralDatasetLUT(GLuint *datasetLut);
    static bool readDataConfAndLocalConf();
    static struct stateInfo *emptyState();
    static bool findAndRegisterAvailableDatasets();
    static bool cleanUpMain();
    static bool tempConfigDefaults();
    static bool configFromCli(int argCount, char *arguments[]);
    static void catchSegfault(int signum);
    static int32_t initStates();

    static bool unlockSkeleton(int32_t increment);
    static bool lockSkeleton(uint32_t targetRevision);
    static bool sendRemoteSignal();
    static bool sendClientSignal();
    static bool sendQuitSignal();
    static bool sendServerSignal();
    static uint32_t log2uint32(register uint32_t x);
    static uint32_t ones32(register uint32_t x);
    static void loadTreeLUTFallback();



signals:
    void calcDisplayedEdgeLengthSignal();



};


/*
2. Funktion zum durch-switchen von nodes ohne comment, auch branchpoints ohne comment sollen dabei gefunden werden (fuer offizielle Version nicht noetig)
1. Menu schlieﬂen bei klick irgendwo rein
2. cursor wechsel resize
3. allg. cursor Fadenkreuz

*/
#endif
