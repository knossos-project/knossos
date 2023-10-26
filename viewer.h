/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include "remote.h"
#include "slicer/gpucuber.h"
#include "usermove.h"
#include "widgets/preferences/navigationtab.h"
#include "widgets/mainwindow.h"
#include "widgets/viewports/renderoptions.h"
#include "widgets/viewports/viewportbase.h"

#include <QColor>
#include <QCursor>
#include <QDebug>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QObject>
#include <QQuaternion>
#include <QSemaphore>
#include <QTimer>

#include <vector>

enum TreeDisplay {
    OnlySelected = 0x1,
    ShowIn3DVP = 0x2,
    ShowInOrthoVPs = 0x4
};

enum class IdDisplay {
    None = 0x0,
    ActiveNode = 0x1,
    AllNodes = 0x2 | ActiveNode
};

enum class RotationCenter {
    DatasetCenter,
    ActiveNode,
    CurrentPosition
};

struct GLBuffers {
    bool regenVertBuffer{true};
    bool regenHaloBuffer{true};

    // vertex buffers that are available for rendering
    struct RenderBuffer {
        std::vector<floatCoordinate> vertices;
        std::vector<std::array<std::uint8_t, 4>> colors;
        QOpenGLBuffer vertex_buffer{QOpenGLBuffer::VertexBuffer};
        QOpenGLBuffer color_buffer{QOpenGLBuffer::VertexBuffer};

        void clear() {
            vertices.clear();
            colors.clear();
        }

        template<typename T, typename U>
        void emplace_back(T&& coord, U&& color) {
            vertices.emplace_back(std::forward<T>(coord));
            colors.emplace_back(std::forward<U>(color));
        }
    } lineVertBuffer, cylinderBuffer, pointVertBuffer, haloBuffer;
    QOpenGLBuffer radius_buffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer haloRadiusBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer segment_vector_buffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer cylinder_radius_buffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer cylinder_shift_buffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer cylinder_raised_buffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer color_picking_buffer{QOpenGLBuffer::VertexBuffer};
    std::vector<std::array<std::uint8_t, 4>> colorPickingBuffer24, colorPickingBuffer48, colorPickingBuffer64;
};

struct ViewerState {
    ViewerState();

    int texEdgeLength = 512;
    QOpenGLTexture::Filter textureFilter{QOpenGLTexture::Nearest};
    // don't jump between mags on zooming
    bool datasetMagLock;
    // Current position of the user crosshair.
    //   Given in pixel coordinates of the current local dataset (whatever magnification
    //   is currently loaded.)
    Coordinate currentPosition;

    int sampleBuffers{8};
    bool lightOnOff;

    // Draw the colored lines that highlight the orthogonal VP intersections with each other.
    // flag to indicate if user has pulled/is pulling a selection square in a viewport, which should be displayed
    std::pair<int, Qt::KeyboardModifiers> nodeSelectSquareData{-1, Qt::NoModifier};
    std::pair<Coordinate, Coordinate> nodeSelectionSquare;

    int dropFrames{1};
    int walkFrames{10};

    Coordinate tracingDirection;
    ViewportType highlightVp{VIEWPORT_UNDEFINED};

    bool keyRepeat{false};
    bool notFirstKeyRepeat{false};
    floatCoordinate repeatDirection;
    int movementSpeed{40};

    // Advanced Tracing Modes Stuff
    Recentering autoTracingMode{Recentering::OnNode};
    int autoTracingSteps{10};

    float cumDistRenderThres{7.f};

    bool onlyLinesAndPoints{false};
    bool defaultVPSizeAndPos{true};

    //In pennmode right and left click are switched
    bool penmode = false;
    float outsideMovementAreaFactor{80.f};

    // dataset & segmentation rendering options
    double luminanceRangeDelta{1.};
    double luminanceBias{0.};
    bool showOnlyRawData{false};
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> datasetColortable;//user LUT
    bool datasetColortableOn{false};
    // skeleton rendering options
    float depthCutOff{5.f};
    QFlags<TreeDisplay> skeletonDisplayVP3D{TreeDisplay::ShowIn3DVP};
    QFlags<TreeDisplay> skeletonDisplayVPOrtho{TreeDisplay::ShowInOrthoVPs};
    QFlags<TreeDisplay> meshDisplay{TreeDisplay::ShowIn3DVP, TreeDisplay::ShowInOrthoVPs};
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
    float FOVmin{0.5};
    float FOVmax{1};
    CrosshairDisplay crosshairDisplay{CrosshairDisplay::SUBTLE};
    RotationCenter rotationCenter{RotationCenter::ActiveNode};
    int showIntersections{false};
    int showScalebar{false};
    bool enableArbVP{false};
    bool showVpPlanes{true};
    bool showXYplane{true};
    bool showXZplane{true};
    bool showZYplane{true};
    bool showArbplane{true};
    bool showVpDecorations{true};
    std::vector<std::size_t> layerOrder;
    double meshAlphaFactor3d{1};
    double meshAlphaFactorSlicing{0.5};
    bool MeshPickingEnabled{true};

    GLBuffers AllTreesBuffers;
    GLBuffers selectedTreesBuffers;
};

