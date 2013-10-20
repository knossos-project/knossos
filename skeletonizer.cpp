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

#include <cstring>
#include <time.h>
#include <QProgressDialog>
#include "skeletonizer.h"
#include "knossos-global.h"
#include "knossos.h"
#include "client.h"
#include "functions.h"

extern stateInfo *state;

Skeletonizer::Skeletonizer(QObject *parent) : QObject(parent) {

    idleTimeSession = 0;
    lockRadius = 100;
    lockPositions = false;
    strncpy(onCommentLock, "seed", 1024);
    branchpointUnresolved = false;
    autoFilenameIncrementBool = true;
    autoSaveBool = true;
    autoSaveInterval = 5;
    skeletonTime = 0;
    skeletonTimeCorrection = state->time.elapsed();
    definedSkeletonVpView = 0;

    //This number is currently arbitrary, but high values ensure a good performance
    state->skeletonState->skeletonDCnumber = 8000;
    state->skeletonState->workMode = ON_CLICK_DRAG;

    updateSkeletonState();

    //Create a new hash-table that holds the skeleton datacubes
    state->skeletonState->skeletonDCs = Hashtable::ht_new(state->skeletonState->skeletonDCnumber);

    /*@todo
    if(state->skeletonState->skeletonDCs == HT_FAILURE) {
        LOG("Unable to create skeleton hash-table.")
        return false;
    }*/

    state->skeletonState->skeletonRevision = 0;

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
    state->skeletonState->autoFilenameIncrementBool = true;
    state->skeletonState->greatestNodeID = 0;

    state->skeletonState->showXYplane = false;
    state->skeletonState->showXZplane = false;
    state->skeletonState->showYZplane = false;
    state->skeletonState->showNodeIDs = false;
    state->skeletonState->highlightActiveTree = true;
    state->skeletonState->rotateAroundActiveNode = false;
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

    state->skeletonState->skeletonFile = (char*) malloc(8192 * sizeof(char));
    memset(state->skeletonState->skeletonFile, '\0', 8192 * sizeof(char));
    setDefaultSkelFileName();

    state->skeletonState->prevSkeletonFile = (char*) malloc(8192 * sizeof(char));
    memset(state->skeletonState->prevSkeletonFile, '\0', 8192 * sizeof(char));

    state->skeletonState->commentBuffer = (char*) malloc(10240 * sizeof(char));
    memset(state->skeletonState->commentBuffer, '\0', 10240 * sizeof(char));

    state->skeletonState->searchStrBuffer = (char*) malloc(2048 * sizeof(char));
    memset(state->skeletonState->searchStrBuffer, '\0', 2048 * sizeof(char));
    memset(state->skeletonState->skeletonLastSavedInVersion, '\0', sizeof(state->skeletonState->skeletonLastSavedInVersion));

    state->skeletonState->firstSerialSkeleton = (serialSkeletonListElement *)malloc(sizeof(state->skeletonState->firstSerialSkeleton));
    state->skeletonState->firstSerialSkeleton->next = NULL;
    state->skeletonState->firstSerialSkeleton->previous = NULL;
    state->skeletonState->lastSerialSkeleton = (serialSkeletonListElement *)malloc(sizeof(state->skeletonState->lastSerialSkeleton));
    state->skeletonState->lastSerialSkeleton->next = NULL;
    state->skeletonState->lastSerialSkeleton->previous = NULL;
    state->skeletonState->serialSkeletonCounter = 0;
    state->skeletonState->maxUndoSteps = 16;

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
        LOG("Out of memory while trying to allocate memory for a new nodeListElement.")
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
        LOG("Out of memory while trying to allocate memory for a new segmentListElement.")
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

bool Skeletonizer::addNodeToSkeletonStruct(nodeListElement *node) {
    skeletonDC *currentSkeletonDC;
    skeletonDCnode *currentNewSkeletonDCnode;
    Coordinate currentMagPos;

    currentMagPos.x = node->position.x / state->magnification;
    currentMagPos.y = node->position.y / state->magnification;
    currentMagPos.z = node->position.z / state->magnification;

    currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                    Coordinate::Px2DcCoord(currentMagPos));

    if(currentSkeletonDC == HT_FAILURE) {
        currentSkeletonDC = (skeletonDC*) malloc(sizeof(skeletonDC));
        memset(currentSkeletonDC, '\0', sizeof(skeletonDC));
        Hashtable::ht_put(state->skeletonState->skeletonDCs,
               Coordinate::Px2DcCoord(currentMagPos),
               (Byte *)currentSkeletonDC);
    }

    currentNewSkeletonDCnode = (skeletonDCnode*) malloc(sizeof (skeletonDCnode));
    memset(currentNewSkeletonDCnode, '\0', sizeof(skeletonDCnode));

    if(!currentSkeletonDC->firstSkeletonDCnode)
        currentSkeletonDC->firstSkeletonDCnode = currentNewSkeletonDCnode;
    else {
        currentNewSkeletonDCnode->next = currentSkeletonDC->firstSkeletonDCnode;
        currentSkeletonDC->firstSkeletonDCnode = currentNewSkeletonDCnode;
    }
    currentNewSkeletonDCnode->node = node;

    return true;
}

bool Skeletonizer::addSegmentToSkeletonStruct(segmentListElement *segment) {
    int i;
    skeletonDC *currentSkeletonDC;
    skeletonDCsegment *currentNewSkeletonDCsegment;
    float segVectorLength = 0.;
    floatCoordinate segVector;
    Coordinate currentSegVectorPoint, lastSegVectorPoint;
    Coordinate curMagTargetPos, curMagSourcePos;

    if(segment) {
        if(segment->flag == 2)
            return false;
    }
    else
        return false;

    curMagTargetPos.x = segment->target->position.x;
    curMagTargetPos.y = segment->target->position.y;
    curMagTargetPos.z = segment->target->position.z;
    curMagSourcePos.x = segment->source->position.x;
    curMagSourcePos.y = segment->source->position.y;
    curMagSourcePos.z = segment->source->position.z;

    segVector.x = (float)(curMagTargetPos.x - curMagSourcePos.x);
    segVector.y = (float)(curMagTargetPos.y - curMagSourcePos.y);
    segVector.z = (float)(curMagTargetPos.z - curMagSourcePos.z);

    segVectorLength = euclidicNorm(&segVector);
    normalizeVector(&segVector);

    SET_COORDINATE(lastSegVectorPoint,
                   curMagSourcePos.x,
                   curMagSourcePos.y,
                   curMagSourcePos.z);

    //Add the segment pointer to the first skeleton DC
    //Is there a skeleton DC for the first skeleton DC to which a pointer has to be added?
    currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                    Coordinate::Px2DcCoord(curMagSourcePos));
    if(currentSkeletonDC == HT_FAILURE) {
        //A skeleton DC is missing
        //LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.")
        return false;
    }
    currentNewSkeletonDCsegment = (skeletonDCsegment*)malloc(sizeof (skeletonDCsegment));
    memset(currentNewSkeletonDCsegment, '\0', sizeof(skeletonDCsegment));

    if(!currentSkeletonDC->firstSkeletonDCsegment)
        currentSkeletonDC->firstSkeletonDCsegment = currentNewSkeletonDCsegment;
    else {
        currentNewSkeletonDCsegment->next = currentSkeletonDC->firstSkeletonDCsegment;
        currentSkeletonDC->firstSkeletonDCsegment = currentNewSkeletonDCsegment;
    }
    currentNewSkeletonDCsegment->segment = segment;

    //We walk along the entire segment and add pointers to the segment in all skeleton DCs we are passing.
    for(i = 0; i <= ((int)segVectorLength); i++) {
        currentSegVectorPoint.x = curMagSourcePos.x + roundFloat(((float)i) * segVector.x);
        currentSegVectorPoint.y = curMagSourcePos.y + roundFloat(((float)i) * segVector.y);
        currentSegVectorPoint.z = curMagSourcePos.z + roundFloat(((float)i) * segVector.z);

        if(!COMPARE_COORDINATE(Coordinate::Px2DcCoord(lastSegVectorPoint), Coordinate::Px2DcCoord(currentSegVectorPoint))) {
            //We crossed a skeleton DC boundary, now we have to add a pointer to the segment inside the skeleton DC
            currentSkeletonDC = (struct skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs, Coordinate::Px2DcCoord(currentSegVectorPoint));
            if(currentSkeletonDC == HT_FAILURE) {
                currentSkeletonDC = (skeletonDC*)malloc(sizeof(skeletonDC));
                memset(currentSkeletonDC, '\0', sizeof(skeletonDC));
                Hashtable::ht_put(state->skeletonState->skeletonDCs, Coordinate::Px2DcCoord(currentSegVectorPoint), (Byte *)currentSkeletonDC);
            }
            currentNewSkeletonDCsegment = (skeletonDCsegment*)malloc(sizeof (skeletonDCsegment));
            memset(currentNewSkeletonDCsegment, '\0', sizeof(skeletonDCsegment));

            if(!currentSkeletonDC->firstSkeletonDCsegment)
                currentSkeletonDC->firstSkeletonDCsegment = currentNewSkeletonDCsegment;
            else {
                currentNewSkeletonDCsegment->next = currentSkeletonDC->firstSkeletonDCsegment;
                currentSkeletonDC->firstSkeletonDCsegment = currentNewSkeletonDCsegment;
            }
            currentNewSkeletonDCsegment->segment = segment;

            lastSegVectorPoint.x = currentSegVectorPoint.x;
            lastSegVectorPoint.y = currentSegVectorPoint.y;
            lastSegVectorPoint.z = currentSegVectorPoint.z;
        }
    }

    return true;
}

bool Skeletonizer::delNodeFromSkeletonStruct(nodeListElement *node) {
    skeletonDC *currentSkeletonDC;
    skeletonDCnode *currentSkeletonDCnode, *lastSkeletonDCnode = NULL;
    Coordinate curMagPos;

    curMagPos.x = node->position.x / state->magnification;
    curMagPos.y = node->position.y / state->magnification;
    curMagPos.z = node->position.z / state->magnification;

    currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                             Coordinate::Px2DcCoord(curMagPos));
    if(currentSkeletonDC == HT_FAILURE) {
        //A skeleton DC is missing
        LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.")
        return false;
    }

    currentSkeletonDCnode = currentSkeletonDC->firstSkeletonDCnode;

    while(currentSkeletonDCnode) {
        //We found the node we want to delete..
        if(currentSkeletonDCnode->node == node) {
            if(lastSkeletonDCnode != NULL)
                lastSkeletonDCnode->next = currentSkeletonDCnode->next;
            else
                currentSkeletonDC->firstSkeletonDCnode = currentSkeletonDCnode->next;

            free(currentSkeletonDCnode);

            break;
        }

        lastSkeletonDCnode = currentSkeletonDCnode;
        currentSkeletonDCnode = currentSkeletonDCnode->next;
    }

    return true;
}

bool Skeletonizer::delSegmentFromSkeletonStruct(segmentListElement *segment) {
    int i;
    skeletonDC *currentSkeletonDC;
    skeletonDCsegment *lastSkeletonDCsegment, *currentSkeletonDCsegment;
    float segVectorLength = 0.;
    floatCoordinate segVector;
    Coordinate currentSegVectorPoint, lastSegVectorPoint;
    Coordinate curMagTargetPos, curMagSourcePos;

    if(segment) {
        if(segment->flag == 2) {
            return false;
        }
    }
    else {
        return false;
    }
    curMagTargetPos.x = segment->target->position.x / state->magnification;
    curMagTargetPos.y = segment->target->position.y / state->magnification;
    curMagTargetPos.z = segment->target->position.z / state->magnification;
    curMagSourcePos.x = segment->source->position.x / state->magnification;
    curMagSourcePos.y = segment->source->position.y / state->magnification;
    curMagSourcePos.z = segment->source->position.z / state->magnification;

    segVector.x = (float)(curMagTargetPos.x - curMagSourcePos.x);
    segVector.y = (float)(curMagTargetPos.y - curMagSourcePos.y);
    segVector.z = (float)(curMagTargetPos.z - curMagSourcePos.z);

    segVectorLength = euclidicNorm(&segVector);
    normalizeVector(&segVector);

    SET_COORDINATE(lastSegVectorPoint,
                   curMagSourcePos.x,
                   curMagSourcePos.y,
                   curMagSourcePos.z);

    //Is there a skeleton DC for the first skeleton DC from which the segment has to be removed?
    currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                        Coordinate::Px2DcCoord(curMagSourcePos));
    if(currentSkeletonDC == HT_FAILURE) {
        //A skeleton DC is missing
        LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.")
        return false;
    }

    lastSkeletonDCsegment = NULL;
    currentSkeletonDCsegment = currentSkeletonDC->firstSkeletonDCsegment;
    while(currentSkeletonDCsegment) {
        //We found the segment we want to delete..
        if(currentSkeletonDCsegment->segment == segment) {
            //Our segment is the first segment
            if(lastSkeletonDCsegment == NULL) {
                currentSkeletonDC->firstSkeletonDCsegment = currentSkeletonDCsegment->next;
            }
            else {
                lastSkeletonDCsegment->next = currentSkeletonDCsegment->next;
            }
            free(currentSkeletonDCsegment);
            break;
        }

        lastSkeletonDCsegment = currentSkeletonDCsegment;
        currentSkeletonDCsegment = currentSkeletonDCsegment->next;
    }

    //We walk along the entire segment and delete pointers to the segment in all skeleton DCs we are passing.
    for(i = 0; i <= ((int)segVectorLength); i++) {
        currentSegVectorPoint.x = curMagSourcePos.x + roundFloat(((float)i) * segVector.x);
        currentSegVectorPoint.y = curMagSourcePos.y + roundFloat(((float)i) * segVector.y);
        currentSegVectorPoint.z = curMagSourcePos.z + roundFloat(((float)i) * segVector.z);

        if(!COMPARE_COORDINATE(Coordinate::Px2DcCoord(lastSegVectorPoint), Coordinate::Px2DcCoord(currentSegVectorPoint))) {
            //We crossed a skeleton DC boundary, now we have to delete the pointer to the segment inside the skeleton DC
            currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                     Coordinate::Px2DcCoord(currentSegVectorPoint));
            if(currentSkeletonDC == HT_FAILURE) {
                //A skeleton DC is missing
                LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.")
                return false;
            }

            lastSkeletonDCsegment = NULL;
            currentSkeletonDCsegment = currentSkeletonDC->firstSkeletonDCsegment;
            while(currentSkeletonDCsegment) {
                //We found the segment we want to delete..
                if(currentSkeletonDCsegment->segment == segment) {
                    //Our segment is the first segment
                    if(lastSkeletonDCsegment == NULL) {
                        currentSkeletonDC->firstSkeletonDCsegment = currentSkeletonDCsegment->next;
                    }
                    else {
                        lastSkeletonDCsegment->next = currentSkeletonDCsegment->next;
                    }
                    free(currentSkeletonDCsegment);
                    break;
                }

                lastSkeletonDCsegment = currentSkeletonDCsegment;
                currentSkeletonDCsegment = currentSkeletonDCsegment->next;
            }

            lastSegVectorPoint.x = currentSegVectorPoint.x;
            lastSegVectorPoint.y = currentSegVectorPoint.y;
            lastSegVectorPoint.z = currentSegVectorPoint.z;
        }
    }

    return true;
}

