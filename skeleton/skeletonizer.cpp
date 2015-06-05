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
#include "skeletonizer.h"

#include "file_io.h"
#include "functions.h"
#include "knossos.h"
#include "session.h"
#include "skeleton/node.h"
#include "skeleton/tree.h"
#include "version.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <cstring>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

#define CATCH_RADIUS 10

struct stack {
    uint elementsOnStack;
    void **elements;
    int stackpointer;
    int size;
};

Skeletonizer::Skeletonizer(QObject *parent) : QObject(parent), simpleTracing(true) {
    state->skeletonState->branchStack = newStack(1048576);

    // Generate empty tree structures
    state->skeletonState->firstTree = NULL;
    state->skeletonState->treeElements = 0;
    state->skeletonState->totalNodeElements = 0;
    state->skeletonState->totalSegmentElements = 0;
    state->skeletonState->activeTree = NULL;
    state->skeletonState->activeNode = NULL;

    state->skeletonState->segRadiusToNodeRadius = 0.5;
    //state->skeletonState->autoFilenameIncrementBool = true;
    state->skeletonState->greatestNodeID = 0;

    state->skeletonState->showNodeIDs = false;
    state->skeletonState->highlightActiveTree = true;
    state->skeletonState->showIntersections = false;

    state->skeletonState->defaultNodeRadius = 1.5;
    state->skeletonState->overrideNodeRadiusBool = false;
    state->skeletonState->overrideNodeRadiusVal = 1.;

    state->skeletonState->currentComment = NULL;

    state->skeletonState->lastSaveTicks = 0;
    state->skeletonState->autoSaveInterval = 5;
    state->skeletonState->totalComments = 0;
    state->skeletonState->totalBranchpoints = 0;


    state->skeletonState->commentBuffer = (char*) malloc(10240 * sizeof(char));
    memset(state->skeletonState->commentBuffer, '\0', 10240 * sizeof(char));

    state->skeletonState->searchStrBuffer = (char*) malloc(2048 * sizeof(char));
    memset(state->skeletonState->searchStrBuffer, '\0', 2048 * sizeof(char));
    memset(state->skeletonState->skeletonLastSavedInVersion, '\0', sizeof(state->skeletonState->skeletonLastSavedInVersion));

    state->skeletonState->unsavedChanges = false;
}

nodeListElement *Skeletonizer::addNodeListElement(uint nodeID,
              float radius,
              nodeListElement **currentNode,
              Coordinate *position,
              int inMag) {

    nodeListElement *newElement = NULL;

    /*
     * This skeleton modifying function does not lock the skeleton and therefore
     * has to be called from functions that do lock and NEVER directly.
     *
     */

    newElement = (nodeListElement*) malloc(sizeof(nodeListElement));
    if(newElement == NULL) {
        qDebug() << "Out of memory while trying to allocate memory for a new nodeListElement.";
        return NULL;
    }
    memset(newElement, '\0', sizeof(nodeListElement));

    if((*currentNode) == NULL) {
        // Requested to add a node to a list that hasn't yet been started.
        *currentNode = newElement;
        newElement->previous = NULL;
        newElement->next = NULL;
    }
    else {
        newElement->previous = *currentNode;
        newElement->next = (*currentNode)->next;
        (*currentNode)->next = newElement;
        if(newElement->next) {
            newElement->next->previous = newElement;
        }
    }

    newElement->position = *position;
    //Assign radius
    newElement->radius = radius;
    //Set node ID. This ID is unique in every tree list (there should only exist 1 tree list, see initSkeletonizer()).
    //Take the provided nodeID.

    newElement->numSegs = 0;
    newElement->nodeID = nodeID;
    newElement->isBranchNode = false;
    newElement->selected = false;
    newElement->createdInMag = inMag;

    return newElement;
}

segmentListElement *Skeletonizer::addSegmentListElement (segmentListElement **currentSegment,
    nodeListElement *sourceNode,
    nodeListElement *targetNode) {
    segmentListElement *newElement = NULL;

    /*
    * This skeleton modifying function does not lock the skeleton and therefore
    * has to be called from functions that do lock and NEVER directly.
    */

    newElement = (segmentListElement*) malloc(sizeof(segmentListElement));
    if(newElement == NULL) {
        qDebug() << "Out of memory while trying to allocate memory for a new segmentListElement.";
        return NULL;
    }
    memset(newElement, '\0', sizeof(segmentListElement));
    if(*currentSegment == NULL) {
        // Requested to start a new list
        *currentSegment = newElement;
        newElement->previous = NULL;
        newElement->next = NULL;
    }
    else {
        newElement->previous = *currentSegment;
        newElement->next = (*currentSegment)->next;
        (*currentSegment)->next = newElement;
        if(newElement->next)
            newElement->next->previous = newElement;
    }

    //Valid segment
    newElement->flag = SEGMENT_FORWARD;
    newElement->source = sourceNode;
    newElement->target = targetNode;
    return newElement;
}

treeListElement* Skeletonizer::findTreeByTreeID(int treeID) {
    const auto treeIt = state->skeletonState->treesByID.find(treeID);
    return treeIt != std::end(state->skeletonState->treesByID) ? treeIt->second : nullptr;
}

uint64_t Skeletonizer::UI_addSkeletonNode(Coordinate *clickedCoordinate, ViewportType VPtype) {
    color4F treeCol;
    /* -1 causes new color assignment */
    treeCol.r = -1.;
    treeCol.g = -1.;
    treeCol.b = -1.;
    treeCol.a = 1.;

    if(!state->skeletonState->activeTree) {
        addTreeListElement(0, treeCol);
    }

    auto addedNodeID = addNode(
                          0,
                          state->skeletonState->defaultNodeRadius,
                          state->skeletonState->activeTree->treeID,
                          clickedCoordinate,
                          VPtype,
                          state->magnification,
                          -1,
                          true);
    if(!addedNodeID) {
        qDebug() << "Error: Could not add new node!";
        return 0;
    }

    setActiveNode(NULL, addedNodeID);

    if(state->skeletonState->activeTree->size == 1) {
        /* First node in this tree */
        pushBranchNode(true, true, NULL, addedNodeID);
        addComment("First Node", NULL, addedNodeID);
    }
    return addedNodeID;
}

uint Skeletonizer::addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, ViewportType VPtype, int makeNodeActive) {
    if(!state->skeletonState->activeNode) {
        qDebug() << "Please create a node before trying to link nodes.";
        return false;
    }

    //Add a new node at the target position first.
    auto targetNodeID = addNode(
                           0,
                           state->skeletonState->defaultNodeRadius,
                           state->skeletonState->activeTree->treeID,
                           clickedCoordinate,
                           VPtype,
                           state->magnification,
                           -1,
                           true);
    if(!targetNodeID) {
        qDebug() << "Could not add new node while trying to add node and link with active node!";
        return false;
    }

    addSegment(state->skeletonState->activeNode->nodeID, targetNodeID);

    if(makeNodeActive) {
        setActiveNode(NULL, targetNodeID);
    }
    if (state->skeletonState->activeTree->size == 1) {
        /* First node in this tree */
        pushBranchNode(true, true, NULL, targetNodeID);
        addComment("First Node", NULL, targetNodeID);
    }

    return targetNodeID;
}

void Skeletonizer::autoSaveIfElapsed() {
    if(state->skeletonState->autoSaveBool) {
        if(state->skeletonState->autoSaveInterval) {
            const auto minutes = (state->time.elapsed() - state->skeletonState->lastSaveTicks) / 60000.0;
            if (minutes >= state->skeletonState->autoSaveInterval) {//timeout elapsed

                state->skeletonState->lastSaveTicks = state->time.elapsed();//save timestamp

                if (state->skeletonState->unsavedChanges && state->skeletonState->firstTree != nullptr) {//there’re real changes
                    emit autosaveSignal();
                }
            }
        }
    }
}

