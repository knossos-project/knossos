#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <GL/gl.h>

#include "skeletonizer.h"
#include "knossos-global.h"
#include "renderer.h"
#include "remote.h"
#include "gui.h"
#include "knossos.h"
#include "client.h"
#include "viewer.h"

extern stateInfo *state;
extern stateInfo *tempConfig;

static nodeListElement *addNodeListElement(
              int32_t nodeID,
              float radius,
              nodeListElement **currentNode,
              Coordinate *position,
              int32_t inMag) {

    nodeListElement *newElement = NULL;

    /*
     * This skeleton modifying function does not lock the skeleton and therefore
     * has to be called from functions that do lock and NEVER directly.
     *
     */

    newElement = (nodeListElement*) malloc(sizeof(struct nodeListElement));
    if(newElement == NULL) {
        LOG("Out of memory while trying to allocate memory for a new nodeListElement.");
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
    newElement->nodeID = nodeID;
    newElement->isBranchNode = FALSE;
    newElement->createdInMag = inMag;

    return newElement;
}

static segmentListElement* addSegmentListElement (segmentListElement **currentSegment,
    nodeListElement *sourceNode,
    nodeListElement *targetNode) {
    segmentListElement *newElement = NULL;

    /*
    * This skeleton modifying function does not lock the skeleton and therefore
    * has to be called from functions that do lock and NEVER directly.
    */

    newElement = (segmentListElement*) malloc(sizeof(struct segmentListElement));
    if(newElement == NULL) {
        LOG("Out of memory while trying to allocate memory for a new segmentListElement.");
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


static treeListElement* findTreeByTreeID(int32_t treeID) {
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

static bool addNodeToSkeletonStruct(nodeListElement *node) {
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

    return TRUE;
}

static bool addSegmentToSkeletonStruct(segmentListElement *segment) {
    uint32_t i;
    skeletonDC *currentSkeletonDC;
    skeletonDCsegment *currentNewSkeletonDCsegment;
    float segVectorLength = 0.;
    floatCoordinate segVector;
    Coordinate currentSegVectorPoint, lastSegVectorPoint;
    Coordinate curMagTargetPos, curMagSourcePos;

    if(segment) {
        if(segment->flag == 2)
            return FALSE;
    }
    else
        return FALSE;

    curMagTargetPos.x = segment->target->position.x / state->magnification;
    curMagTargetPos.y = segment->target->position.y / state->magnification;
    curMagTargetPos.z = segment->target->position.z / state->magnification;
    curMagSourcePos.x = segment->source->position.x / state->magnification;
    curMagSourcePos.y = segment->source->position.y / state->magnification;
    curMagSourcePos.z = segment->source->position.z / state->magnification;

    segVector.x = (float)(curMagTargetPos.x - curMagSourcePos.x);
    segVector.y = (float)(curMagTargetPos.y - curMagSourcePos.y);
    segVector.z = (float)(curMagTargetPos.z - curMagSourcePos.z);

    segVectorLength = Renderer::euclidicNorm(&segVector);
    Renderer::normalizeVector(&segVector);

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
        LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.");
        return FALSE;
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
        currentSegVectorPoint.x = curMagSourcePos.x + Renderer::roundFloat(((float)i) * segVector.x);
        currentSegVectorPoint.y = curMagSourcePos.y + Renderer::roundFloat(((float)i) * segVector.y);
        currentSegVectorPoint.z = curMagSourcePos.z + Renderer::roundFloat(((float)i) * segVector.z);

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

    return TRUE;
}

static bool delNodeFromSkeletonStruct(nodeListElement *node) {
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
        LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.");
        return FALSE;
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

    return TRUE;
}

static bool delSegmentFromSkeletonStruct(segmentListElement *segment) {
    uint32_t i;
    skeletonDC *currentSkeletonDC;
    skeletonDCsegment *lastSkeletonDCsegment, *currentSkeletonDCsegment;
    float segVectorLength = 0.;
    floatCoordinate segVector;
    Coordinate currentSegVectorPoint, lastSegVectorPoint;
    Coordinate curMagTargetPos, curMagSourcePos;
    Renderer *r = new Renderer();

    if(segment) {
        if(segment->flag == 2) {
            return FALSE;
        }
    }
    else {
        return FALSE;
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

    segVectorLength = Renderer::euclidicNorm(&segVector);
    Renderer::normalizeVector(&segVector);

    SET_COORDINATE(lastSegVectorPoint,
                   curMagSourcePos.x,
                   curMagSourcePos.y,
                   curMagSourcePos.z);

    //Is there a skeleton DC for the first skeleton DC from which the segment has to be removed?
    currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                        Coordinate::Px2DcCoord(curMagSourcePos));
    if(currentSkeletonDC == HT_FAILURE) {
        //A skeleton DC is missing
        LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.");
        return FALSE;
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
        currentSegVectorPoint.x = curMagSourcePos.x + Renderer::roundFloat(((float)i) * segVector.x);
        currentSegVectorPoint.y = curMagSourcePos.y + Renderer::roundFloat(((float)i) * segVector.y);
        currentSegVectorPoint.z = curMagSourcePos.z + Renderer::roundFloat(((float)i) * segVector.z);

        if(!COMPARE_COORDINATE(Coordinate::Px2DcCoord(lastSegVectorPoint), Coordinate::Px2DcCoord(currentSegVectorPoint))) {
            //We crossed a skeleton DC boundary, now we have to delete the pointer to the segment inside the skeleton DC
            currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                     Coordinate::Px2DcCoord(currentSegVectorPoint));
            if(currentSkeletonDC == HT_FAILURE) {
                //A skeleton DC is missing
                LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.");
                return FALSE;
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

    return TRUE;
}

/*static void WRAP_popBranchNode() {
    popBranchNode(CHANGE_MANUAL);
    state->skeletonState->askingPopBranchConfirmation = FALSE;
}*/

static void popBranchNodeCanceled() {
    state->skeletonState->askingPopBranchConfirmation = FALSE;
}

bool Skeletonizer::initSkeletonizer() {
    if(state->skeletonState->skeletonDCnumber != tempConfig->skeletonState->skeletonDCnumber)
        state->skeletonState->skeletonDCnumber = tempConfig->skeletonState->skeletonDCnumber;

    //updateSkeletonState();

    //Create a new hash-table that holds the skeleton datacubes
    state->skeletonState->skeletonDCs = Hashtable::ht_new(state->skeletonState->skeletonDCnumber);
    if(state->skeletonState->skeletonDCs == HT_FAILURE) {
        LOG("Unable to create skeleton hash-table.");
        return FALSE;
    }

    state->skeletonState->skeletonRevision = 0;

    state->skeletonState->nodeCounter = newDynArray(128);
    state->skeletonState->nodesByNodeID = newDynArray(1048576);
    state->skeletonState->branchStack = newStack(2048);

    // Generate empty tree structures
    state->skeletonState->firstTree = NULL;
    state->skeletonState->treeElements = 0;
    state->skeletonState->totalNodeElements = 0;
    state->skeletonState->totalSegmentElements = 0;
    state->skeletonState->activeTree = NULL;
    state->skeletonState->activeNode = NULL;

    state->skeletonState->mergeOnLoadFlag = 0;
    state->skeletonState->segRadiusToNodeRadius = 0.5;
    state->skeletonState->autoFilenameIncrementBool = TRUE;
    state->skeletonState->greatestNodeID = 0;

    state->skeletonState->showXYplane = FALSE;
    state->skeletonState->showXZplane = FALSE;
    state->skeletonState->showYZplane = FALSE;
    state->skeletonState->showNodeIDs = FALSE;
    state->skeletonState->highlightActiveTree = TRUE;
    state->skeletonState->rotateAroundActiveNode = TRUE;
    state->skeletonState->showIntersections = FALSE;

    state->skeletonState->displayListSkeletonSkeletonizerVP = 0;
    state->skeletonState->displayListView = 0;
    state->skeletonState->displayListDataset = 0;

    state->skeletonState->defaultNodeRadius = 1.5;
    state->skeletonState->overrideNodeRadiusBool = FALSE;
    state->skeletonState->overrideNodeRadiusVal = 1.;

    state->skeletonState->currentComment = NULL;

    state->skeletonState->lastSaveTicks = 0;
    state->skeletonState->autoSaveInterval = 5;

    state->skeletonState->skeletonFile = (char*) malloc(8192 * sizeof(char));
    memset(state->skeletonState->skeletonFile, '\0', 8192 * sizeof(char));
    setDefaultSkelFileName();

    state->skeletonState->prevSkeletonFile = (char*) malloc(8192 * sizeof(char));
    memset(state->skeletonState->prevSkeletonFile, '\0', 8192 * sizeof(char));

    state->skeletonState->commentBuffer = (char*) malloc(10240 * sizeof(char));
    memset(state->skeletonState->commentBuffer, '\0', 10240 * sizeof(char));

    state->skeletonState->searchStrBuffer = (char*) malloc(2048 * sizeof(char));
    memset(state->skeletonState->searchStrBuffer, '\0', 2048 * sizeof(char));

    state->skeletonState->saveCnt = 0;

    if((state->boundary.x >= state->boundary.y) && (state->boundary.x >= state->boundary.z))
        state->skeletonState->volBoundary = state->boundary.x * 2;
    if((state->boundary.y >= state->boundary.x) && (state->boundary.y >= state->boundary.y))
        state->skeletonState->volBoundary = state->boundary.y * 2;
    if((state->boundary.z >= state->boundary.x) && (state->boundary.z >= state->boundary.y))
        state->skeletonState->volBoundary = state->boundary.x * 2;

    state->skeletonState->viewChanged = TRUE;
    state->skeletonState->skeletonChanged = TRUE;
    state->skeletonState->datasetChanged = TRUE;
    state->skeletonState->skeletonSliceVPchanged = TRUE;
    state->skeletonState->unsavedChanges = FALSE;

    state->skeletonState->askingPopBranchConfirmation = FALSE;

    return TRUE;
}

bool Skeletonizer::UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype) {
    int32_t addedNodeID;

    color4F treeCol;
    /* -1 causes new color assignment */
    treeCol.r = -1.;
    treeCol.g = -1.;
    treeCol.b = -1.;
    treeCol.a = 1.;

    if(!state->skeletonState->activeTree)
        addTreeListElement(TRUE, CHANGE_MANUAL, 0, treeCol);

    addedNodeID = addNode(CHANGE_MANUAL,
                          0,
                          state->skeletonState->defaultNodeRadius,
                          state->skeletonState->activeTree->treeID,
                          clickedCoordinate,
                          VPtype,
                          state->magnification,
                          -1,
                          TRUE);
    if(!addedNodeID) {
        LOG("Error: Could not add new node!");
        return FALSE;
    }

    setActiveNode(CHANGE_MANUAL, NULL, addedNodeID);

    if((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                               state->skeletonState->activeTree->treeID) == 1) {
        /* First node in this tree */

        pushBranchNode(CHANGE_MANUAL, TRUE, TRUE, NULL, addedNodeID);
        addComment(CHANGE_MANUAL, "First Node", NULL, addedNodeID);
    }

    Remote::checkIdleTime();

    return TRUE;
}

uint32_t Skeletonizer::UI_addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int32_t makeNodeActive) {
    int32_t targetNodeID;

    if(!state->skeletonState->activeNode) {
        LOG("Please create a node before trying to link nodes.");
        tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
        return FALSE;
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
                           TRUE);
    if(!targetNodeID) {
        LOG("Could not add new node while trying to add node and link with active node!");
        return FALSE;
    }

    addSegment(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, targetNodeID);

    if(makeNodeActive) {
        setActiveNode(CHANGE_MANUAL, NULL, targetNodeID);
    }
    if((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                               state->skeletonState->activeTree->treeID) == 1) {
        /* First node in this tree */

        pushBranchNode(CHANGE_MANUAL, TRUE, TRUE, NULL, targetNodeID);
        addComment(CHANGE_MANUAL, "First Node", NULL, targetNodeID);

        Remote::checkIdleTime();
    }

    return targetNodeID;
}

bool Skeletonizer::updateSkeletonState() {
    //Time to auto-save
    if(state->skeletonState->autoSaveBool || state->clientState->saveMaster) {
        if(state->skeletonState->autoSaveInterval) {
            if((SDL_GetTicks() - state->skeletonState->lastSaveTicks) / 60000 >= state->skeletonState->autoSaveInterval) {
                state->skeletonState->lastSaveTicks = SDL_GetTicks();
                GUI::UI_saveSkeleton(TRUE);
            }
        }
    }

    if(state->skeletonState->skeletonDCnumber != tempConfig->skeletonState->skeletonDCnumber) {
        state->skeletonState->skeletonDCnumber = tempConfig->skeletonState->skeletonDCnumber;
    }
    if(state->skeletonState->workMode != tempConfig->skeletonState->workMode) {
        setSkeletonWorkMode(CHANGE_MANUAL, tempConfig->skeletonState->workMode);
    }
    return TRUE;
}
bool Skeletonizer::nextCommentlessNode() { }
bool Skeletonizer::previousCommentlessNode() { }

bool Skeletonizer::updateSkeletonFileName(int32_t targetRevision, int32_t increment, char *filename) {
    int32_t extensionDelim = -1, countDelim = -1;
    char betweenDots[8192];
    char count[8192];
    char origFilename[8192], skeletonFileBase[8192];
    int32_t i, j;

    /* This is a SYNCHRONIZABLE skeleton function. Be a bit careful. */

    if(Knossos::lockSkeleton(targetRevision) == FALSE) {
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
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
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(TRUE);

    return TRUE;
}

//uint32_t saveNMLSkeleton() { }

int32_t Skeletonizer::saveSkeleton() {
    struct treeListElement *currentTree = NULL;
    struct nodeListElement *currentNode = NULL;
    PTRSIZEINT currentBranchPointID;
    struct segmentListElement *currentSegment = NULL;
    struct commentListElement *currentComment = NULL;
    struct stack *reverseBranchStack = NULL, *tempReverseStack = NULL;
    int32_t r;
    xmlChar attrString[128];

    xmlDocPtr xmlDocument;
    xmlNodePtr thingsXMLNode, nodesXMLNode, edgesXMLNode, currentXMLNode,
               paramsXMLNode, branchesXMLNode, commentsXMLNode;

    memset(attrString, '\0', 128 * sizeof(xmlChar));

    /*
     *  This function should always be called through UI_saveSkeleton
     *  for proper error and file name display to the user.
     */

    /*
     *  We need to do this to be able to save the branch point stack to a file
     *  and still have the branch points available to the user afterwards.
     *
     */

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
        pushBranchNode(CHANGE_MANUAL, FALSE, FALSE, currentNode, 0);
    }

    xmlDocument = xmlNewDoc(BAD_CAST"1.0");
    thingsXMLNode = xmlNewDocNode(xmlDocument, NULL, BAD_CAST"things", NULL);
    xmlDocSetRootElement(xmlDocument, thingsXMLNode);

    paramsXMLNode = xmlNewTextChild(thingsXMLNode, NULL, BAD_CAST"parameters", NULL);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"experiment", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%s", state->name);
    xmlNewProp(currentXMLNode, BAD_CAST"name", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"lastsavedin", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%s", KVERSION);
    xmlNewProp(currentXMLNode, BAD_CAST"version", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"createdin", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%s", state->skeletonState->skeletonCreatedInVersion);
    xmlNewProp(currentXMLNode, BAD_CAST"version", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"scale", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->scale.x / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"x", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->scale.y / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"y", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->scale.z / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"z", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"offset", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->offset.x / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"x", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->offset.y / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"y", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->offset.z / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"z", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"time", NULL);
    xmlStrPrintf(attrString,
                 128,
                 BAD_CAST"%d",
                 state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + SDL_GetTicks());
    xmlNewProp(currentXMLNode, BAD_CAST"ms", attrString);
    memset(attrString, '\0', 128);

    if(state->skeletonState->activeNode) {
        currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"activeNode", NULL);
        xmlStrPrintf(attrString,
                     128,
                     BAD_CAST"%d",
                     state->skeletonState->activeNode->nodeID);
        xmlNewProp(currentXMLNode, BAD_CAST"id", attrString);
        memset(attrString, '\0', 128);
    }

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"editPosition", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->viewerState->currentPosition.x + 1);
    xmlNewProp(currentXMLNode, BAD_CAST"x", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->viewerState->currentPosition.y + 1);
    xmlNewProp(currentXMLNode, BAD_CAST"y", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->viewerState->currentPosition.z + 1);
    xmlNewProp(currentXMLNode, BAD_CAST"z", attrString);
    memset(attrString, '\0', 128);
    {
        int j = 0;
        char element [8];
        currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"skeletonVPState", NULL);
        for (j = 0; j < 16; j++){
            sprintf (element, "E%d", j);
            xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->skeletonState->skeletonVpModelView[j]);
            xmlNewProp(currentXMLNode, BAD_CAST(element), attrString);
            memset(attrString, '\0', 128);
        }
    }
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->skeletonState->translateX);
    xmlNewProp(currentXMLNode, BAD_CAST"translateX", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->skeletonState->translateY);
    xmlNewProp(currentXMLNode, BAD_CAST"translateY", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"vpSettingsZoom", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->viewerState->viewPorts[VIEWPORT_XY].texture.zoomLevel);
    xmlNewProp(currentXMLNode, BAD_CAST"XYPlane", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->viewerState->viewPorts[VIEWPORT_XZ].texture.zoomLevel);
    xmlNewProp(currentXMLNode, BAD_CAST"XZPlane", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->viewerState->viewPorts[VIEWPORT_YZ].texture.zoomLevel);
    xmlNewProp(currentXMLNode, BAD_CAST"YZPlane", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->skeletonState->zoomLevel);
    xmlNewProp(currentXMLNode, BAD_CAST"SkelVP", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"idleTime", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%i", state->skeletonState->idleTime);
    xmlNewProp(currentXMLNode, BAD_CAST"ms", attrString);
    memset(attrString, '\0', 128);

    currentTree = state->skeletonState->firstTree;
    if((currentTree == NULL) && (state->skeletonState->currentComment == NULL)) {
        return FALSE; //No Skeleton to save
    }

    while(currentTree) {
        /*
         * Every "thing" has associated nodes and edges.
         *
         */
        currentXMLNode = xmlNewTextChild(thingsXMLNode, NULL, BAD_CAST"thing", NULL);
        xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentTree->treeID);
        xmlNewProp(currentXMLNode, BAD_CAST"id", attrString);
        memset(attrString, '\0', 128);
        if(currentTree->colorSetManually) {
            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentTree->color.r);
            xmlNewProp(currentXMLNode, BAD_CAST"color.r", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentTree->color.g);
            xmlNewProp(currentXMLNode, BAD_CAST"color.g", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentTree->color.b);
            xmlNewProp(currentXMLNode, BAD_CAST"color.b", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentTree->color.a);
            xmlNewProp(currentXMLNode, BAD_CAST"color.a", attrString);
            memset(attrString, '\0', 128);
        }
        else {
            xmlStrPrintf(attrString, 128, BAD_CAST"%f", -1.);
            xmlNewProp(currentXMLNode, BAD_CAST"color.r", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", -1.);
            xmlNewProp(currentXMLNode, BAD_CAST"color.g", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", -1.);
            xmlNewProp(currentXMLNode, BAD_CAST"color.b", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", 1.);
            xmlNewProp(currentXMLNode, BAD_CAST"color.a", attrString);
            memset(attrString, '\0', 128);
        }
        memset(attrString, '\0', 128);
        xmlNewProp(currentXMLNode, BAD_CAST"comment", (xmlChar*)currentTree->comment);

        nodesXMLNode = xmlNewTextChild(currentXMLNode, NULL, BAD_CAST"nodes", NULL);
        edgesXMLNode = xmlNewTextChild(currentXMLNode, NULL, BAD_CAST"edges", NULL);

        currentNode = currentTree->firstNode;
        while(currentNode) {
            currentXMLNode = xmlNewTextChild(nodesXMLNode, NULL, BAD_CAST"node", NULL);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->nodeID);
            xmlNewProp(currentXMLNode, BAD_CAST"id", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentNode->radius);
            xmlNewProp(currentXMLNode, BAD_CAST"radius", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->position.x + 1);
            xmlNewProp(currentXMLNode, BAD_CAST"x", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->position.y + 1);
            xmlNewProp(currentXMLNode, BAD_CAST"y", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->position.z + 1);
            xmlNewProp(currentXMLNode, BAD_CAST"z", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->createdInVp);
            xmlNewProp(currentXMLNode, BAD_CAST"inVp", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->createdInMag);
            xmlNewProp(currentXMLNode, BAD_CAST"inMag", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->timestamp);
            xmlNewProp(currentXMLNode, BAD_CAST"time", attrString);
            memset(attrString, '\0', 128);

            currentSegment = currentNode->firstSegment;
            while(currentSegment) {
                if(currentSegment->flag == SEGMENT_FORWARD) {
                    currentXMLNode = xmlNewTextChild(edgesXMLNode, NULL, BAD_CAST"edge", NULL);

                    xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentSegment->source->nodeID);
                    xmlNewProp(currentXMLNode, BAD_CAST"source", attrString);
                    memset(attrString, '\0', 128);

                    xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentSegment->target->nodeID);
                    xmlNewProp(currentXMLNode, BAD_CAST"target", attrString);
                    memset(attrString, '\0', 128);
                }

                currentSegment = currentSegment->next;
            }

            currentNode = currentNode->next;
        }

        currentTree = currentTree->next;
    }


    commentsXMLNode = xmlNewTextChild(thingsXMLNode, NULL, BAD_CAST"comments", NULL);
    currentComment = state->skeletonState->currentComment;
    if(state->skeletonState->currentComment != NULL) {
        do {
            currentXMLNode = xmlNewTextChild(commentsXMLNode, NULL, BAD_CAST"comment", NULL);
            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentComment->node->nodeID);
            xmlNewProp(currentXMLNode, BAD_CAST"node", attrString);
            memset(attrString, '\0', 128);
            xmlNewProp(currentXMLNode, BAD_CAST"content", BAD_CAST currentComment->content);
            currentComment = currentComment->next;
        } while (currentComment != state->skeletonState->currentComment);
    }


    branchesXMLNode = xmlNewTextChild(thingsXMLNode, NULL, BAD_CAST"branchpoints", NULL);
    while((currentBranchPointID = (PTRSIZEINT)popStack(reverseBranchStack))) {
        currentXMLNode = xmlNewTextChild(branchesXMLNode,
                                         NULL,
                                         BAD_CAST"branchpoint",
                                         NULL);

#ifdef ARCH_64
        xmlStrPrintf(attrString, 128, BAD_CAST"%"PRId64, currentBranchPointID);
#else
        xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentBranchPointID);
