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

#include "knossos-global.h"
#include <QObject>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

/**
  * @class EventModel
  * @brief This is the EventHandler from Knossos 3.2 adjusted for the QT version
  *
  * The class adopts the core functionality from EventHandler with the exception that
  * SDL Events are replaced through the corresponding QT-Events.
  *
  * @warning The api documentation for event-handlig in QT 5 seems to be outdated :
  * Keyboard modifier like SHIFT, ALT and CONTROl has to be requested from QApplication
  * If two modifier are pressed at the same time the method is to use the testFlag method
  * of the QKeyModifiers object. All other variants were not successful.
  * If new functionality has to be integrated which uses modifier just use the way it is done in this class
  * You will save a couple of hours ..
  */

class Renderer;
class EventModel : public QObject
{
    Q_OBJECT
public:
    Renderer *reference;
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
    bool grap;
    QKeyEvent *eventReference;
    int eventCoordinate[3];
    int eventViewport[0];
    bool clicked;
protected:

signals:
    void userMoveSignal(int x, int y, int z, int serverMovement);
    void userMoveArbSignal(float x, float y, float z, int serverMovement);
    void updatePositionSignal(int serverMovement);
    void pasteCoordinateSignal();
    void zoomOrthoSignal(float step);
    void updateViewerStateSignal();

    void updateWidgetSignal();
    void workModeAddSignal();
    void workModeLinkSignal();
    void workModeDropSignal();
    void deleteActiveNodeSignal();
    void genTestNodesSignal(uint number);
    void addSkeletonNodeSignal(Coordinate *clickedCoordinate, Byte VPtype);

    void setActiveNodeSignal(int targetRevision, nodeListElement *node, int nodeID);
    void setRemoteStateTypeSignal(int type);
    void setRecenteringPositionSignal(int x, int y, int z);
    void nextCommentSignal(char *searchString);
    void previousCommentSignal(char *searchString);
    void nextCommentlessNodeSignal();
    void previousCommentlessNodeSignal();
    void delSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel, int serialize);
    void editNodeSignal(int targetRevision, int nodeID, nodeListElement *node, float newRadius, int newXPos, int newYPos, int newZPos, int inMag);
    void addCommentSignal(int targetRevision, const char *content, nodeListElement *node, int nodeID, int serialize);
    bool editCommentSignal(int targetRevision, commentListElement *currentComment, int nodeID, char *newContent, nodeListElement *newNode, int newNodeID, int serialize);
    void addSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, int serialize);
    void jumpToActiveNodeSignal();
    void saveSkeletonSignal();
    void idleTimeSignal();
    void updateTools();
    void updateTreeCountSignal();
    void updateZoomWidgetSignal();
    void updateCommentsTable();
    void updateSlicePlaneWidgetSignal();
    void pushBranchNodeSignal(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, int branchNodeID, int serialize);
    void popBranchNodeSignal(int targetRevision, int serialize);

    void moveToNextTreeSignal();
    void moveToPrevTreeSignal();
    void moveToPrevNodeSignal();
    void moveToNextNodeSignal();

    nodeListElement *findNodeInRadiusSignal(Coordinate searchPosition);
    segmentListElement *findSegmentByNodeIDSignal(int sourceNodeID, int targetNodeID);
    nodeListElement *findNodeByNodeIDSignal(int nodeID);
    uint addSkeletonNodeAndLinkWithActiveSignal(Coordinate *clickedCoordinate, Byte VPtype, int makeNodeActive);
    treeListElement *addTreeListElement(int sync, int targetRevision, int treeID, color4F color, int serialize);

    void undoSignal();
public slots:

};

#endif // EVENTMODEL_H
