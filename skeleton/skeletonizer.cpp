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
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "skeletonizer.h"

#include "file_io.h"
#include "functions.h"
#include "segmentation/cubeloader.h"
#include "segmentation/segmentation.h"
#include "skeleton/node.h"
#include "skeleton/skeleton_dfs.h"
#include "skeleton/tree.h"
#include "version.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <QDataStream>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <cstring>
#include <queue>
#include <type_traits>
#include <unordered_set>
#include <vector>

#define CATCH_RADIUS 10

template<typename T, typename Func>
bool connectedComponent(T & node, Func func) {
    std::queue<T*> queue;
    std::unordered_set<const T*> visitedNodes;
    visitedNodes.emplace(&node);
    queue.push(&node);
    while (!queue.empty()) {
        auto * nextNode = queue.front();
        queue.pop();

        if (func(*nextNode)) {
            return true;
        }

        for (auto & segment : nextNode->segments) {
            auto & neighbor = segment.forward ? segment.target : segment.source;
            if (visitedNodes.find(&neighbor) == visitedNodes.end()) {
                visitedNodes.emplace(&neighbor);
                queue.push(&neighbor);
            }
        }
    }
    return false;
}

Synapse::State Synapse::state = Synapse::State::PreSynapse;
Synapse Synapse::temporarySynapse = Synapse();

#include <QVector3D> // tmp
std::unique_ptr<std::vector<QVector3D>> Skeletonizer::tmp_hull_points = nullptr;
std::unique_ptr<std::vector<QVector3D>> Skeletonizer::tmp_hull_normals = nullptr;

void Skeletonizer::loadHullPoints(QIODevice & file) {
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("loadHullPoints open failed");
    }
    QTextStream stream(&file);
    QString line;

    std::vector<QVector3D> point_data_verts;
    std::vector<QVector3D> point_data_normals;

    while (!(line = stream.readLine()).isNull()) {
        std::istringstream lineStream(line.toStdString());

        bool valid = true;
        QVector3D current_point_data_verts;
        QVector3D current_point_data_normals;
        for(std::size_t i = 0; i < 3; ++i) {
            valid = valid && (lineStream >> current_point_data_verts[i]);
        }
        for(std::size_t i = 0; i < 3; ++i) {
            valid = valid && (lineStream >> current_point_data_normals[i]);
        }

        if(valid) {
            point_data_verts.emplace_back(current_point_data_verts);
            point_data_normals.emplace_back(current_point_data_normals);
        } else {
            qDebug() << "loadHullPoints loading a point failed";
        }
    }

    for(auto& verty : point_data_verts) {
        verty /= 16.0f;
    }

    tmp_hull_points = std::make_unique<std::vector<QVector3D>>(point_data_verts);
    tmp_hull_normals = std::make_unique<std::vector<QVector3D>>(point_data_normals);
}