#endif

        xmlNewProp(currentXMLNode, BAD_CAST"id", attrString);
        memset(attrString, '\0', 128);
    }

    r = xmlSaveFormatFile(state->skeletonState->skeletonFile, xmlDocument, 1);
    xmlFreeDoc(xmlDocument);
    return r;
}
//uint32_t loadNMLSkeleton() { }

bool Skeletonizer::loadSkeleton() {
xmlDocPtr xmlDocument;
    xmlNodePtr currentXMLNode, thingsXMLNode, thingOrParamXMLNode, nodesEdgesXMLNode;
    int32_t neuronID = 0, nodeID = 0, merge = FALSE;
    int32_t nodeID1, nodeID2, greatestNodeIDbeforeLoading = 0, greatestTreeIDbeforeLoading = 0;
    float radius;
    Byte VPtype;
    int32_t inMag, magnification = 0;
    int32_t globalMagnificationSpecified = FALSE;
    xmlChar *attribute;
    struct treeListElement *currentTree;
    struct nodeListElement *currentNode = NULL;
    Coordinate *currentCoordinate, loadedPosition;
    Coordinate offset;
    floatCoordinate scale;
    int32_t time, activeNodeID = 0;
    int32_t skeletonTime = 0;
    color4F neuronColor;

    LOG("Starting to load skeleton...");

    /*
     *  This function should always be called through UI_loadSkeleton for
     *  proper error and file name display to the user.
     */

    /*
     *  There's no sanity check for values read from files, yet.
     */

    /*
     *  These defaults should usually always be overridden by values
     *  given in the file.
     */

    SET_COORDINATE(offset, state->offset.x, state->offset.y, state->offset.z);
    SET_COORDINATE(scale, state->scale.x, state->scale.y, state->scale.z);
    SET_COORDINATE(loadedPosition, 0, 0, 0);

    currentCoordinate = (Coordinate*) malloc(sizeof(Coordinate));
    if(currentCoordinate == NULL) {
        LOG("Out of memory.");
        return FALSE;
    }
    memset(currentCoordinate, '\0', sizeof(currentCoordinate));

    xmlDocument = xmlParseFile(state->skeletonState->skeletonFile);

    if(xmlDocument == NULL) {
        LOG("Document not parsed successfully.");
        return FALSE;
    }

    thingsXMLNode = xmlDocGetRootElement(xmlDocument);
    if(thingsXMLNode == NULL) {
        LOG("Empty document.");
        xmlFreeDoc(xmlDocument);
        return FALSE;
    }

    if(xmlStrEqual(thingsXMLNode->name, (const xmlChar *)"things") == FALSE) {
        LOG("Root element %s in file %s unrecognized. Not loading.",
            thingsXMLNode->name,
            state->skeletonState->skeletonFile);
        return FALSE;
    }

    if(!state->skeletonState->mergeOnLoadFlag) {
        merge = FALSE;
        clearSkeleton(CHANGE_MANUAL, TRUE);
    }
    else {
        merge = TRUE;
        greatestNodeIDbeforeLoading = state->skeletonState->greatestNodeID;
        greatestTreeIDbeforeLoading = state->skeletonState->greatestTreeID;
    }

    /* If "createdin"-node does not exist, skeleton was created in a version
     * before 3.2 */
    strcpy(state->skeletonState->skeletonCreatedInVersion, "pre-3.2");

    thingOrParamXMLNode = thingsXMLNode->xmlChildrenNode;
    while(thingOrParamXMLNode) {
        if(xmlStrEqual(thingOrParamXMLNode->name, (const xmlChar *)"parameters")) {
            currentXMLNode = thingOrParamXMLNode->children;
            while(currentXMLNode) {
                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"createdin")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"version");
                    if(attribute){
                        strcpy(state->skeletonState->skeletonCreatedInVersion, (char *)attribute);
                    }
                }
                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"magnification")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"factor");
                    /*
                     * This is for legacy skeleton files.
                     * In the past, magnification was specified on a per-file basis
                     * or not specified at all.
                     * A magnification factor of 0 shall represent an unknown magnification.
                     *
                     */
                    if(attribute) {
                        magnification = atoi((char *)attribute);
                        globalMagnificationSpecified = TRUE;
                    }
                    else
                        magnification = 0;
                }
                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"offset")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"x");
                    if(attribute)
                        offset.x = atoi((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"y");
                    if(attribute)
                        offset.y = atoi((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"z");
                    if(attribute)
                        offset.z = atoi((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"time")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"ms");
                    if(attribute)
                        skeletonTime = atoi((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"activeNode")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"id");
                    if(attribute)
                        activeNodeID = atoi((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"scale")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"x");
                    if(attribute)
                        scale.x = atof((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"y");
                    if(attribute)
                        scale.y = atof((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"z");
                    if(attribute)
                        scale.z = atof((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"editPosition")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"x");
                    if(attribute)
                        loadedPosition.x = atoi((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"y");
                    if(attribute)
                        loadedPosition.y = atoi((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"z");
                    if(attribute)
                        loadedPosition.z = atoi((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"skeletonVPState")) {
                    int j = 0;
                    char element [8];
                    for (j = 0; j < 16; j++){
                        sprintf (element, "E%d", j);
                        attribute = xmlGetProp(currentXMLNode, (const xmlChar *)element);
                        state->skeletonState->skeletonVpModelView[j] = atof((char *)attribute);
                    }
                    glMatrixMode(GL_MODELVIEW);
                    glLoadMatrixf(state->skeletonState->skeletonVpModelView);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"translateX");
                    state->skeletonState->translateX = atof((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"translateY");
                    state->skeletonState->translateY = atof((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"vpSettingsZoom")) {

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"XYPlane");
                    if(attribute)
                        state->viewerState->viewPorts[VIEWPORT_XY].texture.zoomLevel = atof((char *)attribute);
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"XZPlane");
                    if(attribute)
                        state->viewerState->viewPorts[VIEWPORT_XZ].texture.zoomLevel = atof((char *)attribute);
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"YZPlane");
                    if(attribute)
                        state->viewerState->viewPorts[VIEWPORT_YZ].texture.zoomLevel = atof((char *)attribute);
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"SkelVP");
                    if(attribute)
                        state->skeletonState->zoomLevel = atof((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"idleTime")) {

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"ms");
                    if(attribute)
                        state->skeletonState->idleTime = atof((char *)attribute);
                    state->skeletonState->idleTimeNow = SDL_GetTicks();
                    state->skeletonState->idleTimeLoadOffset =atof((char *)attribute);
                }
                state->skeletonState->idleTimeTicksOffset = SDL_GetTicks();
                currentXMLNode = currentXMLNode->next;
            }
        }

        if(xmlStrEqual(thingOrParamXMLNode->name,
                       (const xmlChar *)"comments")) {

            currentXMLNode = thingOrParamXMLNode->children;
            while(currentXMLNode) {
                if(xmlStrEqual(currentXMLNode->name,
                               (const xmlChar *)"comment")) {

                    attribute = xmlGetProp(currentXMLNode,
                                           (const xmlChar *)"node");
                    if(attribute) {
                        if(!merge)
                            nodeID = atoi((char *)attribute);
                        else
                            nodeID = atoi((char *)attribute) + greatestNodeIDbeforeLoading;

                        currentNode = findNodeByNodeID(nodeID);
                    }
                    attribute = xmlGetProp(currentXMLNode,
                                           (const xmlChar *)"content");
                    if(attribute && currentNode) {
                        addComment(CHANGE_MANUAL, (char *)attribute, currentNode, 0);
                    }
                }

                currentXMLNode = currentXMLNode->next;
            }
        }


        if(xmlStrEqual(thingOrParamXMLNode->name,
                       (const xmlChar *)"branchpoints")) {

            currentXMLNode = thingOrParamXMLNode->children;
            while(currentXMLNode) {
                if(xmlStrEqual(currentXMLNode->name,
                               (const xmlChar *)"branchpoint")) {

                    attribute = xmlGetProp(currentXMLNode,
                                           (const xmlChar *)"id");
                    if(attribute) {
                        if(!merge)
                            nodeID = atoi((char *)attribute);
                        else
                            nodeID = atoi((char *)attribute) + greatestNodeIDbeforeLoading;

                        currentNode = findNodeByNodeID(nodeID);
                        if(currentNode)
                            pushBranchNode(CHANGE_MANUAL, TRUE, FALSE, currentNode, 0);
                    }
                }

                currentXMLNode = currentXMLNode->next;
            }
        }

        if(xmlStrEqual(thingOrParamXMLNode->name, (const xmlChar *)"thing")) {
            /* Add tree */
            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "id");
            if(attribute) {
                neuronID = atoi((char *)attribute);
                free(attribute);
            }
            else
                neuronID = 0;   /* Whatever. */

            /* -1. causes default color assignment based on ID*/
            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "color.r");
            if(attribute) {
                neuronColor.r = (float)strtod((char *)attribute, (char **)NULL);
                free(attribute);
            }
            else {
                neuronColor.r = -1.;
            }

            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "color.g");
            if(attribute) {
                neuronColor.g = (float)strtod((char *)attribute, (char **)NULL);
                free(attribute);
            }
            else
                neuronColor.g = -1.;

            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "color.b");
            if(attribute) {
                neuronColor.b = (float)strtod((char *)attribute, (char **)NULL);
                free(attribute);
            }
            else
                neuronColor.b = -1.;

            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "color.a");
            if(attribute) {
                neuronColor.a = (float)strtod((char *)attribute, (char **)NULL);
                free(attribute);
            }
            else
                neuronColor.a = 1.;

            if(!merge) {
                currentTree = addTreeListElement(TRUE, CHANGE_MANUAL, neuronID, neuronColor);
                setActiveTreeByID(neuronID);
            }
            else {
                neuronID += greatestTreeIDbeforeLoading;
                currentTree = addTreeListElement(TRUE, CHANGE_MANUAL, neuronID, neuronColor);
                setActiveTreeByID(currentTree->treeID);
                neuronID = currentTree->treeID;
            }

            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *)"comment");
            if(attribute) {
                addTreeComment(CHANGE_MANUAL, currentTree->treeID, (char *)attribute);
                free(attribute);
            }

            nodesEdgesXMLNode = thingOrParamXMLNode->children;
            while(nodesEdgesXMLNode) {
                if(xmlStrEqual(nodesEdgesXMLNode->name, (const xmlChar *)"nodes")) {
                    currentXMLNode = nodesEdgesXMLNode->children;
                    while(currentXMLNode) {
                        if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"node")) {
                            /* Add node. */

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"id");
                            if(attribute) {
                                nodeID = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                nodeID = 0;

                            LOG("Adding node: %d", nodeID);
                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"radius");
                            if(attribute) {
                                radius = (float)strtod((char *)attribute, (char **)NULL);
                                free(attribute);
                            }
                            else
                                radius = state->skeletonState->defaultNodeRadius;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"x");
                            if(attribute) {
                                currentCoordinate->x = atoi((char *)attribute) - 1;
                                if(globalMagnificationSpecified)
                                    currentCoordinate->x = currentCoordinate->x * magnification;
                                free(attribute);
                            }
                            else
                                currentCoordinate->x = 0;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"y");
                            if(attribute) {
                                currentCoordinate->y = atoi((char *)attribute) - 1;
                                if(globalMagnificationSpecified)
                                    currentCoordinate->y = currentCoordinate->y * magnification;
                                free(attribute);
                            }
                            else
                                currentCoordinate->y = 0;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"z");
                            if(attribute) {
                                currentCoordinate->z = atoi((char *)attribute) - 1;
                                if(globalMagnificationSpecified)
                                    currentCoordinate->z = currentCoordinate->z * magnification;
                                free(attribute);
                            }
                            else
                                currentCoordinate->z = 0;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"inVp");
                            if(attribute) {
                                VPtype = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                VPtype = VIEWPORT_UNDEFINED;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"inMag");
                            if(attribute) {
                                inMag = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                inMag = magnification; /* For legacy skeleton files */

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"time");
                            if(attribute) {
                                time = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                time = skeletonTime; /* For legacy skeleton files */

                            if(!merge)
                                addNode(CHANGE_MANUAL, nodeID, radius, neuronID, currentCoordinate, VPtype, inMag, time, FALSE);
                            else {
                                nodeID += greatestNodeIDbeforeLoading;
                                addNode(CHANGE_MANUAL, nodeID, radius, neuronID, currentCoordinate, VPtype, inMag, time, FALSE);
                            }
                        }

                        currentXMLNode = currentXMLNode->next;
                    }
                }
                else if(xmlStrEqual(nodesEdgesXMLNode->name, (const xmlChar *)"edges")) {
                    currentXMLNode = nodesEdgesXMLNode->children;
                    while(currentXMLNode) {
                        if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"edge")) {
                            /* Add edge */
                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"source");
                            if(attribute) {
                                nodeID1 = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                nodeID1 = 0;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"target");
                            if(attribute) {
                                nodeID2 = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                nodeID2 = 0;

                            // printf("Trying to add a segment between %d and %d\n", nodeID1, nodeID2);
                            if(!merge)
                                addSegment(CHANGE_MANUAL, nodeID1, nodeID2);
                            else
                                addSegment(CHANGE_MANUAL, nodeID1 + greatestNodeIDbeforeLoading, nodeID2 + greatestNodeIDbeforeLoading);

                        }
                        currentXMLNode = currentXMLNode->next;
                    }
                }

                nodesEdgesXMLNode = nodesEdgesXMLNode->next;
            }
        }

        thingOrParamXMLNode = thingOrParamXMLNode->next;
    }

    free(currentCoordinate);
    xmlFreeDoc(xmlDocument);

    GUI::addRecentFile(state->skeletonState->skeletonFile, FALSE);

    if(activeNodeID) {
        setActiveNode(CHANGE_MANUAL, NULL, activeNodeID);
    }

    if((loadedPosition.x != 0) &&
       (loadedPosition.y != 0) &&
       (loadedPosition.z != 0)) {
        tempConfig->viewerState->currentPosition.x =
            loadedPosition.x - 1;
        tempConfig->viewerState->currentPosition.y =
            loadedPosition.y - 1;
        tempConfig->viewerState->currentPosition.z =
            loadedPosition.z - 1;

        Viewer::updatePosition(TELL_COORDINATE_CHANGE);
    }
    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
    state->skeletonState->skeletonTime = skeletonTime;
    state->skeletonState->skeletonTimeCorrection = SDL_GetTicks();
    return TRUE;
}

