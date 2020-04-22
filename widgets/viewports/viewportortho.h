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
#include "mesh/mesh.h"
#include "skeleton/node.h"
#include "viewportbase.h"

#include <atomic>

class ViewportOrtho : public ViewportBase {
    Q_OBJECT
    QOpenGLShaderProgram raw_data_shader;
    QOpenGLShaderProgram overlay_data_shader;

    QAction zoomResetAction{tr("Reset zoom"), &menuButton};

    floatCoordinate handleMovement(const QPoint & pos);
    virtual void zoom(const float zoomStep) override;
    virtual float zoomStep() const override { return 0.75; }

    virtual void renderViewportFast();
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
    virtual ~ViewportOrtho() override;
    void resetTexture(const std::size_t layerCount);
    void applyTextureFilter();
    void setTextureFilter(std::size_t layerId, const QOpenGLTexture::Filter textureFilter);
    static bool showNodeComments;

    void sendCursorPosition();
    Coordinate getMouseCoordinate();
    // vp vectors relative to the first cartesian octant
    floatCoordinate v1;// vector in x direction
    floatCoordinate v2;// vector in y direction
    floatCoordinate  n;// faces away from the vp plane towards the camera
    std::vector<std::atomic_bool> resliceNecessary{decltype(resliceNecessary)(2)};// FIXME legacy;
    double displayedIsoPx;
    double screenPxXPerMag1Px;
    double screenPxYPerMag1Px;

    char * viewPortData;
    viewportTexture texture;
    float screenPxXPerDataPxForZoomFactor(const float zoomFactor) const { return edgeLength / (displayedEdgeLenghtXForZoomFactor(zoomFactor) / texture.texUnitsPerDataPx); }
    virtual float displayedEdgeLenghtXForZoomFactor(const float zoomFactor) const;

public slots:
    void takeSnapshotDatasetSize(SnapshotOptions o);
    virtual void zoomIn() override { zoom(zoomStep()); }
    virtual void zoomOut() override { zoom(1.f/zoomStep()); }
};
