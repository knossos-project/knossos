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

#include "coordinate.h"
#include "hash_list.h"
#include "renderoptions.h"

#include <QAction>
#include <QDialog>
#include <QMatrix4x4>
#include <QMenu>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_1_4>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <boost/optional.hpp>

#include <vector>

enum ViewportType {VIEWPORT_XY, VIEWPORT_XZ, VIEWPORT_ZY, VIEWPORT_ARBITRARY, VIEWPORT_SKELETON, VIEWPORT_UNDEFINED};
Q_DECLARE_METATYPE(ViewportType)

constexpr const double VPZOOMMAX = 0.02;
constexpr const double VPZOOMMIN = 1.;
constexpr const double SKELZOOMMAX = 1024.0;
constexpr const double SKELZOOMMIN = 0.25;

constexpr const int DEFAULT_VP_MARGIN = 5;
constexpr const int DEFAULT_VP_SIZE = 350;

struct GLTexture2D : public QOpenGLTexture {
    GLTexture2D() : QOpenGLTexture{QOpenGLTexture::Target2D} {}
};

struct viewportTexture {
    //Handles for OpenGl
    std::vector<GLTexture2D> texHandle;
    std::vector<std::vector<std::uint8_t>> texData;
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

struct SnapshotOptions {
    boost::optional<QString> path{""};
    ViewportType vp{VIEWPORT_UNDEFINED};
    int size{0};
    bool withAxes{true};
    bool withBox{true};
    bool withMesh{true};
    bool withOverlay{true};
    bool withScale{true};
    bool withSkeleton{true};
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

struct BufferSelection {
    std::uint64_t treeId;
    floatCoordinate coord;
};

constexpr int defaultFontSize = 10;
class nodeListElement;
class segmentListElement;
class Mesh;
class ViewportOrtho;
Coordinate getCoordinateFromOrthogonalClick(const QPointF pos, ViewportOrtho & vp);
class ViewportBase : public QOpenGLWidget, protected QOpenGLFunctions_1_4 { // glBlendFuncSeparate requires 1.4
    Q_OBJECT
protected:
    std::weak_ptr<QOpenGLFramebufferObject> snapshotFbo;
    QVBoxLayout vpLayout;
    QHBoxLayout vpHeadLayout;
    QMenu menu;
    QToolButton menuButton;

    QAction *zoomEndSeparator;
private:
    QOpenGLDebugLogger oglLogger;
    QWidget *dockParent;
    QAction snapshotAction{tr("Snapshot"), &menuButton};
    QAction floatingWindowAction{tr("Undock viewport"), &menuButton};
    QAction fullscreenAction{tr("Fullscreen (F11)"), &menuButton};
    QAction zoomInAction{tr("Zoom in"), &menuButton};
    QAction zoomOutAction{tr("Zoom out"), &menuButton};
    QAction hideAction{tr("Hide viewport"), &menuButton};
    ResizeButton resizeButton;
    bool resizeButtonHold = false;
    void moveVP(const QPoint & globalPos);

    QSet<nodeListElement *> nodeSelection(int x, int y);
    // rendering
    virtual void resizeGL(int width, int height) override;
    bool sphereInFrustum(floatCoordinate pos, float radius);

    void renderMeshBuffer(Mesh & buf, const bool picking = false);

protected:
    QOpenGLShaderProgram meshShader;
    QOpenGLShaderProgram meshTreeColorShader;
    QOpenGLShaderProgram meshIdShader;
    QOffscreenSurface meshPickingSurface;
    QOpenGLContext meshPickingCtx;
    QOpenGLVertexArrayObject meshPickingVao;
    QMatrix4x4 mv, p;
    boost::optional<BufferSelection> pickMesh(const QPoint pos);
    void pickMeshIdAtPosition();

    virtual void zoom(const float zoomStep) = 0;
    virtual float zoomStep() const = 0;
    void applyZoom(const QWheelEvent *event, float direction = 1.0f);
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
    hash_list<nodeListElement *> pickNodes(int centerX, int centerY, int width, int height);
    boost::optional<nodeListElement &> pickNode(int x, int y, int width);
    void handleLinkToggle(const QMouseEvent & event);

    virtual void moveEvent(QMoveEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

    // event-handling
    virtual void focusOutEvent(QFocusEvent * event) override;
    virtual void enterEvent(QEvent * event) override;
    virtual void leaveEvent(QEvent * event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void tabletEvent(QTabletEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override { handleWheelEvent(event); }

    float zoomSpeed{0.5f};
    QPoint mouseDown;
    QPoint prevMouseMove;
    bool stylusDetected{false};
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
    static inline bool oglDebug{false};

    explicit ViewportBase(QWidget *parent, ViewportType viewportType);
    virtual ~ViewportBase() override;

    //This is a bit confusing..the screen coordinate system has always
    //x on the horizontal and y on the verical axis, but the displayed
    //data pixels can have a different axis. Keep this in mind.
    //These values depend on texUnitsPerDataPx (in struct viewportTexture),
    //the current zoom value and the data pixel voxel dimensions.
    double screenPxXPerDataPx;
    double displayedlengthInNmX;

    uint edgeLength; //edge length in screen pixel coordinates; only squarish VPs are allowed

    float frustum[6][4]; // Stores the current view frustum planes

    void renderMesh();
signals:
    void cursorPositionChanged(const Coordinate & position, const ViewportType vpType);
    void pasteCoordinateSignal();
    void updateZoomWidget();
    void snapshotTriggered(const ViewportType type);
    void snapshotFinished(const QImage & image);
public slots:
    void takeSnapshotVpSize(SnapshotOptions o);
    void takeSnapshot(const SnapshotOptions & o);

    virtual void zoomIn() = 0;
    virtual void zoomOut() = 0;
};
