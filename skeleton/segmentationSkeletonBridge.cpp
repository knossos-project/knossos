#include "skeletonizer.h"

#include "skeleton/node.h"
#include "viewer.h"

void Skeletonizer::selectSubobjectNode(const uint64_t objectId, const uint64_t subobjectId, const Coordinate & clickPosition, const int vpid) {
    if (findNodeByNodeID(subobjectId) == nullptr) {
        UI_addSkeletonNode(clickPosition, state->viewerState->vpConfigs[vpid].type, subobjectId);
    }
    auto & node = *findNodeByNodeID(subobjectId);
    selectNodes({&node});
}

void Skeletonizer::linkActiveToSubobjectNode(const uint64_t objectId, const uint64_t subobjectId, const Coordinate & clickPosition, const int vpid) {
    const auto prevActiveNodeId = state->skeletonState->activeNode->nodeID;
    if (findNodeByNodeID(subobjectId) == nullptr) {
        UI_addSkeletonNode(clickPosition, state->viewerState->vpConfigs[vpid].type, subobjectId);
    }
    addSegment(prevActiveNodeId, subobjectId);
}
