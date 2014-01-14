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
#include <vector>
#include "knossos-global.h"

class TreeWithNodes {
    int treeId;
    QVector<nodeListElement> nodeVector;

};

class Skeletonizer : public QObject
{
    Q_OBJECT

public:
    explicit Skeletonizer(QObject *parent = 0);
    static nodeListElement *findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition);

    static nodeListElement *getNodeWithPrevID(nodeListElement *currentNode, bool sameTree);
    static nodeListElement *getNodeWithNextID(nodeListElement *currentNode, bool sameTree);
    static nodeListElement *findNodeByCoordinate(Coordinate *position);
    static treeListElement *getTreeWithPrevID(treeListElement *currentTree);
    static treeListElement *getTreeWithNextID(treeListElement *currentTree);
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
    static void setColorFromNode(struct nodeListElement *node, color4F *color);
    static void setRadiusFromNode(struct nodeListElement *node, float *radius);
    bool delSkelState(skeletonState *skelState);
    bool delTreesFromState(skeletonState *skelState);
    bool delTreeFromState(struct treeListElement *treeToDel, struct skeletonState *skelState);
    static bool hasObfuscatedTime();
    bool delNodeFromState(struct nodeListElement *nodeToDel, struct skeletonState *skelState);
    bool delCommentFromState(struct commentListElement *commentToDel, struct skeletonState *skelState);
    bool delSegmentFromCmd(struct segmentListElement *segToDel);
    static unsigned int commentContainsSubstr(struct commentListElement *comment, int index);

    static bool deleteSelectedTrees();

    static char *integerChecksum(int32_t in);
    static bool isObfuscatedTime(int time);

    static void resetSkeletonMeta();

public:
    static void setDefaultSkelFileName();
    bool searchInComment(char *searchString, commentListElement *comment);
    void popBranchNodeCanceled();
    bool popBranchNode(int targetRevision, int serialize);
    static bool delNodeFromSkeletonStruct(nodeListElement *node);
    static bool updateCircRadius(struct nodeListElement *node);
    static int xorInt(int xorMe);    
    uint skeletonRevision;

    //    skeletonTime is the time spent on the current skeleton in all previous
    //    instances of knossos that worked with the skeleton.
    //    skeletonTimeCorrection is the time that has to be subtracted from
    //    SDL_GetTicks() to yield the number of milliseconds the current skeleton
    //    was loaded in the current knossos instance.

    bool unsavedChanges;
    int skeletonTime;
    int skeletonTimeCorrection;

    int idleTimeSession;
    int idleTime;
    int idleTimeNow;
    int idleTimeLast;

    Hashtable *skeletonDCs;
    struct treeListElement *firstTree;
    struct treeListElement *activeTree;
    struct nodeListElement *activeNode;

    struct commentListElement *currentComment;
    char *commentBuffer;
    char *searchStrBuffer;

    struct stack *branchStack;

    struct dynArray *nodeCounter;
    struct dynArray *nodesByNodeID;

    uint skeletonDCnumber;
    uint workMode;
    uint volBoundary;

    uint totalComments;

    unsigned int userCommentColoringOn;
    unsigned int commentNodeRadiusOn;

    bool lockPositions;
    bool positionLocked;
    char onCommentLock[1024];
    Coordinate lockedPosition;
    long unsigned int lockRadius;

    float rotdx;
    float rotdy;
    int rotationcounter;

    int definedSkeletonVpView;

    float translateX, translateY;

    // Display list,
    //which renders the skeleton defined in skeletonDisplayMode
    //(may be same as in displayListSkeletonSkeletonizerVPSlicePlaneVPs
    GLuint displayListSkeletonSkeletonizerVP;
    // Display list, which renders the skeleton of the slice plane VPs
    GLuint displayListSkeletonSlicePlaneVP;
    // Display list, which applies the basic openGL operations before the skeleton is rendered.
    //(Light settings, view rotations, translations...)
    GLuint displayListView;
    // Display list, which renders the 3 orthogonal slice planes, the coordinate axes, and so on
    GLuint displayListDataset;

    // Stores the model view matrix for user performed VP rotations.
    float skeletonVpModelView[16];

    // Stores the angles of the cube in the SkeletonVP
    float rotationState[16];
    // The next three flags cause recompilation of the above specified display lists.

    //true, if all display lists must be updated
    bool skeletonChanged;
    //true, if the view on the skeleton changed
    bool viewChanged;
    //true, if dataset parameters (size, ...) changed
    bool datasetChanged;
    //true, if only displayListSkeletonSlicePlaneVP must be updated.
    bool skeletonSliceVPchanged;

    //uint skeletonDisplayMode;
    uint displayMode;

    float segRadiusToNodeRadius;
    int overrideNodeRadiusBool;
    float overrideNodeRadiusVal;

    int highlightActiveTree;
    int showIntersections;
    int rotateAroundActiveNode;
    int showXYplane;
    int showXZplane;
    int showYZplane;
    int showNodeIDs;
    bool autoFilenameIncrementBool;

    int treeElements;
    int totalNodeElements;
    int totalSegmentElements;

    int greatestNodeID;
    int greatestTreeID;

    color4F commentColors[NUM_COMMSUBSTR];
    float commentNodeRadii[NUM_COMMSUBSTR];

    //If true, loadSkeleton merges the current skeleton with the provided
    int mergeOnLoadFlag;

    uint lastSaveTicks;
    bool autoSaveBool;
    uint autoSaveInterval;
    uint saveCnt;
    char *skeletonFile;
    char * prevSkeletonFile;

    char *deleteSegment;

    float defaultNodeRadius;

    // Current zoom level. 0: no zoom; near 1: maximum zoom.
    float zoomLevel;

    // temporary vertex buffers that are available for rendering, get cleared
    // every frame */
    mesh lineVertBuffer; /* ONLY for lines */
    mesh pointVertBuffer; /* ONLY for points */

    bool branchpointUnresolved;

    // This is for a workaround around agar bug #171
    bool askingPopBranchConfirmation;
    char skeletonCreatedInVersion[32];

    struct cmdList *undoList;
    struct cmdList *redoList;

