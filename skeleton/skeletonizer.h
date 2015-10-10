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

struct stack {
    uint elementsOnStack;
    void **elements;
    int stackpointer;
    int size;
};
void *popStack(stack *stack);
bool pushStack(stack *stack, void *element);
stack *newStack(int size);
bool delStack(stack *stack);

// skeleton vp orientation
#define SKELVP_XY_VIEW 0
#define SKELVP_XZ_VIEW 1
#define SKELVP_YZ_VIEW 2
#define SKELVP_R90 3
#define SKELVP_R180 4
#define SKELVP_RESET 5

struct SkeletonState {
    SkeletonState() {
        state->skeletonState = this;
    }
    std::unique_ptr<treeListElement> firstTree;
    treeListElement * activeTree{nullptr};
    nodeListElement * activeNode{nullptr};

    std::unordered_map<int, treeListElement *> treesByID;

    std::vector<treeListElement *> selectedTrees;
    std::vector<nodeListElement *> selectedNodes;

    commentListElement * currentComment{nullptr};
    QString commentBuffer;

    stack * branchStack{newStack(1048576)};

    std::unordered_map<uint, nodeListElement *> nodesByNodeID;

    uint volBoundary;

    uint totalComments{0};
    uint totalBranchpoints{0};

    bool lockPositions{false};
    bool positionLocked{false};
    QString onCommentLock{"seed"};
    Coordinate lockedPosition;
    long unsigned int lockRadius{100};

    float rotdx;
    float rotdy;
    int rotationcounter;

    int definedSkeletonVpView{SKELVP_RESET};

    float translateX;
    float translateY;

    // Stores the model view matrix for user performed VP rotations.
    float skeletonVpModelView[16];

    // Stores the angles of the cube in the SkeletonVP
    float rotationState[16];
    // The next three flags cause recompilation of the above specified display lists.

    int treeElements{0};
    int totalNodeElements{0};
    int totalSegmentElements{0};

    uint64_t greatestNodeID{0};
    int greatestTreeID{0};

    //If true, loadSkeleton merges the current skeleton with the provided
    bool mergeOnLoadFlag;

    float defaultNodeRadius{1.5f};

    // Current zoom level. 0: no zoom; near 1: maximum zoom.
    float zoomLevel;

    // temporary vertex buffers that are available for rendering, get cleared
    // every frame */
    mesh lineVertBuffer; /* ONLY for lines */
    mesh pointVertBuffer; /* ONLY for points */

    bool branchpointUnresolved{false};

    QString skeletonCreatedInVersion;
    QString skeletonLastSavedInVersion;

    QString nodeCommentFilter;
    QString treeCommentFilter;
};

