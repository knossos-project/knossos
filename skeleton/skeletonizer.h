/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef SKELETONIZER_H
#define SKELETONIZER_H

#include "session.h"
#include "skeleton/skeleton_dfs.h"
#include "skeleton/tree.h"
#include "version.h"
#include "widgets/viewports/viewportbase.h"

#include <QObject>
#include <QSet>
#include <QSignalBlocker>
#include <QVariantHash>

#include <boost/optional.hpp>

#include <algorithm>
#include <memory>
#include <unordered_map>

class nodeListElement;
class segmentListElement;

enum skelvpOrientation {
    SKELVP_XY_VIEW, SKELVP_XZ_VIEW, SKELVP_ZY_VIEW, SKELVP_R90, SKELVP_R180, SKELVP_RESET, SKELVP_CUSTOM
};

class Synapse {
    nodeListElement * preSynapse{nullptr};
    nodeListElement * postSynapse{nullptr};
    treeListElement * synapticCleft{nullptr};

public:
    enum class State {PreSynapse, Cleft, PostSynapse};
    static const int darkenedAlpha = 60;

    nodeListElement * getPreSynapse() const { return preSynapse; }
    nodeListElement * getPostSynapse() const { return postSynapse; }
    treeListElement * getCleft() const { return synapticCleft; }

    void setPreSynapse(nodeListElement & pre) {
        preSynapse = &pre;
        pre.correspondingSynapse = this;
        pre.isSynapticNode = true;
        if (synapticCleft) {
            synapticCleft->properties.insert("preSynapse", static_cast<long long>(preSynapse->nodeID));
        }
    }

    void setPostSynapse(nodeListElement & post) {
        postSynapse = &post;
        post.correspondingSynapse = this;
        post.isSynapticNode = true;
        if (synapticCleft) {
            synapticCleft->properties.insert("postSynapse", static_cast<long long>(postSynapse->nodeID));
            synapticCleft->render = false;
        }
    }

    void setCleft(treeListElement & cleft) {
        synapticCleft = &cleft;
        cleft.isSynapticCleft = true;
        cleft.correspondingSynapse = this;
        cleft.properties.insert("synapticCleft", true);
        if (preSynapse) {
            synapticCleft->properties.insert("preSynapse", static_cast<long long>(preSynapse->nodeID));
        }
        if (postSynapse) {
            synapticCleft->properties.insert("postSynapse", static_cast<long long>(postSynapse->nodeID));
        }
    }

    void toggleDirection() {
        if(postSynapse != nullptr && preSynapse != nullptr) {
            std::swap(synapticCleft->properties["preSynapse"], synapticCleft->properties["postSynapse"]);
            std::swap(preSynapse, postSynapse);
        }
    }

    void remove(nodeListElement * synapticNode) {
        if (synapticNode == preSynapse) {
            synapticCleft->properties.remove("preSynapse");
            preSynapse = nullptr;
        } else if (synapticNode == postSynapse){
            synapticCleft->properties.remove("postSynapse");
            postSynapse = nullptr;
        }
    }

    void reset() {
        if(preSynapse) {
            preSynapse->isSynapticNode = false;
            preSynapse->correspondingSynapse = nullptr;
            preSynapse = nullptr;
        }
        if(postSynapse) {
            postSynapse->isSynapticNode = false;
            postSynapse->correspondingSynapse = nullptr;
            postSynapse = nullptr;
        }
        if (synapticCleft) {
            synapticCleft->isSynapticCleft = false;
            synapticCleft->correspondingSynapse = nullptr;
            synapticCleft = nullptr;
        }
    }
};

struct SkeletonState {
    SkeletonState();
    treeListElement * activeTree{nullptr};
    nodeListElement * activeNode{nullptr};

    Synapse temporarySynapse;
    Synapse::State synapseState{Synapse::State::PreSynapse};

    std::list<treeListElement> trees;
    std::list<Synapse> synapses;
    std::unordered_map<decltype(treeListElement::treeID), treeListElement *> treesByID;
    std::unordered_map<decltype(nodeListElement::nodeID), nodeListElement *> nodesByNodeID;

    decltype(treeListElement::treeID) nextAvailableTreeID{1};
    decltype(nodeListElement::nodeID) nextAvailableNodeID{1};

    std::vector<treeListElement *> selectedTrees;
    std::vector<nodeListElement *> selectedNodes;

    std::vector<std::uint64_t> branchStack;
    bool branchpointUnresolved{false};

    boost::optional<BufferSelection> meshLastClickInformation;
    bool meshLastClickCurrentlyVisited{false};

