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

#include "skeletonizer.h"

#include "annotation/file_io.h"
#include "buildinfo.h"
#include "dataset.h"
#include "functions.h"
#include "mesh/mesh.h"
#include "segmentation/cubeloader.h"
#include "segmentation/segmentation.h"
#include "skeleton/node.h"
#include "skeleton/skeleton_dfs.h"
#include "skeleton/tree.h"
#include "stateInfo.h"
#include "tinyply/tinyply.h"
#include "viewer.h"
#include "widgets/viewports/viewportbase.h"
#include "widgets/mainwindow.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QXmlStreamAttributes>

#include <cstring>
#include <deque>
#include <iterator>
#include <queue>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

SkeletonState::SkeletonState() : skeletonCreatedInVersion{KREVISION} {}

double SkeletonState::volBoundary() const {
    const auto & scale = Dataset::current().scales[0];
    const auto & boundary = Dataset::current().boundary;
    return 2 * std::max({scale.x * boundary.x, scale.y * boundary.y, scale.z * boundary.z});
}

QSet<nodeListElement *> shortestPath(nodeListElement & lhs, const nodeListElement & rhs) { // using uniform cost search
    auto cmp = [](const auto & lhs, const auto & rhs) { return lhs.second > rhs.second; };
    std::unordered_set<const nodeListElement *> explored;
    std::priority_queue<std::pair<nodeListElement *, int>, std::vector<std::pair<nodeListElement *, int>>, decltype(cmp)> frontier(cmp);
    frontier.emplace(&lhs, 0);
    std::unordered_map<nodeListElement *, nodeListElement *> parentMap{{&lhs, nullptr}};
    while (true) {
        if (frontier.empty()) {
            return {};
        }
        auto [node, cost] = frontier.top();
        frontier.pop();
        if (explored.find(node) != std::end(explored)) {
            continue;
        }
        explored.insert(node);
        if (node == &rhs) {
            QSet<nodeListElement *> path{node};
            auto * parent = parentMap[node];
            while (parent != nullptr) {
                path.insert(parent);
                parent = parentMap[parent];
            }
            return path;
        }
        for (const auto & segment : *(node->getSegments())) {
            auto * neighbor = (node == &segment->source) ? &segment->target : &segment->source;
            if (explored.find(neighbor) == std::end(explored)) {
                frontier.emplace(neighbor, cost + 1);
                parentMap[neighbor] = node;
            }
        }
    }
}

auto connectedComponent(nodeListElement & node) {
    std::queue<const nodeListElement *> queue;
    std::unordered_set<treeListElement *> visitedTrees;
    std::unordered_set<nodeListElement *> visitedNodes;
    visitedNodes.emplace(&node);
    queue.push(&node);
    while (!queue.empty()) {
        auto * nextNode = queue.front();
        queue.pop();

        for (auto & segment : nextNode->segments) {
            auto & neighbor = segment.forward ? segment.target : segment.source;
            if (visitedNodes.find(&neighbor) == visitedNodes.end()) {
                visitedNodes.emplace(&neighbor);
                visitedTrees.emplace(node.correspondingTree);
                queue.push(&neighbor);
            }
        }
    }
    return std::pair{visitedNodes, visitedTrees};
}

Skeletonizer::Skeletonizer() {
    state->skeletonState = &skeletonState;

    QObject::connect(this, &Skeletonizer::resetData, this, &Skeletonizer::guiModeLoaded);
    QObject::connect(this, &Skeletonizer::resetData, this, &Skeletonizer::lockingChanged);
}

treeListElement* Skeletonizer::findTreeByTreeID(decltype(treeListElement::treeID) treeID) {
    const auto treeIt = state->skeletonState->treesByID.find(treeID);
    return treeIt != std::end(state->skeletonState->treesByID) ? treeIt->second : nullptr;
}

QList<treeListElement *> Skeletonizer::findTreesContainingComment(const QString &comment) {
    QList<treeListElement *> hits;
    for (auto & tree : skeletonState.trees) {
        if (tree.getComment().contains(comment, Qt::CaseInsensitive)) {
            hits.append(&tree);
        }
    }
    return hits;
}

boost::optional<nodeListElement &> Skeletonizer::UI_addSkeletonNode(const Coordinate & clickedCoordinate, ViewportType VPtype) {
    if(!state->skeletonState->activeTree) {
        addTree();
    }

    auto addedNode = addNode(
                          boost::none,
                          state->skeletonState->defaultNodeRadius,
                          state->skeletonState->activeTree->treeID,
                          clickedCoordinate,
                          VPtype,
                          Dataset::current().magnification,
                          boost::none,
                          true);
    if(!addedNode) {
        qDebug() << "Error: Could not add new node!";
        return boost::none;
    }

    setActiveNode(&addedNode.get());

    if (state->skeletonState->activeTree->nodes.size() == 1) {//First node tree is branch node
        pushBranchNode(addedNode.get());
        setComment(addedNode.get(), "First Node");
    }
    return addedNode.get();
}

boost::optional<nodeListElement &> Skeletonizer::addSkeletonNodeAndLinkWithActive(const Coordinate & clickedCoordinate, ViewportType VPtype, int makeNodeActive) {
    if(!state->skeletonState->activeNode) {
        qDebug() << "Please create a node before trying to link nodes.";
        return boost::none;
    }

    //Add a new node at the target position first.
    auto targetNode = addNode(
                           boost::none,
                           state->skeletonState->defaultNodeRadius,
                           state->skeletonState->activeTree->treeID,
                           clickedCoordinate,
                           VPtype,
                           Dataset::current().magnification,
                           boost::none,
                           true);
    if(!targetNode) {
        qDebug() << "Could not add new node while trying to add node and link with active node!";
        return boost::none;
    }

    addSegment(*state->skeletonState->activeNode, targetNode.get());

    if(makeNodeActive) {
        setActiveNode(&targetNode.get());
    }
    if (state->skeletonState->activeTree->nodes.size() == 1) {
        /* First node in this tree */
        pushBranchNode(targetNode.get());
        setComment(targetNode.get(), "First Node");
    }

    return targetNode.get();
}

void Skeletonizer::propagateComments(nodeListElement & root, const QSet<QString> & comments, const bool overwrite) {
    std::vector<std::pair<nodeListElement *, nodeListElement *>> stack; // node and parent
    std::unordered_set<nodeListElement *> visitedNodes;
    stack.push_back({&root, nullptr});
    while (!stack.empty()) {
        const auto [nextNode, parent] = stack.back();
        stack.pop_back();
        if (visitedNodes.find(nextNode) != visitedNodes.end()) {
            continue;
        }
        visitedNodes.emplace(nextNode);
        if (!overwrite && nextNode != &root && nextNode->getComment() != "") {
            continue;
        }

        for (auto & segment : nextNode->segments) {
            auto & neighbor = segment.forward ? segment.target : segment.source;
            stack.push_back({&neighbor, nextNode});
        }
        if (parent && !parent->getComment().isEmpty() && comments.find(parent->getComment()) != std::end(comments) && comments.find(nextNode->getComment()) == std::end(comments)) {
            nextNode->setComment(parent->getComment());
        }
    }
}

void Skeletonizer::saveXmlSkeleton(QIODevice & file, const bool onlySelected, const bool saveTime, const bool saveDatasetPath) {
    QXmlStreamWriter xml(&file);
    saveXmlSkeleton(xml, onlySelected, saveTime, saveDatasetPath);
}

