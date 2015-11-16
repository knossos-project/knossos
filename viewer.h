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
#include "mesh.h"
#include "slicer/gpucuber.h"
#include "widgets/mainwindow.h"
#include "widgets/navigationwidget.h"
#include "widgets/viewport.h"

#include <QObject>
#include <QDebug>
#include <QCursor>
#include <QTimer>
#include <QElapsedTimer>
#include <QLineEdit>

#include <atomic>

#define SLOW 1000
#define FAST 10

// MAG is a bit unintiutive here: a lower MAG means in KNOSSOS that a
// a pixel of the lower MAG dataset has a higher resolution, i.e. 10 nm
// pixel size instead of 20 nm
#define MAG_DOWN 1
#define MAG_UP 2

#define ON_CLICK_DRAG    0
#define ON_CLICK_RECENTER 1

#define MAX_COLORVAL 255.

enum class SkeletonDisplay {
    OnlySelected = 0x1,
    ShowIn3DVP = 0x2,
    ShowInOrthoVPs = 0x4
};

enum class IdDisplay {
    None = 0x0,
    ActiveNode = 0x1,
    AllNodes = 0x2 | ActiveNode
};

struct ViewerState {
    ViewerState() {
        state->viewerState = this;
    }
    char *texData;
    char *overlayData;
    char *defaultTexData;
    char *defaultOverlayData;

    int highlightVp{VIEWPORT_UNDEFINED};
    int vpKeyDirection[3]{1,1,1};

    // don't jump between mags on zooming
    bool datasetMagLock;
    // Current position of the user crosshair.
    //   Given in pixel coordinates of the current local dataset (whatever magnification
    //   is currently loaded.)
    Coordinate currentPosition;

    uint recenteringTime{250};
    uint recenteringTimeOrth{250};
    bool walkOrth{false};

    //Keyboard repeat rate
    uint stepsPerSec{40};
    int multisamplingOnOff;
    int lightOnOff;

    // Draw the colored lines that highlight the orthogonal VP intersections with each other.
    // flag to indicate if user has pulled/is pulling a selection square in a viewport, which should be displayed
    std::pair<int, Qt::KeyboardModifiers> nodeSelectSquareData{-1, Qt::NoModifier};
    std::pair<Coordinate, Coordinate> nodeSelectionSquare;

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

    // dataset & segmentation rendering options
    int luminanceRangeDelta{255};
    int luminanceBias{0};
    bool showOverlay{true};
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> datasetColortable;//user LUT
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> datasetAdjustmentTable;//final LUT used during slicing
    bool datasetColortableOn{false};
    bool datasetAdjustmentOn{false};
    // skeleton rendering options
    float depthCutOff{5.f};
    QFlags<SkeletonDisplay> skeletonDisplay{SkeletonDisplay::ShowIn3DVP, SkeletonDisplay::ShowInOrthoVPs};
    QFlags<IdDisplay> idDisplay{IdDisplay::None};
    int highlightActiveTree{true};
    float segRadiusToNodeRadius{.5f};
    int overrideNodeRadiusBool{false};
    float overrideNodeRadiusVal{1.f};
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> nodeColors;
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> treeColors;
    QString highlightedNodePropertyByRadius{""};
    double nodePropertyRadiusScale{1};
    QString highlightedNodePropertyByColor{""};
    double nodePropertyColorMapMin{0};
    double nodePropertyColorMapMax{0};
    // viewport rendering options
    bool drawVPCrosshairs{true};
    int rotateAroundActiveNode;
    int showIntersections{false};
    bool showScalebar{false};
    int showXYplane{true};
    int showXZplane{true};
    int showYZplane{true};
    // temporary vertex buffers that are available for rendering, get cleared
    // every frame */
    mesh lineVertBuffer; /* ONLY for lines */
    mesh pointVertBuffer; /* ONLY for points */
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
class ViewportBase;
class Viewer : public QObject {
    Q_OBJECT
private:
    QElapsedTimer baseTime;
    float alphaCache;
    Rotation rotation;
    floatCoordinate moveCache; //Cache for Movements smaller than pixel coordinate

    ViewerState viewerState;
public:
    explicit Viewer();
    Skeletonizer *skeletonizer;
    MainWindow mainWindow;
    MainWindow *window = &mainWindow;

    std::list<TextureLayer> layers;
    int gpucubeedge = 64;
    bool gpuRendering = false;

    floatCoordinate v1, v2, v3;
    ViewportOrtho *vpUpperLeft, *vpLowerLeft, *vpUpperRight;
    QTimer timer;

    std::atomic_bool dc_xy_changed{true};
    std::atomic_bool dc_xz_changed{true};
    std::atomic_bool dc_zy_changed{true};
    std::atomic_bool oc_xy_changed{true};
    std::atomic_bool oc_xz_changed{true};
    std::atomic_bool oc_zy_changed{true};
    void loadNodeLUT(const QString & path);
    void loadTreeLUT(const QString & path = ":/resources/color_palette/default.json");
    color4F getNodeColor(const nodeListElement & node) const;
signals:
    void coordinateChangedSignal(const Coordinate & pos);
    void updateDatasetOptionsWidgetSignal();
    void movementAreaFactorChangedSignal();
protected:
    void vpGenerateTexture_arb(ViewportOrtho & vp);

    bool dcSliceExtract(char *datacube, Coordinate cubePosInAbsPx, char *slice, size_t dcOffset, ViewportOrtho & vp, bool useCustomLUT);
    bool dcSliceExtract_arb(char *datacube, ViewportOrtho & vp, floatCoordinate *currentPxInDc_float, int s, int *t, bool useCustomLUT);

    void ocSliceExtract(char *datacube, Coordinate cubePosInAbsPx, char *slice, size_t dcOffset, ViewportOrtho & vp);

    void rewire();
public slots:
    void updateCurrentPosition();
    bool changeDatasetMag(uint upOrDownFlag); /* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
    void userMove(const Coordinate &step, UserMoveType userMoveType, const Coordinate & viewportNormal = {0, 0, 0});
    void userMove_arb(const floatCoordinate & floatStep, UserMoveType userMoveType, const Coordinate & viewportNormal = {0, 0, 0});
    bool recalcTextureOffsets();
    bool calcDisplayedEdgeLength();
    void applyTextureFilterSetting(const GLint texFiltering);
    void run();
    void loader_notify();
    void defaultDatasetLUT();
    void loadDatasetLUT(const QString & path);
    void datasetColorAdjustmentsChanged();
    bool vpGenerateTexture(ViewportOrtho & vp);
    void setRotation(const floatCoordinate &axis, const float angle);
    void resetRotation();
    void setVPOrientation(const bool arbitrary);
    void calculateMissingGPUCubes(TextureLayer & layer);
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
