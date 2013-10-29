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

#include <qobject.h>
#include <QtOpenGL>

#include "widgets/mainwindow.h"

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
    static bool configDefaults();
    static bool configFromCli(int argCount, char *arguments[]);   
    bool initStates();
    static void revisionCheck();

    static bool unlockSkeleton(int increment);
    static bool lockSkeleton(uint targetRevision);
    static bool sendRemoteSignal();
    static bool sendClientSignal();
    static bool sendQuitSignal();
    static bool sendServerSignal();
    static uint log2uint32(register uint x);
    static uint ones32(register uint x);

    void loadDefaultTreeLUT();


signals:
    void calcDisplayedEdgeLengthSignal();
    void treeColorAdjustmentChangedSignal();
    bool loadTreeColorTableSignal(QString path, float *table, int type);
public slots:
    void loadTreeLUTFallback();

};

#endif