void Skeletonizer::saveXmlSkeleton(QXmlStreamWriter & xml, const bool onlySelected, const bool saveTime, const bool saveDatasetPath) {
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("things");//root node

    xml.writeStartElement("parameters");
    xml.writeStartElement("experiment");
    xml.writeAttribute("name", QString(Dataset::current().experimentname));
    xml.writeEndElement();

    xml.writeStartElement("lastsavedin");
    xml.writeAttribute("version", QString(KREVISION));
    xml.writeEndElement();

    xml.writeStartElement("createdin");
    xml.writeAttribute("version", state->skeletonState->skeletonCreatedInVersion);
    xml.writeEndElement();

    if (!skeletonState.saveMatlabCoordinates) {
        xml.writeStartElement("nodes_0_based");
        xml.writeEndElement();
    }

    xml.writeStartElement("guiMode");
    xml.writeAttribute("mode", (Annotation::singleton().guiMode == GUIMode::ProofReading) ? "proof reading" : "none");
    xml.writeEndElement();
    xml.writeStartElement("dataset");
    xml.writeAttribute("path", saveDatasetPath? state->viewer->window->widgetContainer.datasetLoadWidget.datasetUrl.toString() : "");
    xml.writeAttribute("overlay", QString::number(static_cast<int>(Segmentation::singleton().enabled)));
    xml.writeEndElement();
    const auto & task = Annotation::singleton().fileTask;
    if (!task.project.isEmpty() || !task.category.isEmpty() || !task.name.isEmpty()) {
        xml.writeStartElement("task");
        xml.writeAttribute("project", task.project);
        xml.writeAttribute("category", task.category);
        xml.writeAttribute("name", task.name);
        xml.writeEndElement();
    }

    xml.writeStartElement("MovementArea");
    xml.writeAttribute("min.x", QString::number(Annotation::singleton().movementAreaMin.x));
    xml.writeAttribute("min.y", QString::number(Annotation::singleton().movementAreaMin.y));
    xml.writeAttribute("min.z", QString::number(Annotation::singleton().movementAreaMin.z));
    const auto size = Annotation::singleton().movementAreaMax - Annotation::singleton().movementAreaMin;
    xml.writeAttribute("size.x", QString::number(size.x));
    xml.writeAttribute("size.y", QString::number(size.y));
    xml.writeAttribute("size.z", QString::number(size.z));
    // backwards compat
    xml.writeAttribute("max.x", QString::number(Annotation::singleton().movementAreaMax.x));
    xml.writeAttribute("max.y", QString::number(Annotation::singleton().movementAreaMax.y));
    xml.writeAttribute("max.z", QString::number(Annotation::singleton().movementAreaMax.z));
    xml.writeEndElement();

    xml.writeStartElement("scale");
    xml.writeAttribute("x", QString::number(Dataset::current().scales[0].x));
    xml.writeAttribute("y", QString::number(Dataset::current().scales[0].y));
    xml.writeAttribute("z", QString::number(Dataset::current().scales[0].z));
    xml.writeEndElement();

    xml.writeStartElement("RadiusLocking");
    xml.writeAttribute("enableCommentLocking", QString::number(state->skeletonState->lockPositions));
    xml.writeAttribute("lockingRadius", QString::number(state->skeletonState->lockRadius));
    xml.writeAttribute("lockToNodesWithComment", QString(state->skeletonState->lockingComment));
    xml.writeEndElement();

    xml.writeStartElement("time");
    const auto time = saveTime? Annotation::singleton().getAnnotationTime() : 0;
    xml.writeAttribute("ms", QString::number(time));
    const auto timeData = QByteArray::fromRawData(reinterpret_cast<const char *>(&time), sizeof(time));
    const QString timeChecksum = QCryptographicHash::hash(timeData, QCryptographicHash::Sha256).toHex().constData();
    xml.writeAttribute("checksum", timeChecksum);
    xml.writeEndElement();

    if (state->skeletonState->activeNode != nullptr) {
        auto * activeNode = state->skeletonState->activeNode;
        if (!onlySelected || activeNode->correspondingTree->selected) {
            xml.writeStartElement("activeNode");
            xml.writeAttribute("id", QString::number(activeNode->nodeID));
            xml.writeEndElement();
        }
    }

    xml.writeStartElement("segmentation");
    xml.writeAttribute("backgroundId", QString::number(Segmentation::singleton().getBackgroundId()));
    xml.writeEndElement();

    xml.writeStartElement("editPosition");
    xml.writeAttribute("x", QString::number(state->viewerState->currentPosition.x + skeletonState.saveMatlabCoordinates));
    xml.writeAttribute("y", QString::number(state->viewerState->currentPosition.y + skeletonState.saveMatlabCoordinates));
    xml.writeAttribute("z", QString::number(state->viewerState->currentPosition.z + skeletonState.saveMatlabCoordinates));
    xml.writeEndElement();

    xml.writeStartElement("skeletonVPState");
    for (int j = 0; j < 16; ++j) {
        xml.writeAttribute(QString("E%1").arg(j), QString::number(state->skeletonState->skeletonVpModelView[j]));
    }
    xml.writeAttribute("translateX", QString::number(state->viewer->window->viewport3D->translateX));
    xml.writeAttribute("translateY", QString::number(state->viewer->window->viewport3D->translateY));
    xml.writeEndElement();

    xml.writeStartElement("vpSettingsZoom");
    xml.writeAttribute("XYPlane", QString::number(state->viewer->window->viewportXY->texture.FOV));
    xml.writeAttribute("XZPlane", QString::number(state->viewer->window->viewportXZ->texture.FOV));
    xml.writeAttribute("YZPlane", QString::number(state->viewer->window->viewportZY->texture.FOV));
    xml.writeAttribute("SkelVP", QString::number(-(0.5 / state->mainWindow->viewport3D->zoomFactor - 0.5)));// legacy zoom: 0 → 0.5
    xml.writeEndElement();

    xml.writeEndElement(); // end parameters

    for (auto & currentTree : skeletonState.trees) {
        if (onlySelected && !currentTree.selected) {
            continue;
        }
        //Every "thing" (tree) has associated nodes and edges.
        xml.writeStartElement("thing");
        xml.writeAttribute("id", QString::number(currentTree.treeID));
        xml.writeAttribute("visible", QString::number(currentTree.render));
        if (currentTree.colorSetManually) {
            xml.writeAttribute("color.r", QString::number(currentTree.color.redF()));
            xml.writeAttribute("color.g", QString::number(currentTree.color.greenF()));
            xml.writeAttribute("color.b", QString::number(currentTree.color.blueF()));
            xml.writeAttribute("color.a", QString::number(currentTree.color.alphaF()));
        } else {
            xml.writeAttribute("color.r", QString("-1."));
            xml.writeAttribute("color.g", QString("-1."));
            xml.writeAttribute("color.b", QString("-1."));
            xml.writeAttribute("color.a", QString("1."));
        }
        for (auto propertyIt = currentTree.properties.constBegin(); propertyIt != currentTree.properties.constEnd(); ++propertyIt) {
            xml.writeAttribute(propertyIt.key(), propertyIt.value().toString());
        }

        xml.writeStartElement("nodes");
        for (const auto & node : currentTree.nodes) {
            xml.writeStartElement("node");
            xml.writeAttribute("id", QString::number(node.nodeID));
            xml.writeAttribute("radius", QString::number(node.radius));
            xml.writeAttribute("x", QString::number(node.position.x + skeletonState.saveMatlabCoordinates));
            xml.writeAttribute("y", QString::number(node.position.y + skeletonState.saveMatlabCoordinates));
            xml.writeAttribute("z", QString::number(node.position.z + skeletonState.saveMatlabCoordinates));
            xml.writeAttribute("inVp", QString::number(node.createdInVp));
            xml.writeAttribute("inMag", QString::number(node.createdInMag));
            xml.writeAttribute("time", QString::number(node.timestamp));
            for (auto propertyIt = node.properties.constBegin(); propertyIt != node.properties.constEnd(); ++propertyIt) {
                xml.writeAttribute(propertyIt.key(), propertyIt.value().toString());
            }
            xml.writeEndElement(); // end node
        }
        xml.writeEndElement(); // end nodes

        xml.writeStartElement("edges");
        for (const auto & node : currentTree.nodes) {
            for (const auto & currentSegment : node.segments) {
                if (currentSegment.forward) {
                    xml.writeStartElement("edge");
                    xml.writeAttribute("source", QString::number(currentSegment.source.nodeID));
                    xml.writeAttribute("target", QString::number(currentSegment.target.nodeID));
                    xml.writeEndElement();
                }
            }
        }
        xml.writeEndElement(); // end edges

        xml.writeEndElement(); // end tree
    }

    xml.writeStartElement("comments");
    TreeTraverser commentTraverser(state->skeletonState->trees);
    for (; !commentTraverser.reachedEnd; ++commentTraverser) {
        if (!onlySelected || skeletonState.nodesByNodeID[(*commentTraverser).nodeID]->correspondingTree->selected) {
            if(!(*commentTraverser).getComment().isEmpty()) {
                xml.writeStartElement("comment");
                xml.writeAttribute("node", QString::number((*commentTraverser).nodeID));
                xml.writeAttribute("content", (*commentTraverser).getComment());
                xml.writeEndElement();
            }
        }
    }
    xml.writeEndElement(); // end comments

    xml.writeStartElement("branchpoints");
    for (const auto branchNodeID : state->skeletonState->branchStack) {
        if (!onlySelected || skeletonState.nodesByNodeID[branchNodeID]->correspondingTree->selected) {
            xml.writeStartElement("branchpoint");
            xml.writeAttribute("id", QString::number(branchNodeID));
            xml.writeEndElement();
        }
    }
    xml.writeEndElement(); // end branchpoints

    xml.writeEndElement(); // end things
    xml.writeEndDocument();
}

std::unordered_map<decltype(treeListElement::treeID), std::reference_wrapper<treeListElement>> Skeletonizer::loadXmlSkeleton(QIODevice & file, const bool merge, const QString & treeCmtOnMultiLoad) {
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("loadXmlSkeleton open failed");
    }
    QXmlStreamReader xml(&file);
    return loadXmlSkeleton(xml, merge, treeCmtOnMultiLoad);
}