treeListElement* Skeletonizer::findTreeByTreeID(int treeID) {
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
                          state->magnification,
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
                           state->magnification,
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

void Skeletonizer::saveXmlSkeleton(QIODevice & file) const {
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("things");//root node

    xml.writeStartElement("parameters");
    xml.writeStartElement("experiment");
    xml.writeAttribute("name", QString(state->name));
    xml.writeEndElement();

    xml.writeStartElement("lastsavedin");
    xml.writeAttribute("version", QString(KVERSION));
    xml.writeEndElement();

    xml.writeStartElement("createdin");
    xml.writeAttribute("version", state->skeletonState->skeletonCreatedInVersion);
    xml.writeEndElement();

    xml.writeStartElement("guiMode");
    xml.writeAttribute("mode", (Session::singleton().guiMode == GUIMode::ProofReading) ? "proof reading" : "none");
    xml.writeEndElement();
    xml.writeStartElement("dataset");
    xml.writeAttribute("path", state->viewer->window->widgetContainer.datasetLoadWidget.datasetUrl.toString());
    xml.writeAttribute("overlay", QString::number(static_cast<int>(Segmentation::enabled)));
    xml.writeEndElement();

    if (!Session::singleton().task.first.isEmpty() || !Session::singleton().task.second.isEmpty()) {
        xml.writeStartElement("task");
        xml.writeAttribute("category", Session::singleton().task.first);
        xml.writeAttribute("name", Session::singleton().task.second);
        xml.writeEndElement();
    }

    xml.writeStartElement("MovementArea");
    xml.writeAttribute("min.x", QString::number(Session::singleton().movementAreaMin.x));
    xml.writeAttribute("min.y", QString::number(Session::singleton().movementAreaMin.y));
    xml.writeAttribute("min.z", QString::number(Session::singleton().movementAreaMin.z));
    xml.writeAttribute("max.x", QString::number(Session::singleton().movementAreaMax.x));
    xml.writeAttribute("max.y", QString::number(Session::singleton().movementAreaMax.y));
    xml.writeAttribute("max.z", QString::number(Session::singleton().movementAreaMax.z));
    xml.writeEndElement();

    xml.writeStartElement("scale");
    xml.writeAttribute("x", QString::number(state->scale.x));
    xml.writeAttribute("y", QString::number(state->scale.y));
    xml.writeAttribute("z", QString::number(state->scale.z));
    xml.writeEndElement();

    xml.writeStartElement("RadiusLocking");
    xml.writeAttribute("enableCommentLocking", QString::number(state->skeletonState->lockPositions));
    xml.writeAttribute("lockingRadius", QString::number(state->skeletonState->lockRadius));
    xml.writeAttribute("lockToNodesWithComment", QString(state->skeletonState->lockingComment));
    xml.writeEndElement();

    xml.writeStartElement("time");
    const auto time = Session::singleton().getAnnotationTime();
    xml.writeAttribute("ms", QString::number(time));
    const auto timeData = QByteArray::fromRawData(reinterpret_cast<const char * const>(&time), sizeof(time));
    const QString timeChecksum = QCryptographicHash::hash(timeData, QCryptographicHash::Sha256).toHex().constData();
    xml.writeAttribute("checksum", timeChecksum);
    xml.writeEndElement();

    if (state->skeletonState->activeNode != nullptr) {
        xml.writeStartElement("activeNode");
        xml.writeAttribute("id", QString::number(state->skeletonState->activeNode->nodeID));
        xml.writeEndElement();
    }

    xml.writeStartElement("segmentation");
    xml.writeAttribute("backgroundId", QString::number(Segmentation::singleton().getBackgroundId()));
    xml.writeEndElement();

    xml.writeStartElement("editPosition");
    xml.writeAttribute("x", QString::number(state->viewerState->currentPosition.x + 1));
    xml.writeAttribute("y", QString::number(state->viewerState->currentPosition.y + 1));
    xml.writeAttribute("z", QString::number(state->viewerState->currentPosition.z + 1));
    xml.writeEndElement();

    xml.writeStartElement("skeletonVPState");
    for (int j = 0; j < 16; ++j) {
        xml.writeAttribute(QString("E%1").arg(j), QString::number(state->skeletonState->skeletonVpModelView[j]));
    }
    xml.writeAttribute("translateX", QString::number(state->skeletonState->translateX));
    xml.writeAttribute("translateY", QString::number(state->skeletonState->translateY));
    xml.writeEndElement();

    xml.writeStartElement("vpSettingsZoom");
    xml.writeAttribute("XYPlane", QString::number(state->viewer->window->viewportXY.get()->texture.FOV));
    xml.writeAttribute("XZPlane", QString::number(state->viewer->window->viewportXZ.get()->texture.FOV));
    xml.writeAttribute("YZPlane", QString::number(state->viewer->window->viewportZY.get()->texture.FOV));
    xml.writeAttribute("SkelVP", QString::number(state->skeletonState->zoomLevel));
    xml.writeEndElement();

    xml.writeEndElement(); // end parameters

    for (auto & currentTree : skeletonState.trees) {
        //Every "thing" (tree) has associated nodes and edges.
        xml.writeStartElement("thing");
        xml.writeAttribute("id", QString::number(currentTree.treeID));

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
            xml.writeAttribute("x", QString::number(node.position.x + 1));
            xml.writeAttribute("y", QString::number(node.position.y + 1));
            xml.writeAttribute("z", QString::number(node.position.z + 1));
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
        if(!(*commentTraverser).getComment().isEmpty()) {
            xml.writeStartElement("comment");
            xml.writeAttribute("node", QString::number((*commentTraverser).nodeID));
            xml.writeAttribute("content", (*commentTraverser).getComment());
            xml.writeEndElement();
        }
    }
    xml.writeEndElement(); // end comments

    xml.writeStartElement("branchpoints");
    for (const auto branchNodeID : state->skeletonState->branchStack) {
        xml.writeStartElement("branchpoint");
        xml.writeAttribute("id", QString::number(branchNodeID));
        xml.writeEndElement();
    }
    xml.writeEndElement(); // end branchpoints

    xml.writeEndElement(); // end things
    xml.writeEndDocument();
}

void Skeletonizer::loadXmlSkeleton(QIODevice & file, const QString & treeCmtOnMultiLoad) {
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("loadXmlSkeleton open failed");
    }

    const bool merge = state->skeletonState->mergeOnLoadFlag;
    if (!merge) {
        clearSkeleton();
        // Default for annotations created in very old versions that don't have an annotationTime.
        // It is okay to set it to 0 in newer versions, because the time will be
        // read from file anyway in case of no merge.
        Session::singleton().setAnnotationTime(0);
    }

    // If "createdin"-node does not exist, skeleton was created in a version before 3.2
    state->skeletonState->skeletonCreatedInVersion = "pre-3.2";
    state->skeletonState->skeletonLastSavedInVersion = "pre-3.2";

    Session::singleton().guiMode = GUIMode::None;

    QElapsedTimer bench;
    QXmlStreamReader xml(&file);

    QString experimentName, taskCategory, taskName;
    std::uint64_t activeNodeID = 0;
    Coordinate loadedPosition;
    std::vector<std::uint64_t> branchVector;
    std::vector<std::pair<std::uint64_t, QString>> commentsVector;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> edgeVector;

    std::unordered_map<decltype(nodeListElement::nodeID), std::reference_wrapper<nodeListElement>> nodeMap;
    std::unordered_map<decltype(treeListElement::treeID), std::reference_wrapper<treeListElement>> treeMap;

    const QSet<QString> knownElements({"scale", "offset", "skeletonDisplayMode"});
    QSet<QString> skippedElements;

    bench.start();
    const auto blockState = this->signalsBlocked();
    blockSignals(true);
    if (!xml.readNextStartElement() || xml.name() != "things") {
        throw std::runtime_error(tr("loadXmlSkeleton invalid xml token: %1").arg(xml.name().toString()).toStdString());
    }
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
                } else if (xml.name() == "guiMode") {
                    if (attributes.value("mode").toString() == "proof reading") {
                        Session::singleton().guiMode = GUIMode::ProofReading;
                    }
                } else if(xml.name() == "dataset") {
                    const auto path = attributes.value("path").toString();
                    const bool overlay = attributes.value("overlay").isEmpty() ? Segmentation::enabled : static_cast<bool>(attributes.value("overlay").toInt());
                    if (experimentName != state->name || overlay != Segmentation::enabled) {
                        state->viewer->window->widgetContainer.datasetLoadWidget.loadDataset(overlay, path, true);
                    }
                } else if(xml.name() == "MovementArea") {
                    if (!merge) {
                        Coordinate movementAreaMin;//0
                        Coordinate movementAreaMax = state->boundary;

                        for (const auto & attribute : attributes) {
                            const auto & name = attribute.name();
                            const auto & value = attribute.value();
                            if (name == "min.x") {
                                movementAreaMin.x = value.toInt();
                            } else if (name == "min.y") {
                                movementAreaMin.y = value.toInt();
                            } else if (name == "min.z") {
                                movementAreaMin.z = value.toInt();
                            } else if (name == "max.x") {
                                movementAreaMax.x = value.toInt();
                            } else if (name == "max.y") {
                                movementAreaMax.y = value.toInt();
                            } else if (name == "max.z") {
                                movementAreaMax.z = value.toInt();
                            }
                        }

                        Session::singleton().updateMovementArea(movementAreaMin, movementAreaMax);//range checked
                    }
                } else if(xml.name() == "time") { // in case of a merge the current annotation's time is kept.
                    if (!merge) {
                        const auto ms = attributes.value("ms").toULongLong();
                        Session::singleton().setAnnotationTime(ms);
                    }
                } else if(xml.name() == "activeNode") {
                    if (!merge) {
                        activeNodeID = attributes.value("id").toULongLong();
                    }
                } else if(xml.name() == "segmentation") {
                    Segmentation::singleton().setBackgroundId(attributes.value("backgroundId").toULongLong());
                } else if(xml.name() == "editPosition") {
                    QStringRef attribute = attributes.value("x");
                    if(attribute.isNull() == false)
                        loadedPosition.x = attribute.toLocal8Bit().toInt();
                    attribute = attributes.value("y");
                    if(attribute.isNull() == false)
                        loadedPosition.y = attribute.toLocal8Bit().toInt();
                    attribute = attributes.value("z");
                    if(attribute.isNull() == false)
                        loadedPosition.z = attribute.toLocal8Bit().toInt();
                } else if(xml.name() == "skeletonVPState") {
                    if (!merge) {
                        for (int j = 0; j < 16; ++j) {
                            state->skeletonState->skeletonVpModelView[j] = attributes.value(QString("E%1").arg(j)).toFloat();
                        }
                        glMatrixMode(GL_MODELVIEW);
                        glLoadMatrixf(state->skeletonState->skeletonVpModelView);

                        state->skeletonState->translateX = attributes.value("translateX").toFloat();
                        state->skeletonState->translateY = attributes.value("translateY").toFloat();
                    }
                } else if(xml.name() == "vpSettingsZoom") {
                    QStringRef attribute = attributes.value("XYPlane");
                    if(attribute.isNull() == false) {
                        state->viewer->window->viewportXY.get()->texture.FOV = attribute.toString().toFloat();
                    }
                    attribute = attributes.value("XZPlane");
                    if(attribute.isNull() == false) {
                        state->viewer->window->viewportXZ.get()->texture.FOV = attribute.toString().toFloat();
                    }
                    attribute = attributes.value("YZPlane");
                    if(attribute.isNull() == false) {
                        state->viewer->window->viewportZY.get()->texture.FOV = attribute.toString().toFloat();
                    }
                    attribute = attributes.value("SkelVP");
                    if(attribute.isNull() == false) {
                        state->skeletonState->zoomLevel = attribute.toString().toFloat();
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
                            const auto annotationTime = Session::singleton().getAnnotationTime();
                            const auto idleTime = attribute.toString().toInt();
                            //subract from annotationTime
                            Session::singleton().setAnnotationTime(annotationTime - idleTime);
                        }
                    }
                } else if(xml.name() == "task") {
                    for (auto && attribute : attributes) {
                        auto key = attribute.name();
                        if (key == "category") {
                            taskCategory = attribute.value().toString();
                        } else if (key == "name") {
                            taskName = attribute.value().toString();
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
                        }
                        else {
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
            int treeID{0};
            bool okr{false}, okg{false}, okb{false}, oka{false};
            float red{-1.0f}, green{-1.0f}, blue{-1.0f}, alpha{-1.0f};
            QVariantHash properties;
            for (const auto & attribute : xml.attributes()) {
                const auto & name = attribute.name();
                const auto & value = attribute.value();
                if (name == "id") {
                    treeID = value.toInt();
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
                                    nodeID = {value.toULongLong()};
                                } else if (name == "radius") {
                                    radius = {value.toFloat()};
                                } else if (name == "x") {
                                    currentCoordinate.x = {value.toInt() - 1};
                                } else if (name == "y") {
                                    currentCoordinate.y = {value.toInt() - 1};
                                } else if (name == "z") {
                                    currentCoordinate.z = {value.toInt() - 1};
                                } else if (name == "inVp") {
                                    inVP = static_cast<ViewportType>(value.toInt());
                                } else if (name == "inMag") {
                                    inMag = {value.toInt()};
                                } else if (name == "time") {
                                    ms = {value.toULongLong()};
                                } else if (name != "comment") { // comments are added later in the comments section
                                    properties.insert(name.toString(), value.toString());
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
    if(xml.hasError()) {
        throw std::runtime_error(tr("loadXmlSkeleton xml error: %1 at %2").arg(xml.errorString()).arg(xml.lineNumber()).toStdString());
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
                addFinishedSynapse(*synapticCleft, *preSynapse, *postSynapse);
            } else {
                qWarning() << tr("Incomplete synapse %1 could not be loaded. The “preSynapse”, “postSynapse” or “synapticCleft“ property are either missing or invalid.").arg(tree.treeID);
            }
        }
    }

    blockSignals(blockState);
    emit resetData();
    emit lockingChanged();
    emit propertiesChanged(numberProperties);
    emit guiModeLoaded();

    qDebug() << "loading skeleton: "<< bench.nsecsElapsed() / 1e9 << "s";

    if (!merge) {
        auto * node = Skeletonizer::singleton().findNodeByNodeID(activeNodeID);
        setActiveNode(node);

        if((loadedPosition.x != 0) &&
           (loadedPosition.y != 0) &&
           (loadedPosition.z != 0)) {
            Coordinate jump = loadedPosition - 1 - state->viewerState->currentPosition;
            state->viewer->userMove(jump, USERMOVE_NEUTRAL);
        }
    }
    if (skeletonState.activeNode == nullptr && !skeletonState.trees.empty() && !skeletonState.trees.front().nodes.empty()) {
        setActiveNode(&skeletonState.trees.front().nodes.front());
    }

    QMessageBox msgBox(state->viewer->window);
    auto msg = tr("");
    if (!skippedElements.empty()) {
        msg += tr("• Some unknown elements have been skipped.\n\n").arg(xml.lineNumber());
        QString buffer;
        QDebug{&buffer} << skippedElements;
        msgBox.setDetailedText(tr("Skipped elements: %1").arg(buffer));
    }
    const auto mismatchedDataset = !experimentName.isEmpty() && experimentName != state->name;
    if (mismatchedDataset) {
        msg += tr("• The annotation (created in dataset “%1”) does not belong to the currently loaded dataset (“%2”).\n\n").arg(experimentName).arg(state->name);
    }
    const auto currentTaskCategory = Session::singleton().task.first;
    const auto currentTaskName = Session::singleton().task.second;
    const auto mismatchedTask = !currentTaskCategory.isEmpty() && !currentTaskName.isEmpty() && (currentTaskCategory != taskCategory || currentTaskName != taskName);
    if (mismatchedTask) {
        msg += tr("• The associated task “%1” (%2) is different from the currently active “%3” (%4).\n\n").arg(taskName).arg(taskCategory).arg(currentTaskName).arg(currentTaskCategory);
    }
    msg.chop(2);// remove the 2 newlines at the end
    if (!msg.isEmpty()) {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Incompatible Annotation File\nAlthough the file was loaded successfully, working with it is not recommended.");
        msgBox.setInformativeText(msg);
        msgBox.exec();
    }
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
    Session::singleton().unsavedChanges = true;
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

    for (auto segmentIt = std::begin(nodeToDel->segments); segmentIt != std::end(nodeToDel->segments); segmentIt = std::begin(nodeToDel->segments)) {
        delSegment(segmentIt);
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
        for (const auto * seg : *nodeToDel->getSegments()) {
            if (seg->source == *nodeToDel) {
                // activate next node in (any) tracing direction
                newActiveNode = &seg->target;
                break;
            } else { // if there's no next node in tracing direction, take a node in (any) opposite tracing direction
                newActiveNode = &seg->source;
            }
        }
    }

    nodeToDel->correspondingTree->nodes.erase(nodeToDel->iterator);

    emit nodeRemovedSignal(nodeID);

    if (resetActiveNode) {
        if (newActiveNode == nullptr) { // nodeToDel was not connected
            newActiveNode = findNearbyNode(tree, pos);
        }
        state->viewer->skeletonizer->setActiveNode(newActiveNode);
    }

    Session::singleton().unsavedChanges = true;

    return true;
}

bool Skeletonizer::delTree(int treeID) {
    auto * const treeToDel = findTreeByTreeID(treeID);
    if (treeToDel == nullptr) {
        qDebug("There exists no tree with ID %d. Unable to delete it.", treeID);
        return false;
    }

    const auto blockState = blockSignals(true);//chunk operation

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

    for (auto nodeIt = std::begin(treeToDel->nodes); nodeIt != std::end(treeToDel->nodes); nodeIt = std::begin(treeToDel->nodes)) {
        delNode(0, &*nodeIt);
    }
    blockSignals(blockState);


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

    Session::singleton().unsavedChanges = true;
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

bool Skeletonizer::setActiveTreeByID(int treeID) {
    treeListElement *currentTree;
    currentTree = findTreeByTreeID(treeID);
    if (currentTree == nullptr) {
        qDebug("There exists no tree with ID %d!", treeID);
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

    Session::singleton().unsavedChanges = true;

    return true;
}

bool Skeletonizer::setActiveNode(nodeListElement *node) {
    if (node == state->skeletonState->activeNode) {
        return true;
    }

    state->skeletonState->activeNode = node;

    if (node == nullptr) {
        selectNodes({});
        if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeTracing)) {
            Segmentation::singleton().clearObjectSelection();
        }
    } else {
        if (!node->selected) {
            selectNodes({node});
        }
        if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeTracing)) {
            selectObjectForNode(*node);
        }
        if (skeletonState.lockPositions && node->getComment().contains(skeletonState.lockingComment, Qt::CaseInsensitive)) {
            lockPosition(node->position);
        }
        setActiveTreeByID(node->correspondingTree->treeID);
    }

    Session::singleton().unsavedChanges = true;

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

boost::optional<nodeListElement &> Skeletonizer::addNode(boost::optional<std::uint64_t> nodeID, const float radius, const int treeID, const Coordinate & position
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
        time = Session::singleton().getAnnotationTime() + Session::singleton().currentTimeSliceMs();
    }

    tempTree->nodes.emplace_back(nodeID.get(), radius, position, inMag, VPtype, time.get(), properties, *tempTree);
    auto & tempNode = tempTree->nodes.back();
    tempNode.iterator = std::prev(std::end(tempTree->nodes));
    updateCircRadius(&tempNode);
    updateSubobjectCountFromProperty(tempNode);

    state->skeletonState->nodesByNodeID.emplace(nodeID.get(), &tempNode);

    if (nodeID == state->skeletonState->nextAvailableNodeID) {
        skeletonState.nextAvailableNodeID = findNextAvailableID(skeletonState.nextAvailableNodeID, skeletonState.nodesByNodeID);
    }
    Session::singleton().unsavedChanges = true;

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
    sourceSegIt->length = sourceSegIt->sisterSegment->length = (targetNode.position - sourceNode.position).length();

    updateCircRadius(&sourceNode);
    updateCircRadius(&targetNode);

    Session::singleton().unsavedChanges = true;

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
    const auto blockState = blockSignals(true);
    blockSignals(blockState);
    skeletonState = SkeletonState{};
    Session::singleton().resetMovementArea();
    Session::singleton().setAnnotationTime(0);
    emit resetData();
}

