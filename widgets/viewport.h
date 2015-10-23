#ifndef VIEWPORT_H
#define VIEWPORT_H

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

#include "coordinate.h"
#include "scriptengine/decorators/meshdecorator.h"
#include "scriptengine/scripting.h"
#include "stateInfo.h"
#include <QDebug>
#include <QDockWidget>
#include <QFont>
#include <QOpenGLDebugLogger>
#include <QOpenGLFunctions_2_0>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <boost/multi_array.hpp>
#include <boost/optional.hpp>

enum {VP_UPPERLEFT, VP_LOWERLEFT, VP_UPPERRIGHT, VP_LOWERRIGHT};

enum ViewportType {VIEWPORT_XY, VIEWPORT_XZ, VIEWPORT_YZ, VIEWPORT_SKELETON, VIEWPORT_UNDEFINED, VIEWPORT_ARBITRARY};
Q_DECLARE_METATYPE(ViewportType)
/* VIEWPORT_ORTHO has the same value as the XY VP, this is a feature, not a bug.
This is used for LOD rendering, since all ortho VPs have the (about) the same screenPxPerDataPx
values. The XY vp always used. */
const auto VIEWPORT_ORTHO = VIEWPORT_XY;

// vp zoom max < vp zoom min, because vp zoom level translates to displayed edgeLength.
// close zoom -> smaller displayed edge length
constexpr const double VPZOOMMAX = 0.02000;
constexpr const double VPZOOMMIN = 1.0;
constexpr const double SKELZOOMMAX = 0.4999;
constexpr const double SKELZOOMMIN = 0.0;

struct viewportTexture {
    //Handles for OpenGl
    uint texHandle;
    uint overlayHandle;

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
    float zoomLevel;
};

struct RenderOptions {
    RenderOptions(const bool drawBoundaryAxes = true, const bool drawBoundaryBox = true, const bool drawCrosshairs = true, const bool drawOverlay = true, const bool drawSkeleton = true,
                  const bool drawViewportPlanes = true, const bool highlightActiveNode = true, const bool highlightSelection = true, const bool selectionBuffer = false)
        : drawBoundaryAxes(drawBoundaryAxes), drawBoundaryBox(drawBoundaryBox), drawCrosshairs(drawCrosshairs), drawOverlay(drawOverlay),drawSkeleton(drawSkeleton),
          drawViewportPlanes(drawViewportPlanes), highlightActiveNode(highlightActiveNode), highlightSelection(highlightSelection), selectionBuffer(selectionBuffer) {}
    bool drawBoundaryAxes;
    bool drawBoundaryBox;
    bool drawCrosshairs;
    bool drawOverlay;
    bool drawSkeleton;
    bool drawViewportPlanes;
    bool highlightActiveNode;
    bool highlightSelection;
    bool selectionBuffer;
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
    explicit QViewportFloatWidget(QWidget *parent, int id);
};

constexpr int defaultFonsSize = 10;
class commentListElement;
class nodeListElement;
class segmentListElement;
class ViewportOrtho;
Coordinate getCoordinateFromOrthogonalClick(const int x_dist, const int y_dist, ViewportOrtho & vp);
class ViewportBase : public QOpenGLWidget, protected QOpenGLFunctions_2_0 {
    Q_OBJECT
protected: QVBoxLayout vpLayout;
private:
    QOpenGLDebugLogger oglLogger;
    bool isDocked = true;
    bool isFullOrigDocked;
    QWidget *dockParent;
    QViewportFloatWidget *floatParent = nullptr;
    ResizeButton resizeButton;
    bool resizeButtonHold = false;
    void moveVP(const QPoint & globalPos);

    QSet<nodeListElement *> nodeSelection(int x, int y);
    // rendering
    virtual void resizeGL(int w, int h) override;
    const uint GLNAME_NODEID_OFFSET = 50;//glnames for node ids start at this value
    bool sphereInFrustum(floatCoordinate pos, float radius);

protected:
    QElapsedTimer timeDBase;
    QElapsedTimer timeFBase;