std::unordered_map<decltype(treeListElement::treeID), std::reference_wrapper<treeListElement>> Skeletonizer::loadXmlSkeleton(QXmlStreamReader & xml, const bool merge, const QString & treeCmtOnMultiLoad) {
    // If "createdin"-node does not exist, skeleton was created in a version before 3.2
    state->skeletonState->skeletonCreatedInVersion = "pre-3.2";
    state->skeletonState->skeletonLastSavedInVersion = "pre-3.2";

    Annotation::singleton().guiMode = GUIMode::None;

    bool matlabCoordinates{true};
    QString experimentName, loadedProject, loadedCategory, loadedTaskName;
    std::uint64_t activeNodeID = 0;
    auto nmlScale = boost::make_optional(false, floatCoordinate{});
    auto loadedPosition = boost::make_optional(false, floatCoordinate{});// make_optional gets around GCCs false positive maybe-uninitialized
    std::vector<std::uint64_t> branchVector;
    std::vector<std::pair<std::uint64_t, QString>> commentsVector;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> edgeVector;

    std::unordered_map<decltype(nodeListElement::nodeID), std::reference_wrapper<nodeListElement>> nodeMap;
    std::unordered_map<decltype(treeListElement::treeID), std::reference_wrapper<treeListElement>> treeMap;

    const QSet<QString> knownElements({"offset", "skeletonDisplayMode"});
    QSet<QString> skippedElements;

    QElapsedTimer bench;
    bench.start();
    {
    QSignalBlocker blocker{this};
    xml.readNextStartElement();// <things>
    const auto rootTag = xml.name();
    while(xml.readNextStartElement()) {
        if(xml.name() == "parameters") {
            while(xml.readNextStartElement()) {
                QXmlStreamAttributes attributes = xml.attributes();

                if (xml.name() == "experiment") {
                    experimentName = attributes.value("name").toString();
                } else if (xml.name() == "createdin") {
                    state->skeletonState->skeletonCreatedInVersion = attributes.value("version").toString();
                } else if(xml.name() == "lastsavedin") {
                    state->skeletonState->skeletonLastSavedInVersion = attributes.value("version").toString();
                } else if (xml.name() == "nodes_0_based") {
                    matlabCoordinates = false;
                } else if (xml.name() == "node_position_numbering") {// legacy tag
                    matlabCoordinates = attributes.value("index_origin").toInt();
                } else if (xml.name() == "guiMode") {
                    if (attributes.value("mode").toString() == "proof reading") {
                        Annotation::singleton().guiMode = GUIMode::ProofReading;
                    }
                } else if(xml.name() == "dataset") {
                    const auto path = attributes.value("path").toString();
                    const bool overlay = attributes.value("overlay").isEmpty() ? Segmentation::singleton().enabled : static_cast<bool>(attributes.value("overlay").toInt());
                    if (experimentName != Dataset::current().experimentname || overlay != Segmentation::singleton().enabled) {
                        state->viewer->window->widgetContainer.datasetLoadWidget.loadDataset(overlay, path, true);
                    }
                } else if(xml.name() == "MovementArea") {
                    if (!merge) {
                        Coordinate movementAreaMin;//0
                        Coordinate movementAreaMax = Dataset::current().boundary;
                        bool sizeSet{false};
                        Coordinate movementAreaSize = Dataset::current().boundary;
                        for (const auto & attribute : attributes) {
                            const auto & name = attribute.name();
                            const auto & value = attribute.value();
                            if (name == "min.x") {
                                movementAreaMin.x = value.toInt();
                            } else if (name == "min.y") {
                                movementAreaMin.y = value.toInt();
                            } else if (name == "min.z") {
                                movementAreaMin.z = value.toInt();
                            } else if (name == "size.x") {
                                sizeSet = true;
                                movementAreaSize.x = value.toInt();
                            } else if (name == "size.y") {
                                sizeSet = true;
                                movementAreaSize.y = value.toInt();
                            } else if (name == "size.z") {
                                sizeSet = true;
                                movementAreaSize.z = value.toInt();
                            } else if (name == "max.x") {
                                movementAreaMax.x = value.toInt();
                            } else if (name == "max.y") {
                                movementAreaMax.y = value.toInt();
                            } else if (name == "max.z") {
                                movementAreaMax.z = value.toInt();
                            }
                        }
                        if (sizeSet) {
                            Annotation::singleton().updateMovementArea(movementAreaMin, movementAreaMin + movementAreaSize);//range checked
                        } else {
                            Annotation::singleton().updateMovementArea(movementAreaMin, movementAreaMax + 1); // backwards compat max inclusive area
                        }
                    }
                } else if(xml.name() == "time") { // in case of a merge the current annotation's time is kept.
                    if (!merge) {
                        const auto ms = attributes.value("ms").toULongLong();
                        Annotation::singleton().setAnnotationTime(ms);
                    }
                } else if(xml.name() == "activeNode") {
                    if (!merge) {
                        activeNodeID = attributes.value("id").toULongLong();
                    }
                } else if(xml.name() == "segmentation") {
                    Segmentation::singleton().setBackgroundId(attributes.value("backgroundId").toULongLong());
                } else if(xml.name() == "editPosition") {
                    loadedPosition = floatCoordinate(attributes.value("x").toDouble(), attributes.value("y").toDouble(), attributes.value("z").toDouble());
                    if (nmlScale) {
                        loadedPosition = (nmlScale.get() / Dataset::current().scales[0]).componentMul(loadedPosition.get()) - matlabCoordinates;
                    } else {
                        loadedPosition = loadedPosition.get() - matlabCoordinates;
                    }
                } else if(xml.name() == "skeletonVPState") {
                    if (!merge) {
                          // non-working code for skelvp rotation and translation restoration
//                        for (int j = 0; j < 16; ++j) {
//                            state->skeletonState->skeletonVpModelView[j] = attributes.value(QString("E%1").arg(j)).toFloat();
//                        }
//                        state->skeletonState->translateX = attributes.value("translateX").toFloat();
//                        state->skeletonState->translateY = attributes.value("translateY").toFloat();
                    }
                } else if(xml.name() == "vpSettingsZoom") {
                    QStringRef attribute = attributes.value("XYPlane");
                    if(attribute.isNull() == false) {
                        state->viewer->window->viewportXY->texture.FOV = attribute.toString().toFloat();
                    }
                    attribute = attributes.value("XZPlane");
                    if(attribute.isNull() == false) {
                        state->viewer->window->viewportXZ->texture.FOV = attribute.toString().toFloat();
                    }
                    attribute = attributes.value("YZPlane");
                    if(attribute.isNull() == false) {
                        state->viewer->window->viewportZY->texture.FOV = attribute.toString().toFloat();
                    }
                    if (!attributes.value("SkelVP").isEmpty()) {
                        // zoom can only be applied meaningfully with working rotation and translation
                    }
                } else if(xml.name() == "RadiusLocking") {
                    QStringRef attribute = attributes.value("enableCommentLocking");
                    if(attribute.isNull() == false) {
                        state->skeletonState->lockPositions = attribute.toString().toInt();
                    }
                    attribute = attributes.value("lockingRadius");
                    if(attribute.isNull() == false) {
                        state->skeletonState->lockRadius = attribute.toString().toInt();
                    }
                    attribute = attributes.value("lockToNodesWithComment");
                    if(attribute.isNull() == false) {
                        state->skeletonState->lockingComment = attribute.toString();
                    }
                } else if(xml.name() == "idleTime") { // in case of a merge the current annotation's idleTime is kept.
                    if (!merge) {
                        QStringRef attribute = attributes.value("ms");
                        if (attribute.isNull() == false) {
                            const auto annotationTime = Annotation::singleton().getAnnotationTime();
                            const auto idleTime = attribute.toString().toInt();
                            //subract from annotationTime
                            Annotation::singleton().setAnnotationTime(annotationTime - idleTime);
                        }
                    }
                } else if(xml.name() == "scale") {
                    const auto & attributes = xml.attributes();
                    nmlScale = floatCoordinate(attributes.value("x").toDouble(), attributes.value("y").toDouble(), attributes.value("z").toDouble());
                } else if(xml.name() == "task") {
                    for (auto && attribute : attributes) {
                        auto key = attribute.name();
                        if (key == "project") {
                            loadedProject = attribute.value().toString();
                        } else if (key == "category") {
                            loadedCategory = attribute.value().toString();
                        } else if (key == "name") {
                            loadedTaskName = attribute.value().toString();
                        }
                    }
                } else if (knownElements.find(xml.name().toString()) == std::end(knownElements)) {// known but unused legacy elements are not reported
                    skippedElements.insert(xml.name().toString());
                }
                xml.skipCurrentElement();
            }
        } else if(xml.name() == "properties") {
            while(xml.readNextStartElement()) {
                if(xml.name() == "property") {
                    const auto attributes = xml.attributes();
                    const auto name = attributes.value("name").toString();
                    const auto type = attributes.value("type").toString();
                    if (name.isEmpty() == false && type.isEmpty() == false) {
                        if (type == "number") {
                            numberProperties.insert(name);
                        } else {
                            textProperties.insert(name);
                        }
                    }
                } else {
                    skippedElements.insert(xml.name().toString());
                }
                xml.skipCurrentElement();
            }
        } else if(xml.name() == "branchpoints") {
            while(xml.readNextStartElement()) {
                if(xml.name() == "branchpoint") {
                    QXmlStreamAttributes attributes = xml.attributes();
                    QStringRef attribute = attributes.value("id");
                    if(attribute.isNull() == false) {
                        std::uint64_t nodeID = attribute.toULongLong();;
                        branchVector.emplace_back(nodeID);
                    }
                } else {
                    skippedElements.insert(xml.name().toString());
                }
                xml.skipCurrentElement();
            }
        } else if(xml.name() == "comments") {
            // comments must be buffered and can only be set after thing nodes were parsed
            // and the skeleton structure was created. This is necessary, because sometimes the
            // comments node comes before the thing nodes.
            while(xml.readNextStartElement()) {
                if(xml.name() == "comment") {
                    QXmlStreamAttributes attributes = xml.attributes();
                    QStringRef attribute = attributes.value("node");
                    std::uint64_t nodeID = attribute.toULongLong();
                    attribute = attributes.value("content");
                    if((!attribute.isNull()) && (!attribute.isEmpty())) {
                        commentsVector.emplace_back(nodeID, attribute.toString());
                    }
                } else {
                    skippedElements.insert(xml.name().toString());
                }
                xml.skipCurrentElement();
            }
        } else if(xml.name() == "thing") {
            decltype(treeListElement::treeID) treeID{0};
            bool render = true;
            bool okr{false}, okg{false}, okb{false}, oka{false};
            float red{-1.0f}, green{-1.0f}, blue{-1.0f}, alpha{-1.0f};
            QVariantHash properties;
            for (const auto & attribute : xml.attributes()) {
                const auto & name = attribute.name();
                const auto & value = attribute.value();
                if (name == "id") {
                    bool ok;
                    treeID = value.toULongLong(&ok);
                    if (!ok) {
                        treeID = value.toDouble();
                    }
                } else if (name == "visible") {
                    render = value.toInt();
                } else if (name == "color.r") {
                    red = value.toFloat(&okr);
                } else if (name == "color.g") {
                    green = value.toFloat(&okg);
                } else if (name == "color.b") {
                    blue = value.toFloat(&okb);
                } else if (name == "color.a") {
                    alpha = value.toFloat(&oka);
                } else {
                    properties.insert(name.toString(), value.toString());
                }
            }
            boost::optional<QColor> neuronColor;
            if (okr && okg && okb && oka && red != -1 && green != -1 && blue != -1 && alpha != -1) {
                neuronColor = QColor::fromRgbF(red, green, blue, alpha);
            }
            auto & tree = addTree(boost::make_optional(!merge, treeID), neuronColor, properties);
            tree.render = render;
            if (merge) {
                treeMap.emplace(std::piecewise_construct, std::forward_as_tuple(treeID), std::forward_as_tuple(tree));
                treeID = tree.treeID;// newly assigned tree id
            }
            if (tree.getComment().isEmpty()) {// sets e.g. filename as tree comment when multiple files are loaded
                setComment(tree, treeCmtOnMultiLoad);
            }

            while (xml.readNextStartElement()) {
                if(xml.name() == "nodes") {
                    while(xml.readNextStartElement()) {
                        if(xml.name() == "node") {
                            boost::optional<decltype(nodeListElement::nodeID)> nodeID;
                            float radius = state->skeletonState->defaultNodeRadius;
                            Coordinate currentCoordinate;
                            ViewportType inVP = VIEWPORT_UNDEFINED;
                            int inMag = 0;
                            uint64_t ms = 0;
                            QVariantHash properties;
                            for (const auto & attribute : xml.attributes()) {
                                const auto & name = attribute.name();
                                const auto & value = attribute.value();
                                if (name == "id") {
                                    nodeID = value.toULongLong();
                                } else if (name == "radius") {
                                    radius = (nmlScale ? nmlScale->x / Dataset::current().scales[0].x : 1) * value.toDouble();
                                } else if (name == "x") {
                                    currentCoordinate.x = (nmlScale ? nmlScale->x / Dataset::current().scales[0].x : 1) * value.toDouble() - matlabCoordinates;
                                } else if (name == "y") {
                                    currentCoordinate.y = (nmlScale ? nmlScale->y / Dataset::current().scales[0].y : 1) * value.toDouble() - matlabCoordinates;
                                } else if (name == "z") {
                                    currentCoordinate.z = (nmlScale ? nmlScale->z / Dataset::current().scales[0].z : 1) * value.toDouble() - matlabCoordinates;
                                } else if (name == "inVp") {
                                    inVP = static_cast<ViewportType>(value.toInt());
                                } else if (name == "inMag") {
                                    inMag = {value.toInt()};
                                } else if (name == "time") {
                                    ms = {value.toULongLong()};
                                } else if (name != "comment") { // comments are added later in the comments section
                                    const auto property = name.toString();
                                    properties.insert(property, value.toString());
                                }
                            }

                            if (merge) {
                                auto & noderef = addNode(boost::none, radius, treeID, currentCoordinate, inVP, inMag, ms, false, properties).get();
                                nodeMap.emplace(std::piecewise_construct, std::forward_as_tuple(nodeID.get()), std::forward_as_tuple(noderef));
                            } else {
                                addNode(nodeID, radius, treeID, currentCoordinate, inVP, inMag, ms, false, properties);
                            }
                        } else {
                            skippedElements.insert(xml.name().toString());
                        }
                        xml.skipCurrentElement();
                    } // end while nodes
                } else if(xml.name() == "edges") {
                    while(xml.readNextStartElement()) {
                        if(xml.name() == "edge") {
                            const auto attributes = xml.attributes();
                            std::uint64_t sourceNodeId = attributes.value("source").toULongLong();
                            std::uint64_t targetNodeId = attributes.value("target").toULongLong();
                            edgeVector.emplace_back(sourceNodeId, targetNodeId);
                        } else {
                            skippedElements.insert(xml.name().toString());
                        }
                        xml.skipCurrentElement();
                    }
                } else {
                    skippedElements.insert(xml.name().toString());
                    xml.skipCurrentElement();
                }
            }
        } else {
            skippedElements.insert(xml.name().toString());
            xml.skipCurrentElement();
        }
    }
    xml.readNext();//</things>
    if (xml.hasError()) {
        throw std::runtime_error(tr("loadXmlSkeleton xml error in line %2: »%1«").arg(xml.errorString()).arg(xml.lineNumber()).toStdString());
    }
    if (rootTag != "things") {
        throw std::runtime_error{tr("loadXmlSkeleton: expected root tag <things>, got »%1«").arg(xml.name()).toStdString()};
    }


    for (const auto & property : numberProperties) {
        textProperties.remove(property);
    }

    auto getElem = [](const auto & map, const auto id, auto findByID){
        const auto findIt = map.find(id);
        if (findIt != std::end(map)) {
            return boost::make_optional(&findIt->second.get());
        } else {
            auto * ptr = findByID(id);
            return boost::make_optional(ptr != nullptr, ptr);
        }
    };

    for (const auto & elem : edgeVector) {
        auto sourceNode = getElem(nodeMap, elem.first, findNodeByNodeID);
        auto targetNode = getElem(nodeMap, elem.second, findNodeByNodeID);
        if (sourceNode && targetNode) {
            addSegment(*sourceNode.get(), *targetNode.get());
        } else {
            qWarning() << "Could not add segment between nodes" << elem.first << "and" << elem.second;
        }
    }

    for (const auto & elem : branchVector) {
        if (auto currentNode = getElem(nodeMap, elem, findNodeByNodeID)) {
            pushBranchNode(*currentNode.get());
        } else {
            qWarning() << tr("branch for non-existent node %1").arg(elem);
        }
    }

    for (const auto & elem : commentsVector) {
        if (auto currentNode = getElem(nodeMap, elem.first, findNodeByNodeID)) {
            setComment(*currentNode.get(), elem.second);
        }
    }

    for (const auto & tree : state->skeletonState->trees) {
        if(tree.properties.contains("synapticCleft")) {
            auto preSynapse = tree.properties.contains("preSynapse") ? getElem(nodeMap, tree.properties["preSynapse"].toULongLong(), findNodeByNodeID) : boost::none;
            auto postSynapse = tree.properties.contains("postSynapse") ? getElem(nodeMap, tree.properties["postSynapse"].toULongLong(), findNodeByNodeID) : boost::none;
            auto synapticCleft = getElem(treeMap, tree.treeID, findTreeByTreeID);
            if (preSynapse && postSynapse && synapticCleft) {
                addFinishedSynapse(**synapticCleft, **preSynapse, **postSynapse);
            } else {
                qWarning() << tr("Incomplete synapse %1 could not be loaded. The “preSynapse”, “postSynapse” or “synapticCleft“ property are either missing or invalid.").arg(tree.treeID);
            }
        }
    }
    }// QSignalBlocker
    emit resetData();

    qDebug() << "loading skeleton: "<< bench.nsecsElapsed() / 1e9 << "s";

    if (!merge) {
        setActiveNode(Skeletonizer::singleton().findNodeByNodeID(activeNodeID));
        if (loadedPosition) {
            state->viewer->setPosition(loadedPosition.get());
        }
    }
    if (skeletonState.activeNode == nullptr && !skeletonState.trees.empty() && !skeletonState.trees.front().nodes.empty()) {
        setActiveNode(&skeletonState.trees.front().nodes.front());
    }

    QMessageBox msgBox{QApplication::activeWindow()};
    auto msg = tr("");
    if (!skippedElements.empty()) {
        msg += tr("• Some unknown elements have been skipped.\n\n").arg(xml.lineNumber());
        QString buffer;
        QDebug{&buffer} << skippedElements;
        msgBox.setDetailedText(tr("Skipped elements: %1").arg(buffer));
    }
    const auto mismatchedDataset = !experimentName.isEmpty() && experimentName != Dataset::current().experimentname;
    if (mismatchedDataset) {
        msg += tr("• The annotation (created in dataset “%1”) does not belong to the currently loaded dataset (“%2”).\n\n").arg(experimentName).arg(Dataset::current().experimentname);
    }
    const auto task = Annotation::singleton().activeTask;
    Annotation::singleton().fileTask = {loadedProject, loadedCategory, loadedTaskName};
    const auto mismatchedTask = !task.isEmpty() && task != Annotation::singleton().fileTask;
    if (mismatchedTask) {
        state->viewer->mainWindow.widgetContainer.taskManagementWidget.logout();
        msg += tr("• The loaded file’s task “%1” (%2, %3) does not belong to your current task “%4” (%5, %6), so you have been logged out from AAM.\n")
               .arg(loadedTaskName, loadedProject, loadedCategory, task.name, task.project, task.category);
    }
    msg.chop(2);// remove the 2 newlines at the end
    if (!msg.isEmpty()) {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Incompatible Annotation File<br>Although the file was loaded successfully, <strong>working with it is not recommended.</strong>"));
        msgBox.setInformativeText(msg);
        msgBox.exec();
    }
    return treeMap;
}

