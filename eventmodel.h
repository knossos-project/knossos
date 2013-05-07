#ifndef EVENTMODEL_H
#define EVENTMODEL_H

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

#include <QObject>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <stdint.h>
#include <math.h>

#include "knossos-global.h"
#include "renderer.h"

/**
  * @class EventModel
  * @brief This is the EventHandler from Knossos 3.2 adjusted for the QT version
  *
  * The class adopts the core functionality from EventHandler with the exception that
  * SDL Events are replaced through the corresponding QT-Events
  *
  * @todo The calculation in which viewport the event occured can be removed, since Viewport events know their corresponding Viewport.
  *
  * @warning The api documentation for event-handlig in QT 5 seems to be outdated :
  * Keyboard modifier like SHIFT, ALT and CONTROl has to be requested from QApplication
  * If two modifier are pressed at the same time the method is to use the testFlag method
  * of the QKeyModifiers object. All other variants were not successful.
  * If new functionality has to be integrated which uses modifier just use the way it is done in this class
  * You will save a couple of hours ..
  */
class EventModel : public QObject
{
    Q_OBJECT
public:

    explicit EventModel(QObject *parent = 0);
    bool handleMouseButtonLeft(QMouseEvent *event, int32_t VPfound);
    bool handleMouseButtonMiddle(QMouseEvent *event, int32_t VPfound);
    bool handleMouseButtonRight(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotion(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotionLeftHold(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotionMiddleHold(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotionRightHold(QMouseEvent *event, int32_t VPfound);
    bool handleMouseWheelForward(QWheelEvent *event, int32_t VPfound);
    bool handleMouseWheelBackward(QWheelEvent *event, int32_t VPfound);
    bool handleKeyboard(QKeyEvent *event, int32_t VPfound);
    static Coordinate *getCoordinateFromOrthogonalClick(QMouseEvent *event, int32_t VPfound);

    int xrel(int x);
    int yrel(int y);
    int mouseX;
    int mouseY;
protected:

signals:
    void rerender();
    void userMoveSignal(int32_t x, int32_t y, int32_t z, int32_t serverMovement);
    void updatePositionSignal(int32_t serverMovement);
    void pasteCoordinateSignal();
    void zoomOrthoSignal(float step);
    void updateViewerStateSignal();
    // SIGNAL MIT RÜCKGABEWERT?
    uint32_t retrieveVisibleObjectBeneathSquareSignal(uint32_t currentVP, uint32_t x, uint32_t y, uint32_t width);
    void updateWidgetSignal();
    void workModeAddSignal();
    void workModeLinkSignal();
    void deleteActiveNodeSignal();
    void genTestNodesSignal(uint32_t number);
    void addSkeletonNodeSignal(Coordinate *clickedCoordinate, Byte VPtype);
    // SIGNAL WITH RETURN VALUE ???
    void addSkeletonNodeAndLinkWithActiveSignal(Coordinate *clickedCoordinate, Byte VPtype, int32_t makeNodeActive);
    void setActiveNodeSignal(int32_t targetRevision, nodeListElement *node, int32_t nodeID);
    void setRemoteStateTypeSignal(int32_t type);
    void setRecenteringPositionSignal(int32_t x, int32_t y, int32_t z);
public slots:
    
};

#endif // EVENTMODEL_H
