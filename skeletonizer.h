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


#include <QObject>
#include <QtXml>
#include "knossos-global.h"

// to skeletonizer
struct dynArray {
    void **elements;
    int end;
    int firstSize;
};

// to skeletonizer
struct serialSkeletonListElement {
    struct serialSkeletonListElement *next;
    struct serialSkeletonListElement *previous;
    Byte* content;
};

// to skeletonizer
struct skeletonDC {
    struct skeletonDCsegment *firstSkeletonDCsegment;
    struct skeletonDCnode *firstSkeletonDCnode;
};

// to skeletonizer
struct skeletonDCnode {
    nodeListElement *node;
    struct skeletonDCnode *next;
};

// to skeletonizer
struct skeletonDCsegment {
    struct segmentListElement *segment;
    struct skeletonDCsegment *next;
};

class Skeletonizer : public QObject
{
    Q_OBJECT

public:
    explicit Skeletonizer(QObject *parent = 0);

public:    
    //    skeletonTime is the time spent on the current skeleton in all previous
    //    instances of knossos that worked with the skeleton.
    //    skeletonTimeCorrection is the time that has to be subtracted from
    //    SDL_GetTicks() to yield the number of milliseconds the current skeleton
    //    was loaded in the current knossos instance.


signals:
    void updatePositionSignal(int serverMovement);        
    void saveSkeletonSignal();
    void updateToolsSignal();
    void updateTreeviewSignal();
    void userMoveSignal(int x, int y, int z, int serverMovement);
    void setRemoteStateTypeSignal(int type);
    void setRecenteringPositionSignal(int x, int y, int z);

public slots:
    static nodeListElement *findNearbyNode(TreeListElement *nearbyTree, Coordinate searchPosition);

    static nodeListElement *getNodeWithPrevID(nodeListElement *currentNode, bool sameTree);
    static nodeListElement *getNodeWithNextID(nodeListElement *currentNode, bool sameTree);
    static nodeListElement *findNodeByCoordinate(Coordinate *position);
    static TreeListElement *getTreeWithPrevID(TreeListElement *currentTree);
    static TreeListElement *getTreeWithNextID(TreeListElement *currentTree);
    static int addNode(int targetRevision,
                    int nodeID,
                    float radius,
                    int treeID,
                    Coordinate *position,
                    Byte VPtype,
                    int inMag,
                    int time,
                    int respectLocks,
                    int serialize);

    static void *popStack(stack *stack);
    static bool pushStack(stack *stack, void *element);
    static stack *newStack(int size);
    static bool delStack(stack *stack);
    static bool delDynArray(dynArray *array);
    static void *getDynArray(dynArray *array, int pos);
    static bool setDynArray(dynArray *array, int pos, void *value);
    static dynArray *newDynArray(int size);

    static bool delSegmentFromSkeletonStruct(segmentListElement *segment);
    static nodeListElement *addNodeListElement(int nodeID, float radius, nodeListElement **currentNode, Coordinate *position, int inMag);
    static segmentListElement* addSegmentListElement (segmentListElement **currentSegment, nodeListElement *sourceNode, nodeListElement *targetNode);

    static bool addNodeToSkeletonStruct(nodeListElement *node);
    static bool addSegmentToSkeletonStruct(segmentListElement *segment);
    void WRAP_popBranchNode();
    static void setColorFromNode(nodeListElement *node, Color4F *color);
    static void setRadiusFromNode(nodeListElement *node, float *radius);
    bool delskeletonState(skeletonState *skeletonState);
    bool delTreesFromState(skeletonState *skeletonState);
    bool delTreeFromState(TreeListElement *treeToDel, skeletonState *skeletonState);
    static bool hasObfuscatedTime();
    bool delNodeFromState(nodeListElement *nodeToDel, skeletonState *skeletonState);
    bool delCommentFromState(struct commentListElement *commentToDel, skeletonState *skeletonState);
    bool delSegmentFromCmd(struct segmentListElement *segToDel);
    static unsigned int commentContainsSubstr(struct commentListElement *comment, int index);

    bool deleteSelectedTrees();
    void deleteSelectedNodes();

    static char *integerChecksum(int32_t in);
    static bool isObfuscatedTime(int time);