bool Skeletonizer::saveXmlSkeleton(QIODevice & file) const {
    //  This function should always be called through UI_saveSkeleton
    // for proper error and file name display to the user.

    // We need to do this to be able to save the branch point stack to a file
    //and still have the branch points available to the user afterwards.

    stack * const reverseBranchStack = newStack(2048);
    stack * const tempReverseStack = newStack(2048);
    while (const auto currentBranchPointID = popStack(state->skeletonState->branchStack)) {
        pushStack(reverseBranchStack, currentBranchPointID);
        pushStack(tempReverseStack, currentBranchPointID);
    }

    while (const auto currentBranchPointID = (ptrdiff_t)popStack(tempReverseStack)) {
        auto currentNode = findNodeByNodeID(currentBranchPointID);
        state->viewer->skeletonizer->pushBranchNode(false, false, currentNode, 0);
    }

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
    const auto & createdInVersion = QString(state->skeletonState->skeletonCreatedInVersion);
    xml.writeAttribute("version", createdInVersion);
    xml.writeEndElement();

    xml.writeStartElement("dataset");
    xml.writeAttribute("path", state->viewer->window->widgetContainer->datasetLoadWidget->datasetPath);
    xml.writeEndElement();

    xml.writeStartElement("MovementArea");
    xml.writeAttribute("min.x", QString::number(Session::singleton().movementAreaMin.x));
    xml.writeAttribute("min.y", QString::number(Session::singleton().movementAreaMin.y));
    xml.writeAttribute("min.z", QString::number(Session::singleton().movementAreaMin.z));
    xml.writeAttribute("max.x", QString::number(Session::singleton().movementAreaMax.x));
    xml.writeAttribute("max.y", QString::number(Session::singleton().movementAreaMax.y));
    xml.writeAttribute("max.z", QString::number(Session::singleton().movementAreaMax.z));
    xml.writeEndElement();

    xml.writeStartElement("scale");
    xml.writeAttribute("x", QString::number(state->scale.x / state->magnification));
    xml.writeAttribute("y", QString::number(state->scale.y / state->magnification));
    xml.writeAttribute("z", QString::number(state->scale.z / state->magnification));
    xml.writeEndElement();

    xml.writeStartElement("RadiusLocking");
    xml.writeAttribute("enableCommentLocking", QString::number(state->skeletonState->lockPositions));
    xml.writeAttribute("lockingRadius", QString::number(state->skeletonState->lockRadius));
    xml.writeAttribute("lockToNodesWithComment", QString(state->skeletonState->onCommentLock));
    xml.writeEndElement();

    xml.writeStartElement("time");
    const auto time = Session::singleton().annotationTime();
    xml.writeAttribute("ms", QString::number(time));
    const auto timeData = QByteArray::fromRawData(reinterpret_cast<const char * const>(&time), sizeof(time));
    const QString timeChecksum = QCryptographicHash::hash(timeData, QCryptographicHash::Sha256).toHex().constData();
    xml.writeAttribute("checksum", timeChecksum);
    xml.writeEndElement();

    if (state->skeletonState->activeNode) {
        xml.writeStartElement("activeNode");
        xml.writeAttribute("id", QString::number(state->skeletonState->activeNode->nodeID));
        xml.writeEndElement();
    }

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
    xml.writeAttribute("XYPlane", QString::number(state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel));
    xml.writeAttribute("XZPlane", QString::number(state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel));
    xml.writeAttribute("YZPlane", QString::number(state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel));
    xml.writeAttribute("SkelVP", QString::number(state->skeletonState->zoomLevel));
    xml.writeEndElement();

    xml.writeEndElement(); // end parameters

    for (auto currentTree = state->skeletonState->firstTree; currentTree != nullptr; currentTree = currentTree->next) {
        //Every "thing" (tree) has associated nodes and edges.
        xml.writeStartElement("thing");
        xml.writeAttribute("id", QString::number(currentTree->treeID));

        if (currentTree->colorSetManually) {
            xml.writeAttribute("color.r", QString::number(currentTree->color.r));
            xml.writeAttribute("color.g", QString::number(currentTree->color.g));
            xml.writeAttribute("color.b", QString::number(currentTree->color.b));
            xml.writeAttribute("color.a", QString::number(currentTree->color.a));
        } else {
            xml.writeAttribute("color.r", QString("-1."));
            xml.writeAttribute("color.g", QString("-1."));
            xml.writeAttribute("color.b", QString("-1."));
            xml.writeAttribute("color.a", QString("1."));
        }
        if (currentTree->comment[0] != '\0') {
            xml.writeAttribute("comment", QString(currentTree->comment));
        }

        xml.writeStartElement("nodes");
        for (auto currentNode = currentTree->firstNode; currentNode != nullptr; currentNode = currentNode->next) {
            xml.writeStartElement("node");
            xml.writeAttribute("id", QString::number(currentNode->nodeID));
            xml.writeAttribute("radius", QString::number(currentNode->radius));
            xml.writeAttribute("x", QString::number(currentNode->position.x + 1));
            xml.writeAttribute("y", QString::number(currentNode->position.y + 1));
            xml.writeAttribute("z", QString::number(currentNode->position.z + 1));
            xml.writeAttribute("inVp", QString::number(currentNode->createdInVp));
            xml.writeAttribute("inMag", QString::number(currentNode->createdInMag));
            xml.writeAttribute("time", QString::number(currentNode->timestamp));
            xml.writeEndElement(); // end node
        }
        xml.writeEndElement(); // end nodes

        xml.writeStartElement("edges");
        for (auto currentNode = currentTree->firstNode; currentNode != nullptr; currentNode = currentNode->next) {
            for (auto currentSegment = currentNode->firstSegment; currentSegment != nullptr; currentSegment = currentSegment->next) {
                if (currentSegment->flag == SEGMENT_FORWARD) {
                    xml.writeStartElement("edge");
                    xml.writeAttribute("source", QString::number(currentSegment->source->nodeID));
                    xml.writeAttribute("target", QString::number(currentSegment->target->nodeID));
                    xml.writeEndElement();
                }
            }
        }
        xml.writeEndElement(); // end edges

        xml.writeEndElement(); // end tree
    }

    xml.writeStartElement("comments");
    for (auto currentComment = state->skeletonState->currentComment; currentComment != nullptr; currentComment = currentComment->next) {
        xml.writeStartElement("comment");
        xml.writeAttribute("node", QString::number(currentComment->node->nodeID));
        xml.writeAttribute("content", QString(currentComment->content));
        xml.writeEndElement();
        if (currentComment->next == state->skeletonState->currentComment) {//comment list is circular
            break;
        }
    }
    xml.writeEndElement(); // end comments

    xml.writeStartElement("branchpoints");
    while (const auto currentBranchPointID = reinterpret_cast<std::size_t>(popStack(reverseBranchStack))) {
        xml.writeStartElement("branchpoint");
        xml.writeAttribute("id", QString::number(currentBranchPointID));
        xml.writeEndElement();
    }
    xml.writeEndElement(); // end branchpoints

    xml.writeEndElement(); // end things
    xml.writeEndDocument();
    return true;
}