bool Skeletonizer::delSegment(std::list<segmentListElement>::iterator segToDelIt) {
    if (!segToDelIt->forward) {
        delSegment(segToDelIt->sisterSegment);
        return false;
    }
    // Delete the segment out of the segment list and out of the visualization structure!
    /* A bit cumbersome, but we cannot delete the segment and then find its source node.. */
    auto & source = segToDelIt->source;
    auto & target = segToDelIt->target;
    target.segments.erase(segToDelIt->sisterSegment);
    source.segments.erase(segToDelIt);

    updateCircRadius(&source);
    updateCircRadius(&target);
    Annotation::singleton().unsavedChanges = true;
    emit segmentRemoved(source.nodeID, target.nodeID);
    return true;
}

/*
 * We have to delete the node from 2 structures: the skeleton's nested linked list structure
 * and the skeleton visualization structure (hashtable with skeletonDCs).
 */
bool Skeletonizer::delNode(std::uint64_t nodeID, nodeListElement *nodeToDel) {
    if (!nodeToDel) {
        nodeToDel = findNodeByNodeID(nodeID);
    }
    if (!nodeToDel) {
        qDebug() << tr("The given node %1 doesn't exist. Unable to delete it.").arg(nodeID);
        return false;
    }
    nodeID = nodeToDel->nodeID;

    setComment(*nodeToDel, ""); // deletes from comment hashtable

    if (nodeToDel->isSynapticNode) {
        nodeToDel->correspondingSynapse->remove(nodeToDel);
    }

    if (nodeToDel->isBranchNode) {
        auto foundIt = std::find(std::begin(state->skeletonState->branchStack), std::end(state->skeletonState->branchStack), nodeToDel->nodeID);
        state->skeletonState->branchStack.erase(foundIt);
    }

    state->skeletonState->nodesByNodeID.erase(nodeToDel->nodeID);
    if (nodeID < state->skeletonState->nextAvailableNodeID) {
        state->skeletonState->nextAvailableNodeID = nodeID;
    }

    if (nodeToDel->selected) {
        auto & selectedNodes = state->skeletonState->selectedNodes;
        const auto eraseit = std::find(std::begin(selectedNodes), std::end(selectedNodes), nodeToDel);
        if (eraseit != std::end(selectedNodes)) {
            selectedNodes.erase(eraseit);
        }
    }

    bool resetActiveNode = state->skeletonState->activeNode == nodeToDel;
    auto tree = nodeToDel->correspondingTree;
    const auto pos = nodeToDel->position;

    unsetSubobjectOfHybridNode(*nodeToDel);

    nodeListElement * newActiveNode = nullptr;
    if (resetActiveNode) {
        for (const auto & seg : nodeToDel->segments) {
            if (seg.source == *nodeToDel) {
                // activate next node in (any) tracing direction
                newActiveNode = &seg.target;
                break;
            } else { // if there's no next node in tracing direction, take a node in (any) opposite tracing direction
                newActiveNode = &seg.source;
            }
        }
    }

    for (auto segmentIt = std::begin(nodeToDel->segments); segmentIt != std::end(nodeToDel->segments); segmentIt = std::begin(nodeToDel->segments)) {
        delSegment(segmentIt);
    }

    nodeToDel->correspondingTree->nodes.erase(nodeToDel->iterator);

    emit nodeRemovedSignal(nodeID);

    if (resetActiveNode) {
        if (newActiveNode == nullptr) { // nodeToDel was not connected
            newActiveNode = findNearbyNode(tree, pos);
        }
        setActiveNode(newActiveNode);
    }

    Annotation::singleton().unsavedChanges = true;

    return true;
}

bool Skeletonizer::delTree(decltype(treeListElement::treeID) treeID) {
    auto * const treeToDel = findTreeByTreeID(treeID);
    if (treeToDel == nullptr) {
        qDebug() << tr("There exists no tree with ID %1. Unable to delete it.").arg(treeID);
        return false;
    }

    if(treeToDel->isSynapticCleft) { //if we delete a synapsticCleft, remove tree from synapses and unset the synaptic nodes
        for(auto it = std::begin(state->skeletonState->synapses); it != std::end(state->skeletonState->synapses);) {
            if(treeToDel == it->getCleft()) {
                it->reset();
                it = state->skeletonState->synapses.erase(it);
                break;
            } else {
               ++it;
            }
        }
    }
    {
        QSignalBlocker blocker{this};// bulk operation
        for (auto nodeIt = std::begin(treeToDel->nodes); nodeIt != std::end(treeToDel->nodes); nodeIt = std::begin(treeToDel->nodes)) {
            delNode(0, &*nodeIt);
        }
    }


    if (treeToDel->selected) {
        auto & selectedTrees = state->skeletonState->selectedTrees;
        const auto eraseit = std::find(std::begin(selectedTrees), std::end(selectedTrees), treeToDel);
        if (eraseit != std::end(selectedTrees)) {
            selectedTrees.erase(eraseit);
        }
    }

    //remove if active tree
    if (treeToDel == skeletonState.activeTree) {
        skeletonState.activeTree = nullptr;
    }

    skeletonState.trees.erase(treeToDel->iterator);
    skeletonState.treesByID.erase(treeID);

    //no references to tree left
    emit treeRemovedSignal(treeID);//updates tools

    //if the active tree was not set through the active node but another tree exists
    if (skeletonState.activeTree == nullptr && !skeletonState.trees.empty()) {
        setActiveTreeByID(skeletonState.trees.front().treeID);
    }

    Annotation::singleton().unsavedChanges = true;
    return true;
}

nodeListElement * Skeletonizer::findNearbyNode(treeListElement * nearbyTree, Coordinate searchPosition) {
    nodeListElement * nodeWithCurrentlySmallestDistance = nullptr;
    float smallestDistance = std::numeric_limits<float>::max();//maximum distance

    auto searchTree = [&smallestDistance, &nodeWithCurrentlySmallestDistance, searchPosition](treeListElement & tree){
        for (auto & currentNode : tree.nodes) {
            // We make the nearest node the next active node
            const floatCoordinate distanceVector = searchPosition - currentNode.position;
            const auto distance = distanceVector.length();
            //set nearest distance to distance to first node found, then to distance of any nearer node found.
            if (distance  < smallestDistance) {
                smallestDistance = distance;
                nodeWithCurrentlySmallestDistance = &currentNode;
            }
        }
    };

    //  If available, search for a node within nearbyTree first.
    if (nearbyTree != nullptr) {
        searchTree(*nearbyTree);
    }

    if (nodeWithCurrentlySmallestDistance == nullptr) {
        // Ok, we didn't find any node in nearbyTree.
        // Now we take the nearest node, independent of the tree it belongs to.
        for (auto & currentTree : skeletonState.trees) {
            searchTree(currentTree);
        }
    }

    return nodeWithCurrentlySmallestDistance;
}

