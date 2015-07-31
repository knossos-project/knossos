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

#include "mesh.h"
#include "skeleton/tree.h"
#include "widgets/viewport.h"

#include <QObject>
#include <QSet>
#include <QVariantHash>

#include <boost/optional.hpp>

#include <memory>
#include <unordered_map>

class nodeListElement;
class segmentListElement;
class commentListElement;

struct stack;

struct skeletonState {
    uint skeletonRevision;

    //    skeletonTime is the time spent on the current skeleton in all previous
    //    instances of knossos that worked with the skeleton.

    bool unsavedChanges;
    int skeletonTime;

    std::unique_ptr<treeListElement> firstTree;
    treeListElement *activeTree;
    nodeListElement *activeNode;

    std::unordered_map<int, treeListElement *> treesByID;

    std::vector<treeListElement *> selectedTrees;
    std::vector<nodeListElement *> selectedNodes;

    commentListElement *currentComment;
    char *commentBuffer;
    char *searchStrBuffer;

    stack *branchStack;

    std::unordered_map<uint, nodeListElement *> nodesByNodeID;

    uint volBoundary;

    uint totalComments;
    uint totalBranchpoints;

    bool userCommentColoringOn;
    uint commentNodeRadiusOn;

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

    // Stores the model view matrix for user performed VP rotations.
    float skeletonVpModelView[16];

    // Stores the angles of the cube in the SkeletonVP
    float rotationState[16];
    // The next three flags cause recompilation of the above specified display lists.

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

    uint64_t greatestNodeID;
    int greatestTreeID;

    nodeListElement *selectedCommentNode;

    //If true, loadSkeleton merges the current skeleton with the provided
    bool mergeOnLoadFlag;

    uint lastSaveTicks;
    bool autoSaveBool;
    uint autoSaveInterval;

    float defaultNodeRadius;

    // Current zoom level. 0: no zoom; near 1: maximum zoom.
    float zoomLevel;

    // temporary vertex buffers that are available for rendering, get cleared
    // every frame */
    mesh lineVertBuffer; /* ONLY for lines */
    mesh pointVertBuffer; /* ONLY for points */

    bool branchpointUnresolved;

    char skeletonCreatedInVersion[32];
    char skeletonLastSavedInVersion[32];

    QString nodeCommentFilter;
    QString treeCommentFilter;
};

class Skeletonizer : public QObject {
    Q_OBJECT
public:
    explicit Skeletonizer(QObject *parent = 0);
    static Skeletonizer & singleton() {
        static Skeletonizer skeletonizer;
        return skeletonizer;
    }

signals:
    void branchPoppedSignal();
    void branchPushedSignal();
    void nodeAddedSignal(const nodeListElement & node);
    void nodeChangedSignal(const nodeListElement & node);
    void nodeRemovedSignal(const uint nodeID);
    void treeAddedSignal(const treeListElement & tree);
    void treeChangedSignal(const treeListElement & tree);
    void treeRemovedSignal(const int treeID);
    void treesMerged(const int treeID,const int treeID2);
    void nodeSelectionChangedSignal();
    void treeSelectionChangedSignal();
    void userMoveSignal(int x, int y, int z, UserMoveType userMoveType, ViewportType viewportType);
    void resetData();

