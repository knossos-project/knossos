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

#include <cstring>
#include <vector>

#include <QProgressDialog>

#include "file_io.h"
#include "functions.h"
#include "knossos-global.h"
#include "knossos.h"
#include "viewer.h"

extern stateInfo *state;

Skeletonizer::Skeletonizer(QObject *parent) : QObject(parent) {
    //This number is currently arbitrary, but high values ensure a good performance
    state->skeletonState->skeletonDCnumber = 8000;

    //Create a new hash-table that holds the skeleton datacubes
    state->skeletonState->skeletonDCs = Hashtable::ht_new(state->skeletonState->skeletonDCnumber);

    /*@todo
    if(state->skeletonState->skeletonDCs == HT_FAILURE) {
        qDebug() << "Unable to create skeleton hash-table.";
        return false;
    }*/

    state->skeletonState->nodeCounter = newDynArray(1048576);
    state->skeletonState->nodesByNodeID = newDynArray(1048576);
    state->skeletonState->branchStack = newStack(1048576);

    // Generate empty tree structures
    state->skeletonState->firstTree = NULL;
    state->skeletonState->treeElements = 0;
    state->skeletonState->totalNodeElements = 0;
    state->skeletonState->totalSegmentElements = 0;
    state->skeletonState->activeTree = NULL;
    state->skeletonState->activeNode = NULL;

    state->skeletonState->mergeOnLoadFlag = 0;
    state->skeletonState->segRadiusToNodeRadius = 0.5;
    //state->skeletonState->autoFilenameIncrementBool = true;
    state->skeletonState->greatestNodeID = 0;

    state->skeletonState->showNodeIDs = false;
    state->skeletonState->highlightActiveTree = true;
    state->skeletonState->showIntersections = false;

    state->skeletonState->displayListSkeletonSkeletonizerVP = 0;
    state->skeletonState->displayListView = 0;
    state->skeletonState->displayListDataset = 0;

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

    /*
    state->skeletonState->firstSerialSkeleton = (serialSkeletonListElement *)malloc(sizeof(state->skeletonState->firstSerialSkeleton));
    state->skeletonState->firstSerialSkeleton->next = NULL;
    state->skeletonState->firstSerialSkeleton->previous = NULL;
    state->skeletonState->lastSerialSkeleton = (serialSkeletonListElement *)malloc(sizeof(state->skeletonState->lastSerialSkeleton));
    state->skeletonState->lastSerialSkeleton->next = NULL;
    state->skeletonState->lastSerialSkeleton->previous = NULL;
    state->skeletonState->serialSkeletonCounter = 0;
    state->skeletonState->maxUndoSteps = 16;
    */

    state->skeletonState->saveCnt = 0;

    if((state->boundary.x >= state->boundary.y) && (state->boundary.x >= state->boundary.z))
        state->skeletonState->volBoundary = state->boundary.x * 2;
    if((state->boundary.y >= state->boundary.x) && (state->boundary.y >= state->boundary.y))
        state->skeletonState->volBoundary = state->boundary.y * 2;
    if((state->boundary.z >= state->boundary.x) && (state->boundary.z >= state->boundary.y))
        state->skeletonState->volBoundary = state->boundary.x * 2;

    state->skeletonState->viewChanged = true;
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->datasetChanged = true;
    state->skeletonState->skeletonSliceVPchanged = true;
    state->skeletonState->commentsChanged = false;
    state->skeletonState->unsavedChanges = false;
    state->skeletonState->askingPopBranchConfirmation = false;

}

nodeListElement *Skeletonizer::addNodeListElement(
              int nodeID,
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

    newElement = (nodeListElement*) malloc(sizeof(struct nodeListElement));
    if(newElement == NULL) {
        qDebug() << "Out of memory while trying to allocate memory for a new nodeListElement.";
        return NULL;
    }
    memset(newElement, '\0', sizeof(struct nodeListElement));

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

    SET_COORDINATE(newElement->position, position->x, position->y, position->z);
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

    newElement = (segmentListElement*) malloc(sizeof(struct segmentListElement));
    if(newElement == NULL) {
        qDebug() << "Out of memory while trying to allocate memory for a new segmentListElement.";
        return NULL;
    }
    memset(newElement, '\0', sizeof(struct segmentListElement));
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
    treeListElement *currentTree;

    currentTree = state->skeletonState->firstTree;

    while(currentTree) {
        if(currentTree->treeID == treeID) {
            return currentTree;
        }
        currentTree = currentTree->next;
    }
    return NULL;
}

/** @todo cleanup */
void Skeletonizer::WRAP_popBranchNode() {
    popBranchNode();
    state->skeletonState->askingPopBranchConfirmation = false;
}

void Skeletonizer::popBranchNodeCanceled() {
    state->skeletonState->askingPopBranchConfirmation = false;
}

bool Skeletonizer::UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype) {
    int addedNodeID;

    color4F treeCol;
    /* -1 causes new color assignment */
    treeCol.r = -1.;
    treeCol.g = -1.;
    treeCol.b = -1.;
    treeCol.a = 1.;

    if(!state->skeletonState->activeTree) {
        addTreeListElement(0, treeCol);
    }

    addedNodeID = addNode(
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
        return false;
    }

    setActiveNode(NULL, addedNodeID);

    if((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                               state->skeletonState->activeTree->treeID) == 1) {
        /* First node in this tree */

        pushBranchNode(true, true, NULL, addedNodeID);
        addComment("First Node", NULL, addedNodeID);
    }

    return true;
}