bool Skeletonizer::setActiveTreeByID(decltype(treeListElement::treeID) treeID) {
    treeListElement *currentTree;
    currentTree = findTreeByTreeID(treeID);
    if (currentTree == nullptr) {
        qDebug() << tr("There exists no tree with ID %1!").arg(treeID);
        return false;
    } else if (currentTree == state->skeletonState->activeTree) {
        return true;
    }

    state->skeletonState->activeTree = currentTree;

    selectTrees({currentTree});

    //switch to nearby node of new tree
    if (state->skeletonState->activeNode != nullptr && state->skeletonState->activeNode->correspondingTree != currentTree) {
        //prevent ping pong if tree was activated from setActiveNode
        auto * node = findNearbyNode(currentTree, state->skeletonState->activeNode->position);
        if (node->correspondingTree == currentTree) {
            setActiveNode(node);
        } else if (!currentTree->nodes.empty()) {
            setActiveNode(&currentTree->nodes.front());
        }
    } else if (state->skeletonState->activeNode == nullptr && !currentTree->nodes.empty()) {
        setActiveNode(&currentTree->nodes.front());
    }

    Annotation::singleton().unsavedChanges = true;

    return true;
}

bool Skeletonizer::setActiveNode(nodeListElement *node) {
    if (node == state->skeletonState->activeNode) {
        return true;
    }

    state->skeletonState->activeNode = node;

    if (node == nullptr) {
        selectNodes({});
        if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeTracing)) {
            Segmentation::singleton().clearObjectSelection();
        }
    } else {
        if (!node->selected) {
            selectNodes({node});
        }
        if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeTracing)) {
            selectObjectForNode(*node);
        }
        if (skeletonState.lockPositions && node->getComment().contains(skeletonState.lockingComment, Qt::CaseInsensitive)) {
            lockPosition(node->position);
        }
        setActiveTreeByID(node->correspondingTree->treeID);
    }

    skeletonState.jumpToSkeletonNext = true;
    Annotation::singleton().unsavedChanges = true;

    return true;
}

template<typename T, typename U>
auto findNextAvailableID(const T & lowerBound, const U & map) {
    for (auto i = lowerBound; ; ++i) {
        if (map.find(i) == std::end(map)) {
            return i;
        }
    }
}

boost::optional<nodeListElement &> Skeletonizer::addNode(boost::optional<decltype(nodeListElement::nodeID)> nodeId, const decltype(nodeListElement::position) & position, const treeListElement & tree, const decltype(nodeListElement::properties) & properties) {
    return addNode(nodeId, skeletonState.defaultNodeRadius, tree.treeID, position, ViewportType::VIEWPORT_UNDEFINED, -1, boost::none, false, properties);
}

boost::optional<nodeListElement &> Skeletonizer::addNode(boost::optional<std::uint64_t> nodeID, const float radius, const decltype(treeListElement::treeID) treeID, const Coordinate & position
        , const ViewportType VPtype, const int inMag, boost::optional<uint64_t> time, const bool respectLocks, const QHash<QString, QVariant> & properties) {
    state->skeletonState->branchpointUnresolved = false;

     // respectLocks refers to locking the position to a specific coordinate such as to
     // disallow tracing areas further than a certain distance away from a specific point in the
     // dataset.
    if (respectLocks) {
        if(state->skeletonState->positionLocked) {
            const floatCoordinate lockVector = position - state->skeletonState->lockedPosition;
            float lockDistance = lockVector.length();
            if (lockDistance > state->skeletonState->lockRadius) {
                qDebug() << tr("Node is too far away from lock point (%1), not adding.").arg(lockDistance);
                return boost::none;
            }
        }
    }

    if (nodeID && findNodeByNodeID(nodeID.get())) {
        qDebug() << tr("Node with ID %1 already exists, no node added.").arg(nodeID.get());
        return boost::none;
    }
    auto * const tempTree = findTreeByTreeID(treeID);

    if (!tempTree) {
        qDebug() << tr("There exists no tree with the provided ID %1!").arg(treeID);
        return boost::none;
    }

    if (!nodeID) {
        nodeID = skeletonState.nextAvailableNodeID;
    }

    if (!time) {//time was not provided
        time = Annotation::singleton().getAnnotationTime() + Annotation::singleton().currentTimeSliceMs();
    }

    tempTree->nodes.emplace_back(nodeID.get(), radius, position, inMag, VPtype, time.get(), properties, *tempTree);
    auto & tempNode = tempTree->nodes.back();
    tempNode.iterator = std::prev(std::end(tempTree->nodes));
    updateCircRadius(&tempNode);
    updateSubobjectCountFromProperty(tempNode);

    state->skeletonState->nodesByNodeID.emplace(nodeID.get(), &tempNode);

    bool isPropertiesChanged = false;
    for (auto & property : properties.keys()) {
        if (!numberProperties.contains(property) && !textProperties.contains(property)) {
            textProperties.insert(property);
            isPropertiesChanged = true;
        }
    }
    if (isPropertiesChanged) {
        emit propertiesChanged(numberProperties, textProperties);
    }

    if (nodeID == state->skeletonState->nextAvailableNodeID) {
        skeletonState.nextAvailableNodeID = findNextAvailableID(skeletonState.nextAvailableNodeID, skeletonState.nodesByNodeID);
    }
    Annotation::singleton().unsavedChanges = true;

    emit nodeAddedSignal(tempNode);

    return tempNode;
}

bool Skeletonizer::addSegment(nodeListElement & sourceNode, nodeListElement & targetNode) {
    if (findSegmentBetween(sourceNode, targetNode) != std::end(sourceNode.segments)) {
        qDebug() << "Segment between nodes" << sourceNode.nodeID << "and" << targetNode.nodeID << "exists already.";
        return false;
    }
    if (sourceNode == targetNode) {
        qDebug() << "Cannot link node with itself!";
        return false;
    }

     // Add the segment to the tree structure
    sourceNode.segments.emplace_back(sourceNode, targetNode);
    targetNode.segments.emplace_back(sourceNode, targetNode, false);//although it’s the ›reverse‹ segment, its direction doesn’t change
    auto sourceSegIt = std::prev(std::end(sourceNode.segments));
    auto targetSegIt = std::prev(std::end(targetNode.segments));
    sourceSegIt->sisterSegment = targetSegIt;
    sourceSegIt->sisterSegment->sisterSegment = sourceSegIt;

    /* Do we really skip this node? Test cum dist. to last rendered node! */
    sourceSegIt->length = sourceSegIt->sisterSegment->length = Dataset::current().scales[0].componentMul(targetNode.position - sourceNode.position).length();

    updateCircRadius(&sourceNode);
    updateCircRadius(&targetNode);

    Annotation::singleton().unsavedChanges = true;
    emit segmentAdded(sourceNode.nodeID, targetNode.nodeID);
    return true;
}

void Skeletonizer::toggleLink(nodeListElement & lhs, nodeListElement & rhs) {
    auto segmentIt = findSegmentBetween(lhs, rhs);
    if (segmentIt != std::end(lhs.segments)) {
        delSegment(segmentIt);
    } else if ((segmentIt = findSegmentBetween(rhs, lhs)) != std::end(rhs.segments)) {
        delSegment(segmentIt);
    } else {
        addSegment(lhs, rhs);
    }
}

void Skeletonizer::clearSkeleton() {
    skeletonState = SkeletonState{};
    numberProperties = textProperties = {};
    emit resetData();
}

bool Skeletonizer::mergeTrees(decltype(treeListElement::treeID) treeID1, decltype(treeListElement::treeID) treeID2) {
    if(treeID1 == treeID2) {
        qDebug() << "Could not merge trees. Provided IDs are the same!";
        return false;
    }

    treeListElement * const tree1 = findTreeByTreeID(treeID1);
    treeListElement * const tree2 = findTreeByTreeID(treeID2);

    if(!(tree1) || !(tree2)) {
        qDebug() << "Could not merge trees, provided IDs are not valid!";
        return false;
    }

    for (auto & node : tree2->nodes) {
        node.correspondingTree = tree1;
    }
    tree1->nodes.splice(std::end(tree1->nodes), tree2->nodes);
    if (tree2->mesh) {
        if (tree1->mesh == nullptr) {
            std::swap(tree1->mesh, tree2->mesh);
            tree1->mesh->correspondingTree = tree1;
        } else {
            mergeMeshes(*tree1->mesh.get(), *tree2->mesh.get());
        }
    }
    delTree(tree2->treeID);
    setActiveTreeByID(tree1->treeID);
    emit treesMerged(treeID1, treeID2);//update nodes
    return true;
}

template<typename T>
auto mergeBuffers = [](QOpenGLBuffer & buf1, QOpenGLBuffer & buf2, boost::optional<T> offset = boost::none) {
    buf1.bind();
    auto size1 = buf1.size() / sizeof(T);
    buf2.bind();
    auto size2 = buf2.size() / sizeof(T);
    std::vector<T> mergedData(size1 + size2);
    buf2.read(0, mergedData.data() + size1, size2 * sizeof(T));
    buf1.bind();
    buf1.read(0, mergedData.data(), size1 * sizeof(T));
    if (offset) {
        std::for_each(std::begin(mergedData) + size1, std::end(mergedData), [&offset](T & value) { value += offset.get(); });
    }
    buf1.allocate(mergedData.data(), mergedData.size() * sizeof(T));
    buf1.release();
    buf2.release();
};

void Skeletonizer::mergeMeshes(Mesh & mesh1, Mesh & mesh2) {
    mergeBuffers<float>(mesh1.position_buf, mesh2.position_buf);
    mergeBuffers<float>(mesh1.normal_buf, mesh2.normal_buf);
    mergeBuffers<unsigned int>(mesh1.index_buf, mesh2.index_buf, mesh1.vertex_count);

    std::vector<std::array<std::uint8_t, 4>> colors(mesh1.vertex_count + mesh2.vertex_count);
    const auto copyMesh = [&colors](auto & mesh, const auto meshOffset) {
        if (mesh.useTreeColor) {
            const auto & treeCol = mesh.correspondingTree->color.rgba64();
            const decltype(colors)::value_type treeColor{{treeCol.red8(), treeCol.green8(), treeCol.blue8(), treeCol.alpha8()}};
            std::fill_n(std::begin(colors) + meshOffset, mesh.vertex_count, treeColor);
        } else {
            mesh.color_buf.bind();
            mesh.color_buf.read(0, colors.data() + meshOffset, mesh.color_buf.size());
            mesh.color_buf.release();
        }
    };
    const auto atLeastOneTreeHasPerVertexColors = !mesh1.useTreeColor || !mesh2.useTreeColor;
    if (atLeastOneTreeHasPerVertexColors) {
        copyMesh(mesh1, 0);
        copyMesh(mesh2, mesh1.vertex_count);
        mesh1.color_buf.bind();
        mesh1.color_buf.allocate(colors.data(), colors.size() * sizeof(colors[0]));
        mesh1.color_buf.release();
    }

    mesh1.useTreeColor = !atLeastOneTreeHasPerVertexColors;
    mesh1.vertex_count += mesh2.vertex_count;
    mesh1.index_count += mesh2.index_count;
}