bool Skeletonizer::loadXmlSkeleton(QIODevice & file, const QString & treeCmtOnMultiLoad) {
    uint activeNodeID = 0, greatestNodeIDbeforeLoading = 0;
    int greatestTreeIDbeforeLoading = 0;
    int inMag, magnification = 0;
    int globalMagnificationSpecified = false;

    treeListElement *currentTree;

    Coordinate loadedPosition;

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qErrnoWarning("Document not parsed successfully.");
        return false;
    }

    const bool merge = state->skeletonState->mergeOnLoadFlag;
    if (!merge) {
        clearSkeleton(true);
    } else {
        greatestNodeIDbeforeLoading = state->skeletonState->greatestNodeID;
        greatestTreeIDbeforeLoading = state->skeletonState->greatestTreeID;
    }

    // If "createdin"-node does not exist, skeleton was created in a version
    // before 3.2
    strcpy(state->skeletonState->skeletonCreatedInVersion, "pre-3.2");
    strcpy(state->skeletonState->skeletonLastSavedInVersion, "pre-3.2");

    // Default for skeletons created in very old versions that don't have a
    // skeletonTime.
    // It is okay to set it to 0 in newer versions, because the time will be
    // read from file anyway in case of no merge.
    if(!merge) {
        state->skeletonState->skeletonTime = 0;
    }
    QTime bench;
    QXmlStreamReader xml(&file);
    bool minuteFile = false;//skeleton time in minutes and node time in seconds, TODO remove after transition time

    QString experimentName;
    std::vector<uint> branchVector;
    std::vector<std::pair<uint, QString>> commentsVector;
    std::vector<std::pair<uint, uint>> edgeVector;

    bench.start();
    const auto blockState = this->signalsBlocked();
    blockSignals(true);
    if (!xml.readNextStartElement() || xml.name() != "things") {
        qDebug() << "invalid xml token: " << xml.name();
        return false;
    }
    while(xml.readNextStartElement()) {
        if(xml.name() == "parameters") {
            while(xml.readNextStartElement()) {
                QXmlStreamAttributes attributes = xml.attributes();

                if (xml.name() == "experiment") {
                    experimentName = attributes.value("name").toString();
                } else if (xml.name() == "createdin") {
                    QStringRef attribute = attributes.value("version");
                    if(attribute.isNull() == false) {
                        strcpy(state->skeletonState->skeletonCreatedInVersion, attribute.toLocal8Bit().data());
                    } else {
                        strcpy(state->skeletonState->skeletonCreatedInVersion, "Pre-3.2");
                    }
                } else if(xml.name() == "lastsavedin") {
                    QStringRef attribute = attributes.value("version");
                    if(attribute.isNull() == false) {
                        strcpy(state->skeletonState->skeletonLastSavedInVersion, attribute.toLocal8Bit().data());
                    }
                } else if(xml.name() == "dataset") {
                    QStringRef attribute = attributes.value("path");
                    QString path = attribute.isNull() ? "" : attribute.toString();
                    if (experimentName != state->name) {
                        state->viewer->window->widgetContainer->datasetLoadWidget->loadDataset(path, true);
                    }
                } else if(xml.name() == "magnification" && xml.isStartElement()) {
                    QStringRef attribute = attributes.value("factor");
                     // This is for legacy skeleton files.
                     // In the past, magnification was specified on a per-file basis
                     // or not specified at all.
                     // A magnification factor of 0 shall represent an unknown magnification.
                    if(attribute.isNull() == false) {
                        magnification = attribute.toLocal8Bit().toInt();
                        globalMagnificationSpecified = true;
                    } else {
                        magnification = 0;
                    }
                } else if(xml.name() == "MovementArea") {
                    auto & session = Session::singleton();
                    Coordinate min = session.movementAreaMin;
                    Coordinate max = session.movementAreaMax;

                    QStringRef attribute = attributes.value("min.x");
                    if(attribute.isNull() == false) {
                        min.x = std::min(std::max(0, attribute.toInt()), state->boundary.x);
                    }
                    attribute = attributes.value("min.y");
                    if(attribute.isNull() == false) {
                        min.y = std::min(std::max(0, attribute.toInt()), state->boundary.y);
                    }
                    attribute = attributes.value("min.z");
                    if(attribute.isNull() == false) {
                        min.z = std::min(std::max(0, attribute.toInt()), state->boundary.z);
                    }
                    attribute = attributes.value("max.x");
                    if(attribute.isNull() == false) {
                        max.x = std::min(std::max(0, attribute.toInt()), state->boundary.x);
                    }
                    attribute = attributes.value("max.y");
                    if(attribute.isNull() == false) {
                        max.y = std::min(std::max(0, attribute.toInt()), state->boundary.y);
                    }
                    attribute = attributes.value("max.z");
                    if(attribute.isNull() == false) {
                        max.z = std::min(std::max(0, attribute.toInt()), state->boundary.z);
                    }
                    session.updateMovementArea(min, max);
                } else if(xml.name() == "time" && merge == false) { // in case of a merge the current annotation's time is kept.
                    QStringRef attribute = attributes.value("ms");
                    if (attribute.isNull() == false) {
                        const auto ms = attribute.toInt();
                        Session::singleton().annotationTime(ms);
                    } else { // support for the few minutes files
                        attribute = attributes.value("min");
                        if (attribute.isNull() == false) {
                            minuteFile = true;
                            const auto ms = attribute.toInt() * 60 * 1000;
                            Session::singleton().annotationTime(ms);
                        }
                    }
                } else if(xml.name() == "activeNode" && !merge) {
                    QStringRef attribute = attributes.value("id");
                    if(attribute.isNull() == false) {
                        activeNodeID = attribute.toLocal8Bit().toUInt();
                    }
                } else if(xml.name() == "scale") {
                    QStringRef attribute = attributes.value("x");
                    if(attribute.isNull() == false) {
                        state->scale.x = attribute.toLocal8Bit().toFloat();
                    }
                    attribute = attributes.value("y");
                    if(attribute.isNull() == false) {
                        state->scale.y = attribute.toLocal8Bit().toFloat();
                    }
                    attribute = attributes.value("z");
                    if(attribute.isNull() == false) {
                        state->scale.z = attribute.toLocal8Bit().toFloat();
                    }
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

                } else if(xml.name() == "skeletonVPState" && !merge) {
                    int j = 0;
                    char element [8];
                    for (j = 0; j < 16; j++){
                        sprintf (element, "E%d", j);
                        QStringRef attribute = attributes.value(element);
                        state->skeletonState->skeletonVpModelView[j] = attribute.toString().toFloat();
                    }
                    glMatrixMode(GL_MODELVIEW);
                    glLoadMatrixf(state->skeletonState->skeletonVpModelView);

                    QStringRef attribute = attributes.value("translateX");
                    if(attribute.isNull() == false) {
                        state->skeletonState->translateX = attribute.toString().toFloat();
                    }
                    attribute = attributes.value("translateY");
                    if(attribute.isNull() == false) {
                        state->skeletonState->translateY = attribute.toString().toFloat();
                    }
                } else if(xml.name() == "vpSettingsZoom") {
                    QStringRef attribute = attributes.value("XYPlane");
                    if(attribute.isNull() == false) {
                        state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel = attribute.toString().toFloat();
                    }
                    attribute = attributes.value("XZPlane");
                    if(attribute.isNull() == false) {
                        state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel = attribute.toString().toFloat();
                    }
                    attribute = attributes.value("YZPlane");
                    if(attribute.isNull() == false) {
                        state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel = attribute.toString().toFloat();
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
                        strcpy(state->skeletonState->onCommentLock, static_cast<const char*>(attribute.toString().toStdString().c_str()));
                    }
                } else if(merge == false && xml.name() == "idleTime") { // in case of a merge the current annotation's idleTime is kept.
                    QStringRef attribute = attributes.value("ms");
                    if (attribute.isNull() == false) {
                        const auto annotationTime = Session::singleton().annotationTime();
                        const auto idleTime = attribute.toString().toInt();
                        //subract from annotationTime
                        Session::singleton().annotationTime(annotationTime - idleTime);
                    }
                }
                xml.skipCurrentElement();
            }
        } else if(xml.name() == "branchpoints") {
            while(xml.readNextStartElement()) {
                if(xml.name() == "branchpoint") {
                    QXmlStreamAttributes attributes = xml.attributes();
                    QStringRef attribute = attributes.value("id");
                    uint nodeID;
                    if(attribute.isNull() == false) {
                        if(merge == false) {
                            nodeID = attribute.toLocal8Bit().toUInt();
                        } else {
                            nodeID = attribute.toLocal8Bit().toUInt() + greatestNodeIDbeforeLoading;
                        }
                        branchVector.emplace_back(nodeID);
                    }
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
                    uint nodeID;
                    if(attribute.isNull() == false) {
                        if(merge == false) {
                            nodeID = attribute.toLocal8Bit().toUInt();
                        } else {
                            nodeID = attribute.toLocal8Bit().toUInt() + greatestNodeIDbeforeLoading;
                        }
                    }
                    attribute = attributes.value("content");
                    if((!attribute.isNull()) && (!attribute.isEmpty())) {
                        commentsVector.emplace_back(nodeID, attribute.toLocal8Bit());
                    }
                }
                xml.skipCurrentElement();
            }
        } else if(xml.name() == "thing") {
            QXmlStreamAttributes attributes = xml.attributes();

            int neuronID;
            QStringRef attribute = attributes.value("id");
            if(attribute.isNull() == false) {
                neuronID = attribute.toLocal8Bit().toInt();
            } else {
                neuronID = 0; // whatever
            }
            // color: -1 causes default color assignment
            color4F neuronColor;
            attribute = attributes.value("color.r");
            if(attribute.isNull() == false) {
                neuronColor.r = attribute.toLocal8Bit().toFloat();
            } else {
                neuronColor.r = -1;
            }
            attribute = attributes.value("color.g");
            if(attribute.isNull() == false) {
                neuronColor.g = attribute.toLocal8Bit().toFloat();
            } else {
                neuronColor.g = -1;
            }
            attribute = attributes.value("color.b");
            if(attribute.isNull() == false) {
                neuronColor.b = attribute.toLocal8Bit().toFloat();
            } else {
                neuronColor.b = -1;
            }
            attribute = attributes.value("color.a");
            if(attribute.isNull() == false) {
                neuronColor.a = attribute.toLocal8Bit().toFloat();
            } else {
                neuronColor.a = -1;
            }

            if(merge) {
                neuronID += greatestTreeIDbeforeLoading;
                currentTree = addTreeListElement(neuronID, neuronColor);
                neuronID = currentTree->treeID;
            } else {
                currentTree = addTreeListElement(neuronID, neuronColor);
            }

            attribute = attributes.value("comment"); // the tree comment
            if(attribute.isNull() == false && attribute.length() > 0) {
                addTreeComment(currentTree->treeID, attribute.toLocal8Bit().data());
            } else {
                addTreeComment(currentTree->treeID, treeCmtOnMultiLoad);
            }

            while (xml.readNextStartElement()) {
                if(xml.name() == "nodes") {
                    while(xml.readNextStartElement()) {
                        if(xml.name() == "node") {
                            attributes = xml.attributes();

                            uint nodeID;
                            attribute = attributes.value("id");
                            if(attribute.isNull() == false) {
                                nodeID = attribute.toLocal8Bit().toUInt();
                            } else {
                                nodeID = 0;
                            }

                            float radius;
                            attribute = attributes.value("radius");
                            if(attribute.isNull() == false) {
                                radius = attribute.toLocal8Bit().toFloat();
                            } else {
                                radius = state->skeletonState->defaultNodeRadius;
                            }

                            Coordinate currentCoordinate;
                            attribute = attributes.value("x");
                            if(attribute.isNull() == false) {
                                currentCoordinate.x = attribute.toLocal8Bit().toInt() - 1;
                                if(globalMagnificationSpecified) {
                                    currentCoordinate.x = currentCoordinate.x * magnification;
                                }
                            } else {
                                currentCoordinate.x = 0;
                            }

                            attribute = attributes.value("y");
                            if(attribute.isNull() == false) {
                                currentCoordinate.y = attribute.toLocal8Bit().toInt() - 1;
                                if(globalMagnificationSpecified) {
                                    currentCoordinate.y = currentCoordinate.y * magnification;
                                }
                            } else {
                                currentCoordinate.y = 0;
                            }

                            attribute = attributes.value("z");
                            if(attribute.isNull() == false) {
                                currentCoordinate.z = attribute.toLocal8Bit().toInt() - 1;
                                if(globalMagnificationSpecified) {
                                    currentCoordinate.z = currentCoordinate.z * magnification;
                                }
                            } else {
                                currentCoordinate.z = 0;
                            }

                            ViewportType VPtype;
                            attribute = attributes.value("inVp");
                            if(attribute.isNull() == false) {
                                VPtype = static_cast<ViewportType>(attribute.toLocal8Bit().toInt());
                            } else {
                                VPtype = VIEWPORT_UNDEFINED;
                            }

                            attribute = attributes.value("inMag");
                            if(attribute.isNull() == false) {
                                inMag = attribute.toLocal8Bit().toInt();
                            } else {
                                inMag = magnification; // For legacy skeleton files
                            }

                            attribute = attributes.value("time");
                            auto ms = Session::singleton().annotationTime();// for legacy skeleton files (without time) add current runtime
                            if(attribute.isNull() == false) {
                                auto ms = attribute.toInt();
                                if (minuteFile) {// nml with minutes in it?
                                    ms = ms * 1000;//node time was saved in seconds → ms
                                }
                            }

                            if(merge == false) {
                                addNode(nodeID, radius, neuronID, &currentCoordinate, VPtype, inMag, ms, false);
                            }
                            else {
                                nodeID += greatestNodeIDbeforeLoading;
                                addNode(nodeID, radius, neuronID, &currentCoordinate, VPtype, inMag, ms, false);
                            }
                        }
                        xml.skipCurrentElement();
                    } // end while nodes
                }
                if(xml.name() == "edges") {
                    while(xml.readNextStartElement()) {
                        if(xml.name() == "edge") {
                            attributes = xml.attributes();
                            // Add edge
                            uint sourcecNodeId;
                            attribute = attributes.value("source");
                            if(attribute.isNull() == false) {
                                sourcecNodeId = attribute.toLocal8Bit().toUInt();
                            } else {
                                sourcecNodeId = 0;
                            }
                            uint targetNodeId;
                            attribute = attributes.value("target");
                            if(attribute.isNull() == false) {
                                targetNodeId = attribute.toLocal8Bit().toUInt();
                            } else {
                                targetNodeId = 0;
                            }

                            edgeVector.emplace_back(sourcecNodeId, targetNodeId);
                        }
                        xml.skipCurrentElement();
                    }
                }
            }
        } // end thing
    }
    xml.readNext();//</things>
    if (!xml.isEndDocument()) {
        qDebug() << "unknown content following after line" << xml.lineNumber();
        return false;
    }
    if(xml.hasError()) {
        qDebug() << __FILE__ << ":" << __LINE__ << " xml error: " << xml.errorString() << " at " << xml.lineNumber();
        return false;
    }

    if (!experimentName.isEmpty() && experimentName != state->name) {
        const auto text = tr("The annotation (created in dataset “%1”) does not belong to this dataset (“%2”).").arg(experimentName).arg(state->name);
        QMessageBox::information(state->viewer->window, tr("Wrong dataset"), text);
    }

    for (const auto & elem : edgeVector) {
        if(merge == false) {
            addSegment(elem.first, elem.second);
        } else {
            addSegment(elem.first + greatestNodeIDbeforeLoading, elem.second + greatestNodeIDbeforeLoading);
        }
    }

    for (const auto & elem : branchVector) {
        const auto & currentNode = findNodeByNodeID(elem);
        if(currentNode != nullptr)
            pushBranchNode(true, false, currentNode, 0);
    }

    for (const auto & elem : commentsVector) {
        const auto & currentNode = findNodeByNodeID(elem.first);
        if(currentNode != nullptr) {
            addComment(elem.second, currentNode, 0);
        }
    }

    blockSignals(blockState);
    emit resetData();

    qDebug() << "loading skeleton took: "<< bench.elapsed();

    if (!merge) {
        setActiveNode(NULL, activeNodeID);

        if((loadedPosition.x != 0) &&
           (loadedPosition.y != 0) &&
           (loadedPosition.z != 0)) {
            Coordinate jump = loadedPosition - 1 - state->viewerState->currentPosition;
            emit userMoveSignal(jump.x, jump.y, jump.z, USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
        }
    }
    if (state->skeletonState->activeNode == nullptr && state->skeletonState->firstTree != nullptr) {
        setActiveNode(state->skeletonState->firstTree->firstNode, 0);
    }

    return true;
}

bool Skeletonizer::delSegment(uint sourceNodeID, uint targetNodeID, segmentListElement *segToDel) {
    // Delete the segment out of the segment list and out of the visualization structure!

    if(!segToDel)
        segToDel = findSegmentByNodeIDs(sourceNodeID, targetNodeID);
    else {
        sourceNodeID = segToDel->source->nodeID;
        targetNodeID = segToDel->target->nodeID;
    }

    if(!segToDel) {
        qDebug() << "Cannot delete segment, no segment with corresponding node IDs available!";
        return false;
    }

    /* numSegs counts forward AND backward segments!!! */
    segToDel->source->numSegs--;
    segToDel->target->numSegs--;

    if(segToDel == segToDel->source->firstSegment)
        segToDel->source->firstSegment = segToDel->next;
    else {
        //if(segToDel->previous) //Why??? Previous EXISTS if its not the first seg...
            segToDel->previous->next = segToDel->next;
        if(segToDel->next)
            segToDel->next->previous = segToDel->previous;
    }

    //Delete reverse segment in target node
    if(segToDel->reverseSegment == segToDel->target->firstSegment)
        segToDel->target->firstSegment = segToDel->reverseSegment->next;
    else {
        //if(segToDel->reverseSegment->previous)
            segToDel->reverseSegment->previous->next = segToDel->reverseSegment->next;
        if(segToDel->reverseSegment->next)
            segToDel->reverseSegment->next->previous = segToDel->reverseSegment->previous;
    }

    /* A bit cumbersome, but we cannot delete the segment and then find its source node.. */
    segToDel->length = 0.f;
    updateCircRadius(segToDel->source);

    free(segToDel);
    state->skeletonState->totalSegmentElements--;

    state->skeletonState->unsavedChanges = true;

    return true;
}

/*
 * We have to delete the node from 2 structures: the skeleton's nested linked list structure
 * and the skeleton visualization structure (hashtable with skeletonDCs).
 */