    static void resetSkeletonMeta();

    void UI_popBranchNode();
    static bool delTree(int targetRevision, int treeID, int serialize);
    static bool delActiveTree();
    static bool clearSkeleton(int targetRevision, int loadingSkeleton);
    static bool delActiveNode();
    bool updateSkeletonFileName(int targetRevision, int increment, char *filename);
    bool updateSkeletonState();
    bool genTestNodes(uint number);
    bool UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype);
    static bool setActiveNode(int targetRevision, nodeListElement *node, int nodeID);
    static bool addTreeComment(int targetRevision, int treeID, QString comment);
    bool setSkeletonWorkMode(int targetRevision, uint workMode);
    static bool unlockPosition();
    static bool lockPosition(Coordinate lockCoordinate);
    commentListElement *nextComment(QString searchString);
    commentListElement *previousComment(QString searchString);
    bool previousCommentlessNode();
    bool nextCommentlessNode();
    static bool delSegment(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel, int serialize);
    static bool editNode(int targetRevision, int nodeID, nodeListElement *node, float newRadius, int newXPos, int newYPos, int newZPos, int inMag);
    static bool delNode(int targetRevision, int nodeID, nodeListElement *nodeToDel, int serialize);
    static bool addComment(int targetRevision, QString content, nodeListElement *node, int nodeID, int serialize);
    static bool editComment(int targetRevision, commentListElement *currentComment, int nodeID, QString newContent, nodeListElement *newNode, int newNodeID, int serialize);
    static bool delComment(int targetRevision, commentListElement *currentComment, int commentNodeID, int serialize);
    bool jumpToActiveNode();    
    static bool setActiveTreeByID(int treeID);

    bool loadXmlSkeleton(QString fileName);
    bool saveXmlSkeleton(QString fileName);

    static bool pushBranchNode(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, int branchNodeID, int serialize);
    bool moveToNextTree();
    bool moveToPrevTree();
    bool moveToPrevNode();
    bool moveToNextNode();
    static bool moveNodeToTree(nodeListElement *node, int treeID);
    static TreeListElement* findTreeByTreeID(int treeID);
    static nodeListElement *findNodeByNodeID(int nodeID);
    static bool addSegment(int targetRevision, int sourceNodeID, int targetNodeID, int serialize);
    static void restoreDefaultTreeColor(TreeListElement *tree);
    static void restoreDefaultTreeColor();

    static int splitConnectedComponent(int targetRevision, int nodeID, int serialize);
    static TreeListElement *addTreeListElement(int sync, int targetRevision, int treeID, Color4F color, int serialize);
    static bool mergeTrees(int targetRevision, int treeID1, int treeID2, int serialize);
    static bool updateTreeColors();
    static nodeListElement *findNodeInRadius(Coordinate searchPosition);
    static segmentListElement *findSegmentByNodeIDs(int sourceNodeID, int targetNodeID);
    uint addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int makeNodeActive);

    static QString getDefaultSkelFileName();
    bool searchInComment(char *searchString, commentListElement *comment);
    void popBranchNodeCanceled();
    bool popBranchNode(int targetRevision, int serialize);
    static bool delNodeFromSkeletonStruct(nodeListElement *node);
    static bool updateCircRadius(nodeListElement *node);
    static int xorInt(int xorMe);

    static void saveSerializedSkeleton();
    void undo();
    void redo();
    static Byte *serializeSkeleton();
    void deserializeSkeleton();
    void deleteLastSerialSkeleton();

    static int getTreeBlockSize();
    static int getNodeBlockSize();
    static int getSegmentBlockSize();
    static int getCommentBlockSize();
    static int getVariableBlockSize();
    static int getBranchPointBlockSize();

    void setZoomLevel(float value);
    void setShowIntersections(bool on);
    void setShowXyPlane(bool on);
    void setRotateAroundActiveNode(bool on);

    void setOverrideNodeRadius(bool on);
    void setSegRadiusToNodeRadius(float value);
    void setHighlightActiveTree(bool on);
    void setSkeletonChanged(bool on);
    void setShowNodeIDs(bool on);


};

#endif // SKELETONIZER_H