template<typename Func>
nodeListElement * Skeletonizer::getNodeWithRelationalID(nodeListElement * currentNode, bool sameTree, Func func) {
    if (currentNode == nullptr) {
        // no current node, return active node
        if (state->skeletonState->activeNode != nullptr) {
            return state->skeletonState->activeNode;
        }
        // no active node, simply return first node found
        for (auto & tree : skeletonState.trees) {
            if (!tree.nodes.empty()) {
                return &tree.nodes.front();
            }
        }
        qDebug() << "no nodes to move to";
        return nullptr;
    }

    nodeListElement * extremalNode = nullptr;
    auto globalExtremalIdDistance = std::numeric_limits<decltype(nodeListElement::nodeID)>::max();
    auto searchTree = [&](treeListElement & tree){
        for (auto & node : tree.nodes) {
            if (func(node.nodeID, currentNode->nodeID)) {
                const auto localExtremalIdDistance = std::max(currentNode->nodeID, node.nodeID) - std::min(currentNode->nodeID, node.nodeID);
                if (localExtremalIdDistance == 1) {//smallest distance possible
                    globalExtremalIdDistance = localExtremalIdDistance;
                    extremalNode = &node;
                    return;
                }
                if (localExtremalIdDistance < globalExtremalIdDistance) {
                    globalExtremalIdDistance = localExtremalIdDistance;
                    extremalNode = &node;
                }
            }
        }
    };
    if (sameTree) {
        searchTree(*currentNode->correspondingTree);
    }
    for (auto & tree : skeletonState.trees) {
        searchTree(tree);
    }
    return extremalNode;

}

nodeListElement * Skeletonizer::getNodeWithPrevID(nodeListElement * currentNode, bool sameTree) {
    return getNodeWithRelationalID(currentNode, sameTree, std::less<decltype(nodeListElement::nodeID)>{});
}

nodeListElement * Skeletonizer::getNodeWithNextID(nodeListElement * currentNode, bool sameTree) {
    return getNodeWithRelationalID(currentNode, sameTree, std::greater<decltype(nodeListElement::nodeID)>{});
}

nodeListElement* Skeletonizer::findNodeByNodeID(std::uint64_t nodeID) {
    const auto nodeIt = state->skeletonState->nodesByNodeID.find(nodeID);
    return nodeIt != std::end(state->skeletonState->nodesByNodeID) ? nodeIt->second : nullptr;
}

QList<nodeListElement*> Skeletonizer::findNodesInTree(treeListElement & tree, const QString & comment) {
    QList<nodeListElement *> hits;
    for (auto & node : tree.nodes) {
        if (node.getComment().contains(comment, Qt::CaseInsensitive)) {
            hits.append(&node);
        }
    }
    return hits;
}

treeListElement & Skeletonizer::addTree(boost::optional<decltype(treeListElement::treeID)> treeID, boost::optional<decltype(treeListElement::color)> color, const decltype(treeListElement::properties) & properties) {
    if (!treeID) {
        treeID = skeletonState.nextAvailableTreeID;
    }
    if (findTreeByTreeID(treeID.get()) != nullptr) {
        const auto errorString = tr("Tree with ID %1 already exists").arg(treeID.get());
        qDebug() << errorString;
        throw std::runtime_error(errorString.toStdString());
    }

    skeletonState.trees.emplace_back(treeID.get(), properties);
    treeListElement & newTree = skeletonState.trees.back();
    newTree.iterator = std::prev(std::end(skeletonState.trees));
    state->skeletonState->treesByID.emplace(newTree.treeID, &newTree);

    if (!color) {// set default tree color
        QSignalBlocker blocker{this};// don’t emit treeChanged before treeAdded later
        restoreDefaultTreeColor(newTree);
    } else {
        newTree.color = color.get();
        newTree.colorSetManually = true;
    }

    if (newTree.treeID == state->skeletonState->nextAvailableTreeID) {
        skeletonState.nextAvailableTreeID = findNextAvailableID(skeletonState.nextAvailableTreeID, skeletonState.treesByID);
    }

    Annotation::singleton().unsavedChanges = true;

    emit treeAddedSignal(newTree);

    setActiveTreeByID(newTree.treeID);

    return newTree;
}

template<typename Func>
treeListElement * Skeletonizer::getTreeWithRelationalID(treeListElement * currentTree, Func func) {
    if (currentTree == nullptr) {
        if (skeletonState.activeTree != nullptr) {// no current tree, return active tree
            return skeletonState.activeTree;
        }
        // no active tree, simply return first tree found
        if (!skeletonState.trees.empty()) {
            return &skeletonState.trees.front();
        }
        qDebug() << "no tree to move to";
        return nullptr;
    }

    treeListElement * treeFound = nullptr;
    std::uint64_t extremalIdDistance = skeletonState.trees.size();
    for (auto & tree : skeletonState.trees) {
        if (func(tree.treeID, currentTree->treeID)) {
            const auto distance = std::max(currentTree->treeID, tree.treeID) - std::min(currentTree->treeID, tree.treeID);
            if (distance == 1) {//smallest distance possible
                return &tree;
            }
            if (distance < extremalIdDistance) {
                extremalIdDistance = distance;
                treeFound = &tree;
            }
        }
    }
    return treeFound;
}

treeListElement * Skeletonizer::getTreeWithPrevID(treeListElement * currentTree) {
    return getTreeWithRelationalID(currentTree, std::less<decltype(treeListElement::treeID)>{});
}

treeListElement * Skeletonizer::getTreeWithNextID(treeListElement * currentTree) {
    return getTreeWithRelationalID(currentTree, std::greater<decltype(treeListElement::treeID)>{});
}

void Skeletonizer::setColor(treeListElement & tree, const QColor & color) {
    tree.color = color;
    tree.colorSetManually = true;
    Annotation::singleton().unsavedChanges = true;
    emit treeChangedSignal(tree);
}

void Skeletonizer::setRender(treeListElement & tree, const bool render) {
    tree.render = render;
    emit Skeletonizer::singleton().treeChangedSignal(tree);
}

std::list<segmentListElement>::iterator Skeletonizer::findSegmentBetween(nodeListElement & sourceNode, const nodeListElement & targetNode) {
    for (auto segmentIt = std::begin(sourceNode.segments); segmentIt != std::end(sourceNode.segments); ++segmentIt) {
        if (!segmentIt->forward) {
            continue;
        }
        if (segmentIt->target == targetNode) {
            return segmentIt;
        }
    }
    return std::end(sourceNode.segments);
}

void Skeletonizer::setRadius(nodeListElement & node, const float radius) {
    node.radius = radius;
    updateCircRadius(&node);
    Annotation::singleton().unsavedChanges = true;
    emit nodeChangedSignal(node);
}

void Skeletonizer::setPosition(nodeListElement & node, const Coordinate & position) {
    auto oldPos = node.position;
    node.position = position.capped({0, 0, 0}, Dataset::current().boundary);
    const quint64 newSubobjectId = readVoxel(position);
    Skeletonizer::singleton().movedHybridNode(node, newSubobjectId, oldPos);
    Annotation::singleton().unsavedChanges = true;
    emit nodeChangedSignal(node);
}

bool Skeletonizer::extractConnectedComponent(std::uint64_t nodeID) {
    //  This function takes a node and splits the connected component
    //  containing that node into a new tree, unless the connected component
    //  is equivalent to exactly one entire tree.

    //  It uses breadth-first-search. Depth-first-search would be the same with
    //  a stack instead of a queue for storing pending nodes. There is no
    //  practical difference between the two algorithms for this task.

    auto * firstNode = findNodeByNodeID(nodeID);
    if (!firstNode) {
        return false;
    }

    auto [visitedNodes, visitedTrees] = connectedComponent(*firstNode);

    //  If the total number of nodes visited is smaller than the sum of the
    //  number of nodes in all trees we've seen, the connected component is a
    //  strict subgraph of the graph containing all trees we've seen and we
    //  should split it.

    // We want this function to not do anything when
    // there are no disconnected components in the same tree, but create a new
    // tree when the connected component spans several trees. This is a useful
    // feature when performing skeleton consolidation and allows one to merge
    // many trees at once.
    std::size_t nodeCountSeenTrees = 0;
    for(auto * tree : visitedTrees) {
        nodeCountSeenTrees += tree->nodes.size();
    }
    if (visitedNodes.size() == nodeCountSeenTrees && visitedTrees.size() == 1) {
        return false;
    }

    auto & newTree = addTree();
    // Splitting the connected component.
    for (auto * nodeIt : visitedNodes) {
        if (nodeIt->correspondingTree->nodes.empty()) {//remove empty trees
            delTree(nodeIt->correspondingTree->treeID);
        }
        // Removing node list element from its old position
        // Inserting node list element into new list.
        newTree.nodes.splice(std::end(newTree.nodes), nodeIt->correspondingTree->nodes, nodeIt->iterator);
        nodeIt->correspondingTree = &newTree;
    }
    Annotation::singleton().unsavedChanges = true;
    setActiveTreeByID(newTree.treeID);//the empty tree had no active node

    return true;
}

boost::optional<nodeListElement &> Skeletonizer::unconnectedNode(nodeListElement & node) const {
    for (auto [id, nextNode] : skeletonState.nodesByNodeID) {
        auto * segments = nextNode->getSegments();
        if (nextNode == &node || segments->size() > 1) {
            continue; // enough to look at degree 1 nodes
        } else if (segments->size() == 0) {
            return *nextNode;
        }
        auto path = shortestPath(node, *nextNode);
        if (path.size() == 0) {
            return *nextNode;
        }
    }
    return boost::none;
}

void Skeletonizer::notifyChanged(treeListElement & tree) {
    emit treeChangedSignal(tree);
}
void Skeletonizer::notifyChanged(nodeListElement & node) {
    emit nodeChangedSignal(node);
}

template<typename T>
void Skeletonizer::setComment(T & elem, const QString & newContent) {
    if (newContent.isEmpty()) {
        elem.properties.remove("comment");
    } else {
        elem.setComment(newContent);
    }
    Annotation::singleton().unsavedChanges = true;
    notifyChanged(elem);
}
template void Skeletonizer::setComment(treeListElement &, const QString & newContent);// explicit instantiation for other TUs
template void Skeletonizer::setComment(nodeListElement &, const QString & newContent);

/*
 * Create a synapse, starting with a presynapse
 * 1. 'shift+c': Active Node is marked as a presynapse
 * 2. Start tracing the synaptic cleft ('c' on finish)
 * 3. Next node is a postsynapse
 */
