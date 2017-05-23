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

#ifndef VIEWPORT3D_H
#define VIEWPORT3D_H

#include "viewportbase.h"

#include <QMatrix4x4>
#include <QTimer>

class Viewport3D : public ViewportBase {
    Q_OBJECT
    QPushButton wiggleButton{"w"}, xyButton{"xy"}, xzButton{"xz"}, zyButton{"zy"}, r90Button{"r90"}, r180Button{"r180"}, resetButton{"reset"};
    QAction resetAction{nullptr};
    void resetWiggle();
    virtual void zoom(const float zoomStep) override;
    virtual float zoomStep() const override;
    virtual void paintGL() override;
    bool wiggleDirection{true};
    int wiggle{0};
    QTimer wiggletimer;
    void renderVolumeVP();
    void renderSkeletonVP(const RenderOptions & options = RenderOptions());
    virtual void renderViewport(const RenderOptions &options = RenderOptions()) override;
    void renderArbitrarySlicePane(const ViewportOrtho & vp);
    virtual void renderNode(const nodeListElement & node, const RenderOptions & options = RenderOptions()) override;
    virtual void renderViewportFrontFace() override;

    virtual void handleMouseMotionLeftHold(const QMouseEvent *event) override;
    virtual void handleMouseMotionRightHold(const QMouseEvent *event) override;
    virtual void handleWheelEvent(const QWheelEvent *event) override;
    virtual void handleKeyPress(const QKeyEvent *event) override;
    virtual void handleKeyRelease(const QKeyEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;

protected:
    virtual void renderMeshBufferIds(Mesh &buf) override;

public:
    double zoomFactor{1.0};
    QMatrix4x4 rotation;
    explicit Viewport3D(QWidget *parent, ViewportType viewportType);
    ~Viewport3D();
    virtual void showHideButtons(bool isShow) override;
    void updateVolumeTexture();
    static bool showBoundariesInUm;

public slots:
    void zoomIn() override { zoom(zoomStep()); }
    void zoomOut() override { zoom(1.f/zoomStep()); }
};

#endif // VIEWPORT3D_H