bool Skeletonizer::delNode(uint nodeID, nodeListElement *nodeToDel) {
    if (!nodeToDel) {
        nodeToDel = findNodeByNodeID(nodeID);
    }
    if (!nodeToDel) {
        qDebug("The given node %u doesn't exist. Unable to delete it.", nodeID);
        return false;
    }
    nodeID = nodeToDel->nodeID;
    if (nodeToDel->comment) {
        delComment(nodeToDel->comment, 0);
    }
    if (nodeToDel->isBranchNode) {
        state->skeletonState->totalBranchpoints--;
    }

    for (auto * currentSegment = nodeToDel->firstSegment; currentSegment != nullptr;) {
        auto * const nextSegment = currentSegment->next;
        if (currentSegment->flag == SEGMENT_FORWARD) {
            delSegment(0,0, currentSegment);
        } else if (currentSegment->flag == SEGMENT_BACKWARD) {
            delSegment(0,0, currentSegment->reverseSegment);
        }
        currentSegment = nextSegment;
    }
    nodeToDel->firstSegment = nullptr;

    if (nodeToDel == state->skeletonState->selectedCommentNode) {
        state->skeletonState->selectedCommentNode = nullptr;
    }
    if (nodeToDel->previous != nullptr) {
        nodeToDel->previous->next = nodeToDel->next;
    }
    if (nodeToDel->next != nullptr) {
        nodeToDel->next->previous = nodeToDel->previous;
    }
    if (nodeToDel == nodeToDel->correspondingTree->firstNode) {
        nodeToDel->correspondingTree->firstNode = nodeToDel->next;
    }

    state->skeletonState->nodesByNodeID.erase(nodeToDel->nodeID);

    if (nodeToDel->selected) {
        auto & selectedNodes = state->skeletonState->selectedNodes;
        const auto eraseit = std::find(std::begin(selectedNodes), std::end(selectedNodes), nodeToDel);
        if (eraseit != std::end(selectedNodes)) {
            selectedNodes.erase(eraseit);
        }
    }

    emit nodeRemovedSignal(nodeID);

    if (state->skeletonState->activeNode == nodeToDel) {
        auto * newActiveNode = findNearbyNode(nodeToDel->correspondingTree, nodeToDel->position);
        state->viewer->skeletonizer->setActiveNode(newActiveNode, 0);
    }

    --nodeToDel->correspondingTree->size;

    free(nodeToDel);

    state->skeletonState->totalNodeElements--;

    state->skeletonState->unsavedChanges = true;

    return true;
}

bool Skeletonizer::delTree(int treeID) {
    auto * const treeToDel = findTreeByTreeID(treeID);
    if (treeToDel == nullptr) {
        qDebug("There exists no tree with ID %d. Unable to delete it.", treeID);
        return false;
    }

    const auto blockState = this->signalsBlocked();
    blockSignals(true);//chunk operation
    for (auto * currentNode = treeToDel->firstNode; currentNode != nullptr;) {
        auto * const nextNode = currentNode->next;
        delNode(0, currentNode);
        currentNode = nextNode;//currentNode got invalidated
    }
    treeToDel->firstNode = nullptr;
    blockSignals(blockState);

    if (treeToDel->previous != nullptr) {
        treeToDel->previous->next = treeToDel->next;
    }
    if (treeToDel->next != nullptr) {
        treeToDel->next->previous = treeToDel->previous;
    }
    if (treeToDel == state->skeletonState->firstTree) {
        state->skeletonState->firstTree = treeToDel->next;
    }

    state->skeletonState->treesByID.erase(treeToDel->treeID);

    if (treeToDel->selected) {
        auto & selectedTrees = state->skeletonState->selectedTrees;
        const auto eraseit = std::find(std::begin(selectedTrees), std::end(selectedTrees), treeToDel);
        if (eraseit != std::end(selectedTrees)) {
            selectedTrees.erase(eraseit);
        }
    }

    //remove if active tree
    if (treeToDel == state->skeletonState->activeTree) {
        state->skeletonState->activeTree = nullptr;
    }
    free(treeToDel);
    state->skeletonState->treeElements--;

    //no references to tree left
    emit treeRemovedSignal(treeID);//updates tools

    //if the active tree was not set through the active node but another tree exists
    if (state->skeletonState->activeTree == nullptr && state->skeletonState->firstTree != nullptr) {
        setActiveTreeByID(state->skeletonState->firstTree->treeID);//updates tools too
    }

    state->skeletonState->unsavedChanges = true;
    return true;
}

nodeListElement* Skeletonizer::findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition) {
    nodeListElement *currentNode = NULL;
    nodeListElement *nodeWithCurrentlySmallestDistance = NULL;
    treeListElement *currentTree = NULL;
    floatCoordinate distanceVector;
    float smallestDistance = 0;


    //  If available, search for a node within nearbyTree first.


    if(nearbyTree) {
        currentNode = nearbyTree->firstNode;

        while(currentNode) {
            // We make the nearest node the next active node
            distanceVector.x = (float)searchPosition.x - (float)currentNode->position.x;
            distanceVector.y = (float)searchPosition.y - (float)currentNode->position.y;
            distanceVector.z = (float)searchPosition.z - (float)currentNode->position.z;
            //set nearest distance to distance to first node found, then to distance of any nearer node found.
            if(smallestDistance == 0 || euclidicNorm(&distanceVector) < smallestDistance) {
                smallestDistance = euclidicNorm(&distanceVector);
                nodeWithCurrentlySmallestDistance = currentNode;
            }
            currentNode = currentNode->next;
        }

        if(nodeWithCurrentlySmallestDistance) {
            return nodeWithCurrentlySmallestDistance;
        }
    }

     // Ok, we didn't find any node in nearbyTree.
     // Now we take the nearest node, independent of the tree it belongs to.


    currentTree = state->skeletonState->firstTree;
    smallestDistance = 0;
    while(currentTree) {
        currentNode = currentTree->firstNode;

        while(currentNode) {
            //We make the nearest node the next active node
            distanceVector.x = (float)searchPosition.x - (float)currentNode->position.x;
            distanceVector.y = (float)searchPosition.y - (float)currentNode->position.y;
            distanceVector.z = (float)searchPosition.z - (float)currentNode->position.z;

            if(smallestDistance == 0 || euclidicNorm(&distanceVector) < smallestDistance) {
                smallestDistance = euclidicNorm(&distanceVector);
                nodeWithCurrentlySmallestDistance = currentNode;
            }

            currentNode = currentNode->next;
        }

       currentTree = currentTree->next;
    }

    if(nodeWithCurrentlySmallestDistance) {
        return nodeWithCurrentlySmallestDistance;
    }

    return NULL;
}

nodeListElement* Skeletonizer::findNodeInRadius(Coordinate searchPosition) {
    nodeListElement *currentNode = NULL;
    nodeListElement *nodeWithCurrentlySmallestDistance = NULL;
    treeListElement *currentTree = NULL;
    floatCoordinate distanceVector;
    float smallestDistance = CATCH_RADIUS;

    currentTree = state->skeletonState->firstTree;
    while(currentTree) {
        currentNode = currentTree->firstNode;

        while(currentNode) {
            //We make the nearest node the next active node
            distanceVector.x = (float)searchPosition.x - (float)currentNode->position.x;
            distanceVector.y = (float)searchPosition.y - (float)currentNode->position.y;
            distanceVector.z = (float)searchPosition.z - (float)currentNode->position.z;

            if(euclidicNorm(&distanceVector) < smallestDistance) {
                smallestDistance = euclidicNorm(&distanceVector);
                nodeWithCurrentlySmallestDistance = currentNode;
            }
            currentNode = currentNode->next;
        }
       currentTree = currentTree->next;
    }

    if(nodeWithCurrentlySmallestDistance) {
        return nodeWithCurrentlySmallestDistance;
    }

    return NULL;
}

bool Skeletonizer::setActiveTreeByID(int treeID) {
    treeListElement *currentTree;
    currentTree = findTreeByTreeID(treeID);
    if(!currentTree) {
        qDebug("There exists no tree with ID %d!", treeID);
        return false;
    }

    state->skeletonState->activeTree = currentTree;

    clearTreeSelection();
    if(currentTree->selected == false) {
        currentTree->selected = true;
        state->skeletonState->selectedTrees.push_back(currentTree);
    }

    //switch to nearby node of new tree
    if (state->skeletonState->activeNode != nullptr && state->skeletonState->activeNode->correspondingTree != currentTree) {
        //prevent ping pong if tree was activated from setActiveNode
        auto * node = findNearbyNode(currentTree, state->skeletonState->activeNode->position);
        if (node->correspondingTree == currentTree) {
            setActiveNode(node, 0);
        } else {
            setActiveNode(currentTree->firstNode, 0);
        }
    } else if (state->skeletonState->activeNode == nullptr) {
        setActiveNode(currentTree->firstNode, 0);
    }

    state->skeletonState->unsavedChanges = true;

    emit treeSelectionChangedSignal();

    return true;
}

bool Skeletonizer::setActiveNode(nodeListElement *node, uint nodeID) {
     // If both *node and nodeID are specified, nodeID wins.
     // If neither *node nor nodeID are specified
     // (node == NULL and nodeID == 0), the active node is
     // set to NULL.

    if(nodeID != 0) {
        node = findNodeByNodeID(nodeID);
        if(!node) {
            qDebug("No node with id %u available.", nodeID);
            return false;
        }
    }
    if(node) {
        nodeID = node->nodeID;
    }

    state->skeletonState->activeNode = node;

    clearNodeSelection();
    if(node) {
        if(node->selected == false) {
            node->selected = true;
            state->skeletonState->selectedNodes.push_back(node);
        }
    }

    if(node) {
        setActiveTreeByID(node->correspondingTree->treeID);
    }
    if(node) {
        if(node->comment) {
            state->skeletonState->currentComment = node->comment;
            memset(state->skeletonState->commentBuffer, '\0', 10240);

            strncpy(state->skeletonState->commentBuffer,
                    state->skeletonState->currentComment->content,
                    strlen(state->skeletonState->currentComment->content));
        }
        else
            memset(state->skeletonState->commentBuffer, '\0', 10240);
    }

    state->skeletonState->unsavedChanges = true;

    emit nodeSelectionChangedSignal();

    return true;
}

uint Skeletonizer::findAvailableNodeID() {
    uint nodeID = state->skeletonState->totalNodeElements;
    //Test if node ID over node counter is available. If not, find a valid one.
    while(findNodeByNodeID(nodeID)) {
        nodeID++;
    }
    return std::max(nodeID,(uint)1);
}

