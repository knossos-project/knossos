/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include "annotation/annotation.h"
#include "skeleton/skeleton_dfs.h"
#include "skeleton/tree.h"
#include "widgets/viewports/viewportbase.h"

#include <QObject>
#include <QSet>
#include <QSignalBlocker>
#include <QVariantHash>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

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
    bool jumpToSkeletonNext{true};

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

    int definedSkeletonVpView{SKELVP_RESET};

    // Stores the model view matrix for user performed VP rotations.
    float skeletonVpModelView[16];
    //boundary of the visualized volume in the skeleton viewport
    double volBoundary() const;

    QString skeletonCreatedInVersion;
    QString skeletonLastSavedInVersion;

    bool displayMatlabCoordinates{false};
    bool saveMatlabCoordinates{true};
};

class Skeletonizer : public QObject {
    Q_OBJECT
    QSet<QString> textProperties;
    QSet<QString> numberProperties;
public:
    bool simpleEnough(const std::vector<nodeListElement*> & nodes) {
        return nodes.size() < 100;
    }
    bool simpleEnough(const std::vector<treeListElement*> & trees) {
        return std::accumulate(std::begin(trees), std::end(trees), static_cast<std::size_t>(0), [](auto accum, const auto & elem){
            return accum + elem->nodes.size();
        }) + trees.size() < 100;
    }
    template<typename T, typename Func>
    void bulkOperation(T & elems, Func func) {
        const auto bigEnough = !simpleEnough(elems);
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
    auto & selected();
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
    Skeletonizer(bool global = false);
    static Skeletonizer & singleton() {
        static Skeletonizer skeletonizer(true);
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

    boost::optional<nodeListElement &> addNode(boost::optional<decltype(nodeListElement::nodeID)> nodeId, const decltype(nodeListElement::position) & position, const treeListElement & tree, const decltype(nodeListElement::properties) & properties = {});
    treeListElement & addTreeAndActivate(boost::optional<decltype(treeListElement::treeID)> treeID = boost::none, boost::optional<decltype(treeListElement::color)> color = boost::none, const decltype(treeListElement::properties) & properties = {});
    treeListElement & addTree(boost::optional<decltype(treeListElement::treeID)> treeID = boost::none, boost::optional<decltype(treeListElement::color)> color = boost::none, const decltype(treeListElement::properties) & properties = {});

    template<typename T>
    void setComment(T & elem, const QString & newContent);

    boost::optional<nodeListElement &> addNode(boost::optional<decltype(nodeListElement::nodeID)> nodeID, const float radius, const decltype(treeListElement::treeID) treeID, const Coordinate & position, const ViewportType VPtype, const int inMag, boost::optional<uint64_t> time, const bool respectLocks, const QHash<QString, QVariant> & properties = {});

    void inferTreeSelectionFromNodeSelection();
    void deleteSelectedTrees();
    void deleteSelectedNodes();

    bool delTree(decltype(treeListElement::treeID) treeID);
    void clearSkeleton();
    boost::optional<nodeListElement &> UI_addSkeletonNode(const Coordinate & clickedCoordinate, ViewportType VPtype);
    bool setActiveNode(nodeListElement *node);
    void setColor(treeListElement & tree, const QColor &color);
    void setRender(treeListElement & tree, const bool render);
    void continueSynapse();
    void addFinishedSynapse(treeListElement & cleft, nodeListElement & pre, nodeListElement & post);
    void addSynapseFromNodes(std::vector<nodeListElement *> & nodes);
    bool unlockPosition();
    bool lockPosition(Coordinate lockCoordinate);
    void setRadius(nodeListElement & node, const float radius);
    void setPosition(nodeListElement & node, const Coordinate & position);
    bool delNode(std::uint64_t nodeID, nodeListElement *nodeToDel);
    void setSubobject(nodeListElement & node, const quint64 subobjectId);
    void setSubobjectSelectAndMergeWithPrevious(nodeListElement & node, const quint64 subobjectId, nodeListElement * previousNode);
    void updateSubobjectCountFromProperty(nodeListElement & node);
    void unsetSubobjectOfHybridNode(nodeListElement & node);
    void movedHybridNode(nodeListElement & node, const quint64 newSubobjectId, const Coordinate & oldPos);
    void selectObjectForNode(const nodeListElement & node);
    void jumpToNode(const nodeListElement & node);
    bool setActiveTreeByID(decltype(treeListElement::treeID) treeID);

    void propagateComments(nodeListElement & root, const QSet<QString> & comments, const bool overwrite);

    std::unordered_map<decltype(treeListElement::treeID), std::reference_wrapper<treeListElement>> loadXmlSkeleton(QIODevice &file, const bool merge, const QString & treeCmtOnMultiLoad = "");
    std::unordered_map<decltype(treeListElement::treeID), std::reference_wrapper<treeListElement>> loadXmlSkeleton(QXmlStreamReader & xml, const bool merge, const QString & treeCmtOnMultiLoad);
    void saveXmlSkeleton(QIODevice & file, const bool onlySelected = false, const bool saveTime = true, const bool saveDatasetPath = true);
    void saveXmlSkeleton(QXmlStreamWriter & file, const bool onlySelected = false, const bool saveTime = true, const bool saveDatasetPath = true);

    nodeListElement *popBranchNode();
    void pushBranchNode(nodeListElement & branchNode);
    void goToNode(const NodeGenerator::Direction direction);
    void moveSelectedNodesToTree(decltype(treeListElement::treeID) treeID);
    treeListElement * findTreeByTreeID(decltype(treeListElement::treeID) treeID);
    nodeListElement * findNodeByNodeID(std::uint64_t nodeID);
    static QList<nodeListElement *> findNodesInTree(treeListElement & tree, const QString & comment);
    bool addSegment(nodeListElement &sourceNodeID, nodeListElement &targetNodeID);
    bool delSegment(std::list<segmentListElement>::iterator segToDelIt);
    void toggleLink(nodeListElement & lhs, nodeListElement & rhs);
    void restoreDefaultTreeColor(treeListElement & tree);

    bool extractConnectedComponent(std::uint64_t nodeID);
    boost::optional<nodeListElement &> unconnectedNode(nodeListElement & node) const;
    bool mergeTrees(decltype(treeListElement::treeID) treeID1, decltype(treeListElement::treeID) treeID2);
    void mergeMeshes(Mesh & mesh1, Mesh & mesh2);
    void updateTreeColors();
    static std::list<segmentListElement>::iterator findSegmentBetween(nodeListElement & sourceNode, const nodeListElement & targetNode);
    boost::optional<nodeListElement &> addSkeletonNodeAndLinkWithActive(const Coordinate & clickedCoordinate, ViewportType VPtype, int makeNodeActive);

    static bool updateCircRadius(nodeListElement *node);
    QSet<nodeListElement *> findCycle();
    QSet<nodeListElement *> getPath(std::vector<nodeListElement *> & nodes);
    float radius(const nodeListElement &node) const;
    const QSet<QString> getNumberProperties() const { return numberProperties; }
    const QSet<QString> getTextProperties() const { return textProperties; }
    void convertToNumberProperty(const QString & property);

    void loadMesh(QIODevice &, boost::optional<decltype(treeListElement::treeID)> treeID, const QString & filename);
    std::tuple<QVector<GLfloat>, QVector<std::uint8_t>, QVector<GLuint>> getMesh(const treeListElement & tree);
    void saveMesh(QIODevice & file, const treeListElement & tree);
    void saveMesh(QIODevice & file, const treeListElement & tree, QVector<GLfloat> vertex_components, QVector<std::uint8_t> colors, QVector<GLuint> indices);
    void addMeshToTree(boost::optional<decltype(treeListElement::treeID)> treeID, QVector<float> & verts, QVector<float> & normals, const QVector<unsigned int> & indices, const QVector<std::uint8_t> & colors, int draw_mode = 4/*GL_TRIANGLES*/, bool swap_xy = false);
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
    void segmentAdded(const quint64 sourceID, const quint64 targetID);
    void segmentRemoved(const quint64 sourceID, const quint64 targetID);
    void treeAddedSignal(const treeListElement & tree);
    void treeChangedSignal(const treeListElement & tree);
    void treeRemovedSignal(const std::uint64_t treeID);
    void treesMerged(const std::uint64_t treeID,const std::uint64_t treeID2);
    void nodeSelectionChangedSignal();
    void treeSelectionChangedSignal();
    void resetData();
};

void selectMeshesForObjects();
