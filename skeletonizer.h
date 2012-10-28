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

    bool initSkeletonizer();
    bool UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype);
    uint32_t UI_addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int32_t makeNodeActive);
    bool updateSkeletonState();
    bool nextCommentlessNode();
    bool previousCommentlessNode();

    bool updateSkeletonFileName();
    //uint32_t saveNMLSkeleton();
    int32_t saveSkeleton();
    //uint32_t loadNMLSkeleton();
    bool loadSkeleton();

    void setDefaultSkelFileName();

    bool delActiveNode();
    bool delActiveTree();

    bool delSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID, segmentListElement *segToDel);
    bool delNode(int32_t targetRevision, int32_t nodeID, nodeListElement *nodeToDel);
    bool delTree(int32_t targetRevision, int32_t treeID);

    nodeListElement *findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition);
    nodeListElement *findNodeInRadius(Coordinate searchPosition);

    bool setActiveTreeByID(int32_t treeID);
    bool setActiveNode(int32_t targetRevision, nodeListElement *node, int32_t nodeID);
    int32_t addNode(int32_t targetRevision,
                    int32_t nodeID,
                    float radius,
                    int32_t treeID,
                    Coordinate *position,
                    Byte VPType,
                    int32_t inMag,
                    int32_t time,
                    int32_t respectLocks);
    bool addSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID);

    bool clearSkeleton(int32_t targetRevision, int loadingSkeleton);

    bool mergeTrees(int32_t targetRevision, int32_t treeID1, int32_t treeID2);

    nodeListElement *getNodeWithPrevID(nodeListElement *currentNode);
    nodeListElement *getNodeWithNextID(nodeListElement *currentNode);
    nodeListElement *findNodeByNodeID(int32_t nodeID);
    nodeListElement *findNodeByCoordinate(Coordinate *position);
    treeListElement *addTreeListElement(int32_t sync, int32_t targetRevision, int32_t treeID, color4F color);
    treeListElement* getTreeWithPrevID(treeListElement *currentTree);
    treeListElement* getTreeWithNextID(treeListElement *currentTree);
    bool addTreeComment(int32_t targetRevision, int32_t treeID, char *comment);
    segmentListElement *findSegmentByNodeIDs(int32_t sourceNodeID, int32_t targetNodeID);
    bool genTestNodes(uint32_t number);
    bool editNode(int32_t targetRevision,
                     int32_t nodeID,
                     nodeListElement *node,
                     float newRadius,
                     int32_t newXPos,
                     int32_t newYPos,
                     int32_t newZPos,
                     int32_t inMag);
    void *popStack(stack *stack);
    bool pushStack(stack *stack, void *element);
    stack *newStack(int32_t size);
    bool delStack(stack *stack);
    bool delDynArray(dynArray *array);
    void *getDynArray(dynArray *array, int32_t pos);
    bool setDynArray(dynArray *array, int32_t pos, void *value);
    dynArray *newDynArray(int32_t size);
    int32_t splitConnectedComponent(int32_t targetRevision, int32_t nodeID);
    bool addComment(int32_t targetRevision, char *content, nodeListElement *node, int32_t nodeID);
    bool delComment(int32_t targetRevision, commentListElement *currentComment, int32_t commentNodeID);
    bool editComment(int32_t targetRevision, commentListElement *currentComment, int32_t nodeID, char *newContent, nodeListElement *newNode, int32_t newNodeID);
    commentListElement *nextComment(char *searchString);
    commentListElement *previousComment(char *searchString);
    bool searchInComment(char *searchString, commentListElement *comment);
    bool unlockPosition();
    bool lockPosition(Coordinate lockCoordinate);
    bool popBranchNode(int32_t targetRevision);
    bool pushBranchNode(int32_t targetRevision, int32_t setBranchNodeFlag, int32_t checkDoubleBranchpoint, nodeListElement *branchNode, int32_t branchNodeID);
    bool setSkeletonWorkMode(int32_t targetRevision, uint32_t workMode);
    bool jumpToActiveNode();
    void UI_popBranchNode();
    void restoreDefaultTreeColor();

signals:

public slots:

};

#endif // SKELETONIZER_H