class ViewportBase;
class Viewer : public QObject {
    const bool evilHack;
    Q_OBJECT
private:
    QElapsedTimer keyRepeatTimer;
    floatCoordinate moveCache; //Cache for Movements smaller than pixel coordinate

    ViewerState viewerState;

    void vpGenerateTexture(ViewportArb & vp, const std::size_t layerId);

    void dcSliceExtract(std::uint8_t * datacube, Coordinate cubePosInAbsPx, std::uint8_t * slice, ViewportOrtho & vp, const std::size_t layerId, const boost::optional<decltype(Dataset::LayerRenderSettings::combineSlicesType)> combineType);
    void dcSliceExtract(std::uint8_t * datacube, floatCoordinate *currentPxInDc_float, std::uint8_t * slice, int s, int *t, const floatCoordinate & v2, const std::size_t layerId, float usedSizeInCubePixels);

    void ocSliceExtract(std::uint64_t * datacube, Coordinate cubePosInAbsPx, std::uint8_t * slice, ViewportOrtho & vp, const std::size_t layerId);

    void calcLeftUpperTexAbsPx();

    Remote remote;
public:
    Viewer();
    // there’s a problem (mesa only?)
    // rendering knossos at full speed and displaying a i.e. file dialog or uploading a file
    // so we stop the rendering during display of said dialog
    template<typename F, typename... Args>
    auto suspend(F func, Args... args) {
        timer.stop();
        auto && res = func(args...);
        timer.start();
        return std::move(res);
    }
    void saveSettings();
    void loadSettings();
    MainWindow mainWindow;
    MainWindow *window = &mainWindow;

    std::list<TextureLayer> layers;
    int gpucubeedge{128};
    bool gpuRendering{false};

    ViewportOrtho *viewportXY, *viewportXZ, *viewportZY;
    ViewportArb *viewportArb;
    void zoomRelative(const float factor);
    void zoom(const float newScreenPxXPerDataPx);
    void zoomReset();
    QTimer timer;

    void arbCubes(ViewportArb & vp);
    void setEnableArbVP(const bool on);
    void setDefaultVPSizeAndPos(const bool on);
    void resizeTexEdgeLength(const int cubeEdge, const int superCubeEdge, const std::size_t layerCount);
    void loadNodeLUT(const QString & path);
    void loadTreeLUT(const QString & path = ":/resources/color_palette/default.json");
    QColor getNodeColor(const nodeListElement & node) const;
signals:
    void enabledArbVP(const bool on);
    void changedDefaultVPSizeAndPos();
    void coordinateChangedSignal(const Coordinate & pos);
    void zoomChanged();
    void movementAreaFactorChangedSignal();
    void magnificationLockChanged(bool);
    void layerVisibilityChanged(const std::size_t);
    void layerRenderSettingsChanged();
    void mesh3dAlphaFactorChanged(float);
    void meshSlicingAlphaFactorChanged(float);
public slots:
    void updateCurrentPosition();
    void updateDatasetMag();
    bool updateDatasetMag(const std::size_t layerId, const boost::optional<std::size_t> mag = boost::none);
    void setPosition(const floatCoordinate & pos, UserMoveType userMoveType = USERMOVE_NEUTRAL, const Coordinate & viewportNormal = {0, 0, 0});
    void setPositionWithRecentering(const Coordinate &pos);
    void setPositionWithRecenteringAndRotation(const Coordinate &pos);
    void userMoveVoxels(const Coordinate &step, UserMoveType userMoveType, const floatCoordinate & viewportNormal);
    void userMove(const floatCoordinate & floatStep, UserMoveType userMoveType = USERMOVE_NEUTRAL, const floatCoordinate & viewportNormal = {0, 0, 0});
    void userMoveRound(UserMoveType userMoveType = USERMOVE_NEUTRAL, const floatCoordinate & viewportNormal = {0, 0, 0});
    void userMoveClear();
    void recalcTextureOffsets();
    void calcDisplayedEdgeLength();
    void applyTextureFilterSetting(const QOpenGLTexture::Filter texFiltering);
    void run();
    void loader_notify(const UserMoveType userMoveType = USERMOVE_NEUTRAL, const floatCoordinate & direction = {0, 0, 0});
    void defaultDatasetLUT();
    void loadDatasetLUT(const QString & path);
    void datasetColorAdjustmentsChanged();
    void vpGenerateTexture(ViewportOrtho & vp, const std::size_t layerId);
    void addRotation(const QQuaternion & quaternion);
    void resetRotation();
    void calculateMissingOrthoGPUCubes(TextureLayer & layer);
    void reslice_notify();
    void reslice_notify(const std::size_t layerId);
    void reslice_notify_all(const std::size_t layerId, boost::optional<CoordOfCube> globalCoord = boost::none);
    void segmentation_changed();
    void setMovementAreaFactor(float alpha);
    double highestScreenPxXPerDataPx();
    double lowestScreenPxXPerDataPx();
    std::size_t calcMag(const std::size_t layerId, const float screenPxXPerDataPx);
    void setMagnificationLock(const bool locked);
    void setLayerVisibility(const std::size_t index, const bool enabled);
    void setMesh3dAlphaFactor(const float alpha);
    void setMeshSlicingAlphaFactor(const float alpha);
};