signals:
    void idleTimeSignal();
    void updatePositionSignal(int serverMovement);        
    void saveSkeletonSignal(int increment);
    void updateToolsSignal();
    void updateTreeviewSignal();
    void userMoveSignal(int x, int y, int z, int serverMovement);
    void setRemoteStateTypeSignal(int type);
    void setRecenteringPositionSignal(int x, int y, int z);
    void displayModeChangedSignal();

public slots:
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
    static bool addTreeComment(int targetRevision, int treeID, char *comment);
    bool setSkeletonWorkMode(int targetRevision, uint workMode);
    static bool unlockPosition();
    static bool lockPosition(Coordinate lockCoordinate);
    commentListElement *nextComment(char *searchString);
    commentListElement *previousComment(char *searchString);
    bool previousCommentlessNode();
    bool nextCommentlessNode();
    static bool delSegment(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel, int serialize);
    static bool editNode(int targetRevision, int nodeID, nodeListElement *node, float newRadius, int newXPos, int newYPos, int newZPos, int inMag);
    static bool delNode(int targetRevision, int nodeID, nodeListElement *nodeToDel, int serialize);
    static bool addComment(int targetRevision, const char *content, nodeListElement *node, int nodeID, int serialize);
    static bool editComment(int targetRevision, commentListElement *currentComment, int nodeID, char *newContent, nodeListElement *newNode, int newNodeID, int serialize);
    static bool delComment(int targetRevision, commentListElement *currentComment, int commentNodeID, int serialize);
    bool jumpToActiveNode();    
    static bool setActiveTreeByID(int treeID);

    /* Slots which manipulates attributes */
    void setZoomLevel(float value);
    void setShowIntersections(bool on);
    void setShowXyPlane(bool on);
    void setRotateAroundActiveNode(bool on);

    void setOverrideNodeRadius(bool on);
    void setSegRadiusToNodeRadius(float value);
    void setHighlightActiveTree(bool on);
    void setSkeletonChanged(bool on);
    void setShowNodeIDs(bool on);

    bool loadXmlSkeleton(QString fileName);
    bool saveXmlSkeleton(QString fileName);

    static bool pushBranchNode(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, int branchNodeID, int serialize);
    bool moveToNextTree();
    bool moveToPrevTree();
    bool moveToPrevNode();
    bool moveToNextNode();

    static treeListElement* findTreeByTreeID(int treeID);
    static nodeListElement *findNodeByNodeID(int nodeID);
    static bool addSegment(int targetRevision, int sourceNodeID, int targetNodeID, int serialize);
    static void restoreDefaultTreeColor(treeListElement *tree);
    static void restoreDefaultTreeColor();

    static int splitConnectedComponent(int targetRevision, int nodeID, int serialize);
    static treeListElement *addTreeListElement(int sync, int targetRevision, int treeID, color4F color, int serialize);
    static bool mergeTrees(int targetRevision, int treeID1, int treeID2, int serialize);
    static bool updateTreeColors();
    static nodeListElement *findNodeInRadius(Coordinate searchPosition);
    static segmentListElement *findSegmentByNodeIDs(int sourceNodeID, int targetNodeID);
    uint addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int makeNodeActive);

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
};

#endif // SKELETONIZER_H
