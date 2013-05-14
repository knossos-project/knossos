#ifndef SKELETONIZER_H
#define SKELETONIZER_H

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

#include <QtOpenGL>
#include <QObject>
#include "knossos-global.h"
#include "viewport.h"


class Skeletonizer : public QObject
{
    Q_OBJECT
public:
    explicit Skeletonizer(QObject *parent = 0);

    static nodeListElement *findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition);
    static nodeListElement *findNodeInRadius(Coordinate searchPosition);
    static nodeListElement *getNodeWithPrevID(nodeListElement *currentNode);
    static nodeListElement *getNodeWithNextID(nodeListElement *currentNode);
    static nodeListElement *findNodeByNodeID(int32_t nodeID);
    static nodeListElement *findNodeByCoordinate(Coordinate *position);
    static treeListElement *addTreeListElement(int32_t sync, int32_t targetRevision, int32_t treeID, color4F color);
    static treeListElement *getTreeWithPrevID(treeListElement *currentTree);
    static treeListElement *getTreeWithNextID(treeListElement *currentTree);

    void setViewportReferences(Viewport *vp, Viewport*vp2, Viewport*vp3, Viewport*vp4);
    Viewport *vp, *vp2, *vp3, *vp4;
    static uint32_t UI_addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int32_t makeNodeActive);

    static bool setActiveTreeByID(int32_t treeID);
    static int32_t addNode(int32_t targetRevision,
                    int32_t nodeID,
                    float radius,
                    int32_t treeID,
                    Coordinate *position,
                    Byte VPtype,
                    int32_t inMag,
                    int32_t time,
                    int32_t respectLocks);
    static bool addSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID);
    static bool mergeTrees(int32_t targetRevision, int32_t treeID1, int32_t treeID2);

    static segmentListElement *findSegmentByNodeIDs(int32_t sourceNodeID, int32_t targetNodeID);

    static void *popStack(stack *stack);
    static bool pushStack(stack *stack, void *element);
    static stack *newStack(int32_t size);
    static bool delStack(stack *stack);
    static bool delDynArray(dynArray *array);
    static void *getDynArray(dynArray *array, int32_t pos);
    static bool setDynArray(dynArray *array, int32_t pos, void *value);
    static dynArray *newDynArray(int32_t size);
    static int32_t splitConnectedComponent(int32_t targetRevision, int32_t nodeID);

    static bool popBranchNode(int32_t targetRevision);
    static bool pushBranchNode(int32_t targetRevision, int32_t setBranchNodeFlag, int32_t checkDoubleBranchpoint, nodeListElement *branchNode, int32_t branchNodeID);

    static void UI_popBranchNode();
    static void restoreDefaultTreeColor();
    static bool updateTreeColors();
    static bool delSegmentFromSkeletonStruct(segmentListElement *segment);
    static nodeListElement *addNodeListElement(int32_t nodeID, float radius, nodeListElement **currentNode, Coordinate *position, int32_t inMag);
    static segmentListElement* addSegmentListElement (segmentListElement **currentSegment, nodeListElement *sourceNode, nodeListElement *targetNode);
    static treeListElement* findTreeByTreeID(int32_t treeID);
    static bool addNodeToSkeletonStruct(nodeListElement *node);
    static bool addSegmentToSkeletonStruct(segmentListElement *segment);
    static void WRAP_popBranchNode();

protected:
    int32_t saveSkeleton();
    void setDefaultSkelFileName();
    bool searchInComment(char *searchString, commentListElement *comment);
    bool jumpToActiveNode();
    bool loadSkeleton();
    void popBranchNodeCanceled();
    bool delNodeFromSkeletonStruct(nodeListElement *node);
    bool updateCircRadius(struct nodeListElement *node);
signals:
    void updatePositionSignal(int32_t serverMovement);
    void refreshViewportsSignal();
   void drawGUISignal();
public slots:
    bool delTree(int32_t targetRevision, int32_t treeID);
    bool delActiveTree();
    bool clearSkeleton(int32_t targetRevision, int loadingSkeleton);
    bool delActiveNode();
    bool updateSkeletonFileName(int32_t targetRevision, int32_t increment, char *filename);
    bool updateSkeletonState();
    bool genTestNodes(uint32_t number);
    bool UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype);
    static bool setActiveNode(int32_t targetRevision, nodeListElement *node, int32_t nodeID);
    bool addTreeComment(int32_t targetRevision, int32_t treeID, char *comment);
    bool setSkeletonWorkMode(int32_t targetRevision, uint32_t workMode);
    bool unlockPosition();
    bool lockPosition(Coordinate lockCoordinate);
    commentListElement *nextComment(char *searchString);
    commentListElement *previousComment(char *searchString);
    bool previousCommentlessNode();
    bool nextCommentlessNode();
    bool delSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID, segmentListElement *segToDel);
    bool editNode(int32_t targetRevision, int32_t nodeID, nodeListElement *node, float newRadius, int32_t newXPos, int32_t newYPos, int32_t newZPos, int32_t inMag);
    bool delNode(int32_t targetRevision, int32_t nodeID, nodeListElement *nodeToDel);
    static bool addComment(int32_t targetRevision, const char *content, nodeListElement *node, int32_t nodeID);
    bool editComment(int32_t targetRevision, commentListElement *currentComment, int32_t nodeID, char *newContent, nodeListElement *newNode, int32_t newNodeID);
    bool delComment(int32_t targetRevision, commentListElement *currentComment, int32_t commentNodeID);

};

#endif // SKELETONIZER_H
