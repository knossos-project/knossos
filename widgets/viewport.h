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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "coordinate.h"
#include "mesh/mesh.h"
#include "stateInfo.h"

#include <QAction>
#include <QDebug>
#include <QDialog>
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
constexpr const double SKELZOOMMAX = 1024.0;
constexpr const double SKELZOOMMIN = 0.25;

constexpr const int DEFAULT_VP_MARGIN = 5;
constexpr const int DEFAULT_VP_SIZE = 350;

struct viewportTexture {
    //Handles for OpenGl
    uint texHandle{0};
    GLint textureFilter{GL_LINEAR};
    uint overlayHandle{0};

    //The absPx coordinate of the upper left corner of the texture actually stored in *texture
    floatCoordinate leftUpperPxInAbsPx;
    GLsizei size;

    //These variables specifiy the lengths inside the texture that are currently displayed.
    //Their values depend on the zoom level and the data voxel dimensions (because of aspect
    //ratio correction). Units are texture coordinates.
    float texUsedX;
    float texUsedY;

    float texUnitsPerDataPx;
    //Texture coordinates
    float texLUx, texLUy, texLLx, texLLy, texRUx, texRUy, texRLx, texRLy;
    // Current zoom level. 1: no zoom; near 0: maximum zoom.
    float FOV;
    float usedSizeInCubePixels;
};

struct RenderOptions {
    enum class SelectionPass { NoSelection, NodeID0_24Bits, NodeID24_48Bits, NodeID48_64Bits };
    RenderOptions();
    static RenderOptions nodePickingRenderOptions(SelectionPass pass);
    static RenderOptions meshPickingRenderOptions();
    static RenderOptions snapshotRenderOptions(const bool drawBoundaryAxes, const bool drawBoundaryBox, const bool drawOverlay, const bool drawMesh, const bool drawSkeleton, const bool drawViewportPlanes);

    bool drawBoundaryAxes{true};
    bool drawBoundaryBox{true};
    bool drawCrosshairs{true};
    bool drawOverlay{true};
    bool drawMesh{true};
    bool drawSkeleton{true};
    bool drawViewportPlanes{true};
    bool enableLoddingAndLinesAndPoints{true};
    bool enableTextScaling{false};
    bool highlightActiveNode{true};
    bool highlightSelection{true};
    bool nodePicking{false};
    bool meshPicking{false};
    SelectionPass selectionPass{SelectionPass::NoSelection};
};

struct SnapshotOptions {
    QString path{""};
    ViewportType vp{VIEWPORT_UNDEFINED};
    int size{0};
    bool withAxes{true};
    bool withBox{true};
    bool withOverlay{true};
    bool withSkeleton{true};
    bool withScale{true};
    bool withVpPlanes{true};
};
Q_DECLARE_METATYPE(SnapshotOptions)

class ViewportBase;
class ResizeButton : public QPushButton {
    Q_OBJECT
    virtual void mouseMoveEvent(QMouseEvent * event) override;
public:
    explicit ResizeButton(QWidget *parent) : QPushButton(parent) {}
signals:
    void vpResize(const QPoint & globalPos);
};


class QViewportFloatWidget : public QDialog {
    ViewportBase *vp;
protected:
    virtual void closeEvent(QCloseEvent *event) override;
    virtual void moveEvent(QMoveEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
public:
    bool fullscreen{false};
    boost::optional<QPoint> nonMaximizedPos;
    boost::optional<QSize> nonMaximizedSize;
    explicit QViewportFloatWidget(QWidget *parent, ViewportBase *vp);
};

constexpr int defaultFontSize = 10;
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
    QWidget *dockParent;
    QAction snapshotAction{tr("Snapshot"), &menuButton};
    QAction floatingWindowAction{tr("Undock viewport"), &menuButton};
    QAction fullscreenAction{tr("Fullscreen (F11)"), &menuButton};
    QAction hideAction{tr("Hide viewport"), &menuButton};
    ResizeButton resizeButton;
    bool resizeButtonHold = false;
    void moveVP(const QPoint & globalPos);

    QSet<nodeListElement *> nodeSelection(int x, int y);
    // rendering
    virtual void resizeGL(int width, int height) override;
    bool sphereInFrustum(floatCoordinate pos, float radius);

protected:
    QOpenGLShaderProgram meshShader;
    QOpenGLShaderProgram meshTreeColorShader;
    QOpenGLShaderProgram meshIdShader;

