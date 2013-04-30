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
//#include<GL/glut.h>
#include <QObject>
#include"knossos-global.h"


class Skeletonizer : public QObject
{
    Q_OBJECT
public:
    explicit Skeletonizer(QObject *parent = 0);
    bool updateSkeletonState();

    static bool UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype);
    static uint32_t UI_addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int32_t makeNodeActive);
    static bool nextCommentlessNode();
    static bool previousCommentlessNode();

    static bool updateSkeletonFileName(int32_t targetRevision, int32_t increment, char *filename);
    //uint32_t saveNMLSkeleton();
    static int32_t saveSkeleton();
    //uint32_t loadNMLSkeleton();
    bool loadSkeleton();

    static void setDefaultSkelFileName();

    static bool delActiveNode();
    static bool delActiveTree();

    static bool delSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID, segmentListElement *segToDel);
    static bool delNode(int32_t targetRevision, int32_t nodeID, nodeListElement *nodeToDel);
    static bool delTree(int32_t targetRevision, int32_t treeID);

    static nodeListElement *findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition);
    static nodeListElement *findNodeInRadius(Coordinate searchPosition);

    static bool setActiveTreeByID(int32_t treeID);
    static bool setActiveNode(int32_t targetRevision, nodeListElement *node, int32_t nodeID);
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

    static bool clearSkeleton(int32_t targetRevision, int loadingSkeleton);

    static bool mergeTrees(int32_t targetRevision, int32_t treeID1, int32_t treeID2);

    static nodeListElement *getNodeWithPrevID(nodeListElement *currentNode);
    static nodeListElement *getNodeWithNextID(nodeListElement *currentNode);
    static nodeListElement *findNodeByNodeID(int32_t nodeID);
    static nodeListElement *findNodeByCoordinate(Coordinate *position);
    static treeListElement *addTreeListElement(int32_t sync, int32_t targetRevision, int32_t treeID, color4F color);
    static treeListElement* getTreeWithPrevID(treeListElement *currentTree);
    static treeListElement* getTreeWithNextID(treeListElement *currentTree);
    static bool addTreeComment(int32_t targetRevision, int32_t treeID, char *comment);
    static segmentListElement *findSegmentByNodeIDs(int32_t sourceNodeID, int32_t targetNodeID);
    static bool genTestNodes(uint32_t number);
    static bool editNode(int32_t targetRevision,
                     int32_t nodeID,
                     nodeListElement *node,
                     float newRadius,
                     int32_t newXPos,
                     int32_t newYPos,
                     int32_t newZPos,
                     int32_t inMag);
    static void *popStack(stack *stack);
    static bool pushStack(stack *stack, void *element);
    static stack *newStack(int32_t size);
    static bool delStack(stack *stack);
    static bool delDynArray(dynArray *array);
    static void *getDynArray(dynArray *array, int32_t pos);
    static bool setDynArray(dynArray *array, int32_t pos, void *value);
    static dynArray *newDynArray(int32_t size);
    static int32_t splitConnectedComponent(int32_t targetRevision, int32_t nodeID);
    static bool addComment(int32_t targetRevision, const char *content, nodeListElement *node, int32_t nodeID);
    static bool delComment(int32_t targetRevision, commentListElement *currentComment, int32_t commentNodeID);
    static bool editComment(int32_t targetRevision, commentListElement *currentComment, int32_t nodeID, char *newContent, nodeListElement *newNode, int32_t newNodeID);
    static commentListElement *nextComment(char *searchString);
    static commentListElement *previousComment(char *searchString);
    static bool searchInComment(char *searchString, commentListElement *comment);
    static bool unlockPosition();
    static bool lockPosition(Coordinate lockCoordinate);
    static bool popBranchNode(int32_t targetRevision);
    static bool pushBranchNode(int32_t targetRevision, int32_t setBranchNodeFlag, int32_t checkDoubleBranchpoint, nodeListElement *branchNode, int32_t branchNodeID);
    static bool setSkeletonWorkMode(int32_t targetRevision, uint32_t workMode);
    static bool jumpToActiveNode();
    static void UI_popBranchNode();
    static void restoreDefaultTreeColor();
    static bool updateTreeColors();

signals:
    void updatePositionSignal(int32_t serverMovement);
    void refreshViewportsSignal();


};

#endif // SKELETONIZER_H
