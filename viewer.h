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
#include "widgets/mainwindow.h"
#include "widgets/navigationwidget.h"
#include "widgets/viewport.h"

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QCursor>
#include <QTimer>
#include <QElapsedTimer>
#include <QLineEdit>

#include <atomic>

#define SLOW 1000
#define FAST 10
#define RGB_LUTSIZE 768

// MAG is a bit unintiutive here: a lower MAG means in KNOSSOS that a
// a pixel of the lower MAG dataset has a higher resolution, i.e. 10 nm
// pixel size instead of 20 nm
#define MAG_DOWN 1
#define MAG_UP 2

#define ON_CLICK_DRAG    0
#define ON_CLICK_RECENTER 1

#define MAX_COLORVAL 255.

struct ViewerState {
    ViewerState() {
        state->viewerState = this;
    }
    std::vector<vpConfig> vpConfigs;
    char *texData;
    char *overlayData;
    char *defaultTexData;
    char *defaultOverlayData;
    bool viewerReady{false};

    int highlightVp{VIEWPORT_UNDEFINED};
    int vpKeyDirection[3]{1,1,1};

    // don't jump between mags on zooming
    bool datasetMagLock;

    float depthCutOff{5.f};

    // Current position of the user crosshair.
    //   Given in pixel coordinates of the current local dataset (whatever magnification
    //   is currently loaded.)
    Coordinate currentPosition;

    uint recenteringTime;
    uint recenteringTimeOrth{500};
    bool walkOrth{false};

    //SDL_Surface *screen;

    //Keyboard repeat rate
    uint stepsPerSec{40};
    int multisamplingOnOff;
    int lightOnOff;

    // Draw the colored lines that highlight the orthogonal VP intersections with each other.
    bool drawVPCrosshairs{true};
    // flag to indicate if user has pulled/is pulling a selection square in a viewport, which should be displayed
    int nodeSelectSquareVpId{-1};
    std::pair<Coordinate, Coordinate> nodeSelectionSquare;

    bool showScalebar{false};

    bool selectModeFlag{false};

    uint dropFrames{1};
    uint walkFrames{10};

    float voxelDimX;
    float voxelDimY;
    float voxelXYRatio;
    float voxelDimZ;
    //YZ can't be different to XZ because of the intrinsic properties of the SBF-SEM.
    float voxelXYtoZRatio;

    // allowed are: ON_CLICK_RECENTER 1, ON_CLICK_DRAG 0
    uint clickReaction{ON_CLICK_DRAG};

    int luminanceBias{0};
    int luminanceRangeDelta{255};

    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> datasetColortable;//user LUT
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> datasetAdjustmentTable;//final LUT used during slicing
    bool datasetColortableOn{false};
    bool datasetAdjustmentOn{false};

    // Advanced Tracing Modes Stuff
    navigationMode autoTracingMode{navigationMode::recenter};
    int autoTracingDelay{50};
    int autoTracingSteps{10};

    float cumDistRenderThres{7.f};

    bool defaultVPSizeAndPos{true};
    //there’s a problem (intel drivers on linux only?)
    //rendering knossos at full speed and displaying a file dialog
    //so we reduce the rendering speed during display of said dialog
    uint renderInterval{FAST};

    //In pennmode right and left click are switched
    bool penmode = false;

    QString lockComment;

    float movementAreaFactor{80.f};
    bool showOverlay{true};
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
class Viewer : public QThread {
    Q_OBJECT
private:
    QElapsedTimer baseTime;
    float alphaCache;
    Rotation rotation;
    floatCoordinate moveCache; //Cache for Movements smaller than pixel coordinate

    ViewerState viewerState;
public:
    explicit Viewer(QObject *parent = 0);
    Skeletonizer *skeletonizer;
    EventModel *eventModel;
    Renderer *renderer;
    MainWindow mainWindow;
    MainWindow *window = &mainWindow;

    floatCoordinate v1, v2, v3;
    Viewport *vpUpperLeft, *vpLowerLeft, *vpUpperRight, *vpLowerRight;
    QTimer *timer;
    int frames;

    bool initialized;
    bool moveVPonTop(uint currentVP);
    static bool getDirectionalVectors(float alpha, float beta, floatCoordinate *v1, floatCoordinate *v2, floatCoordinate *v3);
    std::atomic_bool dc_xy_changed{true};
    std::atomic_bool dc_xz_changed{true};
    std::atomic_bool dc_zy_changed{true};
    std::atomic_bool oc_xy_changed{true};
    std::atomic_bool oc_xz_changed{true};
    std::atomic_bool oc_zy_changed{true};

signals:
    void loadSignal();
    void coordinateChangedSignal(int x, int y, int z);
    void updateDatasetOptionsWidgetSignal();
    void movementAreaFactorChangedSignal();
protected:
    void vpGenerateTexture_arb(vpConfig &currentVp);

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
    void applyTextureFilterSetting(const GLint texFiltering);
    void run();
    void loader_notify();
    void defaultDatasetLUT();
    void loadDatasetLUT(const QString & path);
    void datasetColorAdjustmentsChanged();
    bool vpGenerateTexture(vpConfig &currentVp);
    void setRotation(float x, float y, float z, float angle);
    void resetRotation();
    void setVPOrientation(const bool arbitrary);
    void dc_reslice_notify_visible();
    void dc_reslice_notify_all(const Coordinate coord);
    void oc_reslice_notify_visible();
    void oc_reslice_notify_all(const Coordinate coord);
    void setMovementAreaFactor(float alpha);
protected:
    bool calcLeftUpperTexAbsPx();
    void initViewer();
};

#endif // VIEWER_H