uint Skeletonizer::addNode(uint nodeID, float radius, int treeID, Coordinate *position,
                           ViewportType VPtype, int inMag, int time, int respectLocks) {
    nodeListElement *tempNode = NULL;
    treeListElement *tempTree = NULL;
    floatCoordinate lockVector;

    state->skeletonState->branchpointUnresolved = false;

     // respectLocks refers to locking the position to a specific coordinate such as to
     // disallow tracing areas further than a certain distance away from a specific point in the
     // dataset.

    //qDebug() << state->skeletonState->lockedPosition.x  << " " << state->skeletonState->lockedPosition.y  << " " << state->skeletonState->lockedPosition.z;



    if(respectLocks) {
        if(state->skeletonState->positionLocked) {
            if (state->viewerState->lockComment == QString(state->skeletonState->onCommentLock)) {
                unlockPosition();
                return false;
            }

            lockVector.x = (float)position->x - (float)state->skeletonState->lockedPosition.x;
            lockVector.y = (float)position->y - (float)state->skeletonState->lockedPosition.y;
            lockVector.z = (float)position->z - (float)state->skeletonState->lockedPosition.z;

            float lockDistance = euclidicNorm(&lockVector);
            if(lockDistance > state->skeletonState->lockRadius) {
                qDebug("Node is too far away from lock point (%f), not adding.", lockDistance);
                return false;
            }
        }
    }

    if(nodeID) {
        tempNode = findNodeByNodeID(nodeID);
    }

    if(tempNode) {
        qDebug("Node with ID %u already exists, no node added.", nodeID);
        return false;
    }
    tempTree = findTreeByTreeID(treeID);

    if(!tempTree) {
        qDebug("There exists no tree with the provided ID %d!", treeID);
        return false;
    }

    // One node more in all trees
    state->skeletonState->totalNodeElements++;

    // One more node in this tree.
    ++tempTree->size;

    if(nodeID == 0) {
        nodeID = findAvailableNodeID();
    }

    tempNode = addNodeListElement(nodeID, radius, &(tempTree->firstNode), position, inMag);
    tempNode->correspondingTree = tempTree;
    tempNode->comment = NULL;
    tempNode->createdInVp = VPtype;

    if(tempTree->firstNode == NULL) {
        tempTree->firstNode = tempNode;
    }

    updateCircRadius(tempNode);

    if (time == -1) {//time was not provided
        time = Session::singleton().annotationTime() + Session::singleton().currentTimeSliceMs();
    }
    tempNode->timestamp = time;

    state->skeletonState->nodesByNodeID.emplace(nodeID, tempNode);

    if(nodeID > state->skeletonState->greatestNodeID) {
        state->skeletonState->greatestNodeID = nodeID;
    }
    state->skeletonState->unsavedChanges = true;

    emit nodeAddedSignal(*tempNode);

    return nodeID;
}

bool Skeletonizer::addSegment(uint sourceNodeID, uint targetNodeID) {
    nodeListElement *targetNode, *sourceNode;
    segmentListElement *sourceSeg;
    floatCoordinate node1, node2;

    if(findSegmentByNodeIDs(sourceNodeID, targetNodeID)) {
        qDebug("Segment between nodes %u and %u exists already.", sourceNodeID, targetNodeID);
        return false;
    }

    //Check if source and target nodes are existant
    sourceNode = findNodeByNodeID(sourceNodeID);
    targetNode = findNodeByNodeID(targetNodeID);

    if(!(sourceNode) || !(targetNode)) {
        qDebug() << "Could not link the nodes, because at least one is missing!";
        return false;
    }

    if(sourceNode == targetNode) {
        qDebug() << "Cannot link node with itself!";
        return false;
    }

    //One segment more in all trees
    state->skeletonState->totalSegmentElements++;

     // Add the segment to the tree structure

    sourceSeg = addSegmentListElement(&(sourceNode->firstSegment), sourceNode, targetNode);
    sourceSeg->reverseSegment = addSegmentListElement(&(targetNode->firstSegment), sourceNode, targetNode);

    sourceSeg->reverseSegment->flag = SEGMENT_BACKWARD;

    sourceSeg->reverseSegment->reverseSegment = sourceSeg;

    /* numSegs counts forward AND backward segments!!! */
   sourceNode->numSegs++;
   targetNode->numSegs++;

   /* Do we really skip this node? Test cum dist. to last rendered node! */
   node1.x = (float)sourceNode->position.x;
   node1.y = (float)sourceNode->position.y;
   node1.z = (float)sourceNode->position.z;

   node2.x = (float)targetNode->position.x - node1.x;
   node2.y = (float)targetNode->position.y - node1.y;
   node2.z = (float)targetNode->position.z - node1.z;

   sourceSeg->length = sourceSeg->reverseSegment->length
       = sqrtf(scalarProduct(&node2, &node2));

    updateCircRadius(sourceNode);

    state->skeletonState->unsavedChanges = true;

    return true;
}

void Skeletonizer::clearTreeSelection() {
    for (auto &selectedTree : state->skeletonState->selectedTrees) {
        selectedTree->selected = false;
    }
    state->skeletonState->selectedTrees.clear();
}

void Skeletonizer::clearNodeSelection() {
    for (auto &selectedNode : state->skeletonState->selectedNodes) {
        selectedNode->selected = false;
    }
    state->skeletonState->selectedNodes.clear();
}

bool Skeletonizer::clearSkeleton(int /*loadingSkeleton*/) {
    treeListElement *currentTree, *treeToDel;
    auto skeletonState = state->skeletonState;

    const auto blockState = this->signalsBlocked();
    blockSignals(true);
    currentTree = skeletonState->firstTree;
    while(currentTree) {
        treeToDel = currentTree;
        currentTree = treeToDel->next;
        delTree(treeToDel->treeID);
    }
    blockSignals(blockState);
    emit resetData();

    skeletonState->activeNode = NULL;
    skeletonState->activeTree = NULL;

    skeletonState->nodesByNodeID.clear();
    skeletonState->treesByID.clear();
    delStack(skeletonState->branchStack);

    //Generate empty tree structures
    skeletonState->firstTree = NULL;
    skeletonState->totalNodeElements = 0;
    skeletonState->totalSegmentElements = 0;
    skeletonState->treeElements = 0;
    skeletonState->activeTree = NULL;
    skeletonState->activeNode = NULL;
    skeletonState->greatestNodeID = 0;
    skeletonState->greatestTreeID = 0;

    skeletonState->branchStack = newStack(1048576);

    Session::singleton().annotationTime(0);
    strcpy(state->skeletonState->skeletonCreatedInVersion, KVERSION);

    return true;
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

    nodeListElement * currentNode = tree2->firstNode;

    while(currentNode) {
        //Change the corresponding tree
        currentNode->correspondingTree = tree1;

        currentNode = currentNode->next;
    }

    //Now we insert the node list of tree2 before the node list of tree1
    if(tree1->firstNode && tree2->firstNode) {
        //First, we have to find the last node of tree2 (this node has to be connected
        //to the first node inside of tree1)
        nodeListElement * firstNode = tree2->firstNode;
        nodeListElement * lastNode = firstNode;
        while(lastNode->next) {
            lastNode = lastNode->next;
        }

        tree1->firstNode->previous = lastNode;
        lastNode->next = tree1->firstNode;
        tree1->firstNode = firstNode;
    } else if(tree2->firstNode) {
        tree1->firstNode = tree2->firstNode;
    }

    // The new node count for tree1 is the old node count of tree1 plus the node
    // count of tree2
    tree1->size += tree2->size;

    tree2->size = 0;
    tree2->firstNode = nullptr;
    delTree(tree2->treeID);

    setActiveTreeByID(tree1->treeID);

    emit treesMerged(treeID1, treeID2);//update nodes

    return true;
}

nodeListElement* Skeletonizer::getNodeWithPrevID(nodeListElement *currentNode, bool sameTree) {
    nodeListElement *prevNode = NULL;
    nodeListElement *highestNode = NULL;
    unsigned int minDistance = UINT_MAX;
    unsigned int tempMinDistance = minDistance;
    unsigned int maxID = 0;

    if(currentNode == NULL) {
        // no current node, return active node
        if(state->skeletonState->activeNode) {
            return state->skeletonState->activeNode;
        }
        // no active node, simply return first node found
        treeListElement *tree = state->skeletonState->firstTree;
        while(tree) {
            if(tree->firstNode) {
                return tree->firstNode;
            }
            tree = tree->next;
        }
        qDebug() << "no nodes to move to";
        return NULL;
    }
    treeListElement *tree;
    if(sameTree) {
         tree = currentNode->correspondingTree;
    }
    else {
        tree = state->skeletonState->firstTree;
    }
    nodeListElement *node;
    while(tree) {
        node = tree->firstNode;
        while(node) {
            if(node->nodeID > maxID) {
                highestNode = node;
                maxID = node->nodeID;
            }
            if(node->nodeID < currentNode->nodeID) {
                tempMinDistance = currentNode->nodeID - node->nodeID;

                if(tempMinDistance == 1) { //smallest distance possible
                    return node;
                }
                if(tempMinDistance < minDistance) {
                    minDistance = tempMinDistance;
                    prevNode = node;
                }
            }
            node = node->next;
        }
        if(sameTree) {
            break;
        }
        tree = tree->next;
    }

    if(!prevNode && sameTree) {
        prevNode = highestNode;
    }
    return prevNode;
}

nodeListElement* Skeletonizer::getNodeWithNextID(nodeListElement *currentNode, bool sameTree) {
    nodeListElement *nextNode = NULL;
    nodeListElement *lowestNode = NULL;
    unsigned int minDistance = UINT_MAX;
    unsigned int tempMinDistance = minDistance;
    unsigned int minID = UINT_MAX;

    if(currentNode == NULL) {
        // no current node, return active node
        if(state->skeletonState->activeNode) {
            return state->skeletonState->activeNode;
        }
        // no active node, simply return first node found
        treeListElement *tree = state->skeletonState->firstTree;
        while(tree) {
            if(tree->firstNode) {
                return tree->firstNode;
            }
            tree = tree->next;
        }
        qDebug() << "no nodes to move to";
        return NULL;
    }
    treeListElement *tree;
    if(sameTree) {
         tree = currentNode->correspondingTree;
    }
    else {
        tree = state->skeletonState->firstTree;
    }
    nodeListElement *node;
    while(tree) {
        node = tree->firstNode;
        while(node) {
            if(node->nodeID < minID) {
                lowestNode = node;
                minID = node->nodeID;
            }
            if(node->nodeID > currentNode->nodeID) {
                tempMinDistance = node->nodeID - currentNode->nodeID;

                if(tempMinDistance == 1) { //smallest distance possible
                    return node;
                }
                if(tempMinDistance < minDistance) {
                    minDistance = tempMinDistance;
                    nextNode = node;
                }
            }
            node = node->next;
        }
        if(sameTree) {
            break;
        }
        tree = tree->next;
    }

    if(!nextNode && sameTree) {
        nextNode = lowestNode;
    }
    return nextNode;
}

nodeListElement* Skeletonizer::findNodeByNodeID(uint nodeID) {
    const auto nodeIt = state->skeletonState->nodesByNodeID.find(nodeID);
    return nodeIt != std::end(state->skeletonState->nodesByNodeID) ? nodeIt->second : nullptr;
}

int Skeletonizer::findAvailableTreeID() {
    int treeID = state->skeletonState->treeElements;
    //Test if tree ID over tree counter is available. If not, find a valid one.
    while(findTreeByTreeID(treeID)) {
        treeID++;
    }
    return std::max(treeID,1);
}