    virtual void zoom(const float zoomStep) = 0;
    virtual float zoomStep() const = 0;
    // rendering
    virtual void initializeGL() override;
    virtual void hideVP();
    void setFrontFacePerspective();
    void renderScaleBar();
    virtual void renderViewport(const RenderOptions & options = RenderOptions()) = 0;
    void renderText(const Coordinate &pos, const QString &str, const bool fontScaling, const bool centered = false);
    void renderSphere(const Coordinate &pos, float radius, const QColor &color, const RenderOptions & options = RenderOptions());
    void renderCylinder(const Coordinate &base, float baseRadius, const Coordinate &top, float topRadius, const QColor &color, const RenderOptions & options = RenderOptions());
    void renderSkeleton(const RenderOptions & options = RenderOptions());
    virtual void renderSegment(const segmentListElement & segment, const QColor &color, const RenderOptions & options = RenderOptions());
    virtual void renderNode(const nodeListElement & node, const RenderOptions & options = RenderOptions());
    bool updateFrustumClippingPlanes();
    virtual void renderViewportFrontFace();
    QSet<nodeListElement *> pickNodes(int centerX, int centerY, int width, int height);
    boost::optional<nodeListElement &> pickNode(int x, int y, int width);
    void handleLinkToggle(const QMouseEvent & event);

    virtual void moveEvent(QMoveEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

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
    ViewportType viewportType; // floatparent requires this to be initialized first to set its title
    QViewportFloatWidget floatParent;
    const static int numberViewports = 5;

    bool hasCursor{false};
    virtual void showHideButtons(bool isShow) {
        menuButton.setVisible(isShow);
        resizeButton.setVisible(isShow && isDocked);
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

    bool isDocked{true};
    QSize dockSize;
    QPoint dockPos;
    // holds wether vp was made fullscreen from docked state. Determines if vp is docked or floated when leaving fullscreen mode.
    bool isFullOrigDocked;
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
    float displayedlengthInNmX;

    virtual void zoomIn() {}
    virtual void zoomOut() {}

    uint edgeLength; //edge length in screen pixel coordinates; only squarish VPs are allowed

    float frustum[6][4]; // Stores the current view frustum planes
signals:
    void cursorPositionChanged(const Coordinate & position, const ViewportType vpType);
    void pasteCoordinateSignal();
    void updateZoomWidget();
    void snapshotTriggered(const ViewportType type);
public slots:
    void takeSnapshotVpSize(SnapshotOptions o);
    void takeSnapshotDatasetSize(SnapshotOptions o);
    void takeSnapshot(const SnapshotOptions & o);
};

class Viewport3D : public ViewportBase {
    Q_OBJECT
    QPushButton wiggleButton{"w"}, xyButton{"xy"}, xzButton{"xz"}, zyButton{"zy"}, r90Button{"r90"}, r180Button{"r180"}, resetButton{"reset"};

    void resetWiggle();
    virtual void zoom(const float zoomStep) override;
    virtual float zoomStep() const override;
    virtual void paintGL() override;
    bool wiggleDirection{true};
    int wiggle{0};
    QTimer wiggletimer;
    void renderVolumeVP();
    void renderMesh();
    void renderMeshBuffer(Mesh & buf);
    void renderMeshBufferIds(Mesh & buf);
    void pickMeshIdAtPosition();
    boost::optional<BufferSelection> pickMesh(const QPoint pos);
    void renderSkeletonVP(const RenderOptions & options = RenderOptions());
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
    virtual void focusOutEvent(QFocusEvent *event) override;
public:
    double zoomFactor{1.0};
    boost::optional<BufferSelection> meshLastClickInformation;
    bool meshLastClickCurrentlyVisited{false};
    explicit Viewport3D(QWidget *parent, ViewportType viewportType);
    ~Viewport3D();
    virtual void showHideButtons(bool isShow) override;
    void updateVolumeTexture();
    static bool showBoundariesInUm;

    void zoomIn() override { zoom(zoomStep()); }
    void zoomOut() override { zoom(1.0/zoomStep()); }
};

class ViewportOrtho : public ViewportBase {
    Q_OBJECT
    QOpenGLShaderProgram raw_data_shader;
    QOpenGLShaderProgram overlay_data_shader;

    floatCoordinate handleMovement(const QPoint & pos);
    virtual void zoom(const float zoomStep) override;
    virtual float zoomStep() const override { return 0.75; }

    void renderViewportFast();
    virtual void renderViewport(const RenderOptions &options = RenderOptions()) override;
    virtual void renderSegment(const segmentListElement & segment, const QColor &color, const RenderOptions & options = RenderOptions()) override;
    void renderSegPlaneIntersection(const segmentListElement & segment);
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
    ~ViewportOrtho();
    void resetTexture();
    static bool showNodeComments;

    void sendCursorPosition();
    Coordinate getMouseCoordinate();
    // vp vectors relative to the first cartesian octant
    floatCoordinate v1;// vector in x direction
    floatCoordinate v2;// vector in y direction
    floatCoordinate  n;// faces away from the vp plane towards the camera
    std::atomic_bool dcResliceNecessary{true};
    std::atomic_bool ocResliceNecessary{true};
    float displayedIsoPx;
    float screenPxYPerDataPx;
    float displayedlengthInNmY;

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
    virtual void hideVP() override;
public:
    floatCoordinate leftUpperPxInAbsPx_float;
    ViewportArb(QWidget *parent, ViewportType viewportType);
    virtual void showHideButtons(bool isShow) override;

    virtual float displayedEdgeLenghtXForZoomFactor(const float zoomFactor) const override;
};
#endif // VIEWPORT_H