    void autosaveSignal();
    void setRecenteringPositionSignal(int x, int y, int z);
public slots:
    static nodeListElement *findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition);
    static nodeListElement *getNodeWithPrevID(nodeListElement *currentNode, bool sameTree);
    static nodeListElement *getNodeWithNextID(nodeListElement *currentNode, bool sameTree);
    static treeListElement *getTreeWithPrevID(treeListElement *currentTree);
    static treeListElement *getTreeWithNextID(treeListElement *currentTree);
    uint64_t findAvailableNodeID();
    boost::optional<uint64_t> addNode(uint64_t nodeID, const float radius, const int treeID, const Coordinate & position, const ViewportType VPtype, const int inMag, boost::optional<uint64_t> time, const bool respectLocks, const QHash<QString, QVariant> & properties = {});

    static void *popStack(stack *stack);
    static bool pushStack(stack *stack, void *element);
    static stack *newStack(int size);
    static bool delStack(stack *stack);

    static segmentListElement* addSegmentListElement(segmentListElement **currentSegment, nodeListElement *sourceNode, nodeListElement *targetNode);

    void selectNodes(QSet<nodeListElement *> nodes);
    void toggleNodeSelection(const QSet<nodeListElement *> & nodes);
    void selectTrees(const std::vector<treeListElement*> & trees);
    void deleteSelectedTrees();
    void deleteSelectedNodes();

    bool delTree(int treeID);
    void clearSkeleton();
    void autoSaveIfElapsed();
    uint64_t UI_addSkeletonNode(const Coordinate & clickedCoordinate, ViewportType VPtype, const uint64_t nodeId = 0);
    bool setActiveNode(nodeListElement *node, uint nodeID);
    bool addTreeCommentToSelectedTrees(QString comment);
    bool addTreeComment(int treeID, QString comment);
    static bool unlockPosition();
    static bool lockPosition(Coordinate lockCoordinate);
    commentListElement *nextComment(QString searchString);
    commentListElement *previousComment(QString searchString);
    static bool delSegment(uint sourceNodeID, uint targetNodeID, segmentListElement *segToDel);
    bool editNode(uint nodeID, nodeListElement *node, float newRadius, const Coordinate & newPos, int inMag);
    bool delNode(uint nodeID, nodeListElement *nodeToDel);
    bool addComment(QString content, nodeListElement *node, uint nodeID);
    bool editComment(commentListElement *currentComment, uint nodeID, QString newContent, nodeListElement *newNode, uint newNodeID);
    bool setComment(QString newContent, nodeListElement *commentNode, uint commentNodeID);
    bool delComment(commentListElement *currentComment, uint commentNodeID);
    void setSubobjectAndMerge(const quint64 nodeId, const quint64 subobjectId);
    void setSubobjectAndMerge(nodeListElement & node, const quint64 subobjectId);
    void updateSubobjectCountFromProperty(nodeListElement & node);
    void unsetSubobjectOfHybridNode(nodeListElement & node);
    void movedHybridNode(nodeListElement & node, const quint64 newSubobjectId, const Coordinate & oldPos);
    void selectObjectForNode(nodeListElement & node);
    void jumpToNode(const nodeListElement & node);
    bool setActiveTreeByID(int treeID);

    bool loadXmlSkeleton(QIODevice &file, const QString & treeCmtOnMultiLoad = "");
    bool saveXmlSkeleton(QIODevice &file) const;

    nodeListElement *popBranchNodeAfterConfirmation(QWidget * const parent);
    nodeListElement *popBranchNode();
    bool pushBranchNode(int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, uint branchNodeID);
    bool moveToNextTree();
    bool moveToPrevTree();
    bool moveToPrevNode();
    bool moveToNextNode();
    bool moveSelectedNodesToTree(int treeID);
    static treeListElement* findTreeByTreeID(int treeID);
    static QList<treeListElement *> findTrees(const QString & comment);
    static nodeListElement *findNodeByNodeID(uint nodeID);
    static QList<nodeListElement *> findNodesInTree(const treeListElement & tree, const QString & comment);
    static bool addSegment(uint sourceNodeID, uint targetNodeID);
    static void restoreDefaultTreeColor(treeListElement *tree);
    static void restoreDefaultTreeColor();

    bool extractConnectedComponent(int nodeID);
    int findAvailableTreeID();
    treeListElement *addTreeListElement(int treeID, color4F color);
    bool mergeTrees(int treeID1, int treeID2);
    static bool updateTreeColors();
    static nodeListElement *findNodeInRadius(Coordinate searchPosition);
    static segmentListElement *findSegmentByNodeIDs(uint sourceNodeID, uint targetNodeID);
    uint64_t addSkeletonNodeAndLinkWithActive(const Coordinate & clickedCoordinate, ViewportType VPtype, int makeNodeActive);

    bool searchInComment(char *searchString, commentListElement *comment);
    static bool updateCircRadius(nodeListElement *node);

public:
    enum TracingMode {
        skipNextLink
        , linkedNodes
        , unlinkedNodes
    };
    bool simpleTracing;
    TracingMode getTracingMode() const;
    void setTracingMode(TracingMode mode);
    bool areConnected(const nodeListElement & v,const nodeListElement & w) const; // true if a path between the two nodes can be found.

    void setColorFromNode(nodeListElement *node, color4F *color) const;
    float radius(const nodeListElement &node) const;
    float segmentSizeAt(const nodeListElement &node) const;
private:

    TracingMode tracingMode;
};

#endif // SKELETONIZER_H