/** @todo cleanup */
void Skeletonizer::WRAP_popBranchNode() {
    /*
    Skeletonizer::popBranchNode(CHANGE_MANUAL);
    state->skeletonState->askingPopBranchConfirmation = false;
    */
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

    if(!state->skeletonState->activeTree)
        addTreeListElement(true, CHANGE_MANUAL, 0, treeCol, false);

    addedNodeID = addNode(CHANGE_MANUAL,
                          0,
                          state->skeletonState->defaultNodeRadius,
                          state->skeletonState->activeTree->treeID,
                          clickedCoordinate,
                          VPtype,
                          state->magnification,
                          -1,
                          true, true);
    if(!addedNodeID) {
        LOG("Error: Could not add new node!")
        return false;
    }

    setActiveNode(CHANGE_MANUAL, NULL, addedNodeID);

    if((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                               state->skeletonState->activeTree->treeID) == 1) {
        /* First node in this tree */

        pushBranchNode(CHANGE_MANUAL, true, true, NULL, addedNodeID, false);
        addComment(CHANGE_MANUAL, "First Node", NULL, addedNodeID, false);
    }

    emit idleTimeSignal();

    return true;
}

uint Skeletonizer::addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int makeNodeActive) {
    int targetNodeID;

    if(!state->skeletonState->activeNode) {
        LOG("Please create a node before trying to link nodes.")
        state->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
        return false;
    }

    //Add a new node at the target position first.
    targetNodeID = addNode(CHANGE_MANUAL,
                           0,
                           state->skeletonState->defaultNodeRadius,
                           state->skeletonState->activeTree->treeID,
                           clickedCoordinate,
                           VPtype,
                           state->magnification,
                           -1,
                           true, true);
    if(!targetNodeID) {
        LOG("Could not add new node while trying to add node and link with active node!")
        return false;
    }

    addSegment(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, targetNodeID, false);

    if(makeNodeActive) {
        setActiveNode(CHANGE_MANUAL, NULL, targetNodeID);
    }
    if((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                               state->skeletonState->activeTree->treeID) == 1) {
        /* First node in this tree */

        pushBranchNode(CHANGE_MANUAL, true, true, NULL, targetNodeID, false);
        addComment(CHANGE_MANUAL, "First Node", NULL, targetNodeID, false);
        /* @todo checkIdleTime */
    }

    return targetNodeID;
}

bool Skeletonizer::updateSkeletonState() {


    if(state->skeletonState->autoSaveBool || state->clientState->saveMaster) {
        if(state->skeletonState->autoSaveInterval) {
            if((state->time.elapsed() - state->skeletonState->lastSaveTicks) / 60000.0 >= state->skeletonState->autoSaveInterval) {
                state->skeletonState->lastSaveTicks = state->time.elapsed();

                 emit saveSkeletonSignal(true);
            }
        }
    }


    setSkeletonWorkMode(CHANGE_MANUAL, state->skeletonState->workMode);
    return true;
}

bool Skeletonizer::nextCommentlessNode() { return true; }

bool Skeletonizer::previousCommentlessNode() {
    struct nodeListElement *currNode;
    currNode = state->skeletonState->activeNode->previous;
    if(currNode && (!currNode->comment)) {
        LOG("yes")
        setActiveNode(CHANGE_MANUAL,
                          currNode,
                          0);
        jumpToActiveNode();
    }
    return true;
}

