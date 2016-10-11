/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "coordinate.h"
#include "stateInfo.h"

#include <QAction>
#include <QDebug>
#include <QDockWidget>
#include <QFont>
#include <QMouseEvent>
#include <QOpenGLDebugLogger>
#include <QOpenGLFunctions_2_0>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>

#include <atomic>
#include <boost/multi_array.hpp>
#include <boost/optional.hpp>

enum ViewportType {VIEWPORT_XY, VIEWPORT_XZ, VIEWPORT_ZY, VIEWPORT_ARBITRARY, VIEWPORT_SKELETON, VIEWPORT_UNDEFINED};
Q_DECLARE_METATYPE(ViewportType)

constexpr const double VPZOOMMAX = 0.02;
constexpr const double VPZOOMMIN = 1.;
constexpr const double SKELZOOMMAX = 0.4999;
constexpr const double SKELZOOMMIN = 0.0;

struct viewportTexture {
    //Handles for OpenGl
    uint texHandle{0};
    GLint textureFilter{GL_LINEAR};
    uint overlayHandle{0};

    //The absPx coordinate of the upper left corner of the texture actually stored in *texture
    Coordinate leftUpperPxInAbsPx;
    uint edgeLengthDc;
    uint edgeLengthPx;

    //These variables specifiy the area inside the textures which are used
    //for data storage. Storage always starts at texture pixels (0,0).
    int usedTexLengthDc;

    //These variables specifiy the lengths inside the texture that are currently displayed.
    //Their values depend on the zoom level and the data voxel dimensions (because of aspect
    //ratio correction). Units are texture coordinates.
    float displayedEdgeLengthX;
    float displayedEdgeLengthY;

    float texUnitsPerDataPx;

    //Texture coordinates
    float texLUx, texLUy, texLLx, texLLy, texRUx, texRUy, texRLx, texRLy;
    //Coordinates of crosshair inside VP
    float xOffset, yOffset;

    // Current zoom level. 1: no zoom; near 0: maximum zoom.
    float FOV;
};

struct RenderOptions {
    enum class SelectionPass { NoSelection, NodeIDLowerBits, NodeIDHigherBits };
    // default
    RenderOptions();
    // snapshot render options
    RenderOptions(const bool drawBoundaryAxes, const bool drawBoundaryBox, const bool drawOverlay, const bool drawSkeleton, const bool drawViewportPlanes);

    bool drawBoundaryAxes;
    bool drawBoundaryBox;
    bool drawCrosshairs;
    bool drawOverlay;
    bool drawSkeleton;
    bool drawViewportPlanes;
    bool enableSkeletonDownsampling;
    bool enableTextScaling;
    bool highlightActiveNode;
    bool highlightSelection;
    SelectionPass selectionPass{SelectionPass::NoSelection};
    bool pointCloudPicking;
};

class ViewportBase;
class ResizeButton : public QPushButton {
    Q_OBJECT
    virtual void mouseMoveEvent(QMouseEvent * event) override;
public:
    explicit ResizeButton(QWidget *parent) : QPushButton(parent) {}
signals:
    void vpResize(const QPoint & globalPos);
};


class QViewportFloatWidget : public QWidget {
public:
    explicit QViewportFloatWidget(QWidget *parent, ViewportType vpType);
};

constexpr int defaultFonsSize = 10;
class nodeListElement;
class segmentListElement;
class mesh;
class ViewportOrtho;
Coordinate getCoordinateFromOrthogonalClick(const int x_dist, const int y_dist, ViewportOrtho & vp);
class ViewportBase : public QOpenGLWidget, protected QOpenGLFunctions_2_0 {
    Q_OBJECT
protected:
    QVBoxLayout vpLayout;
    QHBoxLayout vpHeadLayout;
    QToolButton menuButton;
private:
    QOpenGLDebugLogger oglLogger;
    bool isDocked = true;
    bool isFullOrigDocked;
    QWidget *dockParent;
    QViewportFloatWidget *floatParent = nullptr;
    QAction snapshotAction{"Snapshot", &menuButton};
    QAction floatingWindowAction{"Undock viewport", &menuButton};
    ResizeButton resizeButton;
    bool resizeButtonHold = false;
    void moveVP(const QPoint & globalPos);