    QString treeCommentFilter;

    float defaultNodeRadius{1.5f};

    bool lockPositions{false};
    bool positionLocked{false};
    QString lockingComment{"seed"};
    Coordinate lockedPosition;
    long unsigned int lockRadius{100};

    float rotdx{0};
    float rotdy{0};
    int rotationcounter{0};

    float translateX{0};
    float translateY{0};

    int definedSkeletonVpView{SKELVP_RESET};

    // Stores the model view matrix for user performed VP rotations.
    float skeletonVpModelView[16];
    //boundary of the visualized volume in the skeleton viewport
    int volBoundary;

    QString skeletonCreatedInVersion{KVERSION};
    QString skeletonLastSavedInVersion;
};

class Skeletonizer : public QObject {
    Q_OBJECT
    QSet<QString> textProperties;
    QSet<QString> numberProperties;
public:
    template<typename T, typename Func>
    void bulkOperation(T & elems, Func func) {
        const auto bigEnough = elems.size() > 100;
        {
            QSignalBlocker blocker{this};
            if (!bigEnough) {// don’t block for small number of elements
                blocker.unblock();
            }
            for (auto * elem : elems) {
                func(*elem);
            }
        }
        if (bigEnough) {// only needed if signals were indeed blocked
            emit resetData();// is also blocked if it was blocked initially
        }
    }
    template<typename T>
    T * active();
    template<typename T>
    void select(QSet<T*>);
    void setActive(nodeListElement & elem);
    void setActive(treeListElement & elem);
    template<typename T>
    void toggleSelection(const QSet<T*> & nodes);
    template<typename Elem>
    void notifySelection();
    void notifyChanged(treeListElement & tree);
    void notifyChanged(nodeListElement & node);

    SkeletonState skeletonState;
    Skeletonizer();
    static Skeletonizer & singleton() {
        static Skeletonizer skeletonizer;
        return skeletonizer;
    }

    template<typename Func>
    treeListElement * getTreeWithRelationalID(treeListElement * currentTree, Func func);
    treeListElement * getTreeWithPrevID(treeListElement * currentTree);
    treeListElement * getTreeWithNextID(treeListElement * currentTree);

    template<typename Func>
    nodeListElement * getNodeWithRelationalID(nodeListElement * currentNode, bool sameTree, Func func);
    nodeListElement * getNodeWithPrevID(nodeListElement * currentNode, bool sameTree);
    nodeListElement * getNodeWithNextID(nodeListElement * currentNode, bool sameTree);
    nodeListElement * findNearbyNode(treeListElement * nearbyTree, Coordinate searchPosition);

    QList<treeListElement *> findTreesContainingComment(const QString &comment);

    boost::optional<nodeListElement &> addNode(boost::optional<decltype(nodeListElement::nodeID)> nodeId, const decltype(nodeListElement::position) & position, const treeListElement & tree, const decltype(nodeListElement::properties) & properties);
    treeListElement & addTree(boost::optional<decltype(treeListElement::treeID)> treeID = boost::none, boost::optional<decltype(treeListElement::color)> color = boost::none, const decltype(treeListElement::properties) & properties = {});

    template<typename T>
    void setComment(T & elem, const QString & newContent);

    boost::optional<nodeListElement &> addNode(boost::optional<decltype(nodeListElement::nodeID)> nodeID, const float radius, const decltype(treeListElement::treeID) treeID, const Coordinate & position, const ViewportType VPtype, const int inMag, boost::optional<uint64_t> time, const bool respectLocks, const QHash<QString, QVariant> & properties = {});

    void selectNodes(QSet<nodeListElement *> nodes);
    void toggleNodeSelection(const QSet<nodeListElement *> & nodes);
    void selectTrees(const std::vector<treeListElement*> & trees);
    void deleteSelectedTrees();
    void deleteSelectedNodes();
    void toggleConnectionOfFirstPairOfSelectedNodes(QWidget * const parent);