/** @deprecated use updateFileName in MainWindow */
bool Skeletonizer::updateSkeletonFileName(int targetRevision, int increment, char *filename) {
    int extensionDelim = -1, countDelim = -1;
    char betweenDots[8192];
    char count[8192];
    char origFilename[8192], skeletonFileBase[8192];
    int i, j;

    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    memset(skeletonFileBase, '\0', 8192);
    memset(origFilename, '\0', 8192);
    strncpy(origFilename, filename, 8192 - 1);

    if(increment) {
        for(i = 8192 - 1; i >= 0; i--) {
            if(filename[i] == '.') {
                extensionDelim = i;
                break;
            }
        }

        for(i--; i >= 0; i--) {
            if(filename[i] == '.') {
                //check if string between dots represents a number
                strncpy(betweenDots, &filename[i+1], extensionDelim - i-1);
                for(j = 0; j < extensionDelim - i - 1; j++) {
                    if(atoi(&betweenDots[j]) == 0 && betweenDots[j] != '0') {
                        goto noCounter;
                    }
                }
                //string between dots is a number
                countDelim = i;
                break;
            }
        }

        noCounter:
        if(countDelim > -1) {
            strncpy(skeletonFileBase,
                    filename,
                    countDelim);
        }
        else if(extensionDelim > -1){
            strncpy(skeletonFileBase,
                    filename,
                    extensionDelim);
        }
        else {
            strncpy(skeletonFileBase,
                    filename,
                    8192 - 1);
        }

        if(countDelim && extensionDelim) {
            strncpy(count, &filename[countDelim + 1], extensionDelim - countDelim);
            state->skeletonState->saveCnt = atoi(count);
            state->skeletonState->saveCnt++;
        }
        else {
            state->skeletonState->saveCnt = 0;
        }

        sprintf(state->skeletonState->skeletonFile, "%s.%.3d.nml\0",
                skeletonFileBase,
                state->skeletonState->saveCnt);
    }

    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("blrds", KIKI_SKELETONFILENAME, increment, origFilename))
            Client::skeletonSyncBroken();
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::saveXmlSkeleton(QString fileName) {
    treeListElement *currentTree = NULL;
    nodeListElement *currentNode = NULL;
    PTRSIZEINT currentBranchPointID;
    segmentListElement *currentSegment = NULL;
    commentListElement *currentComment = NULL;
    stack *reverseBranchStack = NULL, *tempReverseStack = NULL;
    int r;
    int time;

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
       pushBranchNode(CHANGE_MANUAL, false, false, currentNode, 0, false);
   }

    /* */
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)) {        
        qErrnoWarning("Failed to open file");
        return false;
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

    xml.writeStartElement("time");
    xml.writeAttribute("ms", tmp.setNum(xorInt(state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + state->time.elapsed())));
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
    xml.writeEndElement();

    xml.writeStartElement("vpSettingsZoom");
    xml.writeAttribute("XYPlane", tmp.setNum(state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel));
    xml.writeAttribute("XZPlane", tmp.setNum(state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel));
    xml.writeAttribute("YZPlane", tmp.setNum(state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel));
    xml.writeAttribute("SkelPlane", tmp.setNum(state->viewerState->vpConfigs[VIEWPORT_SKELETON].texture.zoomLevel));
    xml.writeEndElement();

    xml.writeEndElement(); // end parameters

    currentTree = state->skeletonState->firstTree;
    if((currentTree == NULL) && (state->skeletonState->currentComment == NULL)) {

        file.close();
        return false; // No Skeleton to save
    }

    while(currentTree) {
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

        qDebug() << currentTree->comment;

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
    while(currentBranchPointID = (PTRSIZEINT)popStack(reverseBranchStack)) {
        xml.writeStartElement("branchpoint");
        xml.writeAttribute("id", tmp.setNum(currentBranchPointID));
        xml.writeEndElement();
    }
    xml.writeEndElement(); // end branchpoints

    xml.writeEndElement(); // end things
    xml.writeEndDocument();
    file.close();

    return true;
}

bool Skeletonizer::loadXmlSkeleton(QString fileName) {
    int neuronID = 0, nodeID = 0, merge = false;
    int nodeID1, nodeID2, greatestNodeIDbeforeLoading = 0, greatestTreeIDbeforeLoading = 0;
    float radius;
    Byte VPtype;
    int inMag, magnification = 0;
    int globalMagnificationSpecified = false;

    struct treeListElement *currentTree;
    struct nodeListElement *currentNode = NULL;
    Coordinate *currentCoordinate, loadedPosition;
    Coordinate offset;
    floatCoordinate scale;
    int time, activeNodeID = 0;
    int skeletonTime = 0;
    color4F neuronColor;

    SET_COORDINATE(offset, state->offset.x, state->offset.y, state->offset.z)
    SET_COORDINATE(scale, state->scale.x, state->scale.y, state->scale.z);
    SET_COORDINATE(loadedPosition, 0, 0, 0);

    currentCoordinate = (Coordinate*) malloc(sizeof(Coordinate));
    if(currentCoordinate == NULL) {
        LOG("Out of memory.");
        return false;
    }
    memset(currentCoordinate, '\0', sizeof(currentCoordinate));

    qDebug() << fileName;

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qErrnoWarning("Document not parsed successfully.");
        return false;
    }

    int lines = 0;
    QTextStream stream(&file);
    while(!stream.atEnd()) {
        lines += 1;
        stream.readLine();
    }

    QProgressDialog progress(QString("Parsing %1").arg(fileName), 0, 0, lines);
    progress.setWindowTitle("Loading Skeleton File");
    progress.setWindowModality(Qt::WindowModal);
    QApplication::processEvents();

    qDebug() << state->skeletonState->skeletonFile;

    if(!state->skeletonState->mergeOnLoadFlag) {
        merge = false;
        clearSkeleton(CHANGE_MANUAL, true);
        qDebug() << state->skeletonState->skeletonFile;
    } else {
        merge = true;
        greatestNodeIDbeforeLoading = state->skeletonState->greatestNodeID;
        greatestTreeIDbeforeLoading = state->skeletonState->greatestTreeID;
    }

    QTime bench;
    int counter = 0;

    file.reset();
    QXmlStreamReader xml(&file);
    bench.start();
    while(!xml.atEnd() and !xml.hasError()) {
        if(xml.lineNumber() % 10 == 0)
            progress.setValue(xml.lineNumber());

        xml.readNextStartElement();
        if(xml.isStartElement()) {

            if(xml.name() == "things") {
                continue;
            }

            if(xml.name() == "parameters") {
                while(xml.readNextStartElement()) {
                    QXmlStreamAttributes attributes = xml.attributes();
                    QString attribute;

                    if(xml.name() == "experiment") {
                        attribute = attributes.value("name").toString();
                        if(!attribute.isNull()) {
                            strcpy(state->skeletonState->skeletonCreatedInVersion, attribute.toStdString().c_str());
                        } else {
                            strcpy(state->skeletonState->skeletonCreatedInVersion, "Pre-3.2");
                        }
                    } else if(xml.name() == "createdin") {
                        attribute  = attributes.value("version").toString();
                        if(!attribute.isNull()) {
                            strcpy(state->skeletonState->skeletonCreatedInVersion, attribute.toStdString().c_str());
                        } else {
                            strcpy(state->skeletonState->skeletonCreatedInVersion, "Pre-3.2");
                        }
                    } else if(xml.name() == "magnification" and xml.isStartElement()) {
                        attribute  = attributes.value("factor").toString();
                        if(!attribute.isNull()) {
                            magnification = attribute.toInt();
                            globalMagnificationSpecified = true;
                        } else {
                            magnification = 0;
                        }

                    } else if(xml.name() == "offset") {
                        attribute = attributes.value("x").toString();
                        if(!attribute.isNull())
                            offset.x = attribute.toInt();

                        attribute = attributes.value("y").toString();
                        if(!attribute.isNull())
                            offset.y = attribute.toInt();

                        attribute = attributes.value("z").toString();
                        if(!attribute.isNull())
                            offset.z = attribute.toInt();
                    } else if(xml.name() == "time") {
                        attribute = attributes.value("ms").toString();
                        if(!attribute.isNull()) {
                            if(hasObfuscatedTime()) {
                                skeletonTime = xorInt(attribute.toInt());
                            } else {
                                skeletonTime = attribute.toInt();
                            }
                        }
                    } else if(xml.name() == "activeNode") {
                        if(!merge) {
                            attribute = attributes.value("id").toString();
                            if(!attribute.isNull()) {
                                activeNodeID = attribute.toInt();
                            }
                        }
                    } else if(xml.name() == "scale") {
                        attribute = attributes.value("x").toString();
                        if(!attribute.isNull()) {
                            scale.x = attribute.toFloat();
                        }

                        attribute = attributes.value("y").toString();
                        if(!attribute.isNull()) {
                            scale.y = attribute.toFloat();
                        }

                        attribute = attributes.value("z").toString();
                        if(!attribute.isNull()) {
                            scale.z = attribute.toFloat();
                        }
                    } else if(xml.name() == "editPosition") {
                        attribute = attributes.value("x").toString();
                        if(!attribute.isNull())
                            loadedPosition.x = attribute.toInt();

                        attribute = attributes.value("y").toString();
                        if(!attribute.isNull())
                            loadedPosition.x = attribute.toInt();

                        attribute = attributes.value("z").toString();
                        if(!attribute.isNull())
                            loadedPosition.x = attribute.toInt();

                    } else if(xml.name() == "skeletonVPState") {
                        int j = 0;
                        char element [8];
                        for (j = 0; j < 16; j++){
                            sprintf (element, "E%d", j);
                            attribute = attributes.value(element).toString();
                            state->skeletonState->skeletonVpModelView[j] = attribute.toFloat();
                        }
                        glMatrixMode(GL_MODELVIEW);
                        glLoadMatrixf(state->skeletonState->skeletonVpModelView);

                        attribute = attributes.value("translateX").toString();
                        if(!attribute.isNull()) {
                            state->skeletonState->translateX = attribute.toFloat();
                        }

                        if(attribute.isNull()) {
                            state->skeletonState->translateY = attribute.toFloat();
                        }
                    } else if(xml.name() == "vpSettingsZoom") {
                        attribute = attributes.value("XYPlane").toString();
                        if(!attribute.isNull()) {
                            state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel = attribute.toFloat();
                        }
                        attribute = attributes.value("XZPlane").toString();
                        if(!attribute.isNull()) {
                            state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel = attribute.toFloat();
                        }
                        attribute = attributes.value("YZPlane").toString();
                        if(!attribute.isNull()) {
                            state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel = attribute.toFloat();
                        }
                        attribute = attributes.value("SkelVP").toString();
                        if(!attribute.isNull())
                            state->skeletonState->zoomLevel = attribute.toFloat();
                    } else if(xml.name() == "idleTime") {
                        attribute = attributes.value("ms").toString();
                        if(!attribute.isNull()) {
                            if(hasObfuscatedTime()) {
                                state->skeletonState->idleTime = xorInt(attribute.toInt());
                            } else {
                                state->skeletonState->idleTime = attribute.toInt();
                            }
                        }
                        state->skeletonState->idleTimeTicksOffset = state->time.elapsed();
                    }

                    xml.skipCurrentElement();
                }

            } else if(xml.name() == "branchpoints") {
                xml.readNextStartElement();

                while(!(xml.tokenType() == QXmlStreamReader::EndElement and xml.name() == "branchpoints")) {

                    if(xml.name() == "branchpoint" and xml.isStartElement()) {
                        QXmlStreamAttributes attributes = xml.attributes();
                        QString attribute;

                        attribute = attributes.value("id").toString();
                        if(!attribute.isNull()) {
                            if(!merge) {
                                nodeID = attribute.toInt();
                            } else {
                                nodeID = attribute.toInt() + greatestNodeIDbeforeLoading;
                            }
                            currentNode = findNodeByNodeID(nodeID);
                            if(currentNode)
                                pushBranchNode(CHANGE_MANUAL, true, false, currentNode, 0, false);
                        }
                    }

                    while(xml.readNext() == QXmlStreamReader::Characters) {

                    }
                }
            }

            else if(xml.name() == "comments") {
                xml.readNextStartElement();

                while(!(xml.tokenType() == QXmlStreamReader::EndElement and xml.name() == "comments")) {

                    QXmlStreamAttributes attributes = xml.attributes();
                    QString attribute;

                    if(xml.name() == "comment" and xml.isStartElement()) {
                        attribute = attributes.value("node").toString();
                        if(!attribute.isNull()) {
                            if(!merge)
                                nodeID = attribute.toInt();
                            else
                                nodeID = attribute.toInt() + greatestNodeIDbeforeLoading;

                            currentNode = findNodeByNodeID(nodeID);
                        }

                        attribute = attributes.value("content").toString();
                        if(!attribute.isNull() && currentNode) {
                            addComment(CHANGE_MANUAL, attribute.toStdString().c_str(), currentNode, 0, false);
                        }
                    }

                    while(xml.readNext() == QXmlStreamReader::Characters) {

                    }
                }

            } else if(xml.name() == "thing") {
                QXmlStreamAttributes thingAttributes = xml.attributes();
                QStringRef thingAttribute;

                thingAttribute = thingAttributes.value("id");
                if(!thingAttribute.isNull()) {
                    neuronID = thingAttribute.toString().toInt();
                } else {
                    neuronID = 0;
                }

                thingAttribute = thingAttributes.value("color.r");
                if(!thingAttribute.isNull()) {
                    neuronColor.r = thingAttribute.toString().toFloat();
                } else {
                    neuronColor.r = -1;
                }

                thingAttribute = thingAttributes.value("color.g");
                if(!thingAttribute.isNull()) {
                    neuronColor.g = thingAttribute.toString().toFloat();
                } else {
                    neuronColor.g = -1;
                }

                thingAttribute = thingAttributes.value("color.b");
                if(!thingAttribute.isNull()) {
                    neuronColor.b = thingAttribute.toString().toFloat();
                } else {
                    neuronColor.b = -1;
                }

                thingAttribute = thingAttributes.value("color.a");
                if(!thingAttribute.isNull()) {
                    neuronColor.a = thingAttribute.toString().toFloat();
                } else {
                    neuronColor.a = -1;
                }

                if(!merge) {
                    currentTree = addTreeListElement(true, CHANGE_MANUAL, neuronID, neuronColor, false);
                    setActiveTreeByID(neuronID);
                } else {
                    neuronID += greatestTreeIDbeforeLoading;
                    currentTree = addTreeListElement(true, CHANGE_MANUAL, neuronID, neuronColor, false);
                    setActiveTreeByID(currentTree->treeID);
                    neuronID = currentTree->treeID;
                }

                thingAttribute = thingAttributes.value("comment");
                if(!thingAttribute.isNull()) {
                    addTreeComment(CHANGE_MANUAL, currentTree->treeID, const_cast<char *>(thingAttribute.toString().toStdString().c_str()));
                }
            } else if(xml.name() == "nodes") {
                QXmlStreamAttributes nodeAttributes;
                QStringRef nodeAttribute;

                while(xml.readNextStartElement()) {
                    if(xml.name() == "node") {
                        nodeAttributes = xml.attributes();

                        nodeAttribute = nodeAttributes.value("id");
                        if(!nodeAttribute.isNull()) {
                            nodeID = nodeAttribute.toString().toInt();
                        } else {
                            nodeID = 0;
                        }

                        nodeAttribute = nodeAttributes.value("radius");
                        if(!nodeAttribute.isNull()) {
                            radius = nodeAttribute.toString().toFloat();
                        } else {
                            radius = state->skeletonState->defaultNodeRadius;
                        }

                        nodeAttribute = nodeAttributes.value("x");
                        if(!nodeAttribute.isNull()) {
                            currentCoordinate->x = nodeAttribute.toString().toInt() - 1;
                            if(globalMagnificationSpecified) {
                                currentCoordinate->x = currentCoordinate->x * magnification;
                            }
                        } else {
                            currentCoordinate->x = 0;
                        }

                        nodeAttribute = nodeAttributes.value("y");
                        if(!nodeAttribute.isNull()) {
                            currentCoordinate->y = nodeAttribute.toString().toInt() - 1;
                            if(globalMagnificationSpecified) {
                                currentCoordinate->y = currentCoordinate->y * magnification;
                            }
                        } else {
                            currentCoordinate->y = 0;
                        }

                        nodeAttribute = nodeAttributes.value("z");
                        if(!nodeAttribute.isNull()) {
                            currentCoordinate->z = nodeAttribute.toString().toInt() - 1;
                            if(globalMagnificationSpecified) {
                                currentCoordinate->z = currentCoordinate->z * magnification;
                            }
                        } else {
                            currentCoordinate->z = 0;
                        }

                        nodeAttribute = nodeAttributes.value("inVp");
                        if(!nodeAttribute.isNull()) {
                            VPtype = nodeAttribute.toString().toInt();
                        } else {
                            VPtype = VIEWPORT_UNDEFINED;
                        }

                        nodeAttribute = nodeAttributes.value("inMag");
                        if(!nodeAttribute.isNull()) {
                            inMag = nodeAttribute.toString().toInt();
                        } else
                            inMag = magnification; // For legacy skeleton files

                        nodeAttribute = nodeAttributes.value("time");
                        if(!nodeAttribute.isNull()) {
                            time = nodeAttribute.toString().toInt();
                        } else
                            time = skeletonTime; // For legacy skeleton files

                        if(!merge)
                            addNode(CHANGE_MANUAL, nodeID, radius, neuronID, currentCoordinate, VPtype, inMag, time, false, false);
                        else {
                            nodeID += greatestNodeIDbeforeLoading;
                            addNode(CHANGE_MANUAL, nodeID, radius, neuronID, currentCoordinate, VPtype, inMag, time, false, false);
                        }
                    }

                    xml.skipCurrentElement();

                } // end while nodes

            } else if(xml.name() == "edges") {

                QXmlStreamAttributes edgeAttributes;
                QStringRef edgeAttribute;

                while(xml.readNextStartElement()) {
                    if(xml.name() == "edge") {
                        edgeAttributes = xml.attributes();

                        // Add edge
                        edgeAttribute = edgeAttributes.value("source");
                        if(!edgeAttribute.isNull())
                            nodeID1 = edgeAttribute.toString().toInt();
                        else
                            nodeID1 = 0;

                        edgeAttribute = edgeAttributes.value("target");
                        if(!edgeAttribute.isNull())
                            nodeID2 = edgeAttribute.toString().toInt();
                         else
                            nodeID2 = 0;


                        if(!merge)
                            addSegment(CHANGE_MANUAL, nodeID1, nodeID2, false);
                        else
                            addSegment(CHANGE_MANUAL, nodeID1 + greatestNodeIDbeforeLoading, nodeID2 + greatestNodeIDbeforeLoading, false);

                    }
                    xml.skipCurrentElement();
                }

            }

        } // end start Element
    } // end while

    if(xml.hasError()) {
        qDebug() << xml.errorString() << " at" << xml.lineNumber();
    }

    qDebug() << bench.elapsed();

    file.close();

    if(activeNodeID) {
        setActiveNode(CHANGE_MANUAL, NULL, activeNodeID);
    }


    /* @todo
    if((loadedPosition.x != 0) &&
       (loadedPosition.y != 0) &&
       (loadedPosition.z != 0)) {
        state->viewerState->currentPosition.x =
            loadedPosition.x - 1;
        state->viewerState->currentPosition.y =
            loadedPosition.y - 1;
        state->viewerState->currentPosition.z =
            loadedPosition.z - 1;
         @todo change to Signal loadSkeleton has to be non-static
        emit updatePositionSignal(TELL_COORDINATE_CHANGE);

} */

    state->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
    state->skeletonState->skeletonTime = skeletonTime;
    state->skeletonState->skeletonTimeCorrection = state->time.elapsed();

    return true;

}


void Skeletonizer::setDefaultSkelFileName() {
    // Generate a default file name based on date and time.
    time_t curtime;
    struct tm *localtimestruct;

    curtime = time(NULL);
    localtimestruct = localtime(&curtime);
    if(localtimestruct->tm_year >= 100)
        localtimestruct->tm_year -= 100;

    state->skeletonState->skeletonFileAsQString = QString("/Users/amos/skeleton-default.000.nml");


    // MainWindow::cpBaseDirectory(state->viewerState->gui->skeletonDirectory, state->skeletonState->skeletonFile, 2048);
}

bool Skeletonizer::delActiveNode() {
    if(state->skeletonState->activeNode) {
        delNode(CHANGE_MANUAL, 0, state->skeletonState->activeNode, true);
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

        delTree(CHANGE_MANUAL, state->skeletonState->activeTree->treeID, true);

        if(nextTree) {
            setActiveTreeByID(nextTree->treeID);
        }
        else {
            state->skeletonState->activeTree = NULL;
        }
    }
    else {
       LOG("No active tree available.")
       return false;
    }

    state->skeletonState->workMode = SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE;

    return true;
}

