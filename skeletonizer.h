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
    static nodeListElement *findNodeByNodeID(int nodeID);
    static nodeListElement *findNodeByCoordinate(Coordinate *position);
    static treeListElement *addTreeListElement(int sync, int targetRevision, int treeID, color4F color);
    static treeListElement *getTreeWithPrevID(treeListElement *currentTree);
    static treeListElement *getTreeWithNextID(treeListElement *currentTree);

    void setViewportReferences(Viewport *vp, Viewport*vp2, Viewport*vp3, Viewport*vp4);
    Viewport *vp, *vp2, *vp3, *vp4;
    static uint addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int makeNodeActive);

    static bool setActiveTreeByID(int treeID);
    static int addNode(int targetRevision,
                    int nodeID,
                    float radius,
                    int treeID,
                    Coordinate *position,
                    Byte VPtype,
                    int inMag,
                    int time,
                    int respectLocks);
    static bool addSegment(int targetRevision, int sourceNodeID, int targetNodeID);
    static bool mergeTrees(int targetRevision, int treeID1, int treeID2);

    static segmentListElement *findSegmentByNodeIDs(int sourceNodeID, int targetNodeID);

    static void *popStack(stack *stack);
    static bool pushStack(stack *stack, void *element);
    static stack *newStack(int size);
    static bool delStack(stack *stack);
    static bool delDynArray(dynArray *array);
    static void *getDynArray(dynArray *array, int pos);
    static bool setDynArray(dynArray *array, int pos, void *value);
    static dynArray *newDynArray(int size);
    static int splitConnectedComponent(int targetRevision, int nodeID);

    static bool popBranchNode(int targetRevision);
    static bool pushBranchNode(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, int branchNodeID);

    static void UI_popBranchNode();
    static void restoreDefaultTreeColor();
    static bool updateTreeColors();
    static bool delSegmentFromSkeletonStruct(segmentListElement *segment);
    static nodeListElement *addNodeListElement(int nodeID, float radius, nodeListElement **currentNode, Coordinate *position, int inMag);
    static segmentListElement* addSegmentListElement (segmentListElement **currentSegment, nodeListElement *sourceNode, nodeListElement *targetNode);
    static treeListElement* findTreeByTreeID(int treeID);
    static bool addNodeToSkeletonStruct(nodeListElement *node);
    static bool addSegmentToSkeletonStruct(segmentListElement *segment);
    static void WRAP_popBranchNode();
    static void setColorFromNode(struct nodeListElement *node, color4F *color);
    static void setRadiusFromNode(struct nodeListElement *node, float *radius);
    bool delSkelState(skeletonState *skelState);
    bool delTreesFromState(skeletonState *skelState);
    bool delTreeFromState(struct treeListElement *treeToDel, struct skeletonState *skelState);
    static bool hasObfuscatedTime();
    bool delNodeFromState(struct nodeListElement *nodeToDel, struct skeletonState *skelState);
    bool delCommentFromState(struct commentListElement *commentToDel, struct skeletonState *skelState);
    bool delSegmentFromCmd(struct segmentListElement *segToDel);
    static bool moveToNextTree();
    static bool moveToPrevTree();
    static bool moveToPrevNode();
    static bool moveToNextNode();
    static unsigned int commentContainsSubstr(struct commentListElement *comment, int index);
    void refreshUndoRedoBuffers();
    void undo();
    void redo();
    cmdListElement *popCmd(struct cmdList *cmdList);
    void pushCmd(struct cmdList *cmdList, struct cmdListElement *cmdListEl);
    void flushCmdList(struct cmdList *cmdList);
    void delCmdListElement(struct cmdListElement *cmdEl);
protected:
    int saveSkeleton();
    void setDefaultSkelFileName();
    bool searchInComment(char *searchString, commentListElement *comment);
    bool loadSkeleton();
    void popBranchNodeCanceled();
    bool delNodeFromSkeletonStruct(nodeListElement *node);
    static bool updateCircRadius(struct nodeListElement *node);
    int xorInt(int xorMe);

signals:
    void updatePositionSignal(int serverMovement);
    void refreshViewportsSignal();
    void drawGUISignal();
public slots:
    bool delTree(int targetRevision, int treeID);
    bool delActiveTree();
    bool clearSkeleton(int targetRevision, int loadingSkeleton);
    bool delActiveNode();
    bool updateSkeletonFileName(int targetRevision, int increment, char *filename);
    bool updateSkeletonState();
    bool genTestNodes(uint number);
    bool UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype);
    static bool setActiveNode(int targetRevision, nodeListElement *node, int nodeID);
    bool addTreeComment(int targetRevision, int treeID, char *comment);
    bool setSkeletonWorkMode(int targetRevision, uint workMode);
    bool unlockPosition();
    bool lockPosition(Coordinate lockCoordinate);
    commentListElement *nextComment(char *searchString);
    commentListElement *previousComment(char *searchString);
    bool previousCommentlessNode();
    bool nextCommentlessNode();
    static bool delSegment(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel);
    bool editNode(int targetRevision, int nodeID, nodeListElement *node, float newRadius, int newXPos, int newYPos, int newZPos, int inMag);
    bool delNode(int targetRevision, int nodeID, nodeListElement *nodeToDel);
    static bool addComment(int targetRevision, const char *content, nodeListElement *node, int nodeID);
    bool editComment(int targetRevision, commentListElement *currentComment, int nodeID, char *newContent, nodeListElement *newNode, int newNodeID);
    bool delComment(int targetRevision, commentListElement *currentComment, int commentNodeID);
    bool jumpToActiveNode();
};

#endif // SKELETONIZER_H
