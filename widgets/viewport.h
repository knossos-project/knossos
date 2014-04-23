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

#include "eventmodel.h"

#include <QWidget>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL>
#include <QDebug>
#include <QFont>
#include <QPushButton>

class Viewport;

enum {VP_UPPERLEFT, VP_LOWERLEFT, VP_UPPERRIGHT, VP_LOWERRIGHT};

class ResizeButton : public QPushButton {
    Q_OBJECT
    void mouseMoveEvent(QMouseEvent * event) override;
public:
    explicit ResizeButton(Viewport * parent);
signals:
    void vpResize(QMouseEvent * event);
};

class Viewport : public QGLWidget
{
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent, QGLWidget *shared, int viewportType, uint newId);
    void drawViewport(int vpID);
    void drawSkeletonViewport();
    void hideButtons();
    void showButtons();
    EventModel *eventDelegate;

protected:
    void initializeGL();
    void initializeOverlayGL();
    void resizeGL(int w, int h);
    void paintGL();
    void enterEvent(QEvent * event);
    void leaveEvent(QEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    uint id; // VP_UPPERLEFT, ...
    int viewportType; // XY_VIEWPORT, ...
    int baseEventX; //last x position
    int baseEventY; //last y position

    ResizeButton *resizeButton;
    QPushButton *xyButton, *xzButton, *yzButton, *r90Button, *r180Button, *resetButton;
    QMenu *contextMenu;
private:
    bool resizeButtonHold;
    void resizeVP(QMouseEvent *event);
    void moveVP(QMouseEvent *event);
signals:    
    void recalcTextureOffsetsSignal();
    void runSignal();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void updateZoomAndMultiresWidget();
    void loadSkeleton(const QString &path);
public slots:
    void zoomOrthogonals(float step);
    void zoomInSkeletonVP();
    void zoomOutSkeletonVP();
    void xyButtonClicked();
    void xzButtonClicked();
    void yzButtonClicked();
    void r90ButtonClicked();
    void r180ButtonClicked();
    void resetButtonClicked();
    bool setOrientation(int orientation);
    void showContextMenu(const QPoint &point);
};

#endif // VIEWPORT_H