bool Skeletonizer::delSegment(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel, int serialize) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    // Delete the segment out of the segment list and out of the visualization structure!


    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(!segToDel)
        segToDel = findSegmentByNodeIDs(sourceNodeID, targetNodeID);
    else {
        sourceNodeID = segToDel->source->nodeID;
        targetNodeID = segToDel->target->nodeID;
    }

    if(serialize) {
        saveSerializedSkeleton();
    }

    if(!segToDel) {
        LOG("Cannot delete segment, no segment with corresponding node IDs available!")
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(!segToDel) {
        segToDel = findSegmentByNodeIDs(sourceNodeID, targetNodeID);
    } else {
        sourceNodeID = segToDel->source->nodeID;
        targetNodeID = segToDel->target->nodeID;
    }

    if(!segToDel) {
        LOG("Cannot delete segment, no segment with corresponding node IDs available!")
        Knossos::unlockSkeleton(false);
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
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brdd", KIKI_DELSEGMENT, sourceNodeID, targetNodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    return true;
}

/*
 * We have to delete the node from 2 structures: the skeleton's nested linked list structure
 * and the skeleton visualization structure (hashtable with skeletonDCs).
 */
bool Skeletonizer::delNode(int targetRevision, int nodeID, nodeListElement *nodeToDel, int serialize) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    struct segmentListElement *currentSegment;
    struct segmentListElement *tempNext = NULL;
    struct nodeListElement *newActiveNode = NULL;
    int treeID;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(!nodeToDel)
        nodeToDel = findNodeByNodeID(nodeID);

    nodeID = nodeToDel->nodeID;

    if(!nodeToDel) {
        LOG("The given node %d doesn't exist. Unable to delete it.", nodeID)
        Knossos::unlockSkeleton(false);
        return false;
    }

    treeID = nodeToDel->correspondingTree->treeID;

    if(serialize){
        saveSerializedSkeleton();
    }

    if(nodeToDel->comment) {
        delComment(CHANGE_MANUAL, nodeToDel->comment, 0, false);
    }

     // First, delete all segments pointing towards and away of the nodeToDelhas
     // been

    currentSegment = nodeToDel->firstSegment;

    while(currentSegment) {

        tempNext = currentSegment->next;

        if(currentSegment->flag == SEGMENT_FORWARD)
            delSegment(CHANGE_MANUAL, 0,0, currentSegment, false);
        else if(currentSegment->flag == SEGMENT_BACKWARD)
            delSegment(CHANGE_MANUAL, 0,0, currentSegment->reverseSegment, false);

        currentSegment = tempNext;
    }

    nodeToDel->firstSegment = NULL;


     // Delete the node out of the visualization structure


    delNodeFromSkeletonStruct(nodeToDel);

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

        setActiveNode(CHANGE_NOSYNC, newActiveNode, 0);
    }

    free(nodeToDel);

    state->skeletonState->totalNodeElements--;
    //state->viewerState->gui->totalNodes--;

    setDynArray(state->skeletonState->nodeCounter,
            treeID,
            (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter, treeID) - 1));

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_DELNODE, nodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }

    Knossos::unlockSkeleton(true);
    return true;
}

bool Skeletonizer::delTree(int targetRevision, int treeID, int serialize) {

    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    struct treeListElement *currentTree;
    struct nodeListElement *currentNode, *nodeToDel;


    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }


    currentTree = findTreeByTreeID(treeID);
    if(!currentTree) {
        LOG("There exists no tree with ID %d. Unable to delete it.", treeID)
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(serialize) {
        saveSerializedSkeleton();
    }

    currentNode = currentTree->firstNode;
    while(currentNode) {
        nodeToDel = currentNode;
        currentNode = nodeToDel->next;
        delNode(CHANGE_MANUAL, 0, nodeToDel, false);
    }
    currentTree->firstNode = NULL;

    if(currentTree == state->skeletonState->firstTree)
        state->skeletonState->firstTree = currentTree->next;
    else {
        if(currentTree->previous)
            currentTree->previous->next = currentTree->next;
        if(currentTree->next)
            currentTree->next->previous = currentTree->previous;
    }

    free(currentTree);

    state->skeletonState->treeElements--;

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;


    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_DELTREE, treeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

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
        LOG("There exists no tree with ID %d!", treeID)
        return false;
    }


    state->skeletonState->activeTree = currentTree;
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;

    state->viewerState->gui->activeTreeID = currentTree->treeID;

    qDebug() << " new active tree id is " << state->skeletonState->activeTree->treeID;

    return true;
}

bool Skeletonizer::setActiveNode(int targetRevision, nodeListElement *node, int nodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

     // If both *node and nodeID are specified, nodeID wins.
     // If neither *node nor nodeID are specified
     // (node == NULL and nodeID == 0), the active node is
     // set to NULL.

    if(targetRevision != CHANGE_NOSYNC) {
        if(Knossos::lockSkeleton(targetRevision) == false) {
            Knossos::unlockSkeleton(false);
            return false;
        }
    }

    if(nodeID != 0) {
        node = findNodeByNodeID(nodeID);
        if(!node) {
            LOG("No node with id %d available.", nodeID)
            Knossos::unlockSkeleton(false);
            return false;
        }
    }

    if(node) {
        nodeID = node->nodeID;
    }


    state->skeletonState->activeNode = node;
    state->skeletonState->viewChanged = true;
    state->skeletonState->skeletonChanged = true;

    if(node) {
        setActiveTreeByID(node->correspondingTree->treeID);
    }

    else
        state->skeletonState->activeTree = NULL;

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

    if(targetRevision != CHANGE_NOSYNC) {
        state->skeletonState->skeletonRevision++;

        if(targetRevision == CHANGE_MANUAL) {
            if(!Client::syncMessage("brd", KIKI_SETACTIVENODE, nodeID)) {
                Client::skeletonSyncBroken();
            }
        }
        else {

        }
        Knossos::unlockSkeleton(true);
    }

    if(node) {
        state->skeletonState->activeNode= node;
        qDebug() << node->nodeID;
    }

    /* */

    return true;
}

int Skeletonizer::addNode(int targetRevision,
                int nodeID,
                float radius,
                int treeID,
                Coordinate *position,
                Byte VPtype,
                int inMag,
                int time,
                int respectLocks,
                int serialize) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    nodeListElement *tempNode = NULL;
    treeListElement *tempTree = NULL;
    floatCoordinate lockVector;
    int lockDistance = 0;


    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    state->skeletonState->branchpointUnresolved = false;

     // respectLocks refers to locking the position to a specific coordinate such as to
     // disallow tracing areas further than a certain distance away from a specific point in the
     // dataset.


    if(respectLocks) {
        if(state->skeletonState->positionLocked) {
            lockVector.x = (float)position->x - (float)state->skeletonState->lockedPosition.x;
            lockVector.y = (float)position->y - (float)state->skeletonState->lockedPosition.y;
            lockVector.z = (float)position->z - (float)state->skeletonState->lockedPosition.z;

            lockDistance = euclidicNorm(&lockVector);
            if(lockDistance > state->skeletonState->lockRadius) {
                LOG("Node is too far away from lock point (%d), not adding.", lockDistance)
                Knossos::unlockSkeleton(false);
                return false;
            }
        }
    }

    if(nodeID) {
        tempNode = findNodeByNodeID(nodeID);
    }

    if(tempNode) {
        LOG("Node with ID %d already exists, no node added.", nodeID)
        Knossos::unlockSkeleton(false);
        return false;
    }
    tempTree = findTreeByTreeID(treeID);

    if(!tempTree) {
        LOG("There exists no tree with the provided ID %d!", treeID)
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(serialize) {
        saveSerializedSkeleton();
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
    state->skeletonState->skeletonRevision++;
    state->skeletonState->unsavedChanges = true;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brddfddddddd", KIKI_ADDNODE,
                                              state->clientState->myId,
                                              nodeID,
                                              radius,
                                              treeID,
                                              VPtype,
                                              inMag,
                                              -1,
                                              position->x,
                                              position->y,
                                              position->z))
            Client::skeletonSyncBroken();
    }

    else {

    }

    Knossos::unlockSkeleton(true);

    return nodeID;
}

