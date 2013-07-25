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

#include <QWidget>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL>
#include <QDebug>
#include "renderer.h"
#include "eventmodel.h"

class QPushButton;
class Viewport : public QGLWidget
{
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent, int plane);
    void drawViewport(int plane);
    void drawSkeletonViewport();
    EventModel *delegate;

protected:
    void initializeGL();
    void initializeOverlayGL();
    void resizeGL(int w, int h);
    void paintGL();
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void customEvent(QEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void paintEvent(QPaintEvent *event);
    int xrel(int x);
    int yrel(int y);
    int plane; // XY_VIEWPORT, ...
    int lastX; //last x position
    int lastY; //last y position
    void drawButtons();

    bool entered;

private:
    bool handleMouseButtonLeft(QMouseEvent *event, int VPfound);
    bool handleMouseButtonMiddle(QMouseEvent *event, int VPfound);
    bool handleMouseButtonRight(QMouseEvent *event, int VPfound);
    bool handleMouseMotionLeftHold(QMouseEvent *event, int VPfound);
    bool handleMouseMotionMiddleHold(QMouseEvent *event, int VPfound);
    bool handleMouseMotionRightHold(QMouseEvent *event, int VPfound);
    bool handleMouseWheelForward(QWheelEvent *event, int VPfound);
    bool handleMouseWheelBackward(QWheelEvent *event, int VPfound);
    bool handleKeyboard(QKeyEvent *event);
    static Coordinate *getCoordinateFromOrthogonalClick(QMouseEvent *event, int VPfound);

signals:
    void renderOrthogonalVPSignal(int plane);
    void renderSkeletonVPSignal(int plane);
    void recalcTextureOffsetsSignal();
    void runSignal();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void updateZoomAndMultiresWidget();
public slots:
    void zoomOrthogonals(float step);
};

#endif // VIEWPORT_H
