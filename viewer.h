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

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QCursor>
#include <QTimer>
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
class Viewport;
class MainWindow;
class Viewer : public QThread
{
    Q_OBJECT

public:
    explicit Viewer(QObject *parent = 0);
    Skeletonizer *skeletonizer;
    Renderer *renderer;
    MainWindow *window;

    floatCoordinate v1, v2, v3;
    Viewport *vpXY, *vpXZ, *vpYZ, *vpSkel;
    vpList *viewports;
    QTimer *timer;    
    int frames;

    bool updateZoomCube();
    static int findVPnumByWindowCoordinate(uint xScreen, uint yScreen);


    bool initialized;
    bool moveVPonTop(uint currentVP);
    static bool getDirectionalVectors(float alpha, float beta, floatCoordinate *v1, floatCoordinate *v2, floatCoordinate *v3);

signals:
    void loadSignal();
    void updateCoordinatesSignal(int x, int y, int z);
    void updateZoomAndMultiresWidgetSignal();
    void idleTimeSignal();
    bool broadcastPosition(uint x, uint y, uint z);
protected:
    bool resetViewPortData(vpConfig *viewport);
    int backlogAddElement(vpBacklog *backlog, Coordinate datacube, uint dcOffset, Byte *slice, uint x_px, uint y_px, uint cubeType);
    bool vpListDel(vpList *list);
    int vpListDelElement( vpList *list,  vpListElement *element);
    bool backlogDel(vpBacklog *backlog);
    vpBacklogElement *isCubeInBacklog(struct vpBacklog *backlog, Coordinate *cube);
    vpBacklogElement *backlogAddElement_arb(vpBacklog *backlog, Coordinate dataCube, uint cubeType);
    pxStripeList *stripesNew();
    int backlogDelElement(vpBacklog *backlog, vpBacklogElement *element);
    vpList *vpListGenerate(viewerState *viewerState);
    vpBacklog *backlogNew();
    int vpListAddElement(vpList *vpList, vpConfig *vpConfig, vpBacklog *backlog);
    vpList* vpListNew();

    bool pxStripeListDelElement(struct pxStripeList *stripes, struct pxStripe *stripe);
    bool pxStripeListDel(struct pxStripeList *stripes);
    bool addPxStripe(vpBacklogElement *backlogElement, floatCoordinate *currentPxInDc_float, uint s, uint t1, uint t2);

    bool vpGenerateTexture(vpListElement *currentVp, viewerState *viewerState);
    bool vpGenerateTexture_arb(struct vpListElement *currentVp, struct viewerState *viewerState);

    bool vpHandleBacklog(vpListElement *currentVp, viewerState *viewerState);
    bool vpHandleBacklog_arb(struct vpListElement *currentVp, struct viewerState *viewerState);

    bool sliceExtract_standard(Byte *datacube, Byte *slice, vpConfig *vpConfig);
    bool sliceExtract_standard_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t);
    bool sliceExtract_standard_Backlog_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *startPxInDc_float, int s, int t1, int t2);

    bool sliceExtract_adjust(Byte *datacube, Byte *slice, vpConfig *vpConfig);
    bool sliceExtract_adjust_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t);
    bool sliceExtract_adjust_Backlog_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *startPxInDc_float,  int s, int t1, int t2);

    bool dcSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig * vpConfig);
    bool dcSliceExtract_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t);
    bool dcSliceExtract_Backlog_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *startPxInDc_float, int s, int t1, int t2);

    bool ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig);
    void rewire();
public slots:
    bool changeDatasetMag(uint upOrDownFlag);
    bool userMove(int x, int y, int z, int serverMovement); /* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
    bool userMove_arb(float x, float y, float z, int serverMovement);
    static bool updatePosition(int serverMovement);
    bool recalcTextureOffsets();
    bool calcDisplayedEdgeLength();    
    bool updateViewerState();    
    void run();
    void runrun();
    bool sendLoadSignal(uint x, uint y, uint z, int magChanged);
    bool loadTreeColorTable(QString path, float *table, int type);
    static bool loadDatasetColorTable(QString path, GLuint *table, int type);
protected:
    bool calcLeftUpperTexAbsPx();
    bool initViewer();
    bool processUserMove();
    QTime delay;

};

#endif // VIEWER_H
