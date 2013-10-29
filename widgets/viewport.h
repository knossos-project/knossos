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
#include <QFont>
#include "eventmodel.h"

static int focus; /* This variable is needed to distinguish the viewport in case of key events. Needed for OSX, don´t remove */

class QPushButton;
class Renderer;

class ResizeButton : public QPushButton {
public:
    static const int SIZE = 22;
    explicit ResizeButton(QWidget *parent);
protected:
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
};

class Viewport : public QGLWidget
{
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent, int viewportType);
    void drawViewport(int viewportType);
    void drawSkeletonViewport();
    void reset();
    void hideButtons();
    void showButtons();
    Renderer *reference;
    EventModel *delegate;

    static const int MIN_VP_SIZE = 50;
    static const int SKELVPBUTTON_W = 35;
    static const int SKELVPBUTTON_H = 20;

protected:
    void initializeGL();
    void initializeOverlayGL();
    void resizeGL(int w, int h);
    void paintGL();
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void enterEvent(QEvent *event);
    //void leaveEvent(QEvent *event);

    int xrel(int x);
    int yrel(int y);
    int viewportType; // XY_VIEWPORT, ...
    int lastX; //last x position
    int lastY; //last y position

    bool entered;
    QPushButton *resizeButton;
    QPushButton *xyButton, *xzButton, *yzButton, *r90Button, *r180Button, *resetButton;

private:
    bool resizeButtonHold;
    void resizeVP(QMouseEvent *event);
    void updateButtonPositions();
    void moveVP(QMouseEvent *event);
    bool handleMouseButtonLeft(QMouseEvent *event, int VPfound);
    bool handleMouseButtonMiddle(QMouseEvent *event, int VPfound);
    bool handleMouseButtonRight(QMouseEvent *event, int VPfound);
    bool handleMouseMotionLeftHold(QMouseEvent *event, int VPfound);
    bool handleMouseMotionMiddleHold(QMouseEvent *event, int VPfound);
    bool handleMouseMotionRightHold(QMouseEvent *event, int VPfound);
    bool handleMouseWheelForward(QWheelEvent *event, int VPfound);
    bool handleMouseWheelBackward(QWheelEvent *event, int VPfound);    
signals:    
    void recalcTextureOffsetsSignal();
    void runSignal();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void updateZoomAndMultiresWidget();
    void loadSkeleton(const QString &path);
public slots:
    void zoomOrthogonals(float step);
    void resizeButtonClicked();
    void xyButtonClicked();
    void xzButtonClicked();
    void yzButtonClicked();
    void r90ButtonClicked();
    void r180ButtonClicked();
    void resetButtonClicked();
};

#endif // VIEWPORT_H