bool Skeletonizer::addSegment(int targetRevision, int sourceNodeID, int targetNodeID, int serialize) {
    nodeListElement *targetNode, *sourceNode;
    segmentListElement *sourceSeg;
    floatCoordinate node1, node2;
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(findSegmentByNodeIDs(sourceNodeID, targetNodeID)) {
        LOG("Segment between nodes %d and %d exists already.", sourceNodeID, targetNodeID)
        Knossos::unlockSkeleton(false);
        return false;
    }

    //Check if source and target nodes are existant
    sourceNode = findNodeByNodeID(sourceNodeID);
    targetNode = findNodeByNodeID(targetNodeID);

    if(!(sourceNode) || !(targetNode)) {
        LOG("Could not link the nodes, because at least one is missing!")
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(sourceNode == targetNode) {
        LOG("Cannot link node with itself!")
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(serialize){
        saveSerializedSkeleton();
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
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brdd", KIKI_ADDSEGMENT, sourceNodeID, targetNodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::clearSkeleton(int targetRevision, int loadingSkeleton) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    treeListElement *currentTree, *treeToDel;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    currentTree = state->skeletonState->firstTree;
    while(currentTree) {
        treeToDel = currentTree;
        currentTree = treeToDel->next;
        delTree(CHANGE_MANUAL, treeToDel->treeID, true);
    }

    state->skeletonState->activeNode = NULL;
    state->skeletonState->activeTree = NULL;

    state->skeletonState->skeletonTime = 0;
    //state->skeletonState->skeletonTimeCorrection = SDL_GetTicks(); SDL TODO

    Hashtable::ht_rmtable(state->skeletonState->skeletonDCs);
    delDynArray(state->skeletonState->nodeCounter);
    delDynArray(state->skeletonState->nodesByNodeID);
    delStack(state->skeletonState->branchStack);

    //Create a new hash-table that holds the skeleton datacubes
    state->skeletonState->skeletonDCs = Hashtable::ht_new(state->skeletonState->skeletonDCnumber);
    if(state->skeletonState->skeletonDCs == HT_FAILURE) {
        LOG("Unable to create skeleton hash-table.")
        Knossos::unlockSkeleton(false);
        return false;
    }

    //Generate empty tree structures
    state->skeletonState->firstTree = NULL;
    state->skeletonState->totalNodeElements = 0;
    state->skeletonState->totalSegmentElements = 0;
    state->skeletonState->treeElements = 0;
    state->skeletonState->activeTree = NULL;
    state->skeletonState->activeNode = NULL;

    if(loadingSkeleton == false) {
        setDefaultSkelFileName();
    }

    state->skeletonState->nodeCounter = newDynArray(1048576);
    state->skeletonState->nodesByNodeID = newDynArray(1048576);
    state->skeletonState->branchStack = newStack(1048576);

    state->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;

    state->skeletonState->skeletonRevision++;
    state->skeletonState->unsavedChanges = true;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("br", KIKI_CLEARSKELETON)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    state->skeletonState->skeletonRevision = 0;

    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::mergeTrees(int targetRevision, int treeID1, int treeID2, int serialize) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    treeListElement *tree1, *tree2;
    nodeListElement *currentNode;
    nodeListElement *firstNode, *lastNode;

    if(treeID1 == treeID2) {
        LOG("Could not merge trees. Provided IDs are the same!")
        return false;
    }
    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    tree1 = findTreeByTreeID(treeID1);
    tree2 = findTreeByTreeID(treeID2);

    if(!(tree1) || !(tree2)) {
        LOG("Could not merge trees, provided IDs are not valid!")
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(serialize) {
        saveSerializedSkeleton();
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
    free(tree2);

    if(state->skeletonState->activeTree->treeID == tree2->treeID) {
       setActiveTreeByID(tree1->treeID);
       state->viewerState->gui->activeTreeID = tree1->treeID;
    }

    state->skeletonState->treeElements--;
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brdd", KIKI_MERGETREE, treeID1, treeID2)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);
    return true;
}

nodeListElement* Skeletonizer::getNodeWithPrevID(nodeListElement *currentNode) {
    nodeListElement *node = currentNode->correspondingTree->firstNode;
    nodeListElement *prevNode = NULL;
    nodeListElement *highestNode = NULL;
    unsigned int minDistance = UINT_MAX;
    unsigned int tempMinDistance = minDistance;
    unsigned int maxID = 0;

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

    if(!prevNode) {
        prevNode = highestNode;
    }

    return prevNode;
}

nodeListElement* Skeletonizer::getNodeWithNextID(nodeListElement *currentNode) {
    nodeListElement *node = currentNode->correspondingTree->firstNode;
    nodeListElement *nextNode = NULL;
    nodeListElement *lowestNode = NULL;
    unsigned int minDistance = UINT_MAX;
    unsigned int tempMinDistance = minDistance;
    unsigned int minID = UINT_MAX;

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

    if(!nextNode) {
        nextNode = lowestNode;
    }

    return nextNode;
}

nodeListElement* Skeletonizer::findNodeByNodeID(int nodeID) {
    nodeListElement *node;
    node = (struct nodeListElement *)getDynArray(state->skeletonState->nodesByNodeID, nodeID);
    return node;
}

nodeListElement* Skeletonizer::findNodeByCoordinate(Coordinate *position) {
    nodeListElement *currentNode;
    treeListElement *currentTree;

    currentTree = state->skeletonState->firstTree;

    while(currentTree) {
        currentNode = currentTree->firstNode;
        while(currentNode) {
            if(COMPARE_COORDINATE(currentNode->position, *position))
                return currentNode;

            currentNode = currentNode->next;
        }
        currentTree = currentTree->next;
    }
    return NULL;
}

treeListElement* Skeletonizer::addTreeListElement(int sync, int targetRevision, int treeID, color4F color, int serialize) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

     //  The variable sync is a workaround for the problem that this function
     // will sometimes be called by other syncable functions that themselves hold
     // the lock and handle synchronization. If set to false, all synchro
     // routines will be skipped.

    treeListElement *newElement = NULL;

    if(sync != false) {
        if(Knossos::lockSkeleton(targetRevision) == false) {
            LOG("addtreelistelement unable to lock.")
            Knossos::unlockSkeleton(false);
            return false;
        }
    }


    newElement = findTreeByTreeID(treeID);
    if(newElement) {
        LOG("Tree with ID %d already exists!", treeID)
        Knossos::unlockSkeleton(true);
        return newElement;
    }

    newElement = (treeListElement*)malloc(sizeof(struct treeListElement));
    if(newElement == NULL) {
        LOG("Out of memory while trying to allocate memory for a new treeListElement.")
        Knossos::unlockSkeleton(false);
        return NULL;
    }
    if(serialize){

        saveSerializedSkeleton();
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


    // calling function sets values < 0 when no color was specified
    if(color.r < 0) {//Set a tree color
        int index = (newElement->treeID - 1) % 256; //first index is 0
        newElement->color.r = state->viewerState->treeAdjustmentTable[index];
        newElement->color.g = state->viewerState->treeAdjustmentTable[index + 256];
        newElement->color.b = state->viewerState->treeAdjustmentTable[index + 512];
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
    LOG("Added new tree with ID: %d.", newElement->treeID)

    if(newElement->treeID > state->skeletonState->greatestTreeID) {
        state->skeletonState->greatestTreeID = newElement->treeID;
    }

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;

    if(sync != false) {
        state->skeletonState->skeletonRevision++;

        if(targetRevision == CHANGE_MANUAL) {
            if(!Client::syncMessage("brdfff", KIKI_ADDTREE, newElement->treeID,
                            newElement->color.r, newElement->color.g, newElement->color.b)) {
                Client::skeletonSyncBroken();
            }
        }

        Knossos::unlockSkeleton(true);
    }
    else {

    }

    if(treeID == 1) {
        Skeletonizer::setActiveTreeByID(1);
    }

    return newElement;
}

treeListElement* Skeletonizer::getTreeWithPrevID(treeListElement *currentTree) {
    treeListElement *tree = state->skeletonState->firstTree;
    treeListElement *prevTree = NULL;
    uint idDistance = state->skeletonState->treeElements;
    uint tempDistance = idDistance;

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

bool Skeletonizer::addTreeComment(int targetRevision, int treeID, char *comment) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    treeListElement *tree = NULL;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        LOG("addTreeComment unable to lock.")
        Knossos::unlockSkeleton(false);
        return false;
    }

    tree = findTreeByTreeID(treeID);

    if(comment && tree) {
        strncpy(tree->comment, comment, 8192);
    }

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("blrds", KIKI_ADDTREECOMMENT, treeID, comment)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    return true;
}

segmentListElement* Skeletonizer::findSegmentByNodeIDs(int sourceNodeID, int targetNodeID) {
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

bool Skeletonizer::genTestNodes(uint number) {
    uint i;
    int nodeID;
    struct nodeListElement *node;
    Coordinate pos;
    color4F treeCol;
    //add new tree for test nodes
    treeCol.r = -1.;
    addTreeListElement(true, CHANGE_MANUAL, 0, treeCol, false);

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

bool Skeletonizer::editNode(int targetRevision,
                 int nodeID,
                 nodeListElement *node,
                 float newRadius,
                 int newXPos,
                 int newYPos,
                 int newZPos,
                 int inMag) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    segmentListElement *currentSegment = NULL;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(!node) {
        node = findNodeByNodeID(nodeID);
    }
    if(!node) {
        LOG("Cannot edit: node id %d invalid.", nodeID)
        Knossos::unlockSkeleton(false);
        return false;
    }

    nodeID = node->nodeID;

    //Since the position can change, we have to rebuild the corresponding spatial skeleton structure
    /*
    delNodeFromSkeletonStruct(node);
    currentSegment = node->firstSegment;
    while(currentSegment) {
        delSegmentFromSkeletonStruct(currentSegment);
        currentSegment = currentSegment->next;
    }*/

    if(!((newXPos < 0) || (newXPos > state->boundary.x)
       || (newYPos < 0) || (newYPos > state->boundary.y)
       || (newZPos < 0) || (newZPos > state->boundary.z))) {
        SET_COORDINATE(node->position, newXPos, newYPos, newZPos);
    }

    if(newRadius != 0.) {
        node->radius = newRadius;
    }
    node->createdInMag = inMag;

    //Since the position can change, we have to rebuild the corresponding spatial skeleton structure
    /*
    addNodeToSkeletonStruct(node);
    currentSegment = node->firstSegment;
    while(currentSegment) {
        addSegmentToSkeletonStruct(currentSegment);
        currentSegment = currentSegment->next;
    }*/

    updateCircRadius(node);
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brddfdddd", KIKI_EDITNODE,
                                            state->clientState->myId,
                                            nodeID,
                                            newRadius,
                                            inMag,
                                            newXPos,
                                            newYPos,
                                            newZPos)) {

            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

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
        LOG("Stack can't hold NULL.")
        return false;
    }

    if(stack->stackpointer + 1 == stack->size) {
        stack->elements = (void**)realloc(stack->elements, stack->size * 2 * sizeof(void *));
        if(stack->elements == NULL) {
            LOG("Out of memory.")
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
        LOG("That doesn't really make any sense, right? Cannot create stack with size <= 0.")
        return NULL;
    }

    newStack = (stack *)malloc(sizeof(struct stack));
    if(newStack == NULL) {
        LOG("Out of memory.")
        _Exit(false);
    }
    memset(newStack, '\0', sizeof(struct stack));

    newStack->elements = (void **)malloc(sizeof(void *) * size);
    if(newStack->elements == NULL) {
        LOG("Out of memory.")
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
        return false;
    }
    return array->elements[pos];
}

bool Skeletonizer::setDynArray(dynArray *array, int pos, void *value) {
    while(pos > array->end) {
        array->elements = (void**)realloc(array->elements, (array->end + 1 +
                                  array->firstSize) * sizeof(void *));
        if(array->elements == NULL) {
            LOG("Out of memory.")
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
        LOG("Out of memory.")
        _Exit(false);
    }
    memset(newArray, '\0', sizeof(struct dynArray));

    newArray->elements = (void**)malloc(sizeof(void *) * size);
    if(newArray->elements == NULL) {
        LOG("Out of memory.")
        _Exit(false);
    }
    memset(newArray->elements, '\0', sizeof(void *) * size);

    newArray->end = size - 1;
    newArray->firstSize = size;

    return newArray;
}

int Skeletonizer::splitConnectedComponent(int targetRevision, int nodeID, int serialize) {
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

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

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
        Knossos::unlockSkeleton(false);
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
        LOG("Out of memory.")
        _Exit(false);
    }

    if(serialize){
        saveSerializedSkeleton();
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
                LOG("Out of memory.")
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
        newTree = addTreeListElement(false, CHANGE_MANUAL, 0, treeCol, false);
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
        state->viewerState->gui->activeTreeID = state->skeletonState->activeTree->treeID;
        state->skeletonState->skeletonChanged = true;
    }
    else {
        LOG("The connected component is equal to the entire tree, not splitting.")
    }

    delStack(remainingNodes);
    delStack(componentNodes);

    free(visitedLeft);
    free(visitedRight);

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_SPLIT_CC, nodeID)) {
            LOG("broken in splitcc.")
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    return nodeCount;
}

bool Skeletonizer::addComment(int targetRevision, const char *content, nodeListElement *node, int nodeID, int serialize) {
    //This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    commentListElement *newComment;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    newComment = (commentListElement*)malloc(sizeof(struct commentListElement));
    memset(newComment, '\0', sizeof(struct commentListElement));

    newComment->content = (char*)malloc(strlen(content) * sizeof(char) + 1);
    memset(newComment->content, '\0', strlen(content) * sizeof(char) + 1);

    if(nodeID) {
        node = findNodeByNodeID(nodeID);
    }
    if(node) {
        newComment->node = node;
        node->comment = newComment;
    }

    if(content) {
        strncpy(newComment->content, content, strlen(content));
        state->skeletonState->skeletonChanged = true;
    }

    if(serialize) {
        saveSerializedSkeleton();
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
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("blrds", KIKI_ADDCOMMENT, node->nodeID, content)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    state->skeletonState->totalComments++;
    return true;
}

bool Skeletonizer::delComment(int targetRevision, commentListElement *currentComment, int commentNodeID, int serialize) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    int nodeID = 0;
    nodeListElement *commentNode = NULL;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(commentNodeID) {
        commentNode = findNodeByNodeID(commentNodeID);
        if(commentNode) {
            currentComment = commentNode->comment;
        }
    }

    if(!currentComment) {
        LOG("Please provide a valid comment element to delete!")
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(serialize){
       saveSerializedSkeleton();
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
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_DELCOMMENT, nodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    state->skeletonState->totalComments--;
    return true;
}

bool Skeletonizer::editComment(int targetRevision, commentListElement *currentComment, int nodeID, char *newContent, nodeListElement *newNode, int newNodeID, int serialize) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    // this function also seems to be kind of useless as you could do just the same
    // thing with addComment() with minimal changes ....?

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(nodeID) {
        currentComment = findNodeByNodeID(nodeID)->comment;
    }
    if(!currentComment) {
        LOG("Please provide a valid comment element to edit!")
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(serialize){
        saveSerializedSkeleton();
    }

    nodeID = currentComment->node->nodeID;

    if(newContent) {
        if(currentComment->content) {
            free(currentComment->content);
        }
        currentComment->content = (char*)malloc(strlen(newContent) * sizeof(char) + 1);
        memset(currentComment->content, '\0', strlen(newContent) * sizeof(char) + 1);
        strncpy(currentComment->content, newContent, strlen(newContent));

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
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("blrdds",
                    KIKI_EDITCOMMENT,
                    nodeID,
                    currentComment->node->nodeID,
                    newContent)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    return true;
}
commentListElement* Skeletonizer::nextComment(char *searchString) {
   commentListElement *firstComment, *currentComment;

    if(!strlen(searchString)) {
        //->previous here because it would be unintuitive for the user otherwise.
        //(we insert new comments always as first elements)
        if(state->skeletonState->currentComment) {
            state->skeletonState->currentComment = state->skeletonState->currentComment->previous;
            setActiveNode(CHANGE_MANUAL,
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
                if(strstr(currentComment->content, searchString) != NULL) {
                    state->skeletonState->currentComment = currentComment;
                    setActiveNode(CHANGE_MANUAL,
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

    return state->skeletonState->currentComment;
}

commentListElement* Skeletonizer::previousComment(char *searchString) {
    commentListElement *firstComment, *currentComment;
    // ->next here because it would be unintuitive for the user otherwise.
    // (we insert new comments always as first elements)

    if(!strlen(searchString)) {
        if(state->skeletonState->currentComment) {
            state->skeletonState->currentComment = state->skeletonState->currentComment->next;
            setActiveNode(CHANGE_MANUAL,
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
                if(strstr(currentComment->content, searchString) != NULL) {
                    state->skeletonState->currentComment = currentComment;
                    setActiveNode(CHANGE_MANUAL,
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
    return state->skeletonState->currentComment;
}

bool Skeletonizer::searchInComment(char *searchString, commentListElement *comment) {
    return true;
}

bool Skeletonizer::unlockPosition() {
    if(state->skeletonState->positionLocked) {
        LOG("Spatial locking disabled.")
    }
    state->skeletonState->positionLocked = false;

    return true;
}

bool Skeletonizer::lockPosition(Coordinate lockCoordinate) {
    LOG("locking to (%d, %d, %d).", lockCoordinate.x, lockCoordinate.y, lockCoordinate.z)
    state->skeletonState->positionLocked = true;
    SET_COORDINATE(state->skeletonState->lockedPosition,
                   lockCoordinate.x,
                   lockCoordinate.y,
                   lockCoordinate.z);

    return true;
}

/* @todo loader gets out of sync (endless loop) */
bool Skeletonizer::popBranchNode(int targetRevision, int serialize) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    // SYNCHRO BUG:
    // both instances will have to confirm branch point deletion if
    // confirmation is asked.

    nodeListElement *branchNode = NULL;
    PTRSIZEINT branchNodeID = 0;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(serialize){
        saveSerializedSkeleton();
    }

    // Nodes on the branch stack may not actually exist anymore
    while(true){
        if (branchNode != NULL)
            if (branchNode->isBranchNode == true) {
                break;
            }
        branchNodeID = (PTRSIZEINT)popStack(state->skeletonState->branchStack);
        if(branchNodeID == 0) {
            // AGAR AG_TextMsg(AG_MSG_INFO, "No branch points remain.");
            LOG("No branch points remain.");

            goto exit_popbranchnode;
        }

        branchNode = findNodeByNodeID(branchNodeID);
    }

    if(branchNode && branchNode->isBranchNode) {
#if QT_POINTER_SIZE == 8
        LOG("Branch point (node ID %"PRId64") deleted.", branchNodeID)
#else
        LOG("Branch point (node ID %d) deleted.", branchNodeID)
#endif


        setActiveNode(CHANGE_NOSYNC, branchNode, 0);

        branchNode->isBranchNode--;
        state->skeletonState->skeletonChanged = true;

        emit userMoveSignal(branchNode->position.x - state->viewerState->currentPosition.x,
                            branchNode->position.y - state->viewerState->currentPosition.y,
                            branchNode->position.z - state->viewerState->currentPosition.z,
                            TELL_COORDINATE_CHANGE);

        state->skeletonState->branchpointUnresolved = true;
    }

exit_popbranchnode:

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("br", KIKI_POPBRANCH)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }

    state->skeletonState->totalBranchpoints--;
    Knossos::unlockSkeleton(true);
    return true;
}

bool Skeletonizer::pushBranchNode(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, int branchNodeID, int serialize) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }
    if(branchNodeID) {
        branchNode = findNodeByNodeID(branchNodeID);
    }
    if(branchNode) {
        if(serialize){
            saveSerializedSkeleton();
        }

        if(branchNode->isBranchNode == 0 || !checkDoubleBranchpoint) {
            pushStack(state->skeletonState->branchStack, (void *)(PTRSIZEINT)branchNode->nodeID);
            if(setBranchNodeFlag) {
                branchNode->isBranchNode = true;

                state->skeletonState->skeletonChanged = true;
                LOG("Branch point (node ID %d) added.", branchNode->nodeID)
            }

        }
        else {
            LOG("Active node is already a branch point")
            Knossos::unlockSkeleton(true);
            return true;
        }
    }
    else {
        LOG("Make a node active before adding branch points.")
        Knossos::unlockSkeleton(true);
        return true;
    }

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_PUSHBRANCH, branchNode->nodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    state->skeletonState->totalBranchpoints++;
    return true;
}

bool Skeletonizer::setSkeletonWorkMode(int targetRevision, uint workMode) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    state->skeletonState->workMode = workMode;

    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_SETSKELETONMODE, workMode)) {
            Client::skeletonSyncBroken();
        }
    }
    else {

    }
    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::jumpToActiveNode() {
    if(state->skeletonState->activeNode) {
        emit userMoveSignal(state->skeletonState->activeNode->position.x - state->viewerState->currentPosition.x,
                            state->skeletonState->activeNode->position.y - state->viewerState->currentPosition.y,
                            state->skeletonState->activeNode->position.z - state->viewerState->currentPosition.z,
                            TELL_COORDINATE_CHANGE);
    }

    return true;
}

void Skeletonizer::UI_popBranchNode() {
    // Inconsistency:
    // Confirmation will not be asked when no branch points remain, except if the remaining
    //  branch point nodes don't exist anymore (nodes have been deleted).

    // This is workaround around agar bug #171
    if(state->skeletonState->askingPopBranchConfirmation == false) {
        state->skeletonState->askingPopBranchConfirmation = true;

        if(state->skeletonState->branchpointUnresolved && state->skeletonState->branchStack->stackpointer != -1) {
            //yesNoPrompt(NULL,
            //            "No node was added after jumping to the last branch point. Do you really want to jump?",
            //            WRAP_popBranchNode,
            //            popBranchNodeCanceled);

        }
        else {
            WRAP_popBranchNode();
        }
    }
}

void Skeletonizer::restoreDefaultTreeColor() {
    if(state->skeletonState->activeTree) {
        int index = (state->skeletonState->activeTree->treeID - 1) % 256;
        state->skeletonState->activeTree->color.r = state->viewerState->defaultTreeTable[index];
        state->skeletonState->activeTree->color.g = state->viewerState->defaultTreeTable[index + 256];
        state->skeletonState->activeTree->color.b = state->viewerState->defaultTreeTable[index + 512];
        state->skeletonState->activeTree->color.a = 1.;

        state->skeletonState->activeTree->colorSetManually = false;
        state->skeletonState->skeletonChanged = true;
        state->skeletonState->unsavedChanges = true;
    }
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

bool Skeletonizer::delSkelState(struct skeletonState *skelState) {
    if(skelState == NULL) {
        return false;
    }
    delTreesFromState(skelState);
    Hashtable::ht_rmtable(skelState->skeletonDCs);
    free(skelState->searchStrBuffer);
    free(skelState->prevSkeletonFile);
    free(skelState->skeletonFile);
    free(skelState);
    skelState = NULL;

    return true;
}

bool Skeletonizer::delTreesFromState(struct skeletonState *skelState) {
    struct treeListElement *current;
    struct treeListElement *treeToDel;

    if(skelState->firstTree == NULL) {
        return false;
    }
    current = skelState->firstTree;
    while(current) {
        treeToDel = current;
        current = current->next;
        delTreeFromState(treeToDel, skelState);
    }
    skelState->treeElements = 0;
    skelState->firstTree = NULL;
    skelState->activeTree = NULL;
    skelState->activeNode = NULL;
    skelState->greatestTreeID = 0;
    skelState->greatestNodeID = 0;
    delStack(skelState->branchStack);
    skelState->branchStack = NULL;
    delDynArray(skelState->nodeCounter);
    skelState->nodeCounter = NULL;
    delDynArray(skelState->nodesByNodeID);
    skelState->nodesByNodeID = NULL;
    free(skelState->commentBuffer);
    skelState->commentBuffer = NULL;
    free(skelState->currentComment);
    skelState->currentComment = NULL;

    return true;
}

bool Skeletonizer::delTreeFromState(treeListElement *treeToDel, skeletonState *skelState) {
    nodeListElement *currentNode = NULL;
    nodeListElement *nodeToDel = NULL;

    if(treeToDel == NULL) {
        return false;
    }

    currentNode = treeToDel->firstNode;
    while(currentNode) {
        nodeToDel = currentNode;
        currentNode = nodeToDel->next;
        delNodeFromState(nodeToDel, skelState);
    }
    treeToDel->firstNode = NULL;

    if(treeToDel->previous) {
        treeToDel->previous->next = treeToDel->next;
    }
    if(treeToDel->next) {
        treeToDel->next->previous = treeToDel->previous;
    }
    free(treeToDel);
    return true;
}

bool Skeletonizer::hasObfuscatedTime() {
    int major = 0, minor = 0;
    char point;

    sscanf(state->skeletonState->skeletonCreatedInVersion, "%d%c%d", &major, &point, &minor);

    if(major > 3) {
        return true;
    }
    if(major == 3 && minor >= 4) {
        return true;
    }
    return false;
}

bool Skeletonizer::delNodeFromState(struct nodeListElement *nodeToDel, struct skeletonState *skelState) {
    struct segmentListElement *currentSegment;
    struct segmentListElement *tempNext;

    if(nodeToDel == NULL) {
        return false;
    }

    if(nodeToDel->comment) {
        delCommentFromState(nodeToDel->comment, skelState);
    }

    /*
     * First, delete all segments pointing towards and away of the nodeToDelhas
     * been */

    currentSegment = nodeToDel->firstSegment;
    while(currentSegment) {
        tempNext = currentSegment->next;
        if(currentSegment->flag == SEGMENT_FORWARD) {
            delSegmentFromCmd(currentSegment);
        }
        else if(currentSegment->flag == SEGMENT_BACKWARD) {
            delSegmentFromCmd(currentSegment->reverseSegment);
        }
        currentSegment = tempNext;
    }
    nodeToDel->firstSegment = NULL;

    //TDitem: is this necessary for undo?
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
    free(nodeToDel);
    return true;
}

bool Skeletonizer::delCommentFromState(struct commentListElement *commentToDel, struct skeletonState *skelState) {
    int nodeID = 0;

    if(commentToDel == NULL) {
        return false;
    }
    if(commentToDel->content) {
        free(commentToDel->content);
    }
    if(commentToDel->node) {
        nodeID = commentToDel->node->nodeID;
        commentToDel->node->comment = NULL;
    }

    if(commentToDel->next == commentToDel) { //only comment in list
        skelState->currentComment = NULL;
    }
    else {
        commentToDel->next->previous = commentToDel->previous;
        commentToDel->previous->next = commentToDel->next;
        if(skelState->currentComment == commentToDel) {
            skelState->currentComment = commentToDel->next;
        }
    }
    free(commentToDel);
    return true;
}

bool Skeletonizer::delSegmentFromCmd(struct segmentListElement *segToDel) {
    if(!segToDel) {
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
        if(segToDel->next) {
            segToDel->next->previous = segToDel->previous;
        }
    }

    //Delete reverse segment in target node
    if(segToDel->reverseSegment == segToDel->target->firstSegment) {
        segToDel->target->firstSegment = segToDel->reverseSegment->next;
    }
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
    return true;
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
            setActiveNode(CHANGE_MANUAL, node, node->nodeID);
            emit setRemoteStateTypeSignal(REMOTE_RECENTERING);
            emit setRecenteringPositionSignal(node->position.x,
                                         node->position.y,
                                         node->position.z);

            Knossos::sendRemoteSignal();
        }
        return true;
    }
    LOG("Reached first tree.")

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
            setActiveNode(CHANGE_MANUAL, node, node->nodeID);

                emit setRemoteStateTypeSignal(REMOTE_RECENTERING);
                emit setRecenteringPositionSignal(node->position.x,
                                             node->position.y,
                                             node->position.z);
                Knossos::sendRemoteSignal();


        }
        return true;
    }
    LOG("Reached last tree.")

    return false;

}

bool Skeletonizer::moveToPrevNode() {

    struct nodeListElement *prevNode = getNodeWithPrevID(state->skeletonState->activeNode);

    if(state->skeletonState->activeNode == NULL) {
        return false;
    }
    if(prevNode) {
        setActiveNode(CHANGE_MANUAL, prevNode, prevNode->nodeID);
        emit setRemoteStateTypeSignal(REMOTE_RECENTERING);
        emit setRecenteringPositionSignal(prevNode->position.x,
                                     prevNode->position.y,
                                     prevNode->position.z);
        Knossos::sendRemoteSignal();
        return true;
    }

    return false;
}

bool Skeletonizer::moveToNextNode() {

    struct nodeListElement *nextNode = getNodeWithNextID(state->skeletonState->activeNode);

    if(state->skeletonState->activeNode == NULL) {
        return false;
    }
    if(nextNode) {
        setActiveNode(CHANGE_MANUAL, nextNode, nextNode->nodeID);
        emit setRemoteStateTypeSignal(REMOTE_RECENTERING);
        emit setRecenteringPositionSignal(nextNode->position.x,
                                     nextNode->position.y,
                                     nextNode->position.z);
        Knossos::sendRemoteSignal();
        return true;
    }

    return false;

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


void Skeletonizer::setZoomLevel(float value) {
    this->zoomLevel = value;
}

void Skeletonizer::setShowIntersections(bool on) {
    this->showIntersections = on;
}

void Skeletonizer::setShowXyPlane(bool on) {
    this->showXYplane = on;
}

void Skeletonizer::setRotateAroundActiveNode(bool on) {
    this->rotateAroundActiveNode = on;
}

void Skeletonizer::setOverrideNodeRadius(bool on) {
    this->overrideNodeRadiusBool = on;
}

void Skeletonizer::setSegRadiusToNodeRadius(float value) {
    this->segRadiusToNodeRadius = value;
}

void Skeletonizer::setHighlightActiveTree(bool on) {
    this->highlightActiveTree = on;
}

void Skeletonizer::setSkeletonChanged(bool on) {
    this->skeletonChanged = on;
}

void Skeletonizer::setShowNodeIDs(bool on) {
    this->showNodeIDs = on;
}

/*_______________________________________________ */
/* @todo ACTIVE_TREE_ID durch Widgets */

Byte *Skeletonizer::serializeSkeleton() {
    
    struct stack *reverseBranchStack = NULL, *tempReverseStack = NULL;
    PTRSIZEINT currentBranchPointID;
    Byte *serialSkeleton = NULL;
    struct treeListElement *currentTree;
    struct nodeListElement *currentNode;
    struct segmentListElement *currentSegment;
    struct commentListElement *currentComment;

    qDebug() << "ABSCHNITT 1";
    uint32_t i = 0, memPosition = 0, totalNodeNumber = 0, totalSegmentNumber = 0, totalCommentNumber = 0;
    uint32_t variablesBlockSize = getVariableBlockSize();
    uint32_t treeBlockSize = getTreeBlockSize();
    uint32_t nodeBlockSize = getNodeBlockSize();
    uint32_t segmentBlockSize = getSegmentBlockSize();
    uint32_t commentBlockSize = getCommentBlockSize();
    uint32_t branchPointBlockSize = getBranchPointBlockSize();

    reverseBranchStack = newStack(2048);
    tempReverseStack = newStack(2048);

    qDebug() << "ABSCHNITT 2";
    while((currentBranchPointID =
        (PTRSIZEINT)popStack(state->skeletonState->branchStack))) {
        pushStack(reverseBranchStack, (void *)currentBranchPointID);
        pushStack(tempReverseStack, (void *)currentBranchPointID);
    }
    while((currentBranchPointID =
          (PTRSIZEINT)popStack(tempReverseStack))) {
        currentNode = (struct nodeListElement *)findNodeByNodeID(currentBranchPointID);
        pushBranchNode(CHANGE_MANUAL, false, false, currentNode, 0, false);
    }


    uint32_t serializedSkeletonSize = variablesBlockSize * sizeof(Byte)
                                    + treeBlockSize * sizeof(Byte)
                                    + nodeBlockSize * sizeof(Byte) * state->skeletonState->totalNodeElements
                                    + segmentBlockSize * sizeof(Byte) * state->skeletonState->totalSegmentElements
                                    + commentBlockSize * sizeof(Byte)
                                    + branchPointBlockSize * sizeof(Byte);

    if(serializedSkeletonSize > 1000000) {
        qDebug() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
    }

    serialSkeleton = (Byte *)malloc(serializedSkeletonSize);
    if(serialSkeleton == NULL){
        LOG("Out of memory");
        _Exit(false);
    }
    memset(serialSkeleton, '\0', serializedSkeletonSize);

    qDebug() << "ABSCHNITT 3";
    //Experiment name
    Client::integerToBytes(&serialSkeleton[memPosition], strlen(state->name));
    memPosition+=sizeof(int32_t);
    strncpy((char *)&serialSkeleton[memPosition], state->name, strlen(state->name));
    memPosition+=strlen(state->name);

    //KNOSSOS version
    Client::integerToBytes(&serialSkeleton[memPosition], strlen(KVERSION));
    memPosition+=sizeof(int32_t);
    strncpy((char *)&serialSkeleton[memPosition], KVERSION, strlen(KVERSION));
    memPosition+=strlen(KVERSION);

    //Created in version
    Client::integerToBytes((Byte *)&serialSkeleton[memPosition], strlen(state->skeletonState->skeletonCreatedInVersion));
    memPosition+=sizeof(int32_t);
    strncpy((char *)&serialSkeleton[memPosition], state->skeletonState->skeletonCreatedInVersion, strlen(state->skeletonState->skeletonCreatedInVersion));
    memPosition+=strlen(state->skeletonState->skeletonCreatedInVersion);

    //Last saved in version
    Client::integerToBytes((Byte *)&serialSkeleton[memPosition], strlen(state->skeletonState->skeletonLastSavedInVersion));
    memPosition+=sizeof(int32_t);
    strncpy((char *)&serialSkeleton[memPosition], state->skeletonState->skeletonLastSavedInVersion, strlen(state->skeletonState->skeletonLastSavedInVersion));
    memPosition+=strlen(state->skeletonState->skeletonLastSavedInVersion);

    //Scale
    Client::floatToBytes((Byte *)&serialSkeleton[memPosition], state->scale.x / state->magnification);
    memPosition+=sizeof(float);
    Client::floatToBytes((Byte *)&serialSkeleton[memPosition], state->scale.y / state->magnification);
    memPosition+=sizeof(float);
    Client::floatToBytes((Byte *)&serialSkeleton[memPosition], state->scale.z / state->magnification);
    memPosition+=sizeof(float);

    //Offset
    Client::integerToBytes(&serialSkeleton[memPosition], state->offset.x / state->magnification);
    memPosition+=sizeof(int32_t);
    Client::integerToBytes(&serialSkeleton[memPosition], state->offset.y / state->magnification);
    memPosition+=sizeof(int32_t);
    Client::integerToBytes(&serialSkeleton[memPosition], state->offset.z / state->magnification);
    memPosition+=sizeof(int32_t);

    //LockOnComment
    Client::integerToBytes(&serialSkeleton[memPosition], state->skeletonState->lockPositions);
    memPosition+=sizeof(int32_t);
    Client::integerToBytes(&serialSkeleton[memPosition], state->skeletonState->lockRadius);
    memPosition+=sizeof(long unsigned int);
    Client::integerToBytes(&serialSkeleton[memPosition], strlen(state->skeletonState->onCommentLock));
    memPosition+=sizeof(int32_t);
    strncpy((char *)&serialSkeleton[memPosition], state->skeletonState->onCommentLock, strlen(state->skeletonState->onCommentLock));
    memPosition+=strlen(state->skeletonState->onCommentLock);

    qDebug() << "ABSCHNITT 4";
    //Display Mode, Work Mode, Skeleton Time, Active Node
    Client::integerToBytes(&serialSkeleton[memPosition], state->skeletonState->displayMode);
    memPosition+=sizeof(int32_t);
    Client::integerToBytes(&serialSkeleton[memPosition], state->skeletonState->workMode);
    memPosition+=sizeof(int32_t);
    Client::integerToBytes((Byte *)&serialSkeleton[memPosition], state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + state->time.elapsed());
    memPosition+=sizeof(int32_t);
    if(state->skeletonState->activeNode){
        Client::integerToBytes(&serialSkeleton[memPosition], state->skeletonState->activeNode->nodeID);
    }
    else{
        Client::integerToBytes(&serialSkeleton[memPosition], 0);
    }
    memPosition+=sizeof(int32_t);

    //Current Position
    Client::integerToBytes(&serialSkeleton[memPosition], state->viewerState->currentPosition.x);
    memPosition+=sizeof(int32_t);
    Client::integerToBytes(&serialSkeleton[memPosition], state->viewerState->currentPosition.y);
    memPosition+=sizeof(int32_t);
    Client::integerToBytes(&serialSkeleton[memPosition], state->viewerState->currentPosition.z);
    memPosition+=sizeof(int32_t);

    qDebug() << "ABSCHNITT 5";
    //SkeletonViewport Display
    for (i = 0; i < 16; i++){
        Client::floatToBytes(&serialSkeleton[memPosition], state->skeletonState->skeletonVpModelView[i]);
        memPosition+=sizeof(float);
    }
    Client::floatToBytes(&serialSkeleton[memPosition], state->skeletonState->translateX);
    memPosition+=sizeof(float);
    Client::floatToBytes(&serialSkeleton[memPosition], state->skeletonState->translateY);
    memPosition+=sizeof(float);

    //Zoom Levels
    Client::floatToBytes((Byte *)&serialSkeleton[memPosition], state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel);
    memPosition+=sizeof(float);
    Client::floatToBytes((Byte *)&serialSkeleton[memPosition], state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel);
    memPosition+=sizeof(float);
    Client::floatToBytes((Byte *)&serialSkeleton[memPosition], state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel);
    memPosition+=sizeof(float);
    Client::floatToBytes((Byte *)&serialSkeleton[memPosition], state->skeletonState->zoomLevel);
    memPosition+=sizeof(float);

    //Idle Time, Tree Elements
    Client::integerToBytes(&serialSkeleton[memPosition], state->skeletonState->idleTime);
    memPosition+=sizeof(int32_t);
    Client::integerToBytes((Byte *)&serialSkeleton[memPosition], state->viewerState->gui->activeTreeID);
    memPosition+=sizeof(int32_t);
    Client::integerToBytes((Byte *)&serialSkeleton[memPosition], state->skeletonState->treeElements);
    memPosition+=sizeof(int32_t);

    currentTree = state->skeletonState->firstTree;
    if((currentTree == NULL) && (state->skeletonState->currentComment == NULL)) {
        return false; //No Skeleton to save
    }

        while(currentTree) {
        Client::integerToBytes(&serialSkeleton[memPosition], currentTree->treeID);
        memPosition+=sizeof(int32_t);
        if(currentTree->colorSetManually){
            Client::floatToBytes(&serialSkeleton[memPosition], currentTree->color.r);
            memPosition+=sizeof(float);
            Client::floatToBytes(&serialSkeleton[memPosition], currentTree->color.b);
            memPosition+=sizeof(float);
            Client::floatToBytes(&serialSkeleton[memPosition], currentTree->color.g);
            memPosition+=sizeof(float);
            Client::floatToBytes(&serialSkeleton[memPosition], currentTree->color.a);
            memPosition+=sizeof(float);
        }
        else{
            Client::floatToBytes(&serialSkeleton[memPosition], -1);
            memPosition+=sizeof(float);
            Client::floatToBytes(&serialSkeleton[memPosition], -1);
            memPosition+=sizeof(float);
            Client::floatToBytes(&serialSkeleton[memPosition], -1);
            memPosition+=sizeof(float);
            Client::floatToBytes(&serialSkeleton[memPosition], 1);
            memPosition+=sizeof(float);
        }
        Client::integerToBytes(&serialSkeleton[memPosition], strlen(currentTree->comment));
        memPosition+=sizeof(int32_t);
        strncpy((char *)&serialSkeleton[memPosition], currentTree->comment, strlen(currentTree->comment));
        memPosition+=strlen(currentTree->comment);

        memPosition+=sizeof(int32_t);
        currentNode = currentTree->firstNode;
        totalNodeNumber = 0;
        while(currentNode){
            Client::integerToBytes(&serialSkeleton[memPosition], currentNode->nodeID);
            memPosition+=sizeof(int32_t);
            Client::floatToBytes(&serialSkeleton[memPosition], currentNode->radius);
            memPosition+=sizeof(float);
            Client::integerToBytes(&serialSkeleton[memPosition], currentNode->position.x);
            memPosition+=sizeof(int32_t);
            Client::integerToBytes(&serialSkeleton[memPosition], currentNode->position.y);
            memPosition+=sizeof(int32_t);
            Client::integerToBytes(&serialSkeleton[memPosition], currentNode->position.z);
            memPosition+=sizeof(int32_t);
            Client::integerToBytes(&serialSkeleton[memPosition], currentNode->createdInVp);
            memPosition+=sizeof(currentNode->createdInVp);
            Client::integerToBytes(&serialSkeleton[memPosition], currentNode->createdInMag);
            memPosition+=sizeof(int32_t);
            Client::integerToBytes(&serialSkeleton[memPosition], currentNode->timestamp);
            memPosition+=sizeof(int32_t);
            currentNode = currentNode->next;
            totalNodeNumber++;
        }
        memPosition-=nodeBlockSize * totalNodeNumber + sizeof(int32_t);
        Client::integerToBytes(&serialSkeleton[memPosition], totalNodeNumber);
        memPosition+=nodeBlockSize * totalNodeNumber + sizeof(int32_t);

    currentTree = currentTree->next;
    }
    memPosition+=sizeof(int32_t);
    currentTree = state->skeletonState->firstTree;
    totalSegmentNumber = 0;
    while(currentTree){
        currentNode = currentTree->firstNode;
        while(currentNode){
            currentSegment = currentNode->firstSegment;
            while(currentSegment){
                if(currentSegment->flag == SEGMENT_FORWARD){
                    Client::integerToBytes(&serialSkeleton[memPosition], currentSegment->source->nodeID);
                    memPosition+=sizeof(int32_t);
                    Client::integerToBytes(&serialSkeleton[memPosition], currentSegment->target->nodeID);
                    memPosition+=sizeof(int32_t);
                    totalSegmentNumber++;
                }
                currentSegment = currentSegment->next;
            }
            currentNode = currentNode->next;
        }
        currentTree = currentTree->next;
    }
    memPosition-=totalSegmentNumber*segmentBlockSize+sizeof(int32_t);
    Client::integerToBytes(&serialSkeleton[memPosition], totalSegmentNumber);
    memPosition+=totalSegmentNumber*segmentBlockSize+sizeof(int32_t);

    memPosition+=sizeof(int32_t);
    if(state->skeletonState->currentComment){
        currentComment = state->skeletonState->currentComment;
        do {
            Client::integerToBytes(&serialSkeleton[memPosition], currentComment->node->nodeID);
            memPosition+=sizeof(int32_t);
            Client::integerToBytes(&serialSkeleton[memPosition], strlen(currentComment->content));
            memPosition+=sizeof(int32_t);
            strncpy((char *)&serialSkeleton[memPosition], currentComment->content, strlen(currentComment->content));
            memPosition+=strlen(currentComment->content);
            currentComment = currentComment->next;
            totalCommentNumber++;
        } while (currentComment != state->skeletonState->currentComment);
    }
    memPosition-=commentBlockSize;
    Client::integerToBytes(&serialSkeleton[memPosition], totalCommentNumber);
    memPosition+=commentBlockSize;

    Client::integerToBytes(&serialSkeleton[memPosition], state->skeletonState->branchStack->elementsOnStack);
    memPosition+=sizeof(int32_t);
    while((currentBranchPointID = (PTRSIZEINT)popStack(reverseBranchStack))) {
        Client::integerToBytes(&serialSkeleton[memPosition], currentBranchPointID);
        memPosition+=sizeof(currentBranchPointID);

    }
    
    return NULL;

}

void Skeletonizer::deserializeSkeleton() {

    
    Byte *serialSkeleton = NULL;
    uint i = 0, j = 0, totalTreeNumber = 0, totalNodeNumber = 0, totalSegmentNumber = 0, totalCommentNumber = 0, totalBranchPointNumber;
    serialSkeleton = state->skeletonState->lastSerialSkeleton->content;
    int memPosition = 0, inMag = 0, time = 0, activeNodeID = 0, activeTreeID = 0, neuronID = 0, nodeID = 0, sourceNodeID = 0, targetNodeID = 0;
    int stringLength = 0;
    char temp[10000];
    color4F neuronColor;
    struct treeListElement *currentTree;
    struct nodeListElement *currentNode;
    float radius;
    Byte VPtype;
    uint workMode;

    Coordinate offset;
    floatCoordinate scale;
    Coordinate *currentCoordinate, loadedPosition;

    SET_COORDINATE(offset, state->offset.x, state->offset.y, state->offset.z);
    SET_COORDINATE(scale, state->scale.x, state->scale.y, state->scale.z);
    SET_COORDINATE(loadedPosition, 0, 0, 0);


    currentCoordinate = (Coordinate *)malloc(sizeof(Coordinate));
    if(currentCoordinate == NULL) {
        LOG("Out of memory.");
        return;
    }
    memset(currentCoordinate, '\0', sizeof(currentCoordinate));

    clearSkeleton(CHANGE_MANUAL, true);


    stringLength = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(stringLength);
    //memset(state->name, '\0', sizeof(state->name));
    //strncpy(state->name, &serialSkeleton[memPosition], stringLength);
    memPosition+=stringLength;

    stringLength = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(stringLength);
    memPosition+=stringLength;

    stringLength = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(stringLength);
    memset(state->skeletonState->skeletonCreatedInVersion, '\0', sizeof(state->skeletonState->skeletonCreatedInVersion));
    strncpy(state->skeletonState->skeletonCreatedInVersion, (char *)&serialSkeleton[memPosition], stringLength);
    memPosition+=stringLength;

    stringLength = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(stringLength);
    memset(state->skeletonState->skeletonLastSavedInVersion, '\0', sizeof(state->skeletonState->skeletonLastSavedInVersion));
    strncpy(state->skeletonState->skeletonLastSavedInVersion, (char *)&serialSkeleton[memPosition], stringLength);
    memPosition+=stringLength;

    scale.x = Client::bytesToFloat(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->scale.x / state->magnification);
    scale.y = Client::bytesToFloat(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->scale.y / state->magnification);
    scale.z = Client::bytesToFloat(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->scale.z / state->magnification);

    offset.x = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->offset.x / state->magnification);
    offset.y = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->offset.x / state->magnification);
    offset.z = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->offset.x / state->magnification);

    state->skeletonState->lockPositions = Client::bytesToInt(&serialSkeleton[memPosition]);

    //state->viewerState->gui->commentLockCheckbox->state = state->skeletonState->lockPositions; @todo

    memPosition+=sizeof(state->skeletonState->lockPositions);
    state->skeletonState->lockRadius = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(long unsigned int);

    stringLength = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(stringLength);
    memset(state->skeletonState->onCommentLock, '\0', sizeof(state->skeletonState->onCommentLock));
    strncpy(state->skeletonState->onCommentLock, (char *)&serialSkeleton[memPosition], stringLength);
    memPosition+=stringLength;

    //state->skeletonState->displayMode = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->skeletonState->displayMode);
    workMode = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->skeletonState->workMode);
    memPosition+=sizeof(state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + state->time.elapsed());
    activeNodeID = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->skeletonState->activeNode->nodeID);

    loadedPosition.x = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->viewerState->currentPosition.x);
    loadedPosition.y = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->viewerState->currentPosition.y);
    loadedPosition.z = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->viewerState->currentPosition.z);

     for (i = 0; i < 16; i++){
//        state->skeletonState->skeletonVpModelView[i] = Client::bytesToFloat(&serialSkeleton[memPosition]);
        memPosition+=sizeof(state->skeletonState->skeletonVpModelView[i]);
    }
//    glMatrixMode(GL_MODELVIEW);
//    glLoadMatrixf(state->skeletonState->skeletonVpModelView);

//    state->skeletonState->translateX = Client::bytesToFloat(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->skeletonState->translateX);
    //state->skeletonState->translateY = Client::bytesToFloat(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->skeletonState->translateY);

    state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel = Client::bytesToFloat(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel);
    state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel = Client::bytesToFloat(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel);
    state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel = Client::bytesToFloat(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel);
    state->skeletonState->zoomLevel = Client::bytesToFloat(&serialSkeleton[memPosition]);
    memPosition+=+sizeof(state->skeletonState->zoomLevel);

    memPosition+=sizeof(state->skeletonState->idleTime);


    activeTreeID = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(activeTreeID);

    totalTreeNumber = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(state->skeletonState->treeElements);

    for (i = 0; i < totalTreeNumber; i++){
        neuronID = Client::bytesToInt(&serialSkeleton[memPosition]);
        memPosition+=sizeof(neuronID);

        neuronColor.r = Client::bytesToFloat(&serialSkeleton[memPosition]);
        memPosition+=sizeof(neuronColor.r);
        neuronColor.b = Client::bytesToFloat(&serialSkeleton[memPosition]);
        memPosition+=sizeof(neuronColor.b);
        neuronColor.g = Client::bytesToFloat(&serialSkeleton[memPosition]);
        memPosition+=sizeof(neuronColor.g);
        neuronColor.a = Client::bytesToFloat(&serialSkeleton[memPosition]);
        memPosition+=sizeof(neuronColor.a);

        currentTree = addTreeListElement(true, CHANGE_MANUAL, neuronID, neuronColor, false);
        setActiveTreeByID(neuronID);

        stringLength = Client::bytesToInt(&serialSkeleton[memPosition]);
        memPosition+=sizeof(stringLength);
        memset(temp, '\0', sizeof(temp));
        strncpy(temp, (char *)&serialSkeleton[memPosition], stringLength);
        memPosition+=stringLength;

        totalNodeNumber = Client::bytesToInt(&serialSkeleton[memPosition]);
        memPosition+=sizeof(int);

        for (j = 0; j < totalNodeNumber; j++){
            nodeID = Client::bytesToInt(&serialSkeleton[memPosition]);
            memPosition+=sizeof(nodeID);

            radius = Client::bytesToFloat(&serialSkeleton[memPosition]);
            memPosition+=sizeof(radius);

            currentCoordinate->x = Client::bytesToInt(&serialSkeleton[memPosition]);
            memPosition+=sizeof(currentCoordinate->x);
            currentCoordinate->y = Client::bytesToInt(&serialSkeleton[memPosition]);
            memPosition+=sizeof(currentCoordinate->y);
            currentCoordinate->z = Client::bytesToInt(&serialSkeleton[memPosition]);
            memPosition+=sizeof(currentCoordinate->z);

            VPtype = serialSkeleton[memPosition];
            memPosition+=sizeof(VPtype);
            inMag = Client::bytesToInt(&serialSkeleton[memPosition]);
            memPosition+=sizeof(inMag);
            time = Client::bytesToInt(&serialSkeleton[memPosition]);
            memPosition+=sizeof(time);

            addNode(CHANGE_MANUAL, nodeID, radius, neuronID, currentCoordinate, VPtype, inMag, time, false, false);
        }
    }

    totalSegmentNumber = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(totalSegmentNumber);
    for(i = 0; i < totalSegmentNumber; i++){
        sourceNodeID = Client::bytesToInt(&serialSkeleton[memPosition]);
        memPosition+=sizeof(sourceNodeID);
        targetNodeID = Client::bytesToInt(&serialSkeleton[memPosition]);
        memPosition+=sizeof(targetNodeID);
        addSegment(CHANGE_MANUAL, sourceNodeID, targetNodeID, false);
    }
    totalCommentNumber = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(totalCommentNumber);
    for(i = 0; i < totalCommentNumber; i++){
        nodeID = Client::bytesToInt(&serialSkeleton[memPosition]);
        memPosition+=sizeof(nodeID);
        currentNode = findNodeByNodeID(nodeID);

        stringLength = Client::bytesToInt(&serialSkeleton[memPosition]);
        memPosition+=sizeof(stringLength);
        memset(temp, '\0', sizeof(temp));
        strncpy(temp, (char *)&serialSkeleton[memPosition], stringLength);
        memPosition+=stringLength;

        if(temp && currentNode) {
            addComment(CHANGE_MANUAL, (char *)temp, currentNode, 0, false);
        }
    }

    totalBranchPointNumber = Client::bytesToInt(&serialSkeleton[memPosition]);
    memPosition+=sizeof(totalBranchPointNumber);
    for (i = 0; i < totalBranchPointNumber; i++){
        nodeID = Client::bytesToInt(&serialSkeleton[memPosition]);
        currentNode = findNodeByNodeID(nodeID);
        if(currentNode){
            pushBranchNode(CHANGE_MANUAL, true, false, currentNode, 0, false);
        }
        memPosition+=sizeof(nodeID);
    }

    if(activeNodeID!=0){
        setActiveNode(CHANGE_MANUAL, NULL, activeNodeID);
    }


    emit setRecenteringPositionSignal(loadedPosition.x, loadedPosition.y, loadedPosition.z);
    Knossos::sendRemoteSignal();

}

