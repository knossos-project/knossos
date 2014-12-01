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


#include <unordered_map>
#include <QObject>
#include <QtCore>
#include "knossos-global.h"

class Skeletonizer : public QObject {
    Q_OBJECT
public:
    explicit Skeletonizer(QObject *parent = 0);

signals:
    void autosaveSignal();
    void updateToolsSignal();
    void updateTreeviewSignal();
    void userMoveSignal(int x, int y, int z, UserMoveType userMoveType, ViewportType viewportType);
    void setRecenteringPositionSignal(int x, int y, int z);
    void setSimpleTracing(bool active);

public slots:
    static nodeListElement *findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition);
    static nodeListElement *getNodeWithPrevID(nodeListElement *currentNode, bool sameTree);
    static nodeListElement *getNodeWithNextID(nodeListElement *currentNode, bool sameTree);
    static treeListElement *getTreeWithPrevID(treeListElement *currentTree);
    static treeListElement *getTreeWithNextID(treeListElement *currentTree);
    static uint addNode(uint nodeID, float radius, int treeID, Coordinate *position, Byte VPtype, int inMag, int time, int respectLocks);

    static void *popStack(stack *stack);
    static bool pushStack(stack *stack, void *element);
    static stack *newStack(int size);
    static bool delStack(stack *stack);

    static nodeListElement *addNodeListElement(uint nodeID, float radius, nodeListElement **currentNode, Coordinate *position, int inMag);
    static segmentListElement* addSegmentListElement (segmentListElement **currentSegment, nodeListElement *sourceNode, nodeListElement *targetNode);

    void WRAP_popBranchNode();
    static void setColorFromNode(struct nodeListElement *node, color4F *color);
    static void setRadiusFromNode(struct nodeListElement *node, float *radius);
    static unsigned int commentContainsSubstr(struct commentListElement *comment, int index);

    void deleteSelectedTrees();
    void deleteSelectedNodes();

    void UI_popBranchNode();
    static bool delTree(int treeID);
    static void clearTreeSelection();
    static void clearNodeSelection();
    static bool clearSkeleton(int loadingSkeleton);
    void autoSaveIfElapsed();
    bool genTestNodes(uint number);
    bool UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype);
    static bool setActiveNode(nodeListElement *node, uint nodeID);
    static bool addTreeComment(int treeID, QString comment);
    static bool unlockPosition();
    static bool lockPosition(Coordinate lockCoordinate);
    commentListElement *nextComment(QString searchString);
    commentListElement *previousComment(QString searchString);
    static bool delSegment(uint sourceNodeID, uint targetNodeID, segmentListElement *segToDel);
    static bool editNode(uint nodeID, nodeListElement *node, float newRadius, int newXPos, int newYPos, int newZPos, int inMag);
    static bool delNode(uint nodeID, nodeListElement *nodeToDel);
    static bool addComment(QString content, nodeListElement *node, uint nodeID);
    static bool editComment(commentListElement *currentComment, uint nodeID, QString newContent, nodeListElement *newNode, uint newNodeID);
    static bool delComment(commentListElement *currentComment, uint commentNodeID);
    void jumpToActiveNode(bool *isSuccess = NULL);
    static bool setActiveTreeByID(int treeID);

    bool loadXmlSkeleton(QIODevice &file, const QString & treeCmtOnMultiLoad = "");
    bool saveXmlSkeleton(QIODevice &file) const;

    static bool pushBranchNode(int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, uint branchNodeID);
    void moveToNextTree(bool *isSuccess = NULL);
    void moveToPrevTree(bool *isSuccess = NULL);
    bool moveToPrevNode();
    bool moveToNextNode();
    static bool moveNodeToTree(nodeListElement *node, int treeID);
    static treeListElement* findTreeByTreeID(int treeID);
    static nodeListElement *findNodeByNodeID(uint nodeID);
    static bool addSegment(uint sourceNodeID, uint targetNodeID);
    static void restoreDefaultTreeColor(treeListElement *tree);
    static void restoreDefaultTreeColor();

    static int splitConnectedComponent(int nodeID);
    treeListElement *addTreeListElement(int treeID, color4F color);
    static bool mergeTrees(int treeID1, int treeID2);
    static bool updateTreeColors();
    static nodeListElement *findNodeInRadius(Coordinate searchPosition);
    static segmentListElement *findSegmentByNodeIDs(uint sourceNodeID, uint targetNodeID);
    uint addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int makeNodeActive);

    bool searchInComment(char *searchString, commentListElement *comment);
    void popBranchNodeCanceled();
    bool popBranchNode();
    static bool updateCircRadius(struct nodeListElement *node);

public:
    enum TracingMode {
        skipNextLink
        , linkedNodes
        , unlinkedNodes
    };

    TracingMode getTracingMode() const;
    void setTracingMode(TracingMode mode);
    static bool areNeighbors(struct nodeListElement v, struct nodeListElement w);
private:
    TracingMode tracingMode;
    static std::unordered_map<uint, uint> dijkstraGraphSearch(nodeListElement *node);
};

#endif // SKELETONIZER_H