void Skeletonizer::setDefaultSkelFileName() {
    /* Generate a default file name based on date and time. */
    time_t curtime;
    struct tm *localtimestruct;

    curtime = time(NULL);
    localtimestruct = localtime(&curtime);
    if(localtimestruct->tm_year >= 100)
        localtimestruct->tm_year -= 100;

#ifdef LINUX
    snprintf(state->skeletonState->skeletonFile,
            8192,
            "skeletonFiles/skeleton-%.2d%.2d%.2d-%.2d%.2d.000.nml",
            localtimestruct->tm_mday,
            localtimestruct->tm_mon + 1,
            localtimestruct->tm_year,
            localtimestruct->tm_hour,
            localtimestruct->tm_min);
#else
    snprintf(state->skeletonState->skeletonFile,
            8192,
            "skeletonFiles\\skeleton-%.2d%.2d%.2d-%.2d%.2d.000.nml",
            localtimestruct->tm_mday,
            localtimestruct->tm_mon + 1,
            localtimestruct->tm_year,
            localtimestruct->tm_hour,
            localtimestruct->tm_min);
#endif
    GUI::cpBaseDirectory(state->viewerState->ag->skeletonDirectory, state->skeletonState->skeletonFile, 2048);
}

bool Skeletonizer::delActiveNode() {
    if(state->skeletonState->activeNode) {
        delNode(CHANGE_MANUAL, 0, state->skeletonState->activeNode);
    }
    else {
        return FALSE;
    }

    return TRUE;
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

        delTree(CHANGE_MANUAL, state->skeletonState->activeTree->treeID);

        if(nextTree) {
            setActiveTreeByID(nextTree->treeID);
        }
        else {
            state->skeletonState->activeTree = NULL;
        }
    }
    else {
       LOG("No active tree available.");
       return FALSE;
    }

    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE;

    return TRUE;
}

