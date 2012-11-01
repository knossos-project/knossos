#ifndef SKELETONIZER_H
#define SKELETONIZER_H

#include<GL/glut.h>
#include <QObject>
#include"knossos-global.h"


class Skeletonizer : public QObject
{
    Q_OBJECT
public:
    uint32_t skeletonRevision;

    /*
     *  skeletonTime is the time spent on the current skeleton in all previous
     *  instances of knossos that worked with the skeleton.
     *  skeletonTimeCorrection is the time that has to be subtracted from
     *  SDL_GetTicks() to yield the number of milliseconds the current skeleton
     *  was loaded in the current knossos instance.
     *
     */

    int32_t unsavedChanges;
    int32_t skeletonTime;
    int32_t skeletonTimeCorrection;

    int32_t idleTimeTicksOffset;
    int32_t idleTimeLoadOffset;
    int32_t idleTimeSession;
    int32_t idleTime;
    int32_t idleTimeNow;
    int32_t idleTimeLast;

    Hashtable *skeletonDCs;
    treeListElement *firstTree;
    treeListElement *activeTree;
    nodeListElement *activeNode;

    commentListElement *currentComment;
    char *commentBuffer;
    char *searchStrBuffer;

    stack *branchStack;

    dynArray *nodeCounter;
    dynArray *nodesByNodeID;

    uint32_t skeletonDCnumber;
    uint32_t workMode;
    uint32_t volBoundary;

    uint32_t numberComments;

    int lockPositions;
    uint32_t positionLocked;
    char onCommentLock[1024];
    Coordinate lockedPosition;
    long unsigned int lockRadius;

    int32_t rotateX;
    int32_t rotateY;
    int32_t rotateZ;

    int32_t definedSkeletonVpView;

    float translateX, translateY;

    /* Display list, which renders the skeleton defined in skeletonDisplayMode
    (may be same as in displayListSkeletonSkeletonizerVPSlicePlaneVPs */
    GLuint displayListSkeletonSkeletonizerVP;
    /* Display list, which renders the skeleton of the slice plane VPs */
    GLuint displayListSkeletonSlicePlaneVP;
    /* Display list, which applies the basic openGL operations before the skeleton is rendered.
    (Light settings, view rotations, translations...) */
    GLuint displayListView;
    /* Display list, which renders the 3 orthogonal slice planes, the coordinate axes, and so on */
    GLuint displayListDataset;

    /* Stores the model view matrix for user performed VP rotations.*/
    float skeletonVpModelView[16];

    /* The next three flags cause recompilation of the above specified display lists. */

    //TRUE, if all display lists must be updated
    int skeletonChanged;
    //TRUE, if the view on the skeleton changed
    int viewChanged;
    //TRUE, if dataset parameters (size, ...) changed
    int datasetChanged;
    //TRUE, if only displayListSkeletonSlicePlaneVP must be updated.
    int skeletonSliceVPchanged;

    //uint32_t skeletonDisplayMode;
    uint32_t displayMode;

    float segRadiusToNodeRadius;
    int32_t overrideNodeRadiusBool;
    float overrideNodeRadiusVal;

    int highlightActiveTree;
    int showIntersections;
    int rotateAroundActiveNode;
    int showXYplane;
    int showXZplane;
    int showYZplane;
    int showNodeIDs;
    int32_t autoFilenameIncrementBool;

    int32_t treeElements;
    int32_t totalNodeElements;
    int32_t totalSegmentElements;

    int32_t greatestNodeID;
    int32_t greatestTreeID;

    //If TRUE, loadSkeleton merges the current skeleton with the provided
    int mergeOnLoadFlag;

    uint32_t lastSaveTicks;
    int32_t autoSaveBool;
    uint32_t autoSaveInterval;
    uint32_t saveCnt;
    char *skeletonFile;
    char * prevSkeletonFile;

    char *deleteSegment;

    float defaultNodeRadius;

    // Current zoom level. 0: no zoom; near 1: maximum zoom.
    float zoomLevel;

    int branchpointUnresolved;

    /* This is for a workaround around agar bug #171*/
    int askingPopBranchConfirmation;
    char skeletonCreatedInVersion[32];


    //methods
    explicit Skeletonizer(QObject *parent = 0);

    static bool initSkeletonizer();
    static bool UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype);
    static uint32_t UI_addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int32_t makeNodeActive);
    static bool updateSkeletonState();
    static bool nextCommentlessNode();
    static bool previousCommentlessNode();

    static bool updateSkeletonFileName(int32_t targetRevision, int32_t increment, char *filename);
    //uint32_t saveNMLSkeleton();
    static int32_t saveSkeleton();
    //uint32_t loadNMLSkeleton();
    static bool loadSkeleton();

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
                    Byte VPType,
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
    static bool addComment(int32_t targetRevision, char *content, nodeListElement *node, int32_t nodeID);
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

signals:

public slots:

};

#endif // SKELETONIZER_H