void Skeletonizer::deleteLastSerialSkeleton(){
    
    struct serialSkeletonListElement *newLastSerialSkeleton = state->skeletonState->lastSerialSkeleton->previous;
    state->skeletonState->lastSerialSkeleton->next = NULL;
    free(state->skeletonState->lastSerialSkeleton->next);
    state->skeletonState->lastSerialSkeleton->previous = NULL;
    free(state->skeletonState->lastSerialSkeleton->previous);
    state->skeletonState->lastSerialSkeleton = NULL;
    free(state->skeletonState->lastSerialSkeleton);
    state->skeletonState->lastSerialSkeleton = newLastSerialSkeleton;
    state->skeletonState->serialSkeletonCounter--;
}

void Skeletonizer::saveSerializedSkeleton(){
    /*
    struct serialSkeletonListElement *serialSkeleton = NULL;
    serialSkeleton = (serialSkeletonListElement *)malloc(sizeof(*serialSkeleton));
    serialSkeleton->content = (Byte *)serializeSkeleton();

    if (state->skeletonState->serialSkeletonCounter == 0){
        state->skeletonState->firstSerialSkeleton = serialSkeleton;
        state->skeletonState->lastSerialSkeleton = serialSkeleton;
    }
    else{
        state->skeletonState->lastSerialSkeleton->next = serialSkeleton;
        serialSkeleton->previous = state->skeletonState->lastSerialSkeleton;
        state->skeletonState->lastSerialSkeleton = state->skeletonState->lastSerialSkeleton->next;
    }
    state->skeletonState->serialSkeletonCounter++;
    while (state->skeletonState->serialSkeletonCounter > state->skeletonState->maxUndoSteps){
        struct serialSkeletonListElement *newFirstSerialSkeleton = state->skeletonState->firstSerialSkeleton->next;
        state->skeletonState->firstSerialSkeleton->next = NULL;
        free(state->skeletonState->firstSerialSkeleton->next);
        state->skeletonState->firstSerialSkeleton = NULL;
        free(state->skeletonState->firstSerialSkeleton);
        state->skeletonState->firstSerialSkeleton = newFirstSerialSkeleton;
        state->skeletonState->firstSerialSkeleton->previous = NULL;
        free(state->skeletonState->firstSerialSkeleton->previous);
        state->skeletonState->serialSkeletonCounter--;
    }
    */
}