bool Skeletonizer::delSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID, segmentListElement *segToDel) {
    /* This is a SYNCHRONIZABLE skeleton function. Be a bit careful. */

    /*
     * Delete the segment out of the segment list and out of the visualization structure!
     */

    if(Knossos::lockSkeleton(targetRevision) == FALSE) {
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    if(!segToDel)
        segToDel = findSegmentByNodeIDs(sourceNodeID, targetNodeID);
    else {
        sourceNodeID = segToDel->source->nodeID;
        targetNodeID = segToDel->target->nodeID;
    }

    if(!segToDel) {
        LOG("Cannot delete segment, no segment with corresponding node IDs available!");
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }


    //Out of skeleton structure
    delSegmentFromSkeletonStruct(segToDel);

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

    free(segToDel);
    state->skeletonState->totalSegmentElements--;

    state->skeletonState->skeletonChanged = TRUE;
    state->skeletonState->unsavedChanges = TRUE;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brdd", KIKI_DELSEGMENT, sourceNodeID, targetNodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(TRUE);

    return TRUE;
}

bool Skeletonizer::delNode(int32_t targetRevision, int32_t nodeID, nodeListElement *nodeToDel) {
    /* This is a SYNCHRONIZABLE skeleton function. Be a bit careful. */

    struct segmentListElement *currentSegment;
    struct segmentListElement *tempNext = NULL;
    struct nodeListElement *newActiveNode = NULL;
    int32_t treeID;

    if(Knossos::lockSkeleton(targetRevision) == FALSE) {
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    if(!nodeToDel)
        nodeToDel = findNodeByNodeID(nodeID);

    nodeID = nodeToDel->nodeID;

    if(!nodeToDel) {
        LOG("The given node %d doesn't exist. Unable to delete it.", nodeID);
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    treeID = nodeToDel->correspondingTree->treeID;

    if(nodeToDel->comment) {
        delComment(CHANGE_MANUAL, nodeToDel->comment, 0);
    }

    /*
     * First, delete all segments pointing towards and away of the nodeToDelhas
     * been */

    currentSegment = nodeToDel->firstSegment;

    while(currentSegment) {

        tempNext = currentSegment->next;

        if(currentSegment->flag == SEGMENT_FORWARD)
            delSegment(CHANGE_MANUAL, 0,0, currentSegment);
        else if(currentSegment->flag == SEGMENT_BACKWARD)
            delSegment(CHANGE_MANUAL, 0,0, currentSegment->reverseSegment);

        currentSegment = tempNext;
    }

    nodeToDel->firstSegment = NULL;

    /*
     * Delete the node out of the visualization structure
     */

    delNodeFromSkeletonStruct(nodeToDel);

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
    state->viewerState->ag->totalNodes--;

    setDynArray(state->skeletonState->nodeCounter,
            treeID,
            (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter, treeID) - 1));

    state->skeletonState->skeletonChanged = TRUE;
    state->skeletonState->unsavedChanges = TRUE;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_DELNODE, nodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }

    Knossos::unlockSkeleton(TRUE);
    return TRUE;
}

bool Skeletonizer::delTree(int32_t targetRevision, int32_t treeID) {
    /* This is a SYNCHRONIZABLE skeleton function. Be a bit careful. */

    struct treeListElement *currentTree;
    struct nodeListElement *currentNode, *nodeToDel;

    if(Knossos::lockSkeleton(targetRevision) == FALSE) {
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    currentTree = findTreeByTreeID(treeID);
    if(!currentTree) {
        LOG("There exists no tree with ID %d. Unable to delete it.", treeID);
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    currentNode = currentTree->firstNode;
    while(currentNode) {
        nodeToDel = currentNode;
        currentNode = nodeToDel->next;
        delNode(CHANGE_MANUAL, 0, nodeToDel);
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

    state->skeletonState->skeletonChanged = TRUE;
    state->skeletonState->unsavedChanges = TRUE;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_DELTREE, treeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(TRUE);

    return TRUE;
}

nodeListElement* Skeletonizer::findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition) {
    nodeListElement *currentNode = NULL;
    nodeListElement *nodeWithCurrentlySmallestDistance = NULL;
    treeListElement *currentTree = NULL;
    floatCoordinate distanceVector;
    float smallestDistance = 0;

    /*
     *  If available, search for a node within nearbyTree first.
     */

    if(nearbyTree) {
        currentNode = nearbyTree->firstNode;

        while(currentNode) {
            // We make the nearest node the next active node
            distanceVector.x = (float)searchPosition.x - (float)currentNode->position.x;
            distanceVector.y = (float)searchPosition.y - (float)currentNode->position.y;
            distanceVector.z = (float)searchPosition.z - (float)currentNode->position.z;
            //set nearest distance to distance to first node found, then to distance of any nearer node found.
            if(smallestDistance == 0 || Renderer::euclidicNorm(&distanceVector) < smallestDistance) {
                smallestDistance = Renderer::euclidicNorm(&distanceVector);
                nodeWithCurrentlySmallestDistance = currentNode;
            }
            currentNode = currentNode->next;
        }

        if(nodeWithCurrentlySmallestDistance) {
            return nodeWithCurrentlySmallestDistance;
        }
    }

    /*
     * Ok, we didn't find any node in nearbyTree.
     * Now we take the nearest node, independent of the tree it belongs to.
     */

    currentTree = state->skeletonState->firstTree;
    smallestDistance = 0;
    while(currentTree) {
        currentNode = currentTree->firstNode;

        while(currentNode) {
            //We make the nearest node the next active node
            distanceVector.x = (float)searchPosition.x - (float)currentNode->position.x;
            distanceVector.y = (float)searchPosition.y - (float)currentNode->position.y;
            distanceVector.z = (float)searchPosition.z - (float)currentNode->position.z;

            if(smallestDistance == 0 || Renderer::euclidicNorm(&distanceVector) < smallestDistance) {
                smallestDistance = Renderer::euclidicNorm(&distanceVector);
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

            if(Renderer::euclidicNorm(&distanceVector) < smallestDistance) {
                smallestDistance = Renderer::euclidicNorm(&distanceVector);
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

bool Skeletonizer::setActiveTreeByID(int32_t treeID) {
    treeListElement *currentTree;

    currentTree = findTreeByTreeID(treeID);
    if(!currentTree) {
        LOG("There exists no tree with ID %d!", treeID);
        return FALSE;
    }

    state->skeletonState->activeTree = currentTree;
    state->skeletonState->skeletonChanged = TRUE;
    state->skeletonState->unsavedChanges = TRUE;

    state->viewerState->ag->activeTreeID = currentTree->treeID;
    return TRUE;
}

bool Skeletonizer::setActiveNode(int32_t targetRevision, nodeListElement *node, int32_t nodeID) {
/* This is a SYNCHRONIZABLE skeleton function. Be a bit careful. */

    /*
     * If both *node and nodeID are specified, nodeID wins.
     * If neither *node nor nodeID are specified
     * (node == NULL and nodeID == 0), the active node is
     * set to NULL.
     *
     */
    if(targetRevision != CHANGE_NOSYNC) {
        if(Knossos::lockSkeleton(targetRevision) == FALSE) {
            Knossos::unlockSkeleton(FALSE);
            return FALSE;
        }
    }

    if(nodeID != 0) {
        node = findNodeByNodeID(nodeID);
        if(!node) {
            LOG("No node with id %d available.", nodeID);
            Knossos::unlockSkeleton(FALSE);
            return FALSE;
        }
    }

    if(node) {
        nodeID = node->nodeID;
    }


    state->skeletonState->activeNode = node;
    state->skeletonState->viewChanged = TRUE;
    state->skeletonState->skeletonChanged = TRUE;

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

    state->skeletonState->unsavedChanges = TRUE;

    if(targetRevision != CHANGE_NOSYNC) {
        state->skeletonState->skeletonRevision++;

        if(targetRevision == CHANGE_MANUAL) {
            if(!Client::syncMessage("brd", KIKI_SETACTIVENODE, nodeID)) {
                Client::skeletonSyncBroken();
            }
        }
        else {
            Viewer::refreshViewports();
        }
        Knossos::unlockSkeleton(TRUE);
    }

    if(node) {
        state->viewerState->ag->activeNodeID = node->nodeID;
    }


    /*
     * Calling drawGUI() here leads to a crash during synchronizationn.
     * Why? TDItem
     *
     */
    // drawGUI();

    return TRUE;
}

int32_t Skeletonizer::addNode(int32_t targetRevision,
                int32_t nodeID,
                float radius,
                int32_t treeID,
                Coordinate *position,
                Byte VPtype,
                int32_t inMag,
                int32_t time,
                int32_t respectLocks) {
    /* This is a SYNCHRONIZABLE skeleton function. Be a bit careful. */
    nodeListElement *tempNode = NULL;
    treeListElement *tempTree = NULL;
    floatCoordinate lockVector;
    int32_t lockDistance = 0;

    if(Knossos::lockSkeleton(targetRevision) == FALSE) {
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    state->skeletonState->branchpointUnresolved = FALSE;

    /*
     * respectLocks refers to locking the position to a specific coordinate such as to
     * disallow tracing areas further than a certain distance away from a specific point in the
     * dataset.
     */

    if(respectLocks) {
        if(state->skeletonState->positionLocked) {
            lockVector.x = (float)position->x - (float)state->skeletonState->lockedPosition.x;
            lockVector.y = (float)position->y - (float)state->skeletonState->lockedPosition.y;
            lockVector.z = (float)position->z - (float)state->skeletonState->lockedPosition.z;

            lockDistance = Renderer::euclidicNorm(&lockVector);
            if(lockDistance > state->skeletonState->lockRadius) {
                LOG("Node is too far away from lock point (%d), not adding.", lockDistance);
                Knossos::unlockSkeleton(FALSE);
                return FALSE;
            }
        }
    }

    if(nodeID) {
        tempNode = findNodeByNodeID(nodeID);
    }

    if(tempNode) {
        LOG("Node with ID %d already exists, no node added.", nodeID);
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }
    tempTree = findTreeByTreeID(treeID);

    if(!tempTree) {
        LOG("There exists no tree with the provided ID %d!", treeID);
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
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

    if(time == -1) {
        time = state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + SDL_GetTicks();
    }

    tempNode->timestamp = time;

    setDynArray(state->skeletonState->nodesByNodeID, nodeID, (void *)tempNode);

    //printf("Added node %p, id %d, tree %p\n", tempNode, tempNode->nodeID,
    //        tempNode->correspondingTree);

    //Add a pointer to the node in the skeleton DC structure
    addNodeToSkeletonStruct(tempNode);
    state->skeletonState->skeletonChanged = TRUE;

    if(nodeID > state->skeletonState->greatestNodeID) {
        state->skeletonState->greatestNodeID = nodeID;
    }
    state->skeletonState->skeletonRevision++;
    state->skeletonState->unsavedChanges = TRUE;

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
        Viewer::refreshViewports();
    }

    Knossos::unlockSkeleton(TRUE);

    return nodeID;
}

bool Skeletonizer::addSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID) {
    nodeListElement *targetNode, *sourceNode;
    segmentListElement *sourceSeg;

    /* This is a SYNCHRONIZABLE skeleton function. Be a bit careful. */

    if(Knossos::lockSkeleton(targetRevision) == FALSE) {
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    if(findSegmentByNodeIDs(sourceNodeID, targetNodeID)) {
        LOG("Segment between nodes %d and %d exists already.", sourceNodeID, targetNodeID);
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    //Check if source and target nodes are existant
    sourceNode = findNodeByNodeID(sourceNodeID);
    targetNode = findNodeByNodeID(targetNodeID);

    if(!(sourceNode) || !(targetNode)) {
        LOG("Could not link the nodes, because at least one is missing!");
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    if(sourceNode == targetNode) {
        LOG("Cannot link node with itself!");
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    //One segment more in all trees
    state->skeletonState->totalSegmentElements++;

    /*
     * Add the segment to the tree structure
    */

    sourceSeg = addSegmentListElement(&(sourceNode->firstSegment), sourceNode, targetNode);
    sourceSeg->reverseSegment = addSegmentListElement(&(targetNode->firstSegment), sourceNode, targetNode);

    sourceSeg->reverseSegment->flag = SEGMENT_BACKWARD;

    sourceSeg->reverseSegment->reverseSegment = sourceSeg;

    /*
     * Add the segment to the skeleton DC structure
    */

    addSegmentToSkeletonStruct(sourceSeg);

    // printf("added segment for nodeID %d: %d, %d, %d -> nodeID %d: %d, %d, %d\n", sourceNode->nodeID, sourceNode->position.x + 1, sourceNode->position.y + 1, sourceNode->position.z + 1, targetNode->nodeID, targetNode->position.x + 1, targetNode->position.y + 1, targetNode->position.z + 1);

    state->skeletonState->skeletonChanged = TRUE;
    state->skeletonState->unsavedChanges = TRUE;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brdd", KIKI_ADDSEGMENT, sourceNodeID, targetNodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(TRUE);

    return TRUE;
}

bool Skeletonizer::clearSkeleton(int32_t targetRevision, int loadingSkeleton) {
    /* This is a SYNCHRONIZABLE skeleton function. Be a bit careful. */

    treeListElement *currentTree, *treeToDel;

    if(Knossos::lockSkeleton(targetRevision) == FALSE) {
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    currentTree = state->skeletonState->firstTree;
    while(currentTree) {
        treeToDel = currentTree;
        currentTree = treeToDel->next;
        delTree(CHANGE_MANUAL, treeToDel->treeID);
    }

    state->skeletonState->activeNode = NULL;
    state->skeletonState->activeTree = NULL;

    state->skeletonState->skeletonTime = 0;
    state->skeletonState->skeletonTimeCorrection = SDL_GetTicks();

    Hashtable::ht_rmtable(state->skeletonState->skeletonDCs);
    delDynArray(state->skeletonState->nodeCounter);
    delDynArray(state->skeletonState->nodesByNodeID);
    delStack(state->skeletonState->branchStack);

    //Create a new hash-table that holds the skeleton datacubes
    state->skeletonState->skeletonDCs = Hashtable::ht_new(state->skeletonState->skeletonDCnumber);
    if(state->skeletonState->skeletonDCs == HT_FAILURE) {
        LOG("Unable to create skeleton hash-table.");
        Knossos::unlockSkeleton(FALSE);
        return FALSE;
    }

    //Generate empty tree structures
    state->skeletonState->firstTree = NULL;
    state->skeletonState->totalNodeElements = 0;
    state->skeletonState->totalSegmentElements = 0;
    state->skeletonState->treeElements = 0;
    state->skeletonState->activeTree = NULL;
    state->skeletonState->activeNode = NULL;

    if(loadingSkeleton == FALSE) {
        setDefaultSkelFileName();
    }

    state->skeletonState->nodeCounter = newDynArray(128);
    state->skeletonState->nodesByNodeID = newDynArray(1048576);
    state->skeletonState->branchStack = newStack(2048);

    tempConfig->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;

    state->skeletonState->skeletonRevision++;
    state->skeletonState->unsavedChanges = TRUE;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("br", KIKI_CLEARSKELETON)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    state->skeletonState->skeletonRevision = 0;

    Knossos::unlockSkeleton(TRUE);

    return TRUE;
}

bool Skeletonizer::mergeTrees(int32_t targetRevision, int32_t treeID1, int32_t treeID2) { }

nodeListElement* Skeletonizer::getNodeWithPrevID(nodeListElement *currentNode) { }
nodeListElement* Skeletonizer::getNodeWithNextID(nodeListElement *currentNode) { }
nodeListElement* Skeletonizer::findNodeByNodeID(int32_t nodeID) { }
nodeListElement* Skeletonizer::findNodeByCoordinate(Coordinate *position) { }
treeListElement* Skeletonizer::addTreeListElement(int32_t sync, int32_t targetRevision, int32_t treeID, color4F color) { }
treeListElement* Skeletonizer::getTreeWithPrevID(treeListElement *currentTree) { }
treeListElement* Skeletonizer::getTreeWithNextID(treeListElement *currentTree) { }
bool Skeletonizer::addTreeComment(int32_t targetRevision, int32_t treeID, char *comment) { }
segmentListElement* Skeletonizer::findSegmentByNodeIDs(int32_t sourceNodeID, int32_t targetNodeID) { }
bool Skeletonizer::genTestNodes(uint32_t number) { }
bool Skeletonizer::editNode(int32_t targetRevision,
                 int32_t nodeID,
                 nodeListElement *node,
                 float newRadius,
                 int32_t newXPos,
                 int32_t newYPos,
                 int32_t newZPos,
                 int32_t inMag) { }
void* Skeletonizer::popStack(stack *stack) { }
bool Skeletonizer::pushStack(stack *stack, void *element) { }
stack* Skeletonizer::newStack(int32_t size) { }
bool Skeletonizer::delStack(stack *stack) { }
bool Skeletonizer::delDynArray(dynArray *array) { }
void* Skeletonizer::getDynArray(dynArray *array, int32_t pos) { }
bool Skeletonizer::setDynArray(dynArray *array, int32_t pos, void *value) { }
dynArray* Skeletonizer::newDynArray(int32_t size) { }
int32_t Skeletonizer::splitConnectedComponent(int32_t targetRevision, int32_t nodeID) { }
bool Skeletonizer::addComment(int32_t targetRevision, char *content, nodeListElement *node, int32_t nodeID) { }
bool Skeletonizer::delComment(int32_t targetRevision, commentListElement *currentComment, int32_t commentNodeID) { }
bool Skeletonizer::editComment(int32_t targetRevision, commentListElement *currentComment, int32_t nodeID, char *newContent, nodeListElement *newNode, int32_t newNodeID) { }
commentListElement* Skeletonizer::nextComment(char *searchString) { }
commentListElement* Skeletonizer::previousComment(char *searchString) { }
bool Skeletonizer::searchInComment(char *searchString, commentListElement *comment) { }
bool Skeletonizer::unlockPosition() { }
bool Skeletonizer::lockPosition(Coordinate lockCoordinate) { }
bool Skeletonizer::popBranchNode(int32_t targetRevision) { }
bool Skeletonizer::pushBranchNode(int32_t targetRevision, int32_t setBranchNodeFlag, int32_t checkDoubleBranchpoint, nodeListElement *branchNode, int32_t branchNodeID) { }
bool Skeletonizer::setSkeletonWorkMode(int32_t targetRevision, uint32_t workMode) { }
bool Skeletonizer::jumpToActiveNode() { }
void Skeletonizer::UI_popBranchNode() { }
void Skeletonizer::restoreDefaultTreeColor() { }
bool Skeletonizer::updateTreeColors() {}