    bool delTree(decltype(treeListElement::treeID) treeID);
    void clearSkeleton();
    boost::optional<nodeListElement &> UI_addSkeletonNode(const Coordinate & clickedCoordinate, ViewportType VPtype);
    bool setActiveNode(nodeListElement *node);
    void setColor(treeListElement & tree, const QColor &color);
    void continueSynapse();
    void addFinishedSynapse(treeListElement & cleft, nodeListElement & pre, nodeListElement & post);
    void addSynapseFromNodes(std::vector<nodeListElement *> & nodes);
    bool unlockPosition();
    bool lockPosition(Coordinate lockCoordinate);
    bool editNode(std::uint64_t nodeID, nodeListElement *node, float newRadius, const Coordinate & newPos, int inMag);
    bool delNode(std::uint64_t nodeID, nodeListElement *nodeToDel);
    void setSubobject(nodeListElement & node, const quint64 subobjectId);
    void setSubobjectSelectAndMergeWithPrevious(nodeListElement & node, const quint64 subobjectId, nodeListElement * previousNode);
    void updateSubobjectCountFromProperty(nodeListElement & node);
    void unsetSubobjectOfHybridNode(nodeListElement & node);
    void movedHybridNode(nodeListElement & node, const quint64 newSubobjectId, const Coordinate & oldPos);
    void selectObjectForNode(const nodeListElement & node);
    void jumpToNode(const nodeListElement & node);
    bool setActiveTreeByID(decltype(treeListElement::treeID) treeID);

    std::unordered_map<decltype(treeListElement::treeID), std::reference_wrapper<treeListElement>> loadXmlSkeleton(QIODevice &file, const bool merge, const QString & treeCmtOnMultiLoad = "");
    void saveXmlSkeleton(QIODevice &file) const;

    nodeListElement *popBranchNodeAfterConfirmation(QWidget * const parent);
    nodeListElement *popBranchNode();
    void pushBranchNode(nodeListElement & branchNode);
    void goToNode(const NodeGenerator::Direction direction);
    void moveSelectedNodesToTree(decltype(treeListElement::treeID) treeID);
    static treeListElement* findTreeByTreeID(decltype(treeListElement::treeID) treeID);
    static nodeListElement *findNodeByNodeID(std::uint64_t nodeID);
    static QList<nodeListElement *> findNodesInTree(treeListElement & tree, const QString & comment);
    bool addSegment(nodeListElement &sourceNodeID, nodeListElement &targetNodeID);
    bool delSegment(std::list<segmentListElement>::iterator segToDelIt);
    void toggleLink(nodeListElement & lhs, nodeListElement & rhs);
    void restoreDefaultTreeColor(treeListElement & tree);

    bool extractConnectedComponent(std::uint64_t nodeID);
    bool mergeTrees(decltype(treeListElement::treeID) treeID1, decltype(treeListElement::treeID) treeID2);
    void mergeMeshes(Mesh & mesh1, Mesh & mesh2);
    void updateTreeColors();
    static std::list<segmentListElement>::iterator findSegmentBetween(nodeListElement & sourceNode, const nodeListElement & targetNode);
    boost::optional<nodeListElement &> addSkeletonNodeAndLinkWithActive(const Coordinate & clickedCoordinate, ViewportType VPtype, int makeNodeActive);

    static bool updateCircRadius(nodeListElement *node);

    bool areConnected(const nodeListElement & v,const nodeListElement & w) const; // true if a path between the two nodes can be found.
    float radius(const nodeListElement &node) const;
    const QSet<QString> getNumberProperties() const { return numberProperties; }
    const QSet<QString> getTextProperties() const { return textProperties; }
    void convertToNumberProperty(const QString & property);

    void loadMesh(QIODevice &, const boost::optional<decltype(treeListElement::treeID)> treeID, const QString & filename);
    void saveMesh(QIODevice & file, const treeListElement & tree);
    void addMeshToTree(boost::optional<decltype(treeListElement::treeID)> treeID, QVector<float> & verts, QVector<float> & normals, QVector<unsigned int> & indices, QVector<std::uint8_t> & colors, int draw_mode = 0, bool swap_xy = false);
    void deleteMeshOfTree(treeListElement & tree);
signals:
    void guiModeLoaded();
    void branchPoppedSignal();
    void branchPushedSignal();
    void lockingChanged();
    void lockedToNode(const std::uint64_t nodeID);
    void unlockedNode();
    void nodeAddedSignal(const nodeListElement & node);
    void nodeChangedSignal(const nodeListElement & node);
    void nodeRemovedSignal(const std::uint64_t nodeID);
    void jumpedToNodeSignal(const nodeListElement & node);
    void propertiesChanged(const QSet<QString> & numberProperties, const QSet<QString> & textProperties);
    void treeAddedSignal(const treeListElement & tree);
    void treeChangedSignal(const treeListElement & tree);
    void treeRemovedSignal(const std::uint64_t treeID);
    void treesMerged(const std::uint64_t treeID,const std::uint64_t treeID2);
    void nodeSelectionChangedSignal();
    void treeSelectionChangedSignal();
    void resetData();
};

#endif // SKELETONIZER_H