    QSet<nodeListElement *> nodeSelection(int x, int y);
    // rendering
    virtual void resizeGL(int w, int h) override;
    bool sphereInFrustum(floatCoordinate pos, float radius);

protected:
    virtual void zoom(const float zoomStep) = 0;
    virtual float zoomStep() const = 0;
    // rendering
    virtual void initializeGL() override;
    void setFrontFacePerspective();
    void renderScaleBar();
    virtual void renderViewport(const RenderOptions & options = RenderOptions()) = 0;
    void renderText(const Coordinate &pos, const QString &str, const bool fontScaling, const bool centered = false);
    uint renderSphere(const Coordinate & pos, const float & radius, const QColor &color, const RenderOptions & options = RenderOptions());
    uint renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius, QColor color, const RenderOptions & options = RenderOptions());
    void renderSkeleton(const RenderOptions & options = RenderOptions());
    virtual void renderSegment(const segmentListElement & segment, const QColor &color, const RenderOptions & options = RenderOptions());
    virtual void renderNode(const nodeListElement & node, const RenderOptions & options = RenderOptions());
    bool updateFrustumClippingPlanes();
    virtual void renderViewportFrontFace();
    bool pickedScalebar(uint centerX, uint centerY, uint width);
    QSet<nodeListElement *> pickNodes(uint centerX, uint centerY, uint width, uint height);
    boost::optional<nodeListElement &> pickNode(uint x, uint y, uint width);
    template<typename F>
    std::vector<GLuint> pickingBox(F renderFunc, uint centerX, uint centerY, uint selectionWidth, uint selectionHeight);
    void handleLinkToggle(const QMouseEvent & event);

    // event-handling
    virtual void enterEvent(QEvent * event) override;
    virtual void leaveEvent(QEvent * event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override { handleWheelEvent(event); }

    QPoint mouseDown;
    QPoint prevMouseMove;
    int xrel(const int x) { return x - prevMouseMove.x(); }
    int yrel(const int y) { return y - prevMouseMove.y(); }
    virtual void handleKeyPress(const QKeyEvent *event);
    virtual void handleKeyRelease(const QKeyEvent *event);
    virtual void handleMouseHover(const QMouseEvent *) {}
    virtual void handleMouseButtonLeft(const QMouseEvent *event);
    virtual void handleMouseReleaseLeft(const QMouseEvent *event);
    virtual void handleMouseMotionLeftHold(const QMouseEvent *event);
    virtual void handleMouseButtonRight(const QMouseEvent *) {}
    virtual void handleMouseMotionRightHold(const QMouseEvent *) {}
    virtual void handleMouseReleaseRight(const QMouseEvent *) {}
    virtual void handleMouseButtonMiddle(const QMouseEvent *event);
    virtual void handleMouseReleaseMiddle(const QMouseEvent *) {}
    virtual void handleMouseMotionMiddleHold(const QMouseEvent *) {}
    virtual void handleWheelEvent(const QWheelEvent *event);

public:
    const static int numberViewports = 5;
    ViewportType viewportType;

    bool hasCursor{false};
    virtual void showHideButtons(bool isShow) {
        menuButton.setVisible(isShow);
        resizeButton.setVisible(isShow);
    }
    void posAdapt() { posAdapt(pos()); }
    void posAdapt(const QPoint & desiredPos) {
        const auto horizontalSpace = parentWidget()->width() - width();
        const auto verticalSpace = parentWidget()->height() - height();
        const auto newX = std::max(0, std::min(horizontalSpace, desiredPos.x()));
        const auto newY = std::max(0, std::min(verticalSpace, desiredPos.y()));
        move(newX, newY);
    }
    void sizeAdapt() { sizeAdapt({size().width(), size().height()}); }
    void sizeAdapt(const QPoint & desiredSize) {
        const auto MIN_VP_SIZE = 50;
        const auto horizontalSpace = parentWidget()->width() - x();
        const auto verticalSpace = parentWidget()->height() - y();
        const auto size = std::max(MIN_VP_SIZE, std::min({horizontalSpace, verticalSpace, std::max(desiredSize.x(), desiredSize.y())}));
        resize({size, size});
    }
    QSize dockSize;
    QPoint dockPos;
    void setDock(bool isDock);
    static bool oglDebug;

    explicit ViewportBase(QWidget *parent, ViewportType viewportType);
    virtual ~ViewportBase();

    //This is a bit confusing..the screen coordinate system has always
    //x on the horizontal and y on the verical axis, but the displayed
    //data pixels can have a different axis. Keep this in mind.
    //These values depend on texUnitsPerDataPx (in struct viewportTexture),
    //the current zoom value and the data pixel voxel dimensions.
    float screenPxXPerDataPx;
    float screenPxYPerDataPx;
    float displayedlengthInNmX;
    float displayedlengthInNmY;

    virtual void zoomIn() {}
    virtual void zoomOut() {}

    Coordinate upperLeftCorner; //upper left corner of viewport in screen pixel coords (max: window borders)
    uint edgeLength; //edge length in screen pixel coordinates; only squarish VPs are allowed

    float frustum[6][4]; // Stores the current view frustum planes
signals:
    void cursorPositionChanged(const Coordinate & position, const ViewportType vpType);
    void pasteCoordinateSignal();
    void compressionRatioToggled();
    void updateDatasetOptionsWidget();
    void snapshotTriggered(const ViewportType type);
public slots:
    void takeSnapshot(const QString & path, const int size, const bool withAxes, const bool withBox, const bool withOverlay, const bool withSkeleton, const bool withScale, const bool withVpPlanes);
};