    virtual void zoom(const float zoomStep) = 0;
    virtual float zoomStep() const = 0;
    // rendering
    virtual void initializeGL() override;
    void setFrontFacePerspective();
    void renderScaleBar(const int fontSize = defaultFonsSize);
    virtual void renderViewport(const RenderOptions &options) = 0;
    void renderText(const Coordinate &pos, const QString &str, const int fontSize = defaultFonsSize, const bool centered = false);
    uint renderSphere(const Coordinate & pos, const float & radius, const color4F & color);
    uint renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius, color4F color);
    void renderSkeleton(const RenderOptions & options = RenderOptions());
    virtual void renderSegment(const segmentListElement & segment, const color4F &color);
    virtual void renderNode(const nodeListElement & node, const RenderOptions &options);
    bool updateFrustumClippingPlanes();
    virtual void renderViewportFrontFace();
    boost::optional<nodeListElement &> retrieveVisibleObjectBeneathSquare(uint x, uint y, uint width);
    QSet<nodeListElement *> retrieveAllObjectsBeneathSquare(uint centerX, uint centerY, uint width, uint height);

    // event-handling
    virtual void enterEvent(QEvent * event) override;
    virtual void leaveEvent(QEvent * event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override {
        if (event->button() == Qt::MouseButton::LeftButton) {
            setDock(!isDocked);
        }
    }
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
    const static int numberViewports = 4;
    ViewportType viewportType; // XY_VIEWPORT, ...
    uint id; // VP_UPPERLEFT,

    bool hasCursor{false};
    virtual void showHideButtons(bool isShow) { resizeButton.setVisible(isShow); }
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

    explicit ViewportBase(QWidget *parent, ViewportType viewportType, const uint id);
    virtual ~ViewportBase();

    static bool initMesh(mesh & toInit, uint initialSize);
    static bool doubleMeshCapacity(mesh & toDouble);
    static bool resizemeshCapacity(mesh & toResize, uint n);

    //This is a bit confusing..the screen coordinate system has always
    //x on the horizontal and y on the verical axis, but the displayed
    //data pixels can have a different axis. Keep this in mind.
    //These values depend on texUnitsPerDataPx (in struct viewportTexture),
    //the current zoom value and the data pixel voxel dimensions.
    float screenPxXPerDataPx;
    float screenPxYPerDataPx;
    float displayedlengthInNmX;
    float displayedlengthInNmY;

    Coordinate upperLeftCorner; //upper left corner of viewport in screen pixel coords (max: window borders)
    uint edgeLength; //edge length in screen pixel coordinates; only squarish VPs are allowed

    float frustum[6][4]; // Stores the current view frustum planes
signals:
    void cursorPositionChanged(const Coordinate & position, const uint id);

    void rotationSignal(const floatCoordinate & axis, const float angle);
    void pasteCoordinateSignal();
    void zoomReset();

    void delSegmentSignal(segmentListElement *segToDel);
    void addSegmentSignal(nodeListElement & sourceNode, nodeListElement & targetNode);

    void compressionRatioToggled();
    void setRecenteringPositionSignal(const floatCoordinate & newPos);
    void setRecenteringPositionWithRotationSignal(const floatCoordinate & newPos, const uint vp);

    void recalcTextureOffsetsSignal();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void updateDatasetOptionsWidget();
public slots:
    void takeSnapshot(const QString & path, const int size, const bool withAxes, const bool withOverlay, const bool withSkeleton, const bool withScale, const bool withVpPlanes);
};

class Viewport3D : public ViewportBase {
    Q_OBJECT
    QPushButton xyButton{"xy"}, xzButton{"xz"}, yzButton{"yz"}, r90Button{"r90"}, r180Button{"r180"}, resetButton{"reset"};

    virtual void zoom(const float zoomStep) override;
    virtual float zoomStep() const override;
    virtual void paintGL() override;
    bool renderVolumeVP();
    bool renderSkeletonVP(const RenderOptions &options);
    bool updateRotationStateMatrix(float M1[16], float M2[16]);
    bool rotateViewport();
    virtual void renderViewport(const RenderOptions &options = RenderOptions()) override;
    void renderArbitrarySlicePane(const ViewportOrtho & vp);
    virtual void renderNode(const nodeListElement & node, const RenderOptions &options) override;
    virtual void renderViewportFrontFace() override;

    virtual void handleMouseMotionLeftHold(const QMouseEvent *event) override;
    virtual void handleMouseMotionRightHold(const QMouseEvent *event) override;
    virtual void handleWheelEvent(const QWheelEvent *event) override;
public:
    explicit Viewport3D(QWidget *parent, ViewportType viewportType, const uint id);
    virtual void showHideButtons(bool isShow) override;
    void updateVolumeTexture();
    static bool showBoundariesInUm;

    void zoomIn() { zoom(zoomStep()); }
    void zoomOut() { zoom(-zoomStep()); }
signals:
    void rotationSignal(const floatCoordinate & axis, const float angle);
};

class ViewportOrtho : public ViewportBase {
    Q_OBJECT
    QOpenGLShaderProgram raw_data_shader;
    QOpenGLShaderProgram overlay_data_shader;

    virtual void zoom(const float zoomStep) override;
    virtual float zoomStep() const override { return 0.1; }

    virtual void initializeGL() override;
    virtual void paintGL() override;
    void createOverlayTextures();
    void updateOverlayTexture();
    void renderViewportFast();
    virtual void renderViewport(const RenderOptions &options = RenderOptions()) override;
    virtual void renderSegment(const segmentListElement & segment, const color4F &color) override;
    uint renderSegPlaneIntersection(const segmentListElement & segment);
    virtual void renderNode(const nodeListElement & node, const RenderOptions &options) override;
    void renderBrush(uint viewportType, Coordinate coord);
    virtual void renderViewportFrontFace() override;

    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    QPointF userMouseSlide = {};
    floatCoordinate arbNodeDragCache = {};
    class nodeListElement *draggedNode = nullptr;
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
public:
    explicit ViewportOrtho(QWidget *parent, ViewportType viewportType, const uint id);
    static bool arbitraryOrientation;
    void setOrientation(ViewportType orientation);
    static bool showNodeComments;

    void sendCursorPosition();
    Coordinate getMouseCoordinate();

    //The absPx coordinate of the upper left corner pixel of the currently on screen displayed data
    Coordinate leftUpperDataPxOnScreen;
    // plane vectors. s*v1 + t*v2 = px
    floatCoordinate n;
    floatCoordinate v1; // vector in x direction
    floatCoordinate v2; // vector in y direction
    floatCoordinate leftUpperPxInAbsPx_float;
    floatCoordinate leftUpperDataPxOnScreen_float;
    int s_max;
    int t_max;

    char * viewPortData;
    viewportTexture texture;
};

#endif // VIEWPORT_H