void Skeletonizer::continueSynapse() {
    if(skeletonState.synapseState == Synapse::State::PreSynapse) { //set active Node as presynapse
        if (skeletonState.activeNode != nullptr) {
            skeletonState.temporarySynapse.setPreSynapse(*skeletonState.activeNode);
            auto & synapticCleft = addTree();
            skeletonState.temporarySynapse.setCleft(synapticCleft);
            skeletonState.synapseState = Synapse::State::Cleft;
        }
    } else if(skeletonState.synapseState == Synapse::State::Cleft) {
        skeletonState.synapseState = Synapse::State::PostSynapse;
    }
}

void Skeletonizer::addFinishedSynapse(treeListElement & cleft, nodeListElement & pre, nodeListElement & post) {
    Synapse syn;
    syn.setCleft(cleft);
    syn.setPreSynapse(pre);
    syn.setPostSynapse(post);
    skeletonState.synapses.push_back(syn);
    cleft.correspondingSynapse = &skeletonState.synapses.back();
    pre.correspondingSynapse = &skeletonState.synapses.back();
    post.correspondingSynapse = &skeletonState.synapses.back();
}

/*
 * Create a synapse from two nodes from different trees
 */
void Skeletonizer::addSynapseFromNodes(std::vector<nodeListElement *> & nodes) {
    if(nodes[0]->correspondingTree == nodes[1]->correspondingTree) return;
    if(nodes[0]->correspondingTree->isSynapticCleft
        || nodes[1]->correspondingTree->isSynapticCleft) return;

    if(nodes[0]->isSynapticNode || nodes[1]->isSynapticNode) {
        auto & synapse = nodes[0]->isSynapticNode ? nodes[0]->correspondingSynapse : nodes[1]->correspondingSynapse;
        synapse->setPreSynapse(*nodes[0]);
        synapse->setPostSynapse(*nodes[1]);
        return;
    }
    //Add a synaptic cleft tree with one node between the two selected nodes
    addFinishedSynapse(addTree(), *nodes[0], *nodes[1]);

    const auto minx = std::min(nodes[0]->position.x, nodes[1]->position.x);
    const auto miny = std::min(nodes[0]->position.y, nodes[1]->position.y);
    const auto minz = std::min(nodes[0]->position.z, nodes[1]->position.z);

    Coordinate coord = { minx + ( std::abs(nodes[0]->position.x-nodes[1]->position.x)/2 ),
                          miny + ( std::abs(nodes[0]->position.y-nodes[1]->position.y)/2 ),
                          minz + ( std::abs(nodes[0]->position.z-nodes[1]->position.z)/2 )};

    addNode(  boost::none,
              state->skeletonState->defaultNodeRadius,
              state->skeletonState->activeTree->treeID,
              coord,
              VIEWPORT_UNDEFINED,
              Dataset::current().magnification,
              boost::none,
              true);
}

bool Skeletonizer::unlockPosition() {
    if(state->skeletonState->positionLocked) {
        qDebug() << "Spatial locking disabled.";
    }
    state->skeletonState->positionLocked = false;
    emit unlockedNode();
    return true;
}

bool Skeletonizer::lockPosition(Coordinate lockCoordinate) {
    qDebug("locking to (%d, %d, %d).", lockCoordinate.x, lockCoordinate.y, lockCoordinate.z);
    state->skeletonState->positionLocked = true;
    state->skeletonState->lockedPosition = lockCoordinate;
    emit lockedToNode(skeletonState.activeNode->nodeID);
    return true;
}

nodeListElement * Skeletonizer::popBranchNode() {
    if (state->skeletonState->branchStack.empty()) {
        return nullptr;
    }
    std::size_t branchNodeID = state->skeletonState->branchStack.back();
    state->skeletonState->branchStack.pop_back();

    auto * branchNode = findNodeByNodeID(branchNodeID);
    assert(branchNode->isBranchNode);
    branchNode->isBranchNode = false;
    state->skeletonState->branchpointUnresolved = true;
    qDebug() << "Branch point" << branchNodeID << "deleted.";

    setActiveNode(branchNode);
    state->viewer->userMove(branchNode->position - state->viewerState->currentPosition, USERMOVE_NEUTRAL);

    emit branchPoppedSignal();
    Annotation::singleton().unsavedChanges = true;
    return branchNode;
}

void Skeletonizer::pushBranchNode(nodeListElement & branchNode) {
    if (!branchNode.isBranchNode) {
        state->skeletonState->branchStack.emplace_back(branchNode.nodeID);
        branchNode.isBranchNode = true;
        qDebug() << "Branch point" << branchNode.nodeID << "added.";
        emit branchPushedSignal();
        Annotation::singleton().unsavedChanges = true;
    } else {
        qDebug() << "Active node is already a branch point";
    }
}

void Skeletonizer::jumpToNode(const nodeListElement & node) {
    state->viewer->setPosition(node.position, USERMOVE_NEUTRAL);
    emit jumpedToNodeSignal(node);
}

void Skeletonizer::restoreDefaultTreeColor(treeListElement & tree) {
    const auto index = tree.treeID % state->viewerState->treeColors.size();
    tree.color = QColor::fromRgb(std::get<0>(state->viewerState->treeColors[index])
                       , std::get<1>(state->viewerState->treeColors[index])
                       , std::get<2>(state->viewerState->treeColors[index]));
    tree.colorSetManually = false;
    Annotation::singleton().unsavedChanges = true;
    emit treeChangedSignal(tree);
}

void Skeletonizer::updateTreeColors() {
    for (auto & tree : skeletonState.trees) {
        restoreDefaultTreeColor(tree);
    }
}

/**
 * @brief Skeletonizer::updateCircRadius if both the source and target node of a segment are outside the viewport,
 *      the segment would be culled away, too, regardless of wether it is in the viewing frustum.
 *      This function solves this by making the circ radius of one node as large as the segment,
 *      so that it cuts the viewport and its segment is rendered.
 * @return
 */
bool Skeletonizer::updateCircRadius(nodeListElement *node) {
    node->circRadius = singleton().radius(*node);
    /* Any segment longer than the current circ radius?*/
    for (const auto & currentSegment : node->segments) {
        if (currentSegment.length > node->circRadius) {
            node->circRadius = currentSegment.length;
        }
    }
    return true;
}

float Skeletonizer::radius(const nodeListElement & node) const {
    const auto propertyName = state->viewerState->highlightedNodePropertyByRadius;
    if(!propertyName.isEmpty() && node.properties.contains(propertyName)) {
        return state->viewerState->nodePropertyRadiusScale * node.properties.value(propertyName).toDouble();
    } else {
        const auto comment = node.getComment();
        if(comment.isEmpty() == false && CommentSetting::useCommentNodeRadius) {
            float newRadius = CommentSetting::getRadius(comment);
            if(newRadius != 0) {
                return newRadius;
            }
        }
    }
    return state->viewerState->overrideNodeRadiusBool ? state->viewerState->overrideNodeRadiusVal : node.radius;
}

void Skeletonizer::convertToNumberProperty(const QString & property) {
    if (numberProperties.contains(property)) {
        throw std::runtime_error("no conversion for number properties");
    }
    for (auto pair : skeletonState.nodesByNodeID) {
        auto & node = *pair.second;
        const auto it = node.properties.find(property);
        if (it != std::end(node.properties)) {
            node.properties[property] = it->toDouble();
        }
    }
    numberProperties.insert(property);
    textProperties.remove(property);
    emit resetData();
}

void Skeletonizer::goToNode(const NodeGenerator::Direction direction) {
    if (skeletonState.activeNode == nullptr) {
        return;
    }
    static NodeGenerator traverser(*skeletonState.activeNode, direction);
    static nodeListElement * lastNode = skeletonState.activeNode;

    if ((lastNode != skeletonState.activeNode) || direction != traverser.direction) {
        traverser = NodeGenerator(*skeletonState.activeNode, direction);
    }
    if (traverser.reachedEnd == false) {
        ++traverser;
    } else {
        return;
    }

    while (traverser.reachedEnd == false) {
        if (&(*traverser) != skeletonState.activeNode) {
            setActiveNode(&(*traverser));
            lastNode = &(*traverser);
            state->viewer->setPositionWithRecentering((*traverser).position);
            return;
        } else if (traverser.reachedEnd == false) {
           ++traverser;
        } else {
            return;
        }
    }
}

void Skeletonizer::moveSelectedNodesToTree(decltype(treeListElement::treeID) treeID) {
    if (auto * newTree = findTreeByTreeID(treeID)) {
        for (auto * const node : state->skeletonState->selectedNodes) {
            newTree->nodes.splice(std::end(newTree->nodes), node->correspondingTree->nodes, node->iterator);
            node->correspondingTree = newTree;
        }
        emit resetData();
        emit setActiveTreeByID(treeID);
    }
}

template<typename T, std::enable_if_t<std::is_same<T, nodeListElement>::value>* = nullptr>
auto & selected() {
    return state->skeletonState->selectedNodes;
}
template<typename T, std::enable_if_t<std::is_same<T, treeListElement>::value>* = nullptr>
auto & selected() {
    return state->skeletonState->selectedTrees;
}

auto getId(const nodeListElement & elem) {
    return elem.nodeID;
}
auto getId(const treeListElement & elem) {
    return elem.treeID;
}

void Skeletonizer::setActive(nodeListElement & elem) {
    setActiveNode(&elem);
}
void Skeletonizer::setActive(treeListElement & elem) {
    setActiveTreeByID(elem.treeID);
}

template<>
nodeListElement * Skeletonizer::active<nodeListElement>() {
    return state->skeletonState->activeNode;
}
template<>
treeListElement * Skeletonizer::active<treeListElement>() {
    return state->skeletonState->activeTree;
}

template<>
void Skeletonizer::notifySelection<nodeListElement>() {
    emit nodeSelectionChangedSignal();
}
template<>
void Skeletonizer::notifySelection<treeListElement>() {
    emit treeSelectionChangedSignal();
}

template<typename T>
void Skeletonizer::select(QSet<T*> elems) {
    for (auto & elem : selected<T>()) {
        if (elems.contains(elem)) {//if selected node is already selected ignore
            elems.remove(elem);
        } else {//unselect everything else
            elems.insert(elem);
        }
    }
    toggleSelection(elems);
}

template<typename T>
void Skeletonizer::toggleSelection(const QSet<T*> & elems) {
    auto & selectedElems = selected<T>();

    std::unordered_set<decltype(getId(std::declval<const T &>()))> removeElems;
    for (auto & elem : elems) {
        elem->selected = !elem->selected;//toggle
        if (elem->selected) {
            selectedElems.emplace_back(elem);
        } else {
            removeElems.emplace(getId(*elem));
        }
    }
    auto eraseIt = std::remove_if(std::begin(selectedElems), std::end(selectedElems), [&removeElems](T const * const elem){
        return removeElems.find(getId(*elem)) != std::end(removeElems);
    });
    if (eraseIt != std::end(selectedElems)) {
        selectedElems.erase(eraseIt, std::end(selectedElems));
    }

    if (selectedElems.size() == 1) {
        setActive(*selectedElems.front());
    } else if (selectedElems.empty() && active<T>() != nullptr) {
        select<T>({active<T>()});// at least one must always be selected
        return;
    }
    notifySelection<T>();
}

