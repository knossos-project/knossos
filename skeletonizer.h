static struct nodeListElement *addNodeListElement(int32_t nodeID, float radius, struct nodeListElement **firstNode, Coordinate *position, int32_t inMag, struct stateInfo *state);
static struct segmentListElement *addSegmentListElement(struct segmentListElement **firstSegment, struct nodeListElement *sourceNode, struct nodeListElement *targetNode, struct stateInfo *state);
static struct treeListElement *findTreeByTreeID(int32_t treeID, struct stateInfo *state);
static uint32_t addNodeToSkeletonStruct(struct nodeListElement *node, struct stateInfo *state);
static uint32_t addSegmentToSkeletonStruct(struct segmentListElement *segment, struct stateInfo *state);
static uint32_t delNodeFromSkeletonStruct(struct nodeListElement *node, struct stateInfo *state);
static uint32_t delSegmentFromSkeletonStruct(struct segmentListElement *segment, struct stateInfo *state);
static void WRAP_popBranchNode();
static void popBranchNodeCanceled();