class Skeletonizer : public QObject {
    Q_OBJECT
    QSet<QString> textProperties;
    QSet<QString> numberProperties;

public:
    SkeletonState skeletonState;
    explicit Skeletonizer(QObject *parent = 0) : QObject(parent) {}
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
    void propertiesChanged(const QSet<QString> & numberProperties);
    void treeAddedSignal(const treeListElement & tree);
    void treeChangedSignal(const treeListElement & tree);
    void treeRemovedSignal(const int treeID);
    void treesMerged(const int treeID,const int treeID2);
    void nodeSelectionChangedSignal();
    void treeSelectionChangedSignal();
    void resetData();
    void setRecenteringPositionSignal(int x, int y, int z);
public slots:
    static nodeListElement *findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition);
    static nodeListElement *getNodeWithPrevID(nodeListElement *currentNode, bool sameTree);
    static nodeListElement *getNodeWithNextID(nodeListElement *currentNode, bool sameTree);
    static treeListElement *getTreeWithPrevID(treeListElement *currentTree);
    static treeListElement *getTreeWithNextID(treeListElement *currentTree);
    uint64_t findAvailableNodeID();
    boost::optional<nodeListElement &> addNode(uint64_t nodeID, const float radius, const int treeID, const Coordinate & position, const ViewportType VPtype, const int inMag, boost::optional<uint64_t> time, const bool respectLocks, const QHash<QString, QVariant> & properties = {});

    static segmentListElement* addSegmentListElement(segmentListElement **currentSegment, nodeListElement *sourceNode, nodeListElement *targetNode);

    void selectNodes(QSet<nodeListElement *> nodes);
    void toggleNodeSelection(const QSet<nodeListElement *> & nodes);
    void selectTrees(const std::vector<treeListElement*> & trees);
    void deleteSelectedTrees();
    void deleteSelectedNodes();

    bool delTree(int treeID);
    void clearSkeleton();
    boost::optional<nodeListElement &> UI_addSkeletonNode(const Coordinate & clickedCoordinate, ViewportType VPtype);
    bool setActiveNode(nodeListElement *node);
    bool addTreeCommentToSelectedTrees(QString comment);
    bool addTreeComment(int treeID, QString comment);
    static bool unlockPosition();
    static bool lockPosition(Coordinate lockCoordinate);
    commentListElement *nextComment(QString searchString);
    commentListElement *previousComment(QString searchString);
    static bool delSegment(segmentListElement *segToDel);
    bool editNode(uint nodeID, nodeListElement *node, float newRadius, const Coordinate & newPos, int inMag);
    bool delNode(uint nodeID, nodeListElement *nodeToDel);
    bool addComment(QString content, nodeListElement &node);
    bool editComment(commentListElement *currentComment, uint nodeID, QString newContent, nodeListElement *newNode, uint newNodeID);
    bool setComment(QString newContent, nodeListElement *commentNode, uint commentNodeID);
    bool delComment(commentListElement *currentComment, uint commentNodeID);
    void setSubobject(nodeListElement & node, const quint64 subobjectId);
    void setSubobjectSelectAndMergeWithPrevious(nodeListElement & node, const quint64 subobjectId, nodeListElement * previousNode);
    void updateSubobjectCountFromProperty(nodeListElement & node);
    void unsetSubobjectOfHybridNode(nodeListElement & node);
    void movedHybridNode(nodeListElement & node, const quint64 newSubobjectId, const Coordinate & oldPos);
    void selectObjectForNode(const nodeListElement & node);
    void jumpToNode(const nodeListElement & node);
    bool setActiveTreeByID(int treeID);

    bool loadXmlSkeleton(QIODevice &file, const QString & treeCmtOnMultiLoad = "");
    bool saveXmlSkeleton(QIODevice &file) const;

    nodeListElement *popBranchNodeAfterConfirmation(QWidget * const parent);
    nodeListElement *popBranchNode();
    bool pushBranchNode(int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement & branchNode);
    bool moveToNextTree();
    bool moveToPrevTree();
    bool moveToPrevNode();
    bool moveToNextNode();
    bool moveSelectedNodesToTree(int treeID);
    static treeListElement* findTreeByTreeID(int treeID);
    static QList<treeListElement *> findTrees(const QString & comment);
    static nodeListElement *findNodeByNodeID(uint nodeID);
    static QList<nodeListElement *> findNodesInTree(const treeListElement & tree, const QString & comment);
    static bool addSegment(nodeListElement &sourceNodeID, nodeListElement &targetNodeID);
    void restoreDefaultTreeColor(treeListElement & tree);

    bool extractConnectedComponent(int nodeID);
    int findAvailableTreeID();
    treeListElement *addTreeListElement(int treeID, color4F color);
    bool mergeTrees(int treeID1, int treeID2);
    void updateTreeColors();
    static nodeListElement *findNodeInRadius(Coordinate searchPosition);
    static segmentListElement *findSegmentBetween(const nodeListElement & sourceNode, const nodeListElement & targetNode);
    boost::optional<nodeListElement &> addSkeletonNodeAndLinkWithActive(const Coordinate & clickedCoordinate, ViewportType VPtype, int makeNodeActive);

    bool searchInComment(char *searchString, commentListElement *comment);
    static bool updateCircRadius(nodeListElement *node);

public:
    bool areConnected(const nodeListElement & v,const nodeListElement & w) const; // true if a path between the two nodes can be found.
    float radius(const nodeListElement &node) const;
    float segmentSizeAt(const nodeListElement &node) const;
};

#endif // SKELETONIZER_H
