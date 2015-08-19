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
#include "scriptengine/scripting.h"
#include "stateInfo.h"

#include <QDebug>
#include <QDockWidget>
#include <QFont>
#include <QOpenGLDebugLogger>
#include <QOpenGLWidget>
#include <QPushButton>
#include <QWidget>

class EventModel;

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

/**
  * @struct vpConfig
  * @brief Contains attributes for widget size, screen pixels per data pixels,
  *        as well as flags about user interaction with the widget
  */
struct vpConfig {
    // s*v1 + t*v2 = px
    floatCoordinate n;
    floatCoordinate v1; // vector in x direction
    floatCoordinate v2; // vector in y direction
    floatCoordinate leftUpperPxInAbsPx_float;
    floatCoordinate leftUpperDataPxOnScreen_float;
    int s_max;
    int t_max;

    char * viewPortData;

    viewportTexture texture;

    //The absPx coordinate of the upper left corner pixel of the currently on screen displayed data
    Coordinate leftUpperDataPxOnScreen;

    //This is a bit confusing..the screen coordinate system has always
    //x on the horizontal and y on the verical axis, but the displayed
    //data pixels can have a different axis. Keep this in mind.
    //These values depend on texUnitsPerDataPx (in struct viewportTexture),
    //the current zoom value and the data pixel voxel dimensions.
    float screenPxXPerDataPx;
    float screenPxYPerDataPx;

    float displayedlengthInNmX;
    float displayedlengthInNmY;

    ViewportType type; // type e {VIEWPORT_XY, VIEWPORT_XZ, VIEWPORT_YZ, VIEWPORT_SKELETON, VIEWPORT_ARBITRARY}
    uint id; // id e {VP_UPPERLEFT, VP_LOWERLEFT, VP_UPPERRIGHT, VP_LOWERRIGHT}
    // CORRECT THIS COMMENT TODO BUG
    //lower left corner of viewport in screen pixel coords (max: window borders)
    //we use here the lower left corner, because the openGL intrinsic coordinate system
    //is defined over the lower left window corner. All operations inside the viewports
    //use a coordinate system with lowest coordinates in the upper left corner.
    Coordinate upperLeftCorner;
    //edge length in screen pixel coordinates; only squarish VPs are allowed

    uint edgeLength;

    class nodeListElement *draggedNode;

    /* Stores the current view frustum planes */
    float frustum[6][4];
};

class Viewport;
class ResizeButton : public QPushButton {
    Q_OBJECT
    void mouseMoveEvent(QMouseEvent * event) override;
public:
    explicit ResizeButton(Viewport * parent);
signals:
    void vpResize(const QPoint & globalPos);
};

#include <QOpenGLFunctions_2_0>

class QViewportFloatWidget : public QWidget {
public:
    explicit QViewportFloatWidget(QWidget *parent, int id);
};

class Viewport : public QOpenGLWidget, protected QOpenGLFunctions_2_0 {
    Q_OBJECT
    QOpenGLDebugLogger oglLogger;
    QElapsedTimer timeDBase;
    QElapsedTimer timeFBase;
public:
    const static int numberViewports = 4;
    explicit Viewport(QWidget *parent, ViewportType viewportType, uint newId);
    static void resetTextureProperties();
    void drawViewport(int vpID);
    void drawSkeletonViewport();
    void setDock(bool isDock);
    void showHideButtons(bool isShow);
    bool renderVolumeVP();
    void updateOverlayTexture();
    void updateVolumeTexture();
    void posAdapt();
    void posAdapt(const QPoint & desiredPos);
    void sizeAdapt();
    void sizeAdapt(const QPoint & desiredSize);

    QSize dockSize;
    QPoint dockPos;
    EventModel *eventDelegate;
    static bool arbitraryOrientation;
    static bool oglDebug;
    static bool showNodeComments;
    static bool showBoundariesInUm;
protected:
    void initializeGL() override;
    void createOverlayTextures();
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void enterEvent(QEvent * event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    uint id; // VP_UPPERLEFT, ...
    ViewportType viewportType; // XY_VIEWPORT, ...
    int baseEventX; //last x position
    int baseEventY; //last y position

    bool isDocked;
    bool isFullOrigDocked;
    QWidget *dockParent;
    QViewportFloatWidget *floatParent;
    ResizeButton *resizeButton;
    QPushButton *xyButton, *xzButton, *yzButton, *r90Button, *r180Button, *resetButton;
    QMenu *contextMenu;
private:
    bool resizeButtonHold;
    void resizeVP(const QPoint & globalPos);
    void moveVP(const QPoint & globalPos);
signals:
    void recalcTextureOffsetsSignal();
    void runSignal();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void updateDatasetOptionsWidget();
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
    bool setOrientation(ViewportType orientation);
    void takeSnapshot(const QString & path, const int size, const bool withAxes, const bool withOverlay, const bool withSkeleton, const bool withScale, const bool withVpPlanes);
};

#endif // VIEWPORT_H