struct PointcloudBuffer {
    PointcloudBuffer(GLenum render_mode)
        : render_mode(render_mode) {
        position_buf.create();
        normal_buf.create();
        color_buf.create();
        index_buf.create();
    }

    // ~PointcloudBuffer() {
    //     position_buf.destroy();
    //     normal_buf.destroy();
    //     color_buf.destroy();
    //     index_buf.destroy();
    // }

    std::size_t vertex_count{0};
    std::size_t index_count{0};
    QOpenGLBuffer position_buf{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer normal_buf{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer color_buf{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer index_buf{QOpenGLBuffer::IndexBuffer};
    GLenum render_mode{GL_POINTS};

    QVector<float> vertex_coords;
    QVector<unsigned int> indices;
};

class Viewport3D : public ViewportBase {
    Q_OBJECT
    QPushButton xyButton{"xy"}, xzButton{"xz"}, zyButton{"zy"}, r90Button{"r90"}, r180Button{"r180"}, resetButton{"reset"};
    std::unordered_map<std::uint64_t, PointcloudBuffer> pointcloudBuffers;
    QOpenGLShaderProgram pointcloudShader;
    QOpenGLShaderProgram pointcloudIdShader;
    struct BufferSelection {
        std::pair<const std::uint64_t, PointcloudBuffer> & buf;
        floatCoordinate coord;
    };
    std::unordered_map<std::uint64_t, BufferSelection> selection_ids;

    virtual void zoom(const float zoomStep) override;
    virtual float zoomStep() const override;
    virtual void paintGL() override;
    bool wiggle3D{false};
    int wigglecounter{0};
    QTimer *wiggletimer;
    bool renderVolumeVP();
    void renderPointCloud();
    void renderPointCloudBuffer(PointcloudBuffer& buf);
    void renderPointCloudBufferIds(PointcloudBuffer& buf);
    boost::optional<floatCoordinate> pointCloudTriangleIDToCoord(const uint32_t triangleID) const;
    uint32_t pointcloudColorToId(std::array<unsigned char, 4> color);
    std::array<unsigned char, 4> pointcloudIdToColor(uint32_t id);
    void pickPointCloudIdAtPosition();
    bool renderSkeletonVP(const RenderOptions & options = RenderOptions());
    virtual void renderViewport(const RenderOptions &options = RenderOptions()) override;
    void renderArbitrarySlicePane(const ViewportOrtho & vp);
    virtual void renderNode(const nodeListElement & node, const RenderOptions & options = RenderOptions()) override;
    virtual void renderViewportFrontFace() override;

    virtual void handleMouseMotionLeftHold(const QMouseEvent *event) override;
    virtual void handleMouseMotionRightHold(const QMouseEvent *event) override;
    virtual void handleMouseReleaseLeft(const QMouseEvent *event) override;
    virtual void handleWheelEvent(const QWheelEvent *event) override;
    virtual void handleKeyPress(const QKeyEvent *event) override;
    virtual void handleKeyRelease(const QKeyEvent *event) override;
public:
    explicit Viewport3D(QWidget *parent, ViewportType viewportType);
    virtual void showHideButtons(bool isShow) override;
    void updateVolumeTexture();
    void addTreePointcloud(std::uint64_t tree_id, QVector<float> & verts, QVector<float> & normals, QVector<unsigned int> & indices, const QVector<float> & color = {1.0f, 0.0f, 0.0f, 1.0f}, int draw_mode = 0);
    void deleteTreePointcloud(std::uint64_t tree_id);
    static bool showBoundariesInUm;

    void zoomIn() override { zoom(zoomStep()); }
    void zoomOut() override { zoom(-zoomStep()); }
};

class ViewportOrtho : public ViewportBase {
    Q_OBJECT
    QOpenGLShaderProgram raw_data_shader;
    QOpenGLShaderProgram overlay_data_shader;

    floatCoordinate handleMovement(const QPoint & pos);
    virtual void zoom(const float zoomStep) override;
    virtual float zoomStep() const override { return 0.75; }

    void createOverlayTextures();
    void renderViewportFast();
    virtual void renderViewport(const RenderOptions &options = RenderOptions()) override;
    virtual void renderSegment(const segmentListElement & segment, const QColor &color, const RenderOptions & options = RenderOptions()) override;
    uint renderSegPlaneIntersection(const segmentListElement & segment);
    virtual void renderNode(const nodeListElement & node, const RenderOptions & options = RenderOptions()) override;
    void renderBrush(const Coordinate coord);
    virtual void renderViewportFrontFace() override;

    floatCoordinate arbNodeDragCache = {};
    class nodeListElement *draggedNode = nullptr;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    bool mouseEventAtValidDatasetPosition(const QMouseEvent *event);
    virtual void handleKeyPress(const QKeyEvent *event) override;
    virtual void handleMouseHover(const QMouseEvent *event) override;
    virtual void handleMouseReleaseLeft(const QMouseEvent *event) override;
    virtual void handleMouseMotionLeftHold(const QMouseEvent *event) override;
    virtual void handleMouseButtonRight(const QMouseEvent *event) override;
    virtual void handleMouseMotionRightHold(const QMouseEvent *event) override;
    virtual void handleMouseReleaseRight(const QMouseEvent *event) override;
    virtual void handleMouseButtonMiddle(const QMouseEvent *event) override;
    virtual void handleMouseMotionMiddleHold(const QMouseEvent *event) override;
    virtual void handleMouseReleaseMiddle(const QMouseEvent *event) override;
    virtual void handleWheelEvent(const QWheelEvent *event) override;

protected:
    virtual void initializeGL() override;
    virtual void paintGL() override;
public:
    explicit ViewportOrtho(QWidget *parent, ViewportType viewportType);
    void resetTexture();
    static bool showNodeComments;

    void sendCursorPosition();
    Coordinate getMouseCoordinate();

    //The absPx coordinate of the upper left corner pixel of the currently on screen displayed data
    Coordinate leftUpperDataPxOnScreen;
    floatCoordinate n;
    floatCoordinate v1; // vector in x direction
    floatCoordinate v2; // vector in y direction
    std::atomic_bool dcResliceNecessary{true};
    std::atomic_bool ocResliceNecessary{true};

    void zoomIn() override { zoom(zoomStep()); }
    void zoomOut() override { zoom(1./zoomStep()); }
    char * viewPortData;
    viewportTexture texture;
    float screenPxXPerDataPxForZoomFactor(const float zoomFactor) const { return edgeLength / (displayedEdgeLenghtXForZoomFactor(zoomFactor) / texture.texUnitsPerDataPx); }
    virtual float displayedEdgeLenghtXForZoomFactor(const float zoomFactor) const;
};

class ViewportArb : public ViewportOrtho {
    Q_OBJECT
    QAction resetAction{"Reset rotation", &menuButton};
    void updateOverlayTexture();

protected:
    virtual void paintGL() override;

public:
    int vpLenghtInDataPx;
    int vpHeightInDataPx;
    floatCoordinate leftUpperPxInAbsPx_float;
    ViewportArb(QWidget *parent, ViewportType viewportType);
    virtual void showHideButtons(bool isShow) override;

    virtual float displayedEdgeLenghtXForZoomFactor(const float zoomFactor) const override;
};
#endif // VIEWPORT_H