int Skeletonizer::getTreeBlockSize(){

    int treeBlockSize = 0;
    
    if(state->skeletonState->firstTree){
        struct treeListElement *currentTree = state->skeletonState->firstTree;
        treeBlockSize+=sizeof(currentTree->treeID);
        treeBlockSize+=sizeof(currentTree->color.r)+sizeof(currentTree->color.b)+sizeof(currentTree->color.g)+sizeof(currentTree->color.a);
        //Number of nodes, Number of segments
        treeBlockSize+=2*sizeof(uint);
        treeBlockSize*=state->skeletonState->treeElements;
        while(currentTree){
            treeBlockSize+=sizeof(int32_t);
            treeBlockSize+=strlen(currentTree->comment);
            currentTree = currentTree->next;
        }
    } 
    return treeBlockSize;
}

int Skeletonizer::getNodeBlockSize(){
    int nodeBlockSize = 0;
    
    struct nodeListElement *currentNode = NULL;
    nodeBlockSize+=sizeof(currentNode->nodeID);
    nodeBlockSize+=sizeof(currentNode->radius);
    nodeBlockSize+=sizeof(currentNode->position.x);
    nodeBlockSize+=sizeof(currentNode->position.y);
    nodeBlockSize+=sizeof(currentNode->position.z);
    nodeBlockSize+=sizeof(currentNode->createdInMag);
    nodeBlockSize+=sizeof(currentNode->createdInVp);
    nodeBlockSize+=sizeof(currentNode->timestamp);
    return nodeBlockSize;
}

