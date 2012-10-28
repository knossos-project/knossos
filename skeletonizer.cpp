#include "skeletonizer.h"
#include "knossos-global.h"
#include "renderer.h"
extern stateInfo *state;

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

    Renderer *r = new Renderer();

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

    segVectorLength = r->euclidicNorm(&segVector);
    r->normalizeVector(&segVector);

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
        currentSegVectorPoint.x = curMagSourcePos.x + r->roundFloat(((float)i) * segVector.x);
        currentSegVectorPoint.y = curMagSourcePos.y + r->roundFloat(((float)i) * segVector.y);
        currentSegVectorPoint.z = curMagSourcePos.z + r->roundFloat(((float)i) * segVector.z);

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

    segVectorLength = r->euclidicNorm(&segVector);
    r->normalizeVector(&segVector);

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
        currentSegVectorPoint.x = curMagSourcePos.x + r->roundFloat(((float)i) * segVector.x);
        currentSegVectorPoint.y = curMagSourcePos.y + r->roundFloat(((float)i) * segVector.y);
        currentSegVectorPoint.z = curMagSourcePos.z + r->roundFloat(((float)i) * segVector.z);

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

bool Skeletonizer::initSkeletonizer() { }
bool Skeletonizer::UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype) { }
uint32_t Skeletonizer::UI_addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int32_t makeNodeActive) { }
bool Skeletonizer::updateSkeletonState() { }
bool Skeletonizer::nextCommentlessNode() { }
bool Skeletonizer::previousCommentlessNode() { }

bool Skeletonizer::updateSkeletonFileName() { }
//uint32_t saveNMLSkeleton() { }
int32_t Skeletonizer::saveSkeleton() { }
//uint32_t loadNMLSkeleton() { }
bool Skeletonizer::loadSkeleton() { }

void Skeletonizer::setDefaultSkelFileName() { }

bool Skeletonizer::delActiveNode() { }
bool Skeletonizer::delActiveTree() { }

bool Skeletonizer::delSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID, segmentListElement *segToDel) { }
bool Skeletonizer::delNode(int32_t targetRevision, int32_t nodeID, nodeListElement *nodeToDel) { }
bool Skeletonizer::delTree(int32_t targetRevision, int32_t treeID) { }

nodeListElement* Skeletonizer::findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition) { }
nodeListElement* Skeletonizer::findNodeInRadius(Coordinate searchPosition) { }

bool Skeletonizer::setActiveTreeByID(int32_t treeID) { }
bool Skeletonizer::setActiveNode(int32_t targetRevision, nodeListElement *node, int32_t nodeID) { }
int32_t Skeletonizer::addNode(int32_t targetRevision,
                int32_t nodeID,
                float radius,
                int32_t treeID,
                Coordinate *position,
                Byte VPType,
                int32_t inMag,
                int32_t time,
                int32_t respectLocks) { }
bool Skeletonizer::addSegment(int32_t targetRevision, int32_t sourceNodeID, int32_t targetNodeID) { }

bool Skeletonizer::clearSkeleton(int32_t targetRevision, int loadingSkeleton) { }

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
