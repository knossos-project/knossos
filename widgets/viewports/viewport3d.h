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

#include "viewportbase.h"

#include <QMatrix4x4>
#include <QThread>
#include <QTimer>

class Viewport3D : public ViewportBase {
    Q_OBJECT
    QThread timerThread;
    QPushButton wiggleButton{"w"}, xyButton{"xy"}, xzButton{"xz"}, zyButton{"zy"}, r90Button{"r90"}, r180Button{"r180"}, resetButton{"reset"};
    QAction resetAction{nullptr};
    QAction showVolumeAction{tr("Show volume"), &menuButton};
    void resetWiggle();
    virtual void zoom(const float zoomStep) override;
    virtual float zoomStep() const override;
    virtual void paintGL() override;
    bool wiggleDirection{true};
    int wiggle{0};
    QTimer wiggletimer, rotationTimer;
    void renderVolumeVP();
    void renderSkeletonVP(const RenderOptions & options = RenderOptions());
    virtual void renderViewport(const RenderOptions &options = RenderOptions()) override;
    virtual void renderNode(const nodeListElement & node, const RenderOptions & options = RenderOptions()) override;
    virtual void renderViewportFrontFace() override;

    virtual void handleMouseMotionLeftHold(const QMouseEvent *event) override;
    virtual void handleMouseMotionRightHold(const QMouseEvent *event) override;
    virtual void handleMouseButtonLeft(const QMouseEvent * event) override;
    virtual void handleMouseButtonRight(const QMouseEvent * event) override;
    virtual void handleMouseReleaseRight(const QMouseEvent * event) override;
    virtual void handleWheelEvent(const QWheelEvent *event) override;
    virtual void handleKeyPress(const QKeyEvent *event) override;
    virtual void handleKeyRelease(const QKeyEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;

public:
    double zoomFactor{1.0};
    QMatrix4x4 rotation;
    float translateX{0};
    float translateY{0};
    explicit Viewport3D(QWidget *parent, ViewportType viewportType);
    virtual ~Viewport3D() override;
    virtual void showHideButtons(bool isShow) override;
    void refocus(const boost::optional<Coordinate> position = boost::none);
    void updateVolumeTexture();
    static inline bool showBoundariesInUm{false};

public slots:
    virtual void zoomIn() override { zoom(zoomStep()); }
    virtual void zoomOut() override { zoom(1.f/zoomStep()); }
};