bool Skeletonizer::mergeTrees(int treeID1, int treeID2) {
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
    delTree(tree2->treeID);
    setActiveTreeByID(tree1->treeID);
    emit treesMerged(treeID1, treeID2);//update nodes
    return true;
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
        const auto blockState = blockSignals(true);
        restoreDefaultTreeColor(newTree);
        blockSignals(blockState);
    } else {
        newTree.color = color.get();
        newTree.colorSetManually = true;
    }

    if (newTree.treeID == state->skeletonState->nextAvailableTreeID) {
        skeletonState.nextAvailableTreeID = findNextAvailableID(skeletonState.nextAvailableTreeID, skeletonState.treesByID);
    }

    Session::singleton().unsavedChanges = true;

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
    int extremalIdDistance = skeletonState.trees.size();
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

void Skeletonizer::setCommentOfSelectedTrees(const QString & comment) {
    const auto blockState = blockSignals(true);
    for (const auto &tree : state->skeletonState->selectedTrees) {
        setComment(*tree, comment);
    }
    blockSignals(blockState);
    emit resetData();
}

void Skeletonizer::setColor(treeListElement & tree, const QColor & color) {
    tree.color = color;
    tree.colorSetManually = true;
    Session::singleton().unsavedChanges = true;
    emit treeChangedSignal(tree);
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

bool Skeletonizer::editNode(std::uint64_t nodeID, nodeListElement *node, float newRadius, const Coordinate & newPos, int inMag) {
    if(!node) {
        node = findNodeByNodeID(nodeID);
    }
    if(!node) {
        qDebug() << tr("Cannot edit: node id %1 invalid.").arg(nodeID);
        return false;
    }

    nodeID = node->nodeID;

    auto oldPos = node->position;
    node->position = newPos.capped({0, 0, 0}, state->boundary);

    if(newRadius != 0.) {
        node->radius = newRadius;
        updateCircRadius(node);
    }
    node->createdInMag = inMag;

    Session::singleton().unsavedChanges = true;

    const quint64 newSubobjectId = readVoxel(newPos);
    Skeletonizer::singleton().movedHybridNode(*node, newSubobjectId, oldPos);

    emit nodeChangedSignal(*node);

    return true;
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

    std::unordered_set<treeListElement*> treesSeen; // Connected component might consist of multiple trees.
    std::unordered_set<nodeListElement*> visitedNodes;
    visitedNodes.insert(firstNode);
    connectedComponent(*firstNode, [&treesSeen, &visitedNodes](nodeListElement & node){
        visitedNodes.emplace(&node);
        treesSeen.emplace(node.correspondingTree);
        return false;
    });

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
    for(auto * tree : treesSeen) {
        nodeCountSeenTrees += tree->nodes.size();
    }
    if (visitedNodes.size() == nodeCountSeenTrees && treesSeen.size() == 1) {
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
    Session::singleton().unsavedChanges = true;
    setActiveTreeByID(newTree.treeID);//the empty tree had no active node

    return true;
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
    Session::singleton().unsavedChanges = true;
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
    if(Synapse::state == Synapse::State::PreSynapse) { //set active Node as presynapse
        Synapse::temporarySynapse.setPreSynapse(skeletonState.activeNode);
        auto & synapticCleft = addTree();
        Synapse::temporarySynapse.setCleft(&synapticCleft);
        Synapse::state = Synapse::State::Cleft;
    } else if(Synapse::state == Synapse::State::Cleft) {
        Synapse::state = Synapse::State::PostSynapse;
    }
}

void Skeletonizer::addFinishedSynapse(treeListElement * cleft, nodeListElement * pre, nodeListElement * post) {
    Synapse syn;
    syn.setCleft(cleft);
    syn.setPreSynapse(pre);
    syn.setPostSynapse(post);
    skeletonState.synapses.push_back(syn);
    cleft->correspondingSynapse = &skeletonState.synapses.back();
    pre->correspondingSynapse = &skeletonState.synapses.back();
    post->correspondingSynapse = &skeletonState.synapses.back();
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
        synapse->setPreSynapse(nodes[0]);
        synapse->setPostSynapse(nodes[1]);
        return;
    }
    //Add a synaptic cleft tree with one node between the two selected nodes
    addFinishedSynapse(&addTree(), nodes[0], nodes[1]);

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
              state->magnification,
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
    Session::singleton().unsavedChanges = true;
    return branchNode;
}

void Skeletonizer::pushBranchNode(nodeListElement & branchNode) {
    if (!branchNode.isBranchNode) {
        state->skeletonState->branchStack.emplace_back(branchNode.nodeID);
        branchNode.isBranchNode = true;
        qDebug() << "Branch point" << branchNode.nodeID << "added.";
        emit branchPushedSignal();
        Session::singleton().unsavedChanges = true;
    } else {
        qDebug() << "Active node is already a branch point";
    }
}

void Skeletonizer::jumpToNode(const nodeListElement & node) {
    state->viewer->userMove(node.position - state->viewerState->currentPosition, USERMOVE_NEUTRAL);
    emit jumpedToNodeSignal(node);
}

nodeListElement* Skeletonizer::popBranchNodeAfterConfirmation(QWidget * const parent) {
    if (state->skeletonState->branchStack.empty()) {
        QMessageBox box(parent);
        box.setIcon(QMessageBox::Information);
        box.setText("No branch points remain.");
        box.exec();
        qDebug() << "No branch points remain.";
    } else if (state->skeletonState->branchpointUnresolved) {
        QMessageBox prompt(parent);
        prompt.setIcon(QMessageBox::Question);
        prompt.setText("Unresolved branch point. \nDo you really want to jump to the next one?");
        prompt.setInformativeText("No node has been added after jumping to the last branch point.");
        prompt.addButton("Jump anyway", QMessageBox::AcceptRole);
        auto * cancel = prompt.addButton("Cancel", QMessageBox::RejectRole);

        prompt.exec();
        if (prompt.clickedButton() == cancel) {
            return nullptr;
        }
    }

    return popBranchNode();
}

void Skeletonizer::restoreDefaultTreeColor(treeListElement & tree) {
    const auto index = (tree.treeID - 1) % state->viewerState->treeColors.size();
    tree.color = QColor::fromRgb(std::get<0>(state->viewerState->treeColors[index])
                       , std::get<1>(state->viewerState->treeColors[index])
                       , std::get<2>(state->viewerState->treeColors[index]));
    tree.colorSetManually = false;
    Session::singleton().unsavedChanges = true;
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

bool Skeletonizer::moveToPrevTree() {
    treeListElement *prevTree = getTreeWithPrevID(state->skeletonState->activeTree);
    if(state->skeletonState->activeTree == nullptr) {
        return false;
    } else if(prevTree != nullptr && prevTree->isSynapticCleft && !prevTree->render) {
        //skip synaptic cleft if synaptic cleft is not rendered
        prevTree = getTreeWithPrevID(prevTree);
    }
    if(prevTree) {
        setActiveTreeByID(prevTree->treeID);
        //set tree's first node to active node if existent
        if (state->skeletonState->activeTree->nodes.empty()) {
            return true;
        } else {
            auto & node = state->skeletonState->activeTree->nodes.front();
            setActiveNode(&node);
            state->viewer->setPositionWithRecentering(node.position);
        }
        return true;
    }
    QMessageBox info;
    info.setIcon(QMessageBox::Information);
    info.setWindowFlags(Qt::WindowStaysOnTopHint);
    info.setWindowTitle("Information");
    info.setText("You reached the first tree.");
    info.exec();

    return false;
}


bool Skeletonizer::moveToNextTree() {
    treeListElement *nextTree = getTreeWithNextID(state->skeletonState->activeTree);
    if(state->skeletonState->activeTree == nullptr) {
        return false;
    } else if(nextTree != nullptr && nextTree->isSynapticCleft && !nextTree->render) {
        //skip synaptic cleft if synaptic cleft is not rendered
        nextTree = getTreeWithNextID(nextTree);
    }
    if(nextTree) {
        setActiveTreeByID(nextTree->treeID);
        //set tree's first node to active node if existent

        if (state->skeletonState->activeTree->nodes.empty()) {
            return true;
        } else {
            auto & node = state->skeletonState->activeTree->nodes.front();
            setActiveNode(&node);
            state->viewer->setPositionWithRecentering(node.position);
        }
        return true;
    }
    QMessageBox info;
    info.setIcon(QMessageBox::Information);
    info.setWindowFlags(Qt::WindowStaysOnTopHint);
    info.setWindowTitle("Information");
    info.setText("You reached the last tree.");
    info.exec();

    return false;
}

void Skeletonizer::goToNode(const bool next = true) {
    if (skeletonState.activeNode == nullptr) {
        return;
    }
    static NodeGenerator traverser(*skeletonState.activeNode);
    static nodeListElement * lastNode = skeletonState.activeNode;
    auto oneStep = next ? []() { ++traverser; } : []() { --traverser; };
    auto canContinue = next ? []() { return !traverser.reachedEnd; } : []() { return !traverser.reachedStart; };

    if (lastNode != skeletonState.activeNode) {
        traverser = NodeGenerator(*skeletonState.activeNode);
    }
    if (canContinue()) {
        oneStep();
    } else {
        return;
    }

    while (!traverser.reachedEnd) {
        if (&(*traverser) != skeletonState.activeNode) {
            setActiveNode(&(*traverser));
            lastNode = &(*traverser);
            state->viewer->setPositionWithRecentering((*traverser).position);
            return;
        } else if (canContinue()) {
            oneStep();
        } else {
            return;
        }
    }
}

void Skeletonizer::moveSelectedNodesToTree(int treeID) {
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

void Skeletonizer::deleteSelectedTrees() {
    //make a copy because the selected objects can change
    const auto treesToDelete = state->skeletonState->selectedTrees;
    const auto blockstate = signalsBlocked();
    blockSignals(true);
    for (const auto & elem : treesToDelete) {
        delTree(elem->treeID);
    }
    blockSignals(blockstate);
    emit resetData();
}

void Skeletonizer::deleteSelectedNodes() {
    //make a copy because the selected objects can change
    const auto nodesToDelete = state->skeletonState->selectedNodes;
    const auto blockstate = signalsBlocked();
    blockSignals(true);
    for (auto & elem : nodesToDelete) {
        delNode(0, elem);
    }
    blockSignals(blockstate);
    emit resetData();
}

void Skeletonizer::toggleConnectionOfFirstPairOfSelectedNodes(QWidget * const parent) {
    auto & node0 = *state->skeletonState->selectedNodes[0];
    auto & node1 = *state->skeletonState->selectedNodes[1];
    //segments are only stored and searched in one direction so we have to search for both
    decltype(nodeListElement::segments)::iterator segIt;
    if ((segIt = findSegmentBetween(node0, node1)) != std::end(node0.segments)) {
        delSegment(segIt);
    } else if ((segIt = findSegmentBetween(node1, node0)) != std::end(node1.segments)) {
        delSegment(segIt);
    } else if (!Session::singleton().annotationMode.testFlag(AnnotationMode::SkeletonCycles) && areConnected(node0, node1)) {
        QMessageBox::information(parent, "Cycle detected!", "If you want to allow cycles, please select 'Advanced Tracing' in the dropdown menu in the toolbar.");
    } else {//nodes are not already linked
        addSegment(node0, node1);
    }
}

bool Skeletonizer::areConnected(const nodeListElement & lhs,const nodeListElement & rhs) const {
    return connectedComponent(lhs, [&rhs](const nodeListElement & node){
        return node == rhs;
    });
}