int Skeletonizer::getSegmentBlockSize(){
    int segmentBlockSize = 0;
    struct segmentListElement *currentSegment;
    segmentBlockSize+=sizeof(currentSegment->source);
    segmentBlockSize+=sizeof(currentSegment->target);
    return segmentBlockSize;
}

int Skeletonizer::getCommentBlockSize(){
    int commentBlockSize = 0;
    //Number of comments
    commentBlockSize+=sizeof(uint);
    if(state->skeletonState->currentComment){
        struct commentListElement *currentComment = state->skeletonState->currentComment;
         if(state->skeletonState->currentComment != NULL) {
            do {
                commentBlockSize+=sizeof(currentComment->node->nodeID);
                commentBlockSize+=sizeof(int32_t);
                commentBlockSize+=strlen(currentComment->content);
                currentComment = currentComment->next;
            } while (currentComment != state->skeletonState->currentComment);
        }
    }
    return commentBlockSize;
}

int Skeletonizer::getVariableBlockSize(){
    int variablesBlockSize = 0;
    variablesBlockSize+=sizeof(int32_t);
    variablesBlockSize+=strlen(state->name);
    variablesBlockSize+=sizeof(int32_t);
    variablesBlockSize+=strlen(KVERSION);
    variablesBlockSize+=sizeof(int32_t);
    variablesBlockSize+=strlen(state->skeletonState->skeletonCreatedInVersion);
    variablesBlockSize+=sizeof(int32_t);
    variablesBlockSize+=strlen(state->skeletonState->skeletonLastSavedInVersion);
    variablesBlockSize+=3*sizeof(state->scale.x / state->magnification);
    variablesBlockSize+=3*sizeof(state->offset.x / state->magnification);
    variablesBlockSize+=sizeof(state->skeletonState->lockPositions)+sizeof(state->skeletonState->lockRadius)+sizeof(int32_t)+strlen(state->skeletonState->onCommentLock);
    variablesBlockSize+=sizeof(state->skeletonState->displayMode);
    variablesBlockSize+=sizeof(state->skeletonState->workMode);
    variablesBlockSize+=sizeof(state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + state->time.elapsed());
    variablesBlockSize+=sizeof(state->skeletonState->activeNode->nodeID);
    variablesBlockSize+=3*sizeof(state->viewerState->currentPosition.x);
    variablesBlockSize+=16*sizeof(state->skeletonState->skeletonVpModelView[0]);
    variablesBlockSize+=2*sizeof(state->skeletonState->translateX);
    variablesBlockSize+=3*sizeof(state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel)+sizeof(state->skeletonState->zoomLevel);
    variablesBlockSize+=sizeof(state->skeletonState->idleTime);
    variablesBlockSize+=sizeof(state->viewerState->gui->activeTreeID); // @todo
    variablesBlockSize+=sizeof(state->skeletonState->treeElements);
    return variablesBlockSize;
}

int Skeletonizer::getBranchPointBlockSize(){
    int branchPointBlockSize = 0;
    //Number of branches
    branchPointBlockSize+=sizeof(int);
    branchPointBlockSize+=state->skeletonState->branchStack->elementsOnStack* sizeof(PTRSIZEINT);
    return branchPointBlockSize;
}

void Skeletonizer::undo() {
    qDebug() << " entered : undo";
    /*if(state->skeletonState->serialSkeletonCounter > 0){
        deserializeSkeleton();
        deleteLastSerialSkeleton();*/
    }

}

void Skeletonizer::redo() {

}