treeListElement* Skeletonizer::addTreeListElement(int treeID, color4F color) {
     // The variable sync is a workaround for the problem that this function
     // will sometimes be called by other syncable functions that themselves hold
     // the lock and handle synchronization. If set to false, all synchro
     // routines will be skipped.

    treeListElement *newElement = NULL;

    newElement = findTreeByTreeID(treeID);
    if(newElement) {
        qDebug("Tree with ID %d already exists!", treeID);
        return newElement;
    }

    newElement = (treeListElement*)malloc(sizeof(treeListElement));
    if(newElement == NULL) {
        qDebug() << "Out of memory while trying to allocate memory for a new treeListElement.";
        return NULL;
    }
    memset(newElement, '\0', sizeof(treeListElement));

    state->skeletonState->treeElements++;

    //Tree ID is unique in tree list
    //Take the provided tree ID if there is one.
    if(treeID != 0) {
        newElement->treeID = treeID;
    }
    else {
        newElement->treeID = findAvailableTreeID();
    }
    clearTreeSelection();
    newElement->render = true;
    newElement->selected = true;
    state->skeletonState->selectedTrees.push_back(newElement);

    // calling function sets values < 0 when no color was specified
    if(color.r < 0) {//Set a tree color
        int index = (newElement->treeID - 1) % 256; //first index is 0

        newElement->color.r =  state->viewerState->treeAdjustmentTable[index];
        newElement->color.g =  state->viewerState->treeAdjustmentTable[index + 256];
        newElement->color.b =  state->viewerState->treeAdjustmentTable[index + 512];
        newElement->color.a = 1.;
    }
    else {
        newElement->color = color;
    }
    newElement->colorSetManually = false;

    memset(newElement->comment, '\0', 8192);

    //Insert the new tree at the beginning of the tree list
    newElement->next = state->skeletonState->firstTree;
    newElement->previous = NULL;
    //The old first element should have the new first element as previous element
    if(state->skeletonState->firstTree) {
        state->skeletonState->firstTree->previous = newElement;
    }
    //We change the old and new first elements
    state->skeletonState->firstTree = newElement;

    state->skeletonState->activeTree = newElement;
    //qDebug("Added new tree with ID: %d.", newElement->treeID);

    state->skeletonState->treesByID.emplace(newElement->treeID, newElement);

    if(newElement->treeID > state->skeletonState->greatestTreeID) {
        state->skeletonState->greatestTreeID = newElement->treeID;
    }

    state->skeletonState->unsavedChanges = true;

    emit treeAddedSignal(*newElement);

    setActiveTreeByID(newElement->treeID);

    return newElement;
}

treeListElement *Skeletonizer::getTreeWithPrevID(treeListElement *currentTree) {
    treeListElement *tree = state->skeletonState->firstTree;
    treeListElement *prevTree = NULL;
    uint idDistance = state->skeletonState->treeElements;
    uint tempDistance = idDistance;

    if(currentTree == NULL) {
        // no current tree, return active tree
        if(state->skeletonState->activeTree) {
            return state->skeletonState->activeTree;
        }
        // no active tree, simply return first tree found
        if(tree) {
            return tree;
        }
        qDebug() << "no tree to move to";
        return NULL;
    }
    while(tree) {
        if(tree->treeID < currentTree->treeID) {
            tempDistance = currentTree->treeID - tree->treeID;
            if(tempDistance == 1) {//smallest distance possible
                return tree;
            }
            if(tempDistance < idDistance) {
                idDistance = tempDistance;
                prevTree = tree;
            }
        }
        tree = tree->next;
    }
    return prevTree;
}

treeListElement* Skeletonizer::getTreeWithNextID(treeListElement *currentTree) {
    treeListElement *tree = state->skeletonState->firstTree;
    treeListElement *nextTree = NULL;
    uint idDistance = state->skeletonState->treeElements;
    uint tempDistance = idDistance;

    if(currentTree == NULL) {
        // no current tree, return active tree
        if(state->skeletonState->activeTree) {
            return state->skeletonState->activeTree;
        }
        // no active tree, simply return first tree found
        if(tree) {
            return tree;
        }
        qDebug() << "no tree to move to";
        return NULL;
    }
    while(tree) {
        if(tree->treeID > currentTree->treeID) {
            tempDistance = tree->treeID - currentTree->treeID;

            if(tempDistance == 1) { //smallest distance possible
                return tree;
            }
            if(tempDistance < idDistance) {
                idDistance = tempDistance;
                nextTree = tree;
            }
        }
        tree = tree->next;
    }
    return nextTree;
}

bool Skeletonizer::addTreeCommentToSelectedTrees(QString comment) {
    const auto blockState = signalsBlocked();
    blockSignals(true);

    for(const auto &tree : state->skeletonState->selectedTrees) {
        addTreeComment(tree->treeID, comment);
    }

    blockSignals(blockState);

    emit resetData();

    return true;
}

bool Skeletonizer::addTreeComment(int treeID, QString comment) {
    treeListElement *tree = NULL;

    tree = findTreeByTreeID(treeID);

    if((!comment.isNull()) && tree) {
        strncpy(tree->comment, comment.toStdString().c_str(), 8192);
    }

    state->skeletonState->unsavedChanges = true;
    emit treeChangedSignal(*tree);

    return true;
}

segmentListElement* Skeletonizer::findSegmentByNodeIDs(uint sourceNodeID, uint targetNodeID) {
    segmentListElement *currentSegment;
    nodeListElement *currentNode;

    currentNode = findNodeByNodeID(sourceNodeID);

    if(!currentNode) { return NULL;}
    currentSegment = currentNode->firstSegment;
    while(currentSegment) {
        if(currentSegment->flag == SEGMENT_BACKWARD) {
            currentSegment = currentSegment->next;
            continue;
        }
        if(currentSegment->target->nodeID == targetNodeID) {
            return currentSegment;
        }
        currentSegment = currentSegment->next;
    }
    return NULL;
}

bool Skeletonizer::editNode(uint nodeID, nodeListElement *node,
                            float newRadius, int newXPos, int newYPos, int newZPos, int inMag) {

    if(!node) {
        node = findNodeByNodeID(nodeID);
    }
    if(!node) {
        qDebug("Cannot edit: node id %u invalid.", nodeID);
        return false;
    }

    nodeID = node->nodeID;

    if(!((newXPos < 0) || (newXPos > state->boundary.x)
       || (newYPos < 0) || (newYPos > state->boundary.y)
       || (newZPos < 0) || (newZPos > state->boundary.z))) {
        node->position = {newXPos, newYPos, newZPos};
    }

    if(newRadius != 0.) {
        node->radius = newRadius;
    }
    node->createdInMag = inMag;

    updateCircRadius(node);
    state->skeletonState->unsavedChanges = true;

    emit nodeChangedSignal(*node);

    return true;
}

void* Skeletonizer::popStack(stack *stack) {
    //The stack should hold values != NULL only, NULL indicates an error.
    void *element = NULL;

    if(stack->stackpointer >= 0) {
        element = stack->elements[stack->stackpointer];
    }
    else {
        return NULL;
    }
    stack->stackpointer--;
    stack->elementsOnStack--;

    return element;
}

bool Skeletonizer::pushStack(stack *stack, void *element) {
    if(element == NULL) {
        qDebug() << "Stack can't hold NULL.";
        return false;
    }

    if(stack->stackpointer + 1 == stack->size) {
        stack->elements = (void**)realloc(stack->elements, stack->size * 2 * sizeof(void *));
        if(stack->elements == NULL) {
            qDebug() << "Out of memory.";
            _Exit(false);
        }
        stack->size = stack->size * 2;
    }

    stack->stackpointer++;
    stack->elementsOnStack++;

    stack->elements[stack->stackpointer] = element;

    return true;
}

stack* Skeletonizer::newStack(int size) {
    struct stack *newStack = NULL;

    if(size <= 0) {
        qDebug() << "That doesn't really make any sense, right? Cannot create stack with size <= 0.";
        return NULL;
    }

    newStack = (stack *)malloc(sizeof(struct stack));
    if(newStack == NULL) {
        qDebug() << "Out of memory.";
        _Exit(false);
    }
    memset(newStack, '\0', sizeof(struct stack));

    newStack->elements = (void **)malloc(sizeof(void *) * size);
    if(newStack->elements == NULL) {
        qDebug() << "Out of memory.";
        _Exit(false);
    }
    memset(newStack->elements, '\0', sizeof(void *) * size);

    newStack->size = size;
    newStack->stackpointer = -1;
    newStack->elementsOnStack = 0;

    return newStack;

}

bool Skeletonizer::delStack(stack *stack) {
    free(stack->elements);
    free(stack);

    return true;
}

bool Skeletonizer::extractConnectedComponent(int nodeID) {
    //  This function takes a node and splits the connected component
    //  containing that node into a new tree, unless the connected component
    //  is equivalent to exactly one entire tree.

    //  It uses breadth-first-search. Depth-first-search would be the same with
    //  a stack instead of a queue for storing pending nodes. There is no
    //  practical difference between the two algorithms for this task.

    auto node = findNodeByNodeID(nodeID);
    if (!node) {
        return false;
    }

    std::set<treeListElement*> treesSeen; // Connected component might consist of multiple trees.
    std::queue<nodeListElement*> queue;
    std::set<nodeListElement*> visitedNodes;
    visitedNodes.insert(node);
    queue.push(node);
    while(queue.size() > 0) {
        auto nextNode = queue.front();
        queue.pop();
        treesSeen.insert(nextNode->correspondingTree);

        auto segment = nextNode->firstSegment;
        while(segment) {
            auto *neighbor = (segment->flag == SEGMENT_FORWARD)? segment->target : segment->source;
            if(neighbor != nullptr && visitedNodes.find(neighbor) == visitedNodes.end()) {
                visitedNodes.insert(neighbor);
                queue.push(neighbor);
            }
            segment = segment->next;
        }
    }

    //  If the total number of nodes visited is smaller than the sum of the
    //  number of nodes in all trees we've seen, the connected component is a
    //  strict subgraph of the graph containing all trees we've seen and we
    //  should split it.

    // We want this function to not do anything when
    // there are no disconnected components in the same tree, but create a new
    // tree when the connected component spans several trees. This is a useful
    // feature when performing skeleton consolidation and allows one to merge
    // many trees at once.
    uint nodeCountSeenTrees = 0;
    for(auto tree : treesSeen) {
        nodeCountSeenTrees += tree->size;
    }
    if(visitedNodes.size() == nodeCountSeenTrees && treesSeen.size() == 1) {
        return false;
    }

    color4F treeCol;
    treeCol.r = -1.;
    auto newTree = addTreeListElement(0, treeCol);
    // Splitting the connected component.
    std::vector<int> deletedTrees;
    nodeListElement *last = NULL;
    for(auto node : visitedNodes) {
        // Removing node list element from its old position
        --node->correspondingTree->size;
        if(node->previous != NULL) {
            node->previous->next = node->next;
        }
        else {
            node->correspondingTree->firstNode = node->next;
        }
        if(node->next != NULL) {
            node->next->previous = node->previous;
        }
        if (node->correspondingTree->size == 0) {//remove empty trees
            deletedTrees.emplace_back(node->correspondingTree->treeID);
            delTree(node->correspondingTree->treeID);
        }
        // Inserting node list element into new list.
        ++newTree->size;
        node->next = NULL;
        if(last != NULL) {
            node->previous = last;
            last->next = node;
        }
        else {
            node->previous = NULL;
            newTree->firstNode = node;
        }
        last = node;
        node->correspondingTree = newTree;
    }
    state->skeletonState->unsavedChanges = true;
    setActiveTreeByID(newTree->treeID);//the empty tree had no active node

    return true;
}