template void Skeletonizer::toggleSelection<nodeListElement>(const QSet<nodeListElement*> & nodes);
template void Skeletonizer::toggleSelection<treeListElement>(const QSet<treeListElement*> & trees);

void Skeletonizer::toggleNodeSelection(const QSet<nodeListElement*> & nodeSet) {
    toggleSelection(nodeSet);
}

void Skeletonizer::selectNodes(QSet<nodeListElement*> nodeSet) {
    select(nodeSet);
}

void Skeletonizer::selectTrees(const std::vector<treeListElement*> & trees) {
    QSet<treeListElement*> treeSet;
    for (const auto & elem : trees) {
        treeSet.insert(elem);
    }
    select(treeSet);
}

void Skeletonizer::inferTreeSelectionFromNodeSelection() {
    QSet<treeListElement*> treeSet;
    for (const auto & elem : skeletonState.selectedNodes) {
        treeSet.insert(elem->correspondingTree);
    }
    select(treeSet);
}

void Skeletonizer::deleteSelectedTrees() {
    // make a copy because the selected objects can change
    const auto treesToDelete = state->skeletonState->selectedTrees;
    bulkOperation(treesToDelete, [this](auto & tree){
        delTree(tree.treeID);
    });
}

void Skeletonizer::deleteSelectedNodes() {
    // make a copy because the selected objects can change
    const auto nodesToDelete = state->skeletonState->selectedNodes;
    bulkOperation(nodesToDelete, [this](auto & node){
        delNode(0, &node);
    });
    if (skeletonState.activeNode != nullptr) {
        selectNodes({state->skeletonState->activeNode});
    }
}

QSet<nodeListElement *> Skeletonizer::findCycle() {
    for (TreeTraverser nodeTraverser(state->skeletonState->trees); !nodeTraverser.reachedEnd; ++nodeTraverser) {
        if (!nodeTraverser.nodeIter.cycle.empty()) {
            return nodeTraverser.nodeIter.cycle;
        }
    }
    return {};
}

QSet<nodeListElement *> Skeletonizer::getPath(std::vector<nodeListElement *> & nodes) {
    for (auto it1 = std::begin(nodes); it1 != std::end(nodes) - 1; it1++)  {
        for (auto it2 = it1+1; it2 != std::end(nodes); it2++) {
            auto path = shortestPath(**it1, **it2);
            if (!path.isEmpty()) {
                return path;
            }
        }
    }
    return {};
}

void Skeletonizer::loadMesh(QIODevice & file, const boost::optional<decltype(treeListElement::treeID)> treeID, const QString & filename) {
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("loadMesh open failed");
    }
    tinyply::PlyFile ply(file);
    QVector<float> vertices;
    QVector<std::uint8_t> colors;
    QVector<unsigned int> indices;
    QVector<float> normals;
    int missingCoords = 0, missingNormals = 0,  missingColors = 0, missingIndices = 0;
    const auto vertexCount = ply.request_properties_from_element("vertex", {"x", "y", "z"}, vertices, missingCoords);
    const auto normalCount = ply.request_properties_from_element("vertex", {"nx", "ny", "nz"}, normals, missingNormals);
    const auto colorCount = ply.request_properties_from_element("vertex", {"red", "green", "blue", "alpha"}, colors, missingColors);
    const auto faceCount = ply.request_properties_from_element("face", {"vertex_indices"}, indices, missingIndices, 3);
    QString warning = tr("");
    if (vertexCount == 0 || faceCount == 0) {
        qWarning() << "no vertices or faces in" << filename;
    }
    if (missingCoords > 0 || (normalCount > 0 && missingNormals > 0) || (missingColors > 0 && missingColors != 4) || missingIndices > 0) {
        qWarning() << (missingCoords > 0) << (missingNormals > 0) << (missingColors > 0 && missingColors != 4) << (missingIndices > 0);
        warning = tr("Malformed ply file. KNOSSOS expects following header format (colors are optional):\n"
                     "ply\n"
                     "format [binary_little_endian|binary_big_endian|ascii] 1.0\n"
                     "element vertex #vertices\n"
                     "property float x\n"
                     "property float y\n"
                     "property float z\n"
                     "property uint8 red\n"
                     "property uint8 green\n"
                     "property uint8 blue\n"
                     "property uint8 alpha\n"
                     "element face #faces\n"
                     "property list uint8 uint vertex_indices\n"
                     "end_header");
    } else {
        try {
            QElapsedTimer t;
            t.start();
            ply.read(file);
            qDebug() << tr("Parsing .ply file took %1 ms. #vertices: %2, normals %3, #colors: %4, #triangles: %5.")
                        .arg(t.elapsed()).arg(vertexCount).arg(normalCount).arg(colorCount).arg(faceCount);
            addMeshToTree(treeID, vertices, normals, indices, colors, GL_TRIANGLES);
        } catch (const std::invalid_argument & e) {
            warning = e.what();
        }
    }
    if (warning.isEmpty() == false) {
        QMessageBox msgBox{QApplication::activeWindow()};
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Failed to load mesh for ") + filename);
        msgBox.setInformativeText(warning);
        msgBox.exec();
    }
}

std::tuple<QVector<GLfloat>, QVector<std::uint8_t>, QVector<GLuint>> Skeletonizer::getMesh(const treeListElement & tree) {
    QVector<GLfloat> vertex_components(tree.mesh->vertex_count * 3);
    QVector<std::uint8_t> colors(tree.mesh->vertex_count * 4);
    QVector<GLuint> indices(tree.mesh->index_count);
    tree.mesh->position_buf.bind();
    tree.mesh->position_buf.read(0, vertex_components.data(), vertex_components.size() * sizeof(vertex_components[0]));
    tree.mesh->position_buf.release();
    tree.mesh->color_buf.bind();
    tree.mesh->color_buf.read(0, colors.data(), colors.size() * sizeof(colors[0]));
    tree.mesh->color_buf.release();
    tree.mesh->index_buf.bind();
    tree.mesh->index_buf.read(0, indices.data(), indices.size() * sizeof(indices[0]));
    tree.mesh->index_buf.release();
    return std::make_tuple(std::move(vertex_components), std::move(colors), std::move(indices));
}

void Skeletonizer::saveMesh(QIODevice & file, const treeListElement & tree) {
    const auto [vertex_components, colors, indices] = getMesh(tree);
    saveMesh(file, tree, vertex_components, colors, indices);
}

void Skeletonizer::saveMesh(QIODevice & file, const treeListElement & tree, QVector<GLfloat> vertex_components, QVector<std::uint8_t> colors, QVector<GLuint> indices) {
    tinyply::PlyFile ply;
    ply.add_properties_to_element("vertex", {"x", "y", "z"}, vertex_components);
    if (tree.mesh->useTreeColor == false) {
        ply.add_properties_to_element("vertex", {"red", "green", "blue", "alpha"}, colors);
    }
    ply.add_properties_to_element("face", {"vertex_indices"}, indices, 3, tinyply::PlyProperty::Type::UINT8);
    if (!ply.write(file, Annotation::singleton().savePlyAsBinary)) {
        std::runtime_error("mesh save failed for tree " + std::to_string(tree.treeID));
    }
}

void Skeletonizer::addMeshToTree(boost::optional<decltype(treeListElement::treeID)> treeID, QVector<float> & verts, QVector<float> & normals, QVector<unsigned int> & indices, QVector<std::uint8_t> & colors, int draw_mode, bool swap_xy) {
    std::vector<int> vertex_face_count(verts.size() / 3);
    try {
        for(const auto indice : indices) {
            ++vertex_face_count.at(indice); // use at() to be able to throw out_of_range exception
        }
    } catch (const std::out_of_range & ex) {
        throw std::runtime_error(tr("Ply file contains triangle index greater than number of vertices: %1").arg(ex.what()).toStdString());
    }

    // generate normals of indexed vertices
    if(normals.empty() && !indices.empty()) {
        normals.resize(verts.size());
        for(int i = 0; i < indices.size()-2; i += 3) {
            QVector3D p1{verts[indices[i]*3]  , verts[indices[i]*3+1]  , verts[indices[i]*3+2]};
            QVector3D p2{verts[indices[i+1]*3], verts[indices[i+1]*3+1], verts[indices[i+1]*3+2]};
            QVector3D p3{verts[indices[i+2]*3], verts[indices[i+2]*3+1], verts[indices[i+2]*3+2]};
            QVector3D e1{p2 - p1};
            QVector3D e2{p3 - p1};

            QVector3D normal{QVector3D::normal(e1, e2)};

            for(int j = 0; j < 3; ++j) {
                normals[indices[i+j]*3]   += normal.x() / vertex_face_count[indices[i+j]];
                normals[indices[i+j]*3+1] += normal.y() / vertex_face_count[indices[i+j]];
                normals[indices[i+j]*3+2] += normal.z() / vertex_face_count[indices[i+j]];
            }
        }
    }

    float tmp_x{0.0f};
    float tmp_x_normal{0.0f};
    for(int i = 0; i < verts.size(); ++i) {
        if(swap_xy) {
            if(i%3==0) { // x
                tmp_x = verts[i];
                tmp_x_normal = normals[i];
            } else if(i%3==1) { // y
                verts[i-1] = verts[i];
                verts[i] = tmp_x;
                normals[i-1] = normals[i];
                normals[i] = tmp_x_normal;
            }
        }
    }

    auto * tree = treeID ? findTreeByTreeID(treeID.get()) : nullptr;
    if (tree == nullptr) {
        tree = &addTree(treeID);
    }
    state->viewer->mainWindow.viewport3D->makeCurrent();
    auto mesh = std::make_unique<Mesh>(tree, colors.empty(), static_cast<GLenum>(draw_mode));
    mesh->vertex_count = verts.size() / 3;
    mesh->index_count = indices.size();
    mesh->position_buf.bind();
    mesh->position_buf.allocate(verts.data(), verts.size() * sizeof (verts.front()));
    mesh->position_buf.release();
    mesh->normal_buf.bind();
    mesh->normal_buf.allocate(normals.data(), normals.size() * sizeof (normals.front()));
    mesh->normal_buf.release();
    mesh->color_buf.bind();
    mesh->color_buf.allocate(colors.data(), colors.size() * sizeof (colors.front()));
    mesh->color_buf.release();
    mesh->index_buf.bind();
    mesh->index_buf.allocate(indices.data(), indices.size() * sizeof (indices.front()));
    mesh->index_buf.release();

    std::swap(tree->mesh, mesh);

    Annotation::singleton().unsavedChanges = true;
}

void Skeletonizer::deleteMeshOfTree(treeListElement & tree) {
    tree.mesh.reset();
    emit treeChangedSignal(tree);
}
