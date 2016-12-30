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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

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
        selectObjectForNode(*previousNode);// reselect object of previous active node as it got unselected in setActiveNode during new node creation
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
        selectObjectForNode(node);
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