uint Skeletonizer::addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int makeNodeActive) {
    int targetNodeID;

    if(!state->skeletonState->activeNode) {
        qDebug() << "Please create a node before trying to link nodes.";
        return false;
    }

    //Add a new node at the target position first.
    targetNodeID = addNode(
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
    if((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                               state->skeletonState->activeTree->treeID) == 1) {
        /* First node in this tree */

        pushBranchNode(true, true, NULL, targetNodeID);
        addComment("First Node", NULL, targetNodeID);
        emit updateToolsSignal();
        emit updateTreeviewSignal();
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
    treeListElement *currentTree = NULL;
    nodeListElement *currentNode = NULL;
    PTRSIZEINT currentBranchPointID;
    segmentListElement *currentSegment = NULL;
    commentListElement *currentComment = NULL;
    stack *reverseBranchStack = NULL, *tempReverseStack = NULL;

    //  This function should always be called through UI_saveSkeleton
    // for proper error and file name display to the user.

    // We need to do this to be able to save the branch point stack to a file
    //and still have the branch points available to the user afterwards.

    reverseBranchStack = newStack(2048);
    tempReverseStack = newStack(2048);
    while((currentBranchPointID =
          (PTRSIZEINT)popStack(state->skeletonState->branchStack))) {
        pushStack(reverseBranchStack, (void *)currentBranchPointID);
        pushStack(tempReverseStack, (void *)currentBranchPointID);
    }

    while((currentBranchPointID =
          (PTRSIZEINT)popStack(tempReverseStack))) {
        currentNode = (struct nodeListElement *)findNodeByNodeID(currentBranchPointID);
        pushBranchNode(false, false, currentNode, 0);
    }

    QString tmp;

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("things");

    xml.writeStartElement("parameters");
    xml.writeStartElement("experiment");
    xml.writeAttribute("name", QString(state->name));
    xml.writeEndElement();

    xml.writeStartElement("lastsavedin");
    xml.writeAttribute("version", QString(KVERSION));
    xml.writeEndElement();

    xml.writeStartElement("createdin");
    char *ptr = state->skeletonState->skeletonCreatedInVersion;
    xml.writeAttribute("version", QString(ptr));
    xml.writeEndElement();

    xml.writeStartElement("scale");
    xml.writeAttribute("x", tmp.setNum((float)state->scale.x / state->magnification));
    xml.writeAttribute("y", tmp.setNum((float)state->scale.y / state->magnification));
    xml.writeAttribute("z", tmp.setNum((float)state->scale.z / state->magnification));
    xml.writeEndElement();

    xml.writeStartElement("offset");
    xml.writeAttribute("x", tmp.setNum(state->offset.x / state->magnification));
    xml.writeAttribute("y", tmp.setNum(state->offset.y / state->magnification));
    xml.writeAttribute("z", tmp.setNum(state->offset.z / state->magnification));
    xml.writeEndElement();

    xml.writeStartElement("RadiusLocking");
    xml.writeAttribute("enableCommentLocking", tmp.setNum(state->skeletonState->lockPositions));
    xml.writeAttribute("lockingRadius", tmp.setNum(state->skeletonState->lockRadius));
    xml.writeAttribute("lockToNodesWithComment", QString(state->skeletonState->onCommentLock));
    xml.writeEndElement();

    xml.writeStartElement("time");
    const int time = state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + state->time.elapsed();
    xml.writeAttribute("ms", tmp.setNum(time));
    const auto timeData = QByteArray::fromRawData(reinterpret_cast<const char * const>(&time), sizeof(time));
    const QString timeChecksum = QCryptographicHash::hash(timeData, QCryptographicHash::Sha256).toHex().constData();
    xml.writeAttribute("checksum", timeChecksum);
    xml.writeEndElement();

    if(state->skeletonState->activeNode) {
        xml.writeStartElement("activeNode");
        xml.writeAttribute("id", tmp.setNum(state->skeletonState->activeNode->nodeID));
        xml.writeEndElement();
    }

    xml.writeStartElement("editPosition");
    xml.writeAttribute("x", tmp.setNum(state->viewerState->currentPosition.x + 1));
    xml.writeAttribute("y", tmp.setNum(state->viewerState->currentPosition.y + 1));
    xml.writeAttribute("z", tmp.setNum(state->viewerState->currentPosition.z + 1));
    xml.writeEndElement();

    xml.writeStartElement("skeletonVPState");
    int j = 0;
    char element[8];
    for(j = 0; j < 16; j++) {
        sprintf(element, "E%d", j);
        ptr = element;
        xml.writeAttribute(QString(ptr), tmp.setNum(state->skeletonState->skeletonVpModelView[j]));
    }
    xml.writeAttribute("translateX", tmp.setNum(state->skeletonState->translateX));
    xml.writeAttribute("translateY", tmp.setNum(state->skeletonState->translateY));
    xml.writeEndElement();

    xml.writeStartElement("vpSettingsZoom");
    xml.writeAttribute("XYPlane", tmp.setNum(state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel));
    xml.writeAttribute("XZPlane", tmp.setNum(state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel));
    xml.writeAttribute("YZPlane", tmp.setNum(state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel));
    xml.writeAttribute("SkelPlane", tmp.setNum(state->viewerState->vpConfigs[VIEWPORT_SKELETON].texture.zoomLevel));
    xml.writeEndElement();

    xml.writeStartElement("idleTime");
    xml.writeAttribute("ms", tmp.setNum(state->skeletonState->idleTime));
    const auto idleTimeData = QByteArray::fromRawData(reinterpret_cast<const char * const>(&state->skeletonState->idleTime), sizeof(state->skeletonState->idleTime));
    const QString idleTimeChecksum = QCryptographicHash::hash(idleTimeData, QCryptographicHash::Sha256).toHex().constData();
    xml.writeAttribute("checksum", idleTimeChecksum);
    xml.writeEndElement();

    xml.writeEndElement(); // end parameters

    currentTree = state->skeletonState->firstTree;
    if((currentTree == NULL) && (state->skeletonState->currentComment == NULL)) {
        return false; // No Skeleton to save
    }

    while(currentTree) {
        //Every "thing" has associated nodes and edges.
        xml.writeStartElement("thing");
        xml.writeAttribute("id", tmp.setNum(currentTree->treeID));

        if(currentTree->colorSetManually) {
            xml.writeAttribute("color.r", tmp.setNum(currentTree->color.r));
            xml.writeAttribute("color.g", tmp.setNum(currentTree->color.g));
            xml.writeAttribute("color.b", tmp.setNum(currentTree->color.b));
            xml.writeAttribute("color.a", tmp.setNum(currentTree->color.a));
        } else {
            xml.writeAttribute("color.r", QString("-1."));
            xml.writeAttribute("color.g", QString("-1."));
            xml.writeAttribute("color.b", QString("-1."));
            xml.writeAttribute("color.a", QString("1."));
        }
        if(currentTree->comment) {
            xml.writeAttribute("comment", QString(currentTree->comment));
        }
        xml.writeStartElement("nodes");
        currentNode = currentTree->firstNode;
        while(currentNode) {
            xml.writeStartElement("node");
            xml.writeAttribute("id", tmp.setNum(currentNode->nodeID));
            xml.writeAttribute("radius", tmp.setNum(currentNode->radius));
            xml.writeAttribute("x", tmp.setNum(currentNode->position.x + 1));
            xml.writeAttribute("y", tmp.setNum(currentNode->position.y + 1));
            xml.writeAttribute("z", tmp.setNum(currentNode->position.z + 1));
            xml.writeAttribute("inVp", tmp.setNum(currentNode->createdInVp));
            xml.writeAttribute("inMag", tmp.setNum(currentNode->createdInMag));
            xml.writeAttribute("time", tmp.setNum(currentNode->timestamp));

            currentNode = currentNode->next;
            xml.writeEndElement(); // end node
        }
        xml.writeEndElement(); // end nodes

        xml.writeStartElement("edges");
        currentNode = currentTree->firstNode;
        while(currentNode) {
            currentSegment = currentNode->firstSegment;
            while(currentSegment) {
                if(currentSegment->flag == SEGMENT_FORWARD) {
                    xml.writeStartElement("edge");
                    xml.writeAttribute("source", tmp.setNum(currentSegment->source->nodeID));
                    xml.writeAttribute("target", tmp.setNum(currentSegment->target->nodeID));
                    xml.writeEndElement();
                }
                currentSegment = currentSegment->next;
            }
            currentNode = currentNode->next;
        }
        xml.writeEndElement(); // end edges
        currentTree = currentTree->next;
        xml.writeEndElement(); // end tree
    }

    currentComment = state->skeletonState->currentComment;
    if(state->skeletonState->currentComment) {
        xml.writeStartElement("comments");
        do {
            xml.writeStartElement("comment");
            xml.writeAttribute("node", tmp.setNum(currentComment->node->nodeID));
            xml.writeAttribute("content", QString(currentComment->content));
            xml.writeEndElement();
            currentComment = currentComment->next;
        } while(currentComment != state->skeletonState->currentComment);
        xml.writeEndElement(); // end comments
    }

    xml.writeStartElement("branchpoints");
    while((currentBranchPointID = (PTRSIZEINT)popStack(reverseBranchStack))) {
        xml.writeStartElement("branchpoint");
        xml.writeAttribute("id", tmp.setNum(currentBranchPointID));
        xml.writeEndElement();
    }
    xml.writeEndElement(); // end branchpoints

    xml.writeEndElement(); // end things
    xml.writeEndDocument();
    return true;
}

bool Skeletonizer::loadXmlSkeleton(QIODevice & file, const QString & treeCmtOnMultiLoad) {
    int merge = false;
    int activeNodeID = 0, greatestNodeIDbeforeLoading = 0, greatestTreeIDbeforeLoading = 0;
    int inMag, magnification = 0;
    int globalMagnificationSpecified = false;

    treeListElement *currentTree;

    Coordinate offset;
    floatCoordinate scale;
    Coordinate loadedPosition;
    SET_COORDINATE(offset, state->offset.x, state->offset.y, state->offset.z);
    SET_COORDINATE(scale, state->scale.x, state->scale.y, state->scale.z);
    SET_COORDINATE(loadedPosition, 0, 0, 0);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qErrnoWarning("Document not parsed successfully.");
        return false;
    }

    if(state->skeletonState->mergeOnLoadFlag == false) {
        merge = false;
        clearSkeleton(true);
    }
    else {
        merge = true;
        greatestNodeIDbeforeLoading = state->skeletonState->greatestNodeID;
        greatestTreeIDbeforeLoading = state->skeletonState->greatestTreeID;
    }

    // If "createdin"-node does not exist, skeleton was created in a version
    // before 3.2
    strcpy(state->skeletonState->skeletonCreatedInVersion, "pre-3.2");
    strcpy(state->skeletonState->skeletonLastSavedInVersion, "pre-3.2");

    // Default for skeletons created in very old versions that don't have that
    //   attribute
    if(!merge) {
        state->skeletonState->skeletonTime = 0;
    }
    QTime bench;
    QXmlStreamReader xml(&file);

    std::vector<uint> branchVector;
    std::vector<std::pair<int, QString>> commentsVector;
    std::vector<std::pair<int, int>> edgeVector;

    bench.start();

    if (!xml.readNextStartElement() || xml.name() != "things") {
        qDebug() << "invalid xml token: " << xml.name();
        return false;
    }
    while(xml.readNextStartElement()) {
        if(xml.name() == "parameters") {
            while(xml.readNextStartElement()) {
                QXmlStreamAttributes attributes = xml.attributes();

                if(xml.name() == "createdin") {
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
                } else if(xml.name() == "magnification" and xml.isStartElement()) {
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
                } else if(xml.name() == "offset") {
                    QStringRef attribute = attributes.value("x");
                    if(attribute.isNull() == false) {
                        offset.x = attribute.toLocal8Bit().toInt();
                    }
                    attribute = attributes.value("y");
                    if(attribute.isNull() == false) {
                        offset.y = attribute.toLocal8Bit().toInt();
                    }
                    attribute = attributes.value("z");
                    if(attribute.isNull() == false) {
                        offset.z = attribute.toLocal8Bit().toInt();
                    }
                } else if(xml.name() == "time" && merge == false) {
                    QStringRef attribute = attributes.value("ms");
                    if(attribute.isNull() == false) {
                        state->skeletonState->skeletonTime = attribute.toLocal8Bit().toInt();
                        if(Skeletonizer::isObfuscatedTime(state->skeletonState->skeletonTime)) {
                            state->skeletonState->skeletonTime = xorInt(state->skeletonState->skeletonTime);
                        }
                    }
                } else if(xml.name() == "activeNode" && !merge) {
                    QStringRef attribute = attributes.value("id");
                    if(attribute.isNull() == false) {
                        activeNodeID = attribute.toLocal8Bit().toInt();
                    }
                } else if(xml.name() == "scale") {
                    QStringRef attribute = attributes.value("x");
                    if(attribute.isNull() == false) {
                        scale.x = attribute.toLocal8Bit().toFloat();
                    }
                    attribute = attributes.value("y");
                    if(attribute.isNull() == false) {
                        scale.y = attribute.toLocal8Bit().toFloat();
                    }
                    attribute = attributes.value("z");
                    if(attribute.isNull() == false) {
                        scale.z = attribute.toLocal8Bit().toFloat();
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
                } else if(merge == false && xml.name() == "idleTime") {
                    QStringRef attribute = attributes.value("ms");
                    if(attribute.isNull() == false) {
                        state->skeletonState->idleTime = attribute.toString().toInt();
                        if(Skeletonizer::isObfuscatedTime(state->skeletonState->idleTime)) {
                            state->skeletonState->idleTime = xorInt(state->skeletonState->idleTime);
                        }
                    }
                }
                xml.skipCurrentElement();
            }
        } else if(xml.name() == "branchpoints") {
            while(xml.readNextStartElement()) {
                if(xml.name() == "branchpoint") {
                    QXmlStreamAttributes attributes = xml.attributes();
                    QStringRef attribute = attributes.value("id");
                    int nodeID;
                    if(attribute.isNull() == false) {
                        if(merge == false) {
                            nodeID = attribute.toLocal8Bit().toInt();
                        } else {
                            nodeID = attribute.toLocal8Bit().toInt() + greatestNodeIDbeforeLoading;
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
                    int nodeID;
                    if(attribute.isNull() == false) {
                        if(merge == false) {
                            nodeID = attribute.toLocal8Bit().toInt();
                        } else {
                            nodeID = attribute.toLocal8Bit().toInt() + greatestNodeIDbeforeLoading;
                        }
                    }
                    attribute = attributes.value("content");
                    if(attribute.isNull() == false) {
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
                setActiveTreeByID(currentTree->treeID);
                neuronID = currentTree->treeID;
            } else {
                currentTree = addTreeListElement(neuronID, neuronColor);
                setActiveTreeByID(neuronID);
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

                            int nodeID;
                            attribute = attributes.value("id");
                            if(attribute.isNull() == false) {
                                nodeID = attribute.toLocal8Bit().toInt();
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

                            Byte VPtype;
                            attribute = attributes.value("inVp");
                            if(attribute.isNull() == false) {
                                VPtype = attribute.toLocal8Bit().toInt();
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
                            int time = state->skeletonState->skeletonTime;// for legacy skeleton files
                            if(attribute.isNull() == false) {
                                time = attribute.toLocal8Bit().toInt();
                            }

                            if(merge == false) {
                                addNode(nodeID, radius, neuronID, &currentCoordinate, VPtype, inMag, time, false);
                            }
                            else {
                                nodeID += greatestNodeIDbeforeLoading;
                                addNode(nodeID, radius, neuronID, &currentCoordinate, VPtype, inMag, time, false);
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
                            int sourcecNodeId;
                            attribute = attributes.value("source");
                            if(attribute.isNull() == false) {
                                sourcecNodeId = attribute.toLocal8Bit().toInt();
                            } else {
                                sourcecNodeId = 0;
                            }
                            int targetNodeId;
                            attribute = attributes.value("target");
                            if(attribute.isNull() == false) {
                                targetNodeId = attribute.toLocal8Bit().toInt();
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

    qDebug() << "loading skeleton took: "<< bench.elapsed();

    if(!merge) {

        if(activeNodeID) {
            if(setActiveNode(NULL, activeNodeID) == false and state->skeletonState->firstTree) { // if nml has invalid active node ID, simply make first node active
                if(state->skeletonState->firstTree->firstNode) {
                    setActiveNode(NULL, state->skeletonState->firstTree->firstNode->nodeID);
                }
            }
        }


        if((loadedPosition.x != 0) &&
           (loadedPosition.y != 0) &&
           (loadedPosition.z != 0)) {
            Coordinate jump;
            SET_COORDINATE(jump, loadedPosition.x - 1 - state->viewerState->currentPosition.x,
                                 loadedPosition.y - 1 - state->viewerState->currentPosition.y,
                                 loadedPosition.z - 1 - state->viewerState->currentPosition.z);
            emit userMoveSignal(jump.x, jump.y, jump.z);
        }

    }
    state->skeletonState->skeletonTimeCorrection = state->time.elapsed();
    return true;
}

bool Skeletonizer::delActiveNode() {
    if(state->skeletonState->activeNode) {
        delNode(0, state->skeletonState->activeNode);
    }
    else {
        return false;
    }
    return true;
}

bool Skeletonizer::delActiveTree() {
    struct treeListElement *nextTree = NULL;

    if(state->skeletonState->activeTree) {
        if(state->skeletonState->activeTree->next) {
            nextTree = state->skeletonState->activeTree->next;
        }
        else if(state->skeletonState->activeTree->previous) {
            nextTree = state->skeletonState->activeTree->previous;
        }

        delTree(state->skeletonState->activeTree->treeID);

        if(nextTree) {
            setActiveTreeByID(nextTree->treeID);
        }
        else {
            state->skeletonState->activeTree = NULL;
        }
    }
    else {
       qDebug() << "No active tree available.";
       return false;
    }

    return true;
}

bool Skeletonizer::delSegment(int sourceNodeID, int targetNodeID, segmentListElement *segToDel) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

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

    //Out of skeleton structure
    //delSegmentFromSkeletonStruct(segToDel);

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

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;

    return true;
}

/*
 * We have to delete the node from 2 structures: the skeleton's nested linked list structure
 * and the skeleton visualization structure (hashtable with skeletonDCs).
 */
bool Skeletonizer::delNode(int nodeID, nodeListElement *nodeToDel) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    struct segmentListElement *currentSegment;
    struct segmentListElement *tempNext = NULL;
    struct nodeListElement *newActiveNode = NULL;
    int treeID;


    if(!nodeToDel)
        nodeToDel = findNodeByNodeID(nodeID);

    nodeID = nodeToDel->nodeID;
    if(!nodeToDel) {
        qDebug("The given node %d doesn't exist. Unable to delete it.", nodeID);
        return false;
    }

    treeID = nodeToDel->correspondingTree->treeID;

    if(nodeToDel->comment) {
        delComment(nodeToDel->comment, 0);
    }

    if(nodeToDel->isBranchNode) {
        state->skeletonState->totalBranchpoints--;
    }

     // First, delete all segments pointing towards and away of the nodeToDelhas
     // been

    currentSegment = nodeToDel->firstSegment;

    while(currentSegment) {

        tempNext = currentSegment->next;

        if(currentSegment->flag == SEGMENT_FORWARD)
            delSegment(0,0, currentSegment);
        else if(currentSegment->flag == SEGMENT_BACKWARD)
            delSegment(0,0, currentSegment->reverseSegment);

        currentSegment = tempNext;
    }

    nodeToDel->firstSegment = NULL;


    if(state->skeletonState->selectedCommentNode == nodeToDel) {
        state->skeletonState->selectedCommentNode = NULL;
    }

    if(nodeToDel == nodeToDel->correspondingTree->firstNode) {
        nodeToDel->correspondingTree->firstNode = nodeToDel->next;
    }
    else {
        if(nodeToDel->previous) {
            nodeToDel->previous->next = nodeToDel->next;
        }
        if(nodeToDel->next) {
            nodeToDel->next->previous = nodeToDel->previous;
        }
    }

    setDynArray(state->skeletonState->nodesByNodeID, nodeToDel->nodeID, NULL);
    if(state->skeletonState->activeNode == nodeToDel) {
        newActiveNode = findNearbyNode(nodeToDel->correspondingTree,
                                       nodeToDel->position);

        setActiveNode(newActiveNode, 0);
    }
    free(nodeToDel);

    state->skeletonState->totalNodeElements--;
    //state->viewerState->gui->totalNodes--;

    setDynArray(state->skeletonState->nodeCounter,
            treeID,
            (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter, treeID) - 1));

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;

    return true;
}

bool Skeletonizer::delTree(int treeID) {

    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    treeListElement *currentTree;
    nodeListElement *currentNode, *nodeToDel;

    currentTree = findTreeByTreeID(treeID);
    if(!currentTree) {
        qDebug("There exists no tree with ID %d. Unable to delete it.", treeID);
        return false;
    }

    currentNode = currentTree->firstNode;
    while(currentNode) {
        nodeToDel = currentNode;
        currentNode = nodeToDel->next;
        delNode(0, nodeToDel);
    }
    currentTree->firstNode = NULL;

    if (currentTree->previous != nullptr) {
        currentTree->previous->next = currentTree->next;
    }
    if (currentTree->next != nullptr) {
        currentTree->next->previous = currentTree->previous;
    }
    if(currentTree == state->skeletonState->firstTree) {
        state->skeletonState->firstTree = currentTree->next;
    }

    free(currentTree);

    state->skeletonState->treeElements--;

    state->skeletonState->skeletonChanged = true;
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
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    return true;
}

bool Skeletonizer::setActiveNode(nodeListElement *node, int nodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

     // If both *node and nodeID are specified, nodeID wins.
     // If neither *node nor nodeID are specified
     // (node == NULL and nodeID == 0), the active node is
     // set to NULL.

    if(nodeID != 0) {
        node = findNodeByNodeID(nodeID);
        if(!node) {
            qDebug("No node with id %d available.", nodeID);
            return false;
        }
    }
    if(node) {
        nodeID = node->nodeID;
    }

    state->skeletonState->activeNode = node;
    state->skeletonState->viewChanged = true;
    state->skeletonState->skeletonChanged = true;

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
    else {
        state->skeletonState->activeTree = NULL;
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

    return true;
}

int Skeletonizer::addNode(int nodeID, float radius, int treeID, Coordinate *position, Byte VPtype, int inMag, int time, int respectLocks) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
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
            if(state->viewerState->gui->lockComment == QString(state->skeletonState->onCommentLock)) {
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
        qDebug("Node with ID %d already exists, no node added.", nodeID);
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
    setDynArray(state->skeletonState->nodeCounter,
                treeID,
                (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter, treeID) + 1));

    if(nodeID == 0) {
        nodeID = state->skeletonState->totalNodeElements;
        //Test if node ID over node counter is available. If not, find a valid one.
        while(findNodeByNodeID(nodeID)) {
            nodeID++;
        }
    }

    tempNode = addNodeListElement(nodeID, radius, &(tempTree->firstNode), position, inMag);
    tempNode->correspondingTree = tempTree;
    tempNode->comment = NULL;
    tempNode->createdInVp = VPtype;

    if(tempTree->firstNode == NULL) {
        tempTree->firstNode = tempNode;
    }

    updateCircRadius(tempNode);

    if(time == -1) {
        time = state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + state->time.elapsed();
    }

    tempNode->timestamp = time;

    setDynArray(state->skeletonState->nodesByNodeID, nodeID, (void *)tempNode);

    //printf("Added node %p, id %d, tree %p\n", tempNode, tempNode->nodeID,
    //        tempNode->correspondingTree);

    //Add a pointer to the node in the skeleton DC structure
   // addNodeToSkeletonStruct(tempNode);
    state->skeletonState->skeletonChanged = true;

    if(nodeID > state->skeletonState->greatestNodeID) {
        state->skeletonState->greatestNodeID = nodeID;
    }
    state->skeletonState->unsavedChanges = true;

    return nodeID;
}

bool Skeletonizer::addSegment(int sourceNodeID, int targetNodeID) {
    nodeListElement *targetNode, *sourceNode;
    segmentListElement *sourceSeg;
    floatCoordinate node1, node2;
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(findSegmentByNodeIDs(sourceNodeID, targetNodeID)) {
        qDebug("Segment between nodes %d and %d exists already.", sourceNodeID, targetNodeID);
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

    // Add the segment to the skeleton DC structure

    //addSegmentToSkeletonStruct(sourceSeg);

    // printf("added segment for nodeID %d: %d, %d, %d -> nodeID %d: %d, %d, %d\n", sourceNode->nodeID, sourceNode->position.x + 1, sourceNode->position.y + 1, sourceNode->position.z + 1, targetNode->nodeID, targetNode->position.x + 1, targetNode->position.y + 1, targetNode->position.z + 1);

    updateCircRadius(sourceNode);

    state->skeletonState->skeletonChanged = true;
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
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    treeListElement *currentTree, *treeToDel;

    currentTree = state->skeletonState->firstTree;
    while(currentTree) {
        treeToDel = currentTree;
        currentTree = treeToDel->next;
        delTree(treeToDel->treeID);
    }

    state->skeletonState->activeNode = NULL;
    state->skeletonState->activeTree = NULL;

    //Hashtable::ht_rmtable(state->skeletonState->skeletonDCs);
    delDynArray(state->skeletonState->nodeCounter);
    delDynArray(state->skeletonState->nodesByNodeID);
    delStack(state->skeletonState->branchStack);

    //Create a new hash-table that holds the skeleton datacubes
    //state->skeletonState->skeletonDCs = Hashtable::ht_new(state->skeletonState->skeletonDCnumber);
    //if(state->skeletonState->skeletonDCs == HT_FAILURE) {
    //    qDebug() << "Unable to create skeleton hash-table.";
    //    Knossos::unlockSkeleton(false);
    //    return false;
    //}

    //Generate empty tree structures
    state->skeletonState->firstTree = NULL;
    state->skeletonState->totalNodeElements = 0;
    state->skeletonState->totalSegmentElements = 0;
    state->skeletonState->treeElements = 0;
    state->skeletonState->activeTree = NULL;
    state->skeletonState->activeNode = NULL;

    state->skeletonState->nodeCounter = newDynArray(1048576);
    state->skeletonState->nodesByNodeID = newDynArray(1048576);
    state->skeletonState->branchStack = newStack(1048576);

    state->skeletonState->unsavedChanges = true;

    resetSkeletonMeta();

    state->skeletonState->unsavedChanges = false;

    return true;
}

bool Skeletonizer::mergeTrees(int treeID1, int treeID2) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    treeListElement *tree1, *tree2;
    nodeListElement *currentNode;
    nodeListElement *firstNode, *lastNode;

    if(treeID1 == treeID2) {
        qDebug() << "Could not merge trees. Provided IDs are the same!";
        return false;
    }

    tree1 = findTreeByTreeID(treeID1);
    tree2 = findTreeByTreeID(treeID2);

    if(!(tree1) || !(tree2)) {
        qDebug() << "Could not merge trees, provided IDs are not valid!";
        return false;
    }

    currentNode = tree2->firstNode;

    while(currentNode) {
        //Change the corresponding tree
        currentNode->correspondingTree = tree1;

        currentNode = currentNode->next;
    }

    //Now we insert the node list of tree2 before the node list of tree1
    if(tree1->firstNode && tree2->firstNode) {
        //First, we have to find the last node of tree2 (this node has to be connected
        //to the first node inside of tree1)
        firstNode = tree2->firstNode;
        lastNode = firstNode;
        while(lastNode->next) {
            lastNode = lastNode->next;
        }

        tree1->firstNode->previous = lastNode;
        lastNode->next = tree1->firstNode;
        tree1->firstNode = firstNode;

    }
    else if(tree2->firstNode) {
        tree1->firstNode = tree2->firstNode;
    }

    // The new node count for tree1 is the old node count of tree1 plus the node
    // count of tree2
    setDynArray(state->skeletonState->nodeCounter,
                tree1->treeID,
                (void *)
                ((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                        tree1->treeID) +
                (PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                        tree2->treeID)));

    // The old tree is gone and has 0 nodes.
    setDynArray(state->skeletonState->nodeCounter,
                tree2->treeID,
                (void *)0);

    //Delete the "empty" tree 2
    if(tree2 == state->skeletonState->firstTree) {
        state->skeletonState->firstTree = tree2->next;
        tree2->next->previous = NULL;
    }
    else {
        tree2->previous->next = tree2->next;
        if(tree2->next)
            tree2->next->previous = tree2->previous;
    }
    if(state->skeletonState->activeTree->treeID == tree2->treeID) {
       setActiveTreeByID(tree1->treeID);
    }
    free(tree2);

    state->skeletonState->treeElements--;
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;

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

nodeListElement* Skeletonizer::findNodeByNodeID(int nodeID) {
    nodeListElement *node;
    node = (struct nodeListElement *)getDynArray(state->skeletonState->nodesByNodeID, nodeID);
    return node;
}

treeListElement* Skeletonizer::addTreeListElement(int treeID, color4F color) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

     //  The variable sync is a workaround for the problem that this function
     // will sometimes be called by other syncable functions that themselves hold
     // the lock and handle synchronization. If set to false, all synchro
     // routines will be skipped.

    treeListElement *newElement = NULL;

    newElement = findTreeByTreeID(treeID);
    if(newElement) {
        qDebug("Tree with ID %d already exists!", treeID);
        return newElement;
    }

    newElement = (treeListElement*)malloc(sizeof(struct treeListElement));
    if(newElement == NULL) {
        qDebug() << "Out of memory while trying to allocate memory for a new treeListElement.";
        return NULL;
    }
    memset(newElement, '\0', sizeof(struct treeListElement));

    state->skeletonState->treeElements++;

    //Tree ID is unique in tree list
    //Take the provided tree ID if there is one.
    if(treeID != 0) {
        newElement->treeID = treeID;
    }
    else {
        newElement->treeID = state->skeletonState->treeElements;
        //Test if tree ID over tree counter is available. If not, find a valid one.
        while(findTreeByTreeID(newElement->treeID)) {
            newElement->treeID++;
        }
    }
    clearTreeSelection();
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

    if(newElement->treeID > state->skeletonState->greatestTreeID) {
        state->skeletonState->greatestTreeID = newElement->treeID;
    }

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;

    if(treeID == 1) {
        Skeletonizer::setActiveTreeByID(1);
    }

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

bool Skeletonizer::addTreeComment(int treeID, QString comment) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    treeListElement *tree = NULL;

    tree = findTreeByTreeID(treeID);

    if((!comment.isNull()) && tree) {
        strncpy(tree->comment, comment.toStdString().c_str(), 8192);
    }

    state->skeletonState->unsavedChanges = true;

    return true;
}

segmentListElement* Skeletonizer::findSegmentByNodeIDs(int sourceNodeID, int targetNodeID) {
    //qDebug() << "entered findSegmentByID";
    segmentListElement *currentSegment;
    nodeListElement *currentNode;

    currentNode = findNodeByNodeID(sourceNodeID);
    //qDebug() << "Current Node ID =" << currentNode;

    if(!currentNode) { return NULL;}
    currentSegment = currentNode->firstSegment;
    //qDebug() << currentSegment;
    while(currentSegment) {
        //qDebug() << "SEGMENT_BACK=2 : " << currentSegment->flag;
        if(currentSegment->flag == SEGMENT_BACKWARD) {
            currentSegment = currentSegment->next;
            continue;
        }
        if(currentSegment->target->nodeID == targetNodeID) {
            //qDebug() << "success";
            return currentSegment;
        }
        currentSegment = currentSegment->next;
    }

    //qDebug() << "returning null";
    return NULL;
}

bool Skeletonizer::genTestNodes(uint number) {
    uint i;
    int nodeID;
    struct nodeListElement *node;
    Coordinate pos;
    color4F treeCol;
    //add new tree for test nodes
    treeCol.r = -1.;
    addTreeListElement(0, treeCol);

    srand(time(NULL));
    pos.x = (int)(((double)rand() / (double)RAND_MAX) * (double)state->boundary.x);
    pos.y = (int)(((double)rand() / (double)RAND_MAX) * (double)state->boundary.y);
    pos.z = (int)(((double)rand() / (double)RAND_MAX) * (double)state->boundary.z);

    nodeID = UI_addSkeletonNode(&pos, rand()%4);

    for(i = 1; i < number; i++) {
        pos.x = (int)(((double)rand() / (double)RAND_MAX) * (double)state->boundary.x);
        pos.y = (int)(((double)rand() / (double)RAND_MAX) * (double)state->boundary.y);
        pos.z = (int)(((double)rand() / (double)RAND_MAX) * (double)state->boundary.z);
        nodeID = UI_addSkeletonNode(&pos, rand()%4);
        node = findNodeByNodeID(nodeID);
        node->comment = (commentListElement*)calloc(1, sizeof(struct commentListElement)); // @tocheck
        node->comment->content = (char*)calloc(1, 512); // @tocheck
        strcpy(node->comment->content, "test");
        node->comment->node = node;

        if(!state->skeletonState->currentComment) {
            state->skeletonState->currentComment = node->comment;
            //We build a circular linked list
            node->comment->next = node->comment;
            node->comment->previous = node->comment;
        } else {
            //We insert into a circular linked list
            state->skeletonState->currentComment->previous->next = node->comment;
            node->comment->next = state->skeletonState->currentComment;
            node->comment->previous = state->skeletonState->currentComment->previous;
            state->skeletonState->currentComment->previous = node->comment;

            state->skeletonState->currentComment = node->comment;
        }
    }
    state->skeletonState->commentsChanged = true;
    return true;
}

bool Skeletonizer::editNode(int nodeID, nodeListElement *node, float newRadius, int newXPos, int newYPos, int newZPos, int inMag) {

    if(!node) {
        node = findNodeByNodeID(nodeID);
    }
    if(!node) {
        qDebug("Cannot edit: node id %d invalid.", nodeID);
        return false;
    }

    nodeID = node->nodeID;

    if(!((newXPos < 0) || (newXPos > state->boundary.x)
       || (newYPos < 0) || (newYPos > state->boundary.y)
       || (newZPos < 0) || (newZPos > state->boundary.z))) {
        SET_COORDINATE(node->position, newXPos, newYPos, newZPos);
    }

    if(newRadius != 0.) {
        node->radius = newRadius;
    }
    node->createdInMag = inMag;

    updateCircRadius(node);
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;

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

bool Skeletonizer::delDynArray(dynArray *array) {
    free(array->elements);
    free(array);

    return true;
}

void* Skeletonizer::getDynArray(dynArray *array, int pos) {
    if(pos > array->end) {
        return nullptr;
    }
    return array->elements[pos];
}

bool Skeletonizer::setDynArray(dynArray *array, int pos, void *value) {
    while(pos > array->end) {
        array->elements = (void**)realloc(array->elements, (array->end + 1 +
                                  array->firstSize) * sizeof(void *));
        if(array->elements == NULL) {
            qDebug() << "Out of memory.";
            _Exit(false);
        }
        memset(&(array->elements[array->end + 1]), '\0', array->firstSize);
        array->end = array->end + array->firstSize;
    }

    array->elements[pos] = value;

    return true;
}

dynArray* Skeletonizer::newDynArray(int size) {
    dynArray *newArray = NULL;

    newArray = (dynArray*)malloc(sizeof(struct dynArray));
    if(newArray == NULL) {
        qDebug() << "Out of memory.";
        _Exit(false);
    }
    memset(newArray, '\0', sizeof(struct dynArray));

    newArray->elements = (void**)malloc(sizeof(void *) * size);
    if(newArray->elements == NULL) {
        qDebug() << "Out of memory.";
        _Exit(false);
    }
    memset(newArray->elements, '\0', sizeof(void *) * size);

    newArray->end = size - 1;
    newArray->firstSize = size;

    return newArray;
}

int Skeletonizer::splitConnectedComponent(int nodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    stack *remainingNodes = NULL;
    stack *componentNodes = NULL;
    nodeListElement *n = NULL, *last = NULL, *node = NULL;
    treeListElement *newTree = NULL, *currentTree = NULL;
    segmentListElement *currentEdge = NULL;
    dynArray *treesSeen = NULL;
    int treesCount = 0;
    int i = 0, id;
    PTRSIZEINT nodeCountAllTrees = 0, nodeCount = 0;
    uint visitedBase, index;
    Byte *visitedRight = NULL, *visitedLeft = NULL, **visited = NULL;
    uint visitedRightLen = 16384, visitedLeftLen = 16384, *visitedLen = NULL;

     //  This function takes a node and splits the connected component
     //  containing that node into a new tree, unless the connected component
     //  is equivalent to exactly one entire tree.

     //  It uses depth-first search. Breadth-first-search would be the same with
     //  a queue instead of a stack for storing pending nodes. There is no
     //  practical difference between the two algorithms for this task.
     //
     //  TODO trees might become empty when the connected component spans more
     //       than one tree.


    node = findNodeByNodeID(nodeID);
    if(!node) {
        return false;
    }


     //  This stack can be rather small without ever having to be resized as
     //  our graphs are usually very sparse.


    remainingNodes = newStack(512);
    componentNodes = newStack(4096);


     //  This is used to keep track of which trees we've seen. The connected
     //  component for a specific node can be part of more than one tree. That
     //  makes it slightly tedious to check whether the connected component is a
     //  strict subgraph.


    treesSeen = newDynArray(16);


     //  These are used to store which nodes have been visited.
     //  Nodes with IDs smaller than the entry node will be stored in
     //  visitedLeft, nodes with equal or larger ID will be stored in
     //  visitedRight. This is done to save some memory. ;)


    visitedRight = (Byte*)malloc(16384 * sizeof(Byte));
    visitedLeft = (Byte*)malloc(16384 * sizeof(Byte));

    if(visitedRight == NULL || visitedLeft == NULL) {
        qDebug() << "Out of memory.";
        _Exit(false);
    }

    memset(visitedLeft, NODE_PRISTINE, 16384);
    memset(visitedRight, NODE_PRISTINE, 16384);

    visitedBase = node->nodeID;
    pushStack(remainingNodes, (void *)node);

    // popStack() returns NULL when the stack is empty.
    while((n = (struct nodeListElement *)popStack(remainingNodes))) {
        if(n->nodeID >= visitedBase) {
            index = n->nodeID - visitedBase;
            visited = &visitedRight;
            visitedLen = &visitedRightLen;
        }
        else {
            index = visitedBase - n->nodeID - 1;
            visited = &visitedLeft;
            visitedLen = &visitedLeftLen;
        }

        // If the index is out bounds of the visited array, we resize the array
        while(index > *visitedLen) {
            *visited = (Byte*)realloc(*visited, (*visitedLen + 16384) * sizeof(Byte));
            if(*visited == NULL) {
                qDebug() << "Out of memory.";
                _Exit(false);
            }

            memset(*visited + *visitedLen, NODE_PRISTINE, 16384 * sizeof(Byte));
            *visitedLen = *visitedLen + 16384;
        }

        if((*visited)[index] == NODE_VISITED) {
            continue;
        }
        (*visited)[index] = NODE_VISITED;

        pushStack(componentNodes, (void *)n);

        // Check if the node is in a tree we haven't yet seen.
        // If it is, add that tree as seen.
        for(i = 0; i < treesCount; i++) {
            if(getDynArray(treesSeen, i) == (void *)n->correspondingTree) {
                break;
            }
        }
        if(i == treesCount) {
            setDynArray(treesSeen, treesCount, (void *)n->correspondingTree);
            treesCount++;
        }

        nodeCount = nodeCount + 1;

        // And now we push all adjacent nodes on the stack.
        currentEdge = n->firstSegment;
        while(currentEdge) {
            if(currentEdge->flag == SEGMENT_FORWARD) {
                pushStack(remainingNodes, (void *)currentEdge->target);
            }
            else if(currentEdge->flag == SEGMENT_BACKWARD) {
                pushStack(remainingNodes, (void *)currentEdge->source);
            }
            currentEdge = currentEdge->next;
        }
    }

     //  If the total number of nodes visited is smaller than the sum of the
     //  number of nodes in all trees we've seen, the connected component is a
     //  strict subgraph of the graph containing all trees we've seen and we
     //  should split it.

     //  Since we're checking for treesCount > 1 below, this implementation is
     //  now slightly redundant. We want this function to not do anything when
     //  there are no disconnected components in the same tree, but create a new
     //  tree when the connected component spans several trees. This is a useful
     //  feature when performing skeleton consolidation and allows one to merge
     //  many trees at once.
     //  Just remove the treesCount > 1 below to get back to the original
     //  behaviour of only splitting strict subgraphs.

    for(i = 0; i < treesCount; i++) {
        currentTree = (struct treeListElement *)getDynArray(treesSeen, i);
        id = currentTree->treeID;
        nodeCountAllTrees += (PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                                     id);
    }

    if(treesCount > 1 || nodeCount < nodeCountAllTrees) {
        color4F treeCol;
        treeCol.r = -1.;
        newTree = state->viewer->skeletonizer->addTreeListElement(0, treeCol);
        // Splitting the connected component.

        while((n = (struct nodeListElement *)popStack(componentNodes))) {
            // Removing node list element from its old position
            setDynArray(state->skeletonState->nodeCounter,
                        n->correspondingTree->treeID,
                        (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                                         n->correspondingTree->treeID) - 1));

            if(n->previous != NULL) {
                n->previous->next = n->next;
            }
            else {
                n->correspondingTree->firstNode = n->next;
            }
            if(n->next != NULL) {
                n->next->previous = n->previous;
            }

            // Inserting node list element into new list.
            setDynArray(state->skeletonState->nodeCounter, newTree->treeID,
                        (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                                         newTree->treeID) + 1));

            n->next = NULL;

            if(last != NULL) {
                n->previous = last;
                last->next = n;
            }
            else {
                n->previous = NULL;
                newTree->firstNode = n;
            }
            last = n;
            n->correspondingTree = newTree;
        }
        state->skeletonState->skeletonChanged = true;
    }
    else {
        qDebug() << "The connected component is equal to the entire tree, not splitting.";
    }

    delStack(remainingNodes);
    delStack(componentNodes);

    free(visitedLeft);
    free(visitedRight);

    state->skeletonState->unsavedChanges = true;

    return nodeCount;
}

bool Skeletonizer::addComment(QString content, nodeListElement *node, int nodeID) {
    //This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    commentListElement *newComment;

    std::string content_stdstr = content.toStdString();
    const char *content_cstr = content_stdstr.c_str();

    newComment = (commentListElement*)malloc(sizeof(struct commentListElement));
    memset(newComment, '\0', sizeof(struct commentListElement));

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
        state->skeletonState->skeletonChanged = true;
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
    return true;
}

bool Skeletonizer::delComment(commentListElement *currentComment, int commentNodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    int nodeID = 0;
    nodeListElement *commentNode = NULL;

    if(commentNodeID) {
        commentNode = findNodeByNodeID(commentNodeID);
        if(commentNode) {
            currentComment = commentNode->comment;
        }
    }

    if(!currentComment) {
        qDebug() << "Please provide a valid comment element to delete!";
        return false;
    }

    if(currentComment->content) {
        free(currentComment->content);
    }
    if(currentComment->node) {
        nodeID = currentComment->node->nodeID;
        currentComment->node->comment = NULL;
        state->skeletonState->skeletonChanged = true;
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
    return true;
}

bool Skeletonizer::editComment(commentListElement *currentComment, int nodeID, QString newContent, nodeListElement *newNode, int newNodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
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

        //write into commentBuffer, so that comment appears in comment text field when added via Shortcut
        /*
        memset(state->skeletonState->commentBuffer, '\0', 10240);
        strncpy(state->skeletonState->commentBuffer,
                state->skeletonState->currentComment->content,
                strlen(state->skeletonState->currentComment->content));
        */
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
            setActiveNode(
                          state->skeletonState->currentComment->node,
                          0);
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
                    setActiveNode(
                                  state->skeletonState->currentComment->node,
                                  0);
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
    emit updateToolsSignal();
    emit updateTreeviewSignal();
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
            setActiveNode(
                          state->skeletonState->currentComment->node,
                          0);
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
                    setActiveNode(
                                  state->skeletonState->currentComment->node,
                                  0);
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
    emit updateToolsSignal();
    emit updateTreeviewSignal();
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
    SET_COORDINATE(state->skeletonState->lockedPosition,
                   lockCoordinate.x,
                   lockCoordinate.y,
                   lockCoordinate.z);

    return true;
}

/* @todo loader gets out of sync (endless loop) */
bool Skeletonizer::popBranchNode() {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    // SYNCHRO BUG:
    // both instances will have to confirm branch point deletion if
    // confirmation is asked.

    nodeListElement *branchNode = NULL;
    PTRSIZEINT branchNodeID = 0;

    // Nodes on the branch stack may not actually exist anymore
    while(true){
        if (branchNode != NULL)
            if (branchNode->isBranchNode == true) {
                break;
            }
        branchNodeID = (PTRSIZEINT)popStack(state->skeletonState->branchStack);
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
#if QT_POINTER_SIZE == 8
        qDebug("Branch point (node ID %ld) deleted.", branchNodeID);
#else
        qDebug("Branch point (node ID %d) deleted.", branchNodeID);
#endif

        setActiveNode(branchNode, 0);

        branchNode->isBranchNode = 0;
        state->skeletonState->skeletonChanged = true;

        emit userMoveSignal(branchNode->position.x - state->viewerState->currentPosition.x,
                            branchNode->position.y - state->viewerState->currentPosition.y,
                            branchNode->position.z - state->viewerState->currentPosition.z);

        state->skeletonState->branchpointUnresolved = true;
    }

exit_popbranchnode:

    state->skeletonState->unsavedChanges = true;

    state->skeletonState->totalBranchpoints--;
    return true;
}

bool Skeletonizer::pushBranchNode(int setBranchNodeFlag, int checkDoubleBranchpoint,
                                  nodeListElement *branchNode, int branchNodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(branchNodeID) {
        branchNode = findNodeByNodeID(branchNodeID);
    }
    if(branchNode) {
        if(branchNode->isBranchNode == 0 || !checkDoubleBranchpoint) {
            pushStack(state->skeletonState->branchStack, (void *)(PTRSIZEINT)branchNode->nodeID);
            if(setBranchNodeFlag) {
                branchNode->isBranchNode = true;

                state->skeletonState->skeletonChanged = true;
                qDebug("Branch point (node ID %d) added.", branchNode->nodeID);
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
                            state->skeletonState->activeNode->position.z - state->viewerState->currentPosition.z);
    }
    return true;
}

void Skeletonizer::UI_popBranchNode() {
    if(state->skeletonState->askingPopBranchConfirmation == false) {
        state->skeletonState->askingPopBranchConfirmation = true;

        if(state->skeletonState->branchpointUnresolved && state->skeletonState->branchStack->stackpointer != -1) {
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Question);
            prompt.setWindowTitle("Cofirmation required");
            prompt.setText("No node was added after jumping to the last branch point. Do you really want to jump?");
            QPushButton *jump = prompt.addButton("Jump", QMessageBox::ActionRole);
            QPushButton *cancel = prompt.addButton("Cancel", QMessageBox::ActionRole);
            prompt.exec();

            if(prompt.clickedButton() == jump) {
                WRAP_popBranchNode();
            } else if(prompt.clickedButton() == cancel) {
                popBranchNodeCanceled();
            } else {
                return;
            }
        }
        else {
            WRAP_popBranchNode();
        }
    }
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
    state->skeletonState->skeletonChanged = true;
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
    state->skeletonState->skeletonChanged = true;
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
    struct segmentListElement *currentSegment = NULL;
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

int Skeletonizer::xorInt(int xorMe) {
    return xorMe ^ (int) 5642179165;
}

void Skeletonizer::setColorFromNode(nodeListElement *node, color4F *color) {
    int nr;

    if(node->isBranchNode) { //branch nodes are always blue
        SET_COLOR((*color), 0.f, 0.f, 1.f, 1.f);
        return;
    }

    if(node->comment != NULL) {
        // default color for comment nodes
        SET_COLOR((*color), 1.f, 1.f, 0.f, 1.f);

        if(state->skeletonState->userCommentColoringOn == false) {
            // conditional node coloring is disabled
            // comment nodes have default color, return
            return;
        }

        if((nr = commentContainsSubstr(node->comment, -1)) == -1) {
            //no substring match => keep default color and return
            return;
        }
        if(state->skeletonState->commentColors[nr].a > 0.f) {
            //substring match, change color
            *color = state->skeletonState->commentColors[nr];
        }
    }
    return;
}

void Skeletonizer::setRadiusFromNode(nodeListElement *node, float *radius) {
    int nr;

        if(state->skeletonState->overrideNodeRadiusBool)
            *radius = state->skeletonState->overrideNodeRadiusVal;
        else
            *radius = node->radius;

        if(node->comment != NULL) {
            if(state->skeletonState->commentNodeRadiusOn == false) {
                // conditional node radius is disabled
                // return
                return;
            }

            if((nr = commentContainsSubstr(node->comment, -1)) == -1) {
                //no substring match => keep default radius and return
                return;
            }

            if(state->skeletonState->commentNodeRadii[nr] > 0.f) {
                //substring match, change radius
                *radius = state->skeletonState->commentNodeRadii[nr];
            }
        }
}

bool Skeletonizer::moveToPrevTree() {

    struct treeListElement *prevTree = getTreeWithPrevID(state->skeletonState->activeTree);
    struct nodeListElement *node;
    if(state->skeletonState->activeTree == NULL) {
        return false;
    }
    if(prevTree) {
        setActiveTreeByID(prevTree->treeID);
        //set tree's first node to active node if existent
        node = state->skeletonState->activeTree->firstNode;
        if(node == NULL) {
            return true;
        }
        else {
            setActiveNode(node, node->nodeID);
            emit setRecenteringPositionSignal(node->position.x,
                                         node->position.y,
                                         node->position.z);

            Knossos::sendRemoteSignal();
            emit updateToolsSignal();
            emit updateTreeviewSignal();
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

    struct treeListElement *nextTree = getTreeWithNextID(state->skeletonState->activeTree);
    struct nodeListElement *node;

    if(state->skeletonState->activeTree == NULL) {
        return false;
    }
    if(nextTree) {
        setActiveTreeByID(nextTree->treeID);
        //set tree's first node to active node if existent
        node = state->skeletonState->activeTree->firstNode;

        if(node == NULL) {
            return true;
        } else {
            setActiveNode(node, node->nodeID);

                emit setRecenteringPositionSignal(node->position.x,
                                             node->position.y,
                                             node->position.z);
                Knossos::sendRemoteSignal();
                emit updateToolsSignal();
                emit updateTreeviewSignal();
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
    struct nodeListElement *prevNode = getNodeWithPrevID(state->skeletonState->activeNode, true);
    if(prevNode) {
        setActiveNode(prevNode, prevNode->nodeID);
        emit setRecenteringPositionSignal(prevNode->position.x,
                                     prevNode->position.y,
                                     prevNode->position.z);
        Knossos::sendRemoteSignal();
        emit updateToolsSignal();
        emit updateTreeviewSignal();
        return true;
    }
    return false;
}

bool Skeletonizer::moveToNextNode() {
    struct nodeListElement *nextNode = getNodeWithNextID(state->skeletonState->activeNode, true);
    if(nextNode) {
        setActiveNode(nextNode, nextNode->nodeID);
        emit setRecenteringPositionSignal(nextNode->position.x,
                                     nextNode->position.y,
                                     nextNode->position.z);
        Knossos::sendRemoteSignal();
        emit updateToolsSignal();
        emit updateTreeviewSignal();
        return true;
    }
    return false;
}

bool Skeletonizer::moveNodeToTree(nodeListElement *node, int treeID) {
    treeListElement *newTree = findTreeByTreeID(treeID);
    if(node == NULL or newTree == NULL) {
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
            setDynArray(state->skeletonState->nodeCounter,
                    node->correspondingTree->treeID,
                    (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter, node->correspondingTree->treeID) - 1));
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
    setDynArray(state->skeletonState->nodeCounter,
            newTree->treeID,
            (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter, newTree->treeID) + 1));

    return true;
}

void Skeletonizer::deleteSelectedTrees() {
    for (const auto & elem : state->skeletonState->selectedTrees) {
        if (elem == state->skeletonState->activeTree) {
            delActiveTree();
        } else {
            delTree(elem->treeID);
        }
    }
    state->skeletonState->selectedTrees.clear();
    if(state->skeletonState->activeTree) {
        state->skeletonState->activeTree->selected = true;
        state->skeletonState->selectedTrees.push_back(state->skeletonState->activeTree);
    }
}

void Skeletonizer::deleteSelectedNodes() {
    const auto nodesToDelete = state->skeletonState->selectedNodes;
    for (auto & elem : nodesToDelete) {
        if (elem == state->skeletonState->activeNode) {
            delActiveNode();
        } else {
            delNode(0, elem);
        }
    }
    state->skeletonState->selectedNodes.clear();
    if(state->skeletonState->activeNode) {
        state->skeletonState->activeNode->selected = true;
        state->skeletonState->selectedNodes.push_back(state->skeletonState->activeNode);
    }
}

// index optionally specifies substr, range is [-1, NUM_COMMSUBSTR - 1].
// If -1, all substrings are compared against the comment.
unsigned int Skeletonizer::commentContainsSubstr(struct commentListElement *comment, int index) {
    int i;

    if(!comment) {
        return -1;
    }
    if(index == -1) { //no index specified
        for(i = 0; i < NUM_COMMSUBSTR; i++) {
            if(state->viewerState->gui->commentSubstr->at(i).length() > 0
                && strstr(comment->content, const_cast<char *>(state->viewerState->gui->commentSubstr->at(i).toStdString().c_str())) != NULL) {
                return i;
            }
        }
    }
    else if(index > -1 && index < NUM_COMMSUBSTR) {
        if(state->viewerState->gui->commentSubstr->at(index).length() > 0
           && strstr(comment->content, const_cast<char *>(state->viewerState->gui->commentSubstr->at(index).toStdString().c_str())) != NULL) {
            return index;
        }
    }
    return -1;
}

bool Skeletonizer::isObfuscatedTime(int time) {
    /* Sad detour in version 3.4 */

    /* The check for whether skeletonTime is bigger than some magic
       number is a workaround for the bug where skeletons saved in
       a version prior to 3.4 and edited in 3.4 sometimes had
       un-obfuscated times. Assuming the time is below ca. 15 days,
       this successfully checks for obfuscation. */

    if((strcmp(state->skeletonState->skeletonLastSavedInVersion, "3.4") == 0) &&
              (time > 1300000000)) {
        return true;
    }
    return false;
}

void Skeletonizer::resetSkeletonMeta() {
    // Whenever the user arrives at a 'clean slate',
    // by clearing the skeleton or by deleting all nodes,
    // skeleton meta information that is either set during
    // tracing or loaded with the skeleton should be reset.

    state->skeletonState->skeletonTime = 0;
    state->skeletonState->idleTime = 0;
    state->skeletonState->idleTimeLast = state->time.elapsed(); // there is no last idle time when resetting
    state->skeletonState->skeletonTimeCorrection = state->time.elapsed();
    strcpy(state->skeletonState->skeletonCreatedInVersion, KVERSION);
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