bool Skeletonizer::addComment(QString content, nodeListElement *node, uint nodeID) {
    commentListElement *newComment;

    std::string content_stdstr = content.toStdString();
    const char *content_cstr = content_stdstr.c_str();

    newComment = (commentListElement*)malloc(sizeof(commentListElement));
    memset(newComment, '\0', sizeof(commentListElement));

    newComment->content = (char*)malloc(strlen(content_cstr) * sizeof(char) + 1);
    memset(newComment->content, '\0', strlen(content_cstr) * sizeof(char) + 1);

    if(nodeID) {
        node = findNodeByNodeID(nodeID);
    }
    if(node) {
        newComment->node = node;
        node->comment = newComment;
    }

    if(content_cstr) {
        strncpy(newComment->content, content_cstr, strlen(content_cstr));
    }

    if(!state->skeletonState->currentComment) {
        state->skeletonState->currentComment = newComment;
        //We build a circular linked list
        newComment->next = newComment;
        newComment->previous = newComment;
    }
    else {
        //We insert into a circular linked list
        state->skeletonState->currentComment->previous->next = newComment;
        newComment->next = state->skeletonState->currentComment;
        newComment->previous = state->skeletonState->currentComment->previous;
        state->skeletonState->currentComment->previous = newComment;

        state->skeletonState->currentComment = newComment;
    }


    //write into commentBuffer, so that comment appears in comment text field when added via Shortcut
    memset(state->skeletonState->commentBuffer, '\0', 10240);
    strncpy(state->skeletonState->commentBuffer,
            state->skeletonState->currentComment->content,
            strlen(state->skeletonState->currentComment->content));

    state->skeletonState->unsavedChanges = true;

    state->skeletonState->totalComments++;

    emit nodeChangedSignal(*node);

    return true;
}

bool Skeletonizer::setComment(QString newContent, nodeListElement *commentNode, uint commentNodeID) {
    if(commentNodeID) {
        commentNode = findNodeByNodeID(commentNodeID);
    }
    if (!commentNode) {
        qDebug() << "Please provide a valid comment node to set!";
        return false;
    }
    if(commentNode->comment) {
        return editComment(commentNode->comment, 0, newContent, NULL, 0);
    }
    return addComment(newContent, commentNode, 0);
}

bool Skeletonizer::delComment(commentListElement *currentComment, uint commentNodeID) {
    nodeListElement *commentNode = NULL;

    if(commentNodeID) {
        commentNode = findNodeByNodeID(commentNodeID);
        if(commentNode) {
            currentComment = commentNode->comment;
        }
    }

    if(!currentComment) {
        qDebug() << "Please provide a valid comment node to delete!";
        return false;
    }

    if(currentComment->content) {
        free(currentComment->content);
    }
    if(currentComment->node) {
        currentComment->node->comment = NULL;
    }

    if(state->skeletonState->currentComment == currentComment) {
        memset(state->skeletonState->commentBuffer, '\0', 10240);
    }

    if(currentComment->next == currentComment) {
        state->skeletonState->currentComment = NULL;
    }
    else {
        currentComment->next->previous = currentComment->previous;
        currentComment->previous->next = currentComment->next;

        if(state->skeletonState->currentComment == currentComment) {
            state->skeletonState->currentComment = currentComment->next;
        }
    }

    free(currentComment);

    state->skeletonState->unsavedChanges = true;

    state->skeletonState->totalComments--;

    emit nodeChangedSignal(*currentComment->node);

    return true;
}

bool Skeletonizer::editComment(commentListElement *currentComment, uint nodeID, QString newContent,
                               nodeListElement *newNode, uint newNodeID) {
    // this function also seems to be kind of useless as you could do just the same
    // thing with addComment() with minimal changes ....?

    std::string newContent_strstd = newContent.toStdString();
    const char *newContent_cstr = newContent_strstd.c_str();

    if(nodeID) {
        currentComment = findNodeByNodeID(nodeID)->comment;
    }
    if(!currentComment) {
        qDebug() << "Please provide a valid comment element to edit!";
        return false;
    }

    nodeID = currentComment->node->nodeID;

    if(newContent_cstr) {
        if(currentComment->content) {
            free(currentComment->content);
        }
        currentComment->content = (char*)malloc(strlen(newContent_cstr) * sizeof(char) + 1);
        memset(currentComment->content, '\0', strlen(newContent_cstr) * sizeof(char) + 1);
        strncpy(currentComment->content, newContent_cstr, strlen(newContent_cstr));
    }

    if(newNodeID) {
        newNode = findNodeByNodeID(newNodeID);
    }
    if(newNode) {
        if(currentComment->node) {
            currentComment->node->comment = NULL;
        }
        currentComment->node = newNode;
        newNode->comment = currentComment;
    }

    state->skeletonState->unsavedChanges = true;

    emit nodeChangedSignal(*currentComment->node);

    return true;
}

commentListElement* Skeletonizer::nextComment(QString searchString) {
   commentListElement *firstComment, *currentComment;

   std::string searchString_stdstr = searchString.toStdString();
   const char *searchString_cstr = searchString_stdstr.c_str();

    if(!strlen(searchString_cstr)) {
        //->previous here because it would be unintuitive for the user otherwise.
        //(we insert new comments always as first elements)
        if(state->skeletonState->currentComment) {
            state->skeletonState->currentComment = state->skeletonState->currentComment->previous;
            setActiveNode(state->skeletonState->currentComment->node, 0);
            jumpToActiveNode();
        }
    }
    else {
        if(state->skeletonState->currentComment) {
            firstComment = state->skeletonState->currentComment->previous;
            currentComment = firstComment;
            do {
                if(strstr(currentComment->content, searchString_cstr) != NULL) {
                    state->skeletonState->currentComment = currentComment;
                    setActiveNode(state->skeletonState->currentComment->node, 0);
                    jumpToActiveNode();
                    break;
                }
                currentComment = currentComment->previous;

            } while (firstComment != currentComment);
        }
    }

    memset(state->skeletonState->commentBuffer, '\0', 10240);

    if(state->skeletonState->currentComment) {
        strncpy(state->skeletonState->commentBuffer,
                state->skeletonState->currentComment->content,
                strlen(state->skeletonState->currentComment->content));

        if(state->skeletonState->lockPositions) {
            if(strstr(state->skeletonState->commentBuffer, state->skeletonState->onCommentLock)) {
                lockPosition(state->skeletonState->currentComment->node->position);
            }
            else {
                unlockPosition();
            }
        }

    }
    return state->skeletonState->currentComment;
}

commentListElement* Skeletonizer::previousComment(QString searchString) {
    commentListElement *firstComment, *currentComment;
    // ->next here because it would be unintuitive for the user otherwise.
    // (we insert new comments always as first elements)

    std::string searchString_stdstr = searchString.toStdString();
    const char *searchString_cstr = searchString_stdstr.c_str();

    if(!strlen(searchString_cstr)) {
        if(state->skeletonState->currentComment) {
            state->skeletonState->currentComment = state->skeletonState->currentComment->next;
            setActiveNode(state->skeletonState->currentComment->node, 0);
            jumpToActiveNode();
        }
    }
    else {
        if(state->skeletonState->currentComment) {
            firstComment = state->skeletonState->currentComment->next;
            currentComment = firstComment;
            do {
                if(strstr(currentComment->content, searchString_cstr) != NULL) {
                    state->skeletonState->currentComment = currentComment;
                    setActiveNode(state->skeletonState->currentComment->node, 0);
                    jumpToActiveNode();
                    break;
                }
                currentComment = currentComment->next;

            } while (firstComment != currentComment);

        }
    }
    memset(state->skeletonState->commentBuffer, '\0', 10240);

    if(state->skeletonState->currentComment) {
        strncpy(state->skeletonState->commentBuffer,
            state->skeletonState->currentComment->content,
            strlen(state->skeletonState->currentComment->content));

        if(state->skeletonState->lockPositions) {
            if(strstr(state->skeletonState->commentBuffer, state->skeletonState->onCommentLock)) {
                lockPosition(state->skeletonState->currentComment->node->position);
            }
            else {
                unlockPosition();
            }
        }
    }
    return state->skeletonState->currentComment;
}

bool Skeletonizer::searchInComment(char */*searchString*/, commentListElement */*comment*/) {
    // BUG unimplemented method!
    return true;
}

bool Skeletonizer::unlockPosition() {
    if(state->skeletonState->positionLocked) {
        qDebug() << "Spatial locking disabled.";
    }
    state->skeletonState->positionLocked = false;

    return true;
}

bool Skeletonizer::lockPosition(Coordinate lockCoordinate) {
    qDebug("locking to (%d, %d, %d).", lockCoordinate.x, lockCoordinate.y, lockCoordinate.z);
    state->skeletonState->positionLocked = true;
    state->skeletonState->lockedPosition = lockCoordinate;

    return true;
}

/* @todo loader gets out of sync (endless loop) */
nodeListElement* Skeletonizer::popBranchNode() {
    nodeListElement *branchNode = NULL;
    std::size_t branchNodeID = 0;

    // Nodes on the branch stack may not actually exist anymore
    while(true){
        if (branchNode != NULL)
            if (branchNode->isBranchNode == true) {
                break;
            }
        branchNodeID = reinterpret_cast<std::size_t>(popStack(state->skeletonState->branchStack));
        if(branchNodeID == 0) {
            QMessageBox box;
            box.setWindowTitle("Knossos Information");
            box.setIcon(QMessageBox::Information);
            box.setInformativeText("No branch points remain");
            box.exec();
            qDebug() << "No branch points remain.";

            goto exit_popbranchnode;
        }

        branchNode = findNodeByNodeID(branchNodeID);
    }

    if(branchNode && branchNode->isBranchNode) {
        qDebug() << "Branch point (" << branchNodeID << ") deleted.";

        setActiveNode(branchNode, 0);

        branchNode->isBranchNode = 0;

        emit userMoveSignal(branchNode->position.x - state->viewerState->currentPosition.x,
                            branchNode->position.y - state->viewerState->currentPosition.y,
                            branchNode->position.z - state->viewerState->currentPosition.z,
                            USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);

        state->skeletonState->branchpointUnresolved = true;
        emit branchPoppedSignal();
    }

exit_popbranchnode:
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->totalBranchpoints--;
    return branchNode;
}

bool Skeletonizer::pushBranchNode(int setBranchNodeFlag, int checkDoubleBranchpoint,
                                  nodeListElement *branchNode, uint branchNodeID) {
    if(branchNodeID) {
        branchNode = findNodeByNodeID(branchNodeID);
    }
    if(branchNode) {
        if(branchNode->isBranchNode == 0 || !checkDoubleBranchpoint) {
            pushStack(state->skeletonState->branchStack, reinterpret_cast<void *>(static_cast<std::size_t>(branchNode->nodeID)));
            if(setBranchNodeFlag) {
                branchNode->isBranchNode = true;

                qDebug("Branch point (node ID %d) added.", branchNode->nodeID);
                emit branchPushedSignal();
            }

        }
        else {
            qDebug() << "Active node is already a branch point";
            return true;
        }
    }
    else {
        qDebug() << "Make a node active before adding branch points.";
        return true;
    }

    state->skeletonState->unsavedChanges = true;

    state->skeletonState->totalBranchpoints++;
    return true;
}

