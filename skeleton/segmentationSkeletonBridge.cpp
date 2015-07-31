#include "skeletonizer.h"

#include "segmentation/segmentation.h"
#include "skeleton/node.h"
#include "skeleton/tree.h"

const auto subobjectPropertyKey = "subobjectId";

template<typename Func>
void ifsoproperty(nodeListElement & node, Func func) {
    auto soPropertyIt = node.properties.find(subobjectPropertyKey);
    if (soPropertyIt != std::end(node.properties)) {
        const auto subobjectId = soPropertyIt->toULongLong();
        func(subobjectId);
    }
}

void Skeletonizer::selectObjectForNode(nodeListElement & node) {
    ifsoproperty(node, [&](const uint64_t subobjectId){
        auto objIndex = Segmentation::singleton().largestObjectContainingSubobjectId(subobjectId, node.position);
        Segmentation::singleton().clearObjectSelection();
        Segmentation::singleton().selectObject(objIndex);
    });
}

void Skeletonizer::setSubobjectAndMerge(const quint64 nodeId, const quint64 subobjectId) {
    auto & node = *Skeletonizer::singleton().findNodeByNodeID(nodeId);
    setSubobjectAndMerge(node, subobjectId);
}

void Skeletonizer::setSubobjectAndMerge(nodeListElement & node, const quint64 subobjectId) {
    node.properties.insert(subobjectPropertyKey, subobjectId);
    ++node.correspondingTree->subobjectCount[subobjectId];

    auto objIndex = Segmentation::singleton().largestObjectContainingSubobjectId(subobjectId, node.position);
    Segmentation::singleton().selectObject(objIndex);
    Segmentation::singleton().mergeSelectedObjects();
}

void Skeletonizer::updateSubobjectCountFromProperty(nodeListElement & node) {
    ifsoproperty(node, [&](const uint64_t subobjectId){
        ++node.correspondingTree->subobjectCount[subobjectId];
    });
}

void decrementSubobjectCount(nodeListElement & node, const Coordinate & oldPos, const uint64_t oldSubobjectId) {
    if (--node.correspondingTree->subobjectCount[oldSubobjectId] == 0) {
        node.correspondingTree->subobjectCount.remove(oldSubobjectId);

        Segmentation::singleton().selectObjectFromSubObject(oldSubobjectId, oldPos);
        Segmentation::singleton().unmergeSelectedObjects(oldPos);
    }
}

void Skeletonizer::unsetSubobjectOfHybridNode(nodeListElement & node) {
    ifsoproperty(node, [&](const uint64_t oldSubobjectId){
        decrementSubobjectCount(node, node.position, oldSubobjectId);
    });
}

void Skeletonizer::movedHybridNode(nodeListElement & node, const quint64 newSubobjectId, const Coordinate & oldPos) {
    ifsoproperty(node, [&](const uint64_t oldSubobjectId){
        if (oldSubobjectId != newSubobjectId) {
            decrementSubobjectCount(node, oldPos, oldSubobjectId);
            setSubobjectAndMerge(node, newSubobjectId);
        }
    });
}
