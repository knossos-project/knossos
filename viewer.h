#ifndef VIEWER_H
#define VIEWER_H

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

#include "functions.h"
#include <QObject>
#include <QThread>
#include <QDebug>
#include <QCursor>
#include <QTimer>
#include <QElapsedTimer>
#include <QLineEdit>
#include "knossos-global.h"

/**
 *
 *  This file contains functions that are called by the managing,
 *  all openGL rendering operations and
 *  all skeletonization operations commanded directly by the user over the GUI. The files gui.c, renderer.c and
 *  skeletonizer.c contain functions mainly used by the corresponding "subsystems". viewer.c contains the main
 *  event loop and routines that handle (extract slices, pack into openGL textures,...) the data coming
 *  from the loader thread.
 */
class Skeletonizer;
class Renderer;
class EventModel;
class Viewport;
class MainWindow;
class Viewer : public QThread {
    Q_OBJECT
private:
    float alphaCache;
    Rotation rotation;
    floatCoordinate moveCache; //Cache for Movements smaller than pixel coordinate

public:
    explicit Viewer(QObject *parent = 0);
    Skeletonizer *skeletonizer;
    EventModel *eventModel;
    Renderer *renderer;
    MainWindow *window;

    floatCoordinate v1, v2, v3;
    Viewport *vpUpperLeft, *vpLowerLeft, *vpUpperRight, *vpLowerRight;
    QTimer *timer;
    int frames;

    bool updateZoomCube();

    bool initialized;
    bool moveVPonTop(uint currentVP);
    static bool getDirectionalVectors(float alpha, float beta, floatCoordinate *v1, floatCoordinate *v2, floatCoordinate *v3);

signals:
    void loadSignal();
    void coordinateChangedSignal(int x, int y, int z);
    void updateDatasetOptionsWidgetSignal();
protected:
    bool resetViewPortData(vpConfig *viewport);

    bool vpGenerateTexture_arb(vpConfig &currentVp);

    bool sliceExtract_standard(Byte *datacube, Byte *slice, vpConfig *vpConfig);
    bool sliceExtract_standard_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t);

    bool sliceExtract_adjust(Byte *datacube, Byte *slice, vpConfig *vpConfig);
    bool sliceExtract_adjust_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t);

    bool dcSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig * vpConfig);
    bool dcSliceExtract_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t);

    void ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig);
    void ocSliceExtractUnique(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig);

    void rewire();
public slots:
    void updateCurrentPosition();
    bool changeDatasetMag(uint upOrDownFlag); /* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
    bool userMove(int x, int y, int z, UserMoveType userMoveType, ViewportType viewportType);
    bool userMove_arb(float x, float y, float z);
    bool recalcTextureOffsets();
    bool calcDisplayedEdgeLength();
    bool updateViewerState();
    void run();
    bool sendLoadSignal(int magChanged);
    bool loadTreeColorTable(QString path, float *table, int type);
    static bool loadDatasetColorTable(QString path, GLuint *table, int type);
    bool vpGenerateTexture(vpConfig &currentVp);
    void setRotation(float x, float y, float z, float angle);
    void resetRotation();
    void setVPOrientation(bool arbitrary);
protected:
    bool calcLeftUpperTexAbsPx();
    bool initViewer();
};

#endif // VIEWER_H
