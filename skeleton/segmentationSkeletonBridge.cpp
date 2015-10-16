#include "skeletonizer.h"

#include "segmentation/segmentation.h"
#include "skeleton/node.h"
#include "skeleton/tree.h"

const auto subobjectPropertyKey = "subobjectId";

template<typename Func>
void ifsoproperty(const nodeListElement & node, Func func) {
    auto soPropertyIt = node.properties.find(subobjectPropertyKey);
    if (soPropertyIt != std::end(node.properties)) {
        const auto subobjectId = soPropertyIt->toULongLong();
        func(subobjectId);
    }
}

void Skeletonizer::selectObjectForNode(const nodeListElement & node) {
    Segmentation::singleton().clearObjectSelection();//no property clears selection
    ifsoproperty(node, [&](const uint64_t subobjectId){
        auto objIndex = Segmentation::singleton().largestObjectContainingSubobjectId(subobjectId, node.position);
        Segmentation::singleton().selectObject(objIndex);
    });
}

void Skeletonizer::setSubobject(nodeListElement & node, const quint64 subobjectId) {
    node.properties.insert(subobjectPropertyKey, subobjectId);
    ++node.correspondingTree->subobjectCount[subobjectId];
}

void setSubobjectSelectAndMerge(nodeListElement & node, const quint64 subobjectId) {
    node.properties.insert(subobjectPropertyKey, subobjectId);
    ++node.correspondingTree->subobjectCount[subobjectId];

    auto objIndex = Segmentation::singleton().largestObjectContainingSubobjectId(subobjectId, node.position);
    Segmentation::singleton().selectObject(objIndex);
    Segmentation::singleton().mergeSelectedObjects();
}

void Skeletonizer::setSubobjectSelectAndMergeWithPrevious(nodeListElement & node, const quint64 subobjectId, nodeListElement * previousNode) {
    if (previousNode != nullptr) {
        selectObjectForNode(*previousNode);//reselect the previous active node as it got unselected during new node creation
    }
    setSubobjectSelectAndMerge(node, subobjectId);
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
            setSubobjectSelectAndMerge(node, newSubobjectId);
        }
    });
}