bool Skeletonizer::jumpToActiveNode() {
    if(state->skeletonState->activeNode) {
        emit userMoveSignal(state->skeletonState->activeNode->position.x - state->viewerState->currentPosition.x,
                            state->skeletonState->activeNode->position.y - state->viewerState->currentPosition.y,
                            state->skeletonState->activeNode->position.z - state->viewerState->currentPosition.z,
                            USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
    }
    return true;
}

nodeListElement* Skeletonizer::popBranchNodeAfterConfirmation(QWidget * const parent) {
    if (state->skeletonState->branchpointUnresolved && state->skeletonState->branchStack->stackpointer != -1) {
        QMessageBox prompt(parent);
        prompt.setIcon(QMessageBox::Question);
        prompt.setText("Nothing has been modified after jumping to the last branch point.");
        prompt.setInformativeText("Do you really want to jump?");
        prompt.addButton("Jump", QMessageBox::AcceptRole);
        auto * cancel = prompt.addButton("Cancel", QMessageBox::RejectRole);

        prompt.exec();
        if (prompt.clickedButton() == cancel) {
            return nullptr;
        }
    }

    return popBranchNode();
}

void Skeletonizer::restoreDefaultTreeColor(treeListElement *tree) {
    if(tree == NULL) {
        return;
    }
    int index = (tree->treeID - 1) % 256;

    tree->color.r = state->viewerState->defaultTreeTable[index];
    tree->color.g = state->viewerState->defaultTreeTable[index + 256];
    tree->color.b = state->viewerState->defaultTreeTable[index + 512];
    tree->color.a = 1.;

    tree->colorSetManually = false;
    state->skeletonState->unsavedChanges = true;
}

void Skeletonizer::restoreDefaultTreeColor() {
    restoreDefaultTreeColor(state->skeletonState->activeTree);
}

bool Skeletonizer::updateTreeColors() {
    treeListElement *tree = state->skeletonState->firstTree;
    while(tree) {
        uint index = (tree->treeID - 1) % 256;
        tree->color.r = state->viewerState->treeAdjustmentTable[index];
        tree->color.g = state->viewerState->treeAdjustmentTable[index +  256];
        tree->color.b = state->viewerState->treeAdjustmentTable[index + 512];
        tree->color.a = 1.;
        tree = tree->next;
    }
    return true;
}

/**
 * @brief Skeletonizer::updateCircRadius if both the source and target node of a segment are outside the viewport,
 *      the segment would be culled away, too, regardless of wether it is in the viewing frustum.
 *      This function solves this by making the circ radius of one node as large as the segment,
 *      so that it cuts the viewport and its segment is rendered.
 * @return
 */
bool Skeletonizer::updateCircRadius(nodeListElement *node) {
    segmentListElement *currentSegment = NULL;
    node->circRadius = node->radius;

    /* Any segment longer than the current circ radius?*/
    currentSegment = node->firstSegment;
    while(currentSegment) {
        if(currentSegment->length > node->circRadius)
            node->circRadius = currentSegment->length;
        currentSegment = currentSegment->next;
    }
    return true;
}

void Skeletonizer::setColorFromNode(nodeListElement *node, color4F *color) {
    if(node->isBranchNode) { //branch nodes are always blue
        *color = {0.f, 0.f, 1.f, 1.f};
        return;
    }

    if(node->comment != NULL && strlen(node->comment->content) != 0) {
        // default color for comment nodes
        *color = {1.f, 1.f, 0.f, 1.f};

        if(CommentSetting::useCommentColors) {
            auto newColor = CommentSetting::getColor(QString(node->comment->content));
            if(newColor.alpha() != 0) {
                *color = color4F(newColor.red()/255., newColor.green()/255., newColor.blue()/255., newColor.alpha()/255.);
            }
        }
    }
}

void Skeletonizer::setRadiusFromNode(nodeListElement *node, float *radius) {
    *radius = node->radius;
    if(state->skeletonState->overrideNodeRadiusBool) {
        *radius = state->skeletonState->overrideNodeRadiusVal;
    }
    if(node->comment != NULL && CommentSetting::useCommentNodeRadius) {
        float newRadius = CommentSetting::getRadius(QString(node->comment->content));
        if(newRadius != 0) {
            *radius = newRadius;
        }
    }
}

bool Skeletonizer::moveToPrevTree() {
    treeListElement *prevTree = getTreeWithPrevID(state->skeletonState->activeTree);
    nodeListElement *node;
    if(state->skeletonState->activeTree == nullptr) {
        return false;
    }
    if(prevTree) {
        setActiveTreeByID(prevTree->treeID);
        //set tree's first node to active node if existent
        node = state->skeletonState->activeTree->firstNode;
        if(node == nullptr) {
            return true;
        } else {
            setActiveNode(node, node->nodeID);
            emit setRecenteringPositionSignal(node->position.x,
                                         node->position.y,
                                         node->position.z);

            Knossos::sendRemoteSignal();
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
    nodeListElement *node;

    if(state->skeletonState->activeTree == nullptr) {
        return false;
    }
    if(nextTree) {
        setActiveTreeByID(nextTree->treeID);
        //set tree's first node to active node if existent
        node = state->skeletonState->activeTree->firstNode;

        if(node == nullptr) {
            return true;
        } else {
            setActiveNode(node, node->nodeID);

                emit setRecenteringPositionSignal(node->position.x,
                                             node->position.y,
                                             node->position.z);
                Knossos::sendRemoteSignal();
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

bool Skeletonizer::moveToPrevNode() {
    nodeListElement *prevNode = getNodeWithPrevID(state->skeletonState->activeNode, true);
    if(prevNode) {
        setActiveNode(prevNode, prevNode->nodeID);
        emit setRecenteringPositionSignal(prevNode->position.x,
                                     prevNode->position.y,
                                     prevNode->position.z);
        Knossos::sendRemoteSignal();
        return true;
    }
    return false;
}

bool Skeletonizer::moveToNextNode() {
    nodeListElement *nextNode = getNodeWithNextID(state->skeletonState->activeNode, true);
    if(nextNode) {
        setActiveNode(nextNode, nextNode->nodeID);
        emit setRecenteringPositionSignal(nextNode->position.x,
                                     nextNode->position.y,
                                     nextNode->position.z);
        Knossos::sendRemoteSignal();
        return true;
    }
    return false;
}

bool Skeletonizer::moveSelectedNodesToTree(int treeID) {
    treeListElement *newTree = findTreeByTreeID(treeID);
    for (auto * const node : state->skeletonState->selectedNodes) {
        if(node == NULL || newTree == NULL) {
            return false;
        }

        nodeListElement *tmpNode, *lastNode;
        if(node->correspondingTree != NULL) {

            if(node->correspondingTree->treeID == newTree->treeID) {
                return false;
            }

            // remove node from old tree
            nodeListElement *lastNode = NULL; // the previous node in the current tree
            tmpNode = node->correspondingTree->firstNode;
            while(tmpNode) {
                if(tmpNode->nodeID == node->nodeID) {
                    break;
                }
                lastNode = tmpNode;
                tmpNode = tmpNode->next;
            }
            if(lastNode == NULL) { // node is the first node in its current tree
                if(node->next) {
                    node->correspondingTree->firstNode = node->next;
                    node->next->previous = NULL;
                }
                else {
                    node->correspondingTree->firstNode = NULL;
                }
            }
            else {
                lastNode->next = node->next;
                if(node->next) {
                    node->next->previous = lastNode;
                }

                // decrement node counter for the old tree
                --node->correspondingTree->size;
            }
        }

        // add node to new tree
        tmpNode = newTree->firstNode;
        lastNode = NULL;
        while(tmpNode) {
            lastNode = tmpNode;
            tmpNode = tmpNode->next;
        }
        if(lastNode == NULL) { // the moved node will be the first one in the new tree
            newTree->firstNode = node;
            node->next = NULL;
            node->previous = NULL;

        }
        else {
            lastNode->next = node;
            node->previous = lastNode;
            node->next = NULL;
        }
        node->correspondingTree = newTree;
        // increment node counter for the new tree
        ++newTree->size;
    }

    emit resetData();
    emit setActiveTreeByID(treeID);

    return true;
}

void Skeletonizer::selectNodes(const std::vector<nodeListElement*> & nodes) {
    clearNodeSelection();
    toggleNodeSelection(nodes);
}

void Skeletonizer::toggleNodeSelection(const std::vector<nodeListElement*> & nodes) {
    auto & selectedNodes = state->skeletonState->selectedNodes;

    std::unordered_set<decltype(nodeListElement::nodeID)> removeNodes;
    for (auto & elem : nodes) {
        elem->selected = !elem->selected;//toggle
        if (elem->selected) {
            selectedNodes.emplace_back(elem);
        } else {
            removeNodes.emplace(elem->nodeID);
        }
    }
    auto eraseIt = std::remove_if(std::begin(selectedNodes), std::end(selectedNodes), [&removeNodes](nodeListElement const * const node){
        return removeNodes.find(node->nodeID) != std::end(removeNodes);
    });
    if (eraseIt != std::end(selectedNodes)) {
        selectedNodes.erase(eraseIt);
    }

    if (selectedNodes.size() == 1) {
        setActiveNode(selectedNodes.front(), 0);
    } else if (selectedNodes.empty() && state->skeletonState->activeNode != nullptr) {
        // at least one must always be selected
        setActiveNode(state->skeletonState->activeNode, 0);
    } else {
        emit nodeSelectionChangedSignal();
    }
}

void Skeletonizer::selectTrees(const std::vector<treeListElement*> & trees) {
    clearTreeSelection();
    for (auto & elem : trees) {
        elem->selected = true;
    }
    auto & selectedTrees = state->skeletonState->selectedTrees;
    selectedTrees = trees;

    if (selectedTrees.size() == 1) {
        setActiveTreeByID(selectedTrees.front()->treeID);
    } else if (selectedTrees.empty() && state->skeletonState->activeTree != nullptr) {
        // at least one must always be selected
        setActiveTreeByID(state->skeletonState->activeTree->treeID);
    } else {
        emit treeSelectionChangedSignal();
    }
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

bool Skeletonizer::areConnected(const nodeListElement & v,const nodeListElement & w) const {
    std::queue<const nodeListElement*> queue;
    std::set<const nodeListElement*> visitedNodes;
    visitedNodes.insert(&v);
    queue.push(&v);
    while(queue.size() > 0) {
        auto nextNode = queue.front();
        queue.pop();
        if(nextNode == &w) {
            return true;
        }
        auto segment = nextNode->firstSegment;
        while(segment) {
            auto neighbor = (segment->flag == SEGMENT_FORWARD)? segment->target : segment->source;
            if(neighbor != nullptr && visitedNodes.find(neighbor) == visitedNodes.end()) {
                visitedNodes.insert(neighbor);
                queue.push(neighbor);
            }
            segment = segment->next;
        }
    }
    return false;
}

Skeletonizer::TracingMode Skeletonizer::getTracingMode() const {
    return tracingMode;
}

void Skeletonizer::setTracingMode(TracingMode mode) {
    tracingMode = mode;//change internal state
    //adjust gui
    if (tracingMode == TracingMode::skipNextLink && !state->viewer->window->addNodeAction->isChecked()) {
        state->viewer->window->addNodeAction->setChecked(true);
    } else if (tracingMode == TracingMode::linkedNodes && !state->viewer->window->linkWithActiveNodeAction->isChecked()) {
        state->viewer->window->linkWithActiveNodeAction->setChecked(true);
    } else if (tracingMode == TracingMode::unlinkedNodes && !state->viewer->window->dropNodesAction->isChecked()) {
        state->viewer->window->dropNodesAction->setChecked(true);
    }
}
