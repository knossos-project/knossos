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
    bool handleMouseButtonLeft(QMouseEvent *event, int VPfound);
    bool handleMouseButtonMiddle(QMouseEvent *event, int VPfound);
    bool handleMouseButtonRight(QMouseEvent *event, int VPfound);
    bool handleMouseMotion(QMouseEvent *event, int VPfound);
    bool handleMouseMotionLeftHold(QMouseEvent *event, int VPfound);
    bool handleMouseMotionMiddleHold(QMouseEvent *event, int VPfound);
    bool handleMouseMotionRightHold(QMouseEvent *event, int VPfound);
    bool handleMouseWheelForward(QWheelEvent *event, int VPfound);
    bool handleMouseWheelBackward(QWheelEvent *event, int VPfound);
    bool handleKeyboard(QKeyEvent *event, int VPfound);
    static Coordinate *getCoordinateFromOrthogonalClick(QMouseEvent *event, int VPfound);

    int xrel(int x);
    int yrel(int y);
    int mouseX;
    int mouseY;
protected:

signals:
    void rerender();
    void userMoveSignal(int x, int y, int z, int serverMovement);
    void updatePositionSignal(int serverMovement);
    void pasteCoordinateSignal();
    void zoomOrthoSignal(float step);
    void updateViewerStateSignal();
    // SIGNAL MIT RÜCKGABEWERT?
    uint retrieveVisibleObjectBeneathSquareSignal(uint currentVP, uint x, uint y, uint width);
    void updateWidgetSignal();
    void workModeAddSignal();
    void workModeLinkSignal();
    void deleteActiveNodeSignal();
    void genTestNodesSignal(uint number);
    void addSkeletonNodeSignal(Coordinate *clickedCoordinate, Byte VPtype);
    // SIGNAL WITH RETURN VALUE ???
    void addSkeletonNodeAndLinkWithActiveSignal(Coordinate *clickedCoordinate, Byte VPtype, int makeNodeActive);
    void setActiveNodeSignal(int targetRevision, nodeListElement *node, int nodeID);
    void setRemoteStateTypeSignal(int type);
    void setRecenteringPositionSignal(int x, int y, int z);
    void nextCommentSignal(char *searchString);
    void previousCommentSignal(char *searchString);
    void nextCommentlessNodeSignal();
    void previousCommentlessNodeSignal();
    void delSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel);
    void editNodeSignal(int targetRevision, int nodeID, nodeListElement *node, float newRadius, int newXPos, int newYPos, int newZPos, int inMag);
    void addCommentSignal(int targetRevision, const char *content, nodeListElement *node, int nodeID);
    bool editCommentSignal(int targetRevision, commentListElement *currentComment, int nodeID, char *newContent, nodeListElement *newNode, int newNodeID);
    void drawGUISignal();
    void addSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID);
    void jumpToActiveNodeSignal();
    void saveSkeletonSignal();
    void saveSkelCallbackSignal();
    void idleTimeSignal();
public slots:
    
};

#endif // EVENTMODEL_H
