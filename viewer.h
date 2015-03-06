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
#include "widgets/navigationwidget.h"
#include "widgets/viewport.h"

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QCursor>
#include <QTimer>
#include <QElapsedTimer>
#include <QLineEdit>

#define SLOW 1000
#define FAST 10
#define RGB_LUTSIZE 768

//	For the viewer.
#define	SLICE_XY 0
#define SLICE_XZ 1
#define	SLICE_YZ 2

// MAG is a bit unintiutive here: a lower MAG means in KNOSSOS that a
// a pixel of the lower MAG dataset has a higher resolution, i.e. 10 nm
// pixel size instead of 20 nm
#define MAG_DOWN 1
#define MAG_UP 2

#define ON_CLICK_DRAG    0
#define ON_CLICK_RECENTER 1

#define MAX_COLORVAL 255.

struct viewerState {
    vpConfig *vpConfigs;
    char *texData;
    char *overlayData;
    char *defaultTexData;
    char *defaultOverlayData;
    uint splash;
    bool viewerReady;
    GLuint splashTexture;

    int highlightVp;
    int vpKeyDirection[3];

    //Min distance to currently centered data cube for rendering of spatial skeleton structure.
    //Unit: data cubes.
    int zoomCube;

    // don't jump between mags on zooming
    bool datasetMagLock;

    float depthCutOff;

    // Current position of the user crosshair.
    //   Given in pixel coordinates of the current local dataset (whatever magnification
    //   is currently loaded.)
    Coordinate currentPosition;

    uint recenteringTime;
    uint recenteringTimeOrth;
    bool walkOrth;

    //SDL_Surface *screen;

    //Keyboard repeat rate
    uint stepsPerSec;
    GLint filterType;
    int multisamplingOnOff;
    int lightOnOff;

    // Draw the colored lines that highlight the orthogonal VP intersections with each other.
    bool drawVPCrosshairs;
    // flag to indicate if user has pulled/is pulling a selection square in a viewport, which should be displayed
    int nodeSelectSquareVpId;
    std::pair<Coordinate, Coordinate> nodeSelectionSquare;

    //Show height/width-labels inside VPs
    bool showVPLabels;

    bool selectModeFlag;

    uint dropFrames;
    uint walkFrames;

    float voxelDimX;
    float voxelDimY;
    float voxelXYRatio;
    float voxelDimZ;
    //YZ can't be different to XZ because of the intrinsic properties of the SBF-SEM.
    float voxelXYtoZRatio;

    // allowed are: ON_CLICK_RECENTER 1, ON_CLICK_DRAG 0
    uint clickReaction;

    int luminanceBias;
    int luminanceRangeDelta;

    GLuint datasetColortable[3][256];
    GLuint datasetAdjustmentTable[3][256];
    bool datasetColortableOn;
    bool datasetAdjustmentOn;
    GLuint neutralDatasetTable[3][256];

    bool treeLutSet;
    bool treeColortableOn;
    float treeColortable[RGB_LUTSIZE];
    float treeAdjustmentTable[RGB_LUTSIZE];
    float defaultTreeTable[RGB_LUTSIZE];

    // Advanced Tracing Modes Stuff
    navigationMode autoTracingMode;
    int autoTracingDelay;
    int autoTracingSteps;

    float cumDistRenderThres;

    bool defaultVPSizeAndPos;
    uint renderInterval;

    QString lockComment;
};

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
    QElapsedTimer baseTime;
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
    bool dc_xy_changed = true;
    bool dc_xz_changed = true;
    bool dc_zy_changed = true;
    bool oc_xy_changed = true;
    bool oc_xz_changed = true;
    bool oc_zy_changed = true;

signals:
    void loadSignal();
    void coordinateChangedSignal(int x, int y, int z);
    void updateDatasetOptionsWidgetSignal();
protected:
    bool resetViewPortData(vpConfig *viewport);

    bool vpGenerateTexture_arb(vpConfig &currentVp);

    bool dcSliceExtract(char *datacube, Coordinate cubePosInAbsPx, char *slice, size_t dcOffset, vpConfig *vpConfig, bool useCustomLUT);
    bool dcSliceExtract_arb(char *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t, bool useCustomLUT);

    void ocSliceExtract(char *datacube, Coordinate cubePosInAbsPx, char *slice, size_t dcOffset, vpConfig *vpConfig);

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
    void sendLoadSignal();
    bool loadTreeColorTable(QString path, float *table, int type);
    static bool loadDatasetColorTable(QString path, GLuint *table, int type);
    bool vpGenerateTexture(vpConfig &currentVp);
    void setRotation(float x, float y, float z, float angle);
    void resetRotation();
    void setVPOrientation(bool arbitrary);
    void dc_reslice_notify();
    void oc_reslice_notify();
protected:
    bool calcLeftUpperTexAbsPx();
    bool initViewer();
};

#endif // VIEWER_H
