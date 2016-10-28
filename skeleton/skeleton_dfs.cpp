#include "skeleton/skeleton_dfs.h"
#include "skeleton/skeletonizer.h"

NodeGenerator::NodeGenerator(nodeListElement & node, const Direction direction) : direction(direction), reachedEnd{false} {
    queuedNodes.emplace(&node);
    queue.emplace_back(&node);
}

bool NodeGenerator::operator!=(NodeGenerator & other) {
    return reachedEnd != other.reachedEnd;
}

NodeGenerator & NodeGenerator::operator++() {
    if (queue.empty()) {
        throw std::runtime_error{"no more nodes to iterate"};
    }

    auto & node = *queue.back();
    queue.pop_back();
    for (auto & segment : node.segments) {
        if ((direction == Direction::Forward && segment.forward) ||
            (direction == Direction::Backward && segment.forward == false) ||
            direction == Direction::Any) {
            auto & neighbor = segment.forward ? segment.target : segment.source;
            if (queuedNodes.find(&neighbor) == std::end(queuedNodes)) {
                queuedNodes.emplace(&neighbor);
                queue.emplace_back(&neighbor);
            }
        }
    }

    if (queue.empty()) {
        reachedEnd = true;
    }
    return *this;
}

nodeListElement & NodeGenerator::operator*() {
    return *queue.back();
}

TreeTraverser::TreeTraverser(std::list<treeListElement> & trees, nodeListElement * node) : trees{&trees} {
            progress = std::begin(state->skeletonState->nodesByNodeID);
    if (node != nullptr) {
        nodeIter = NodeGenerator(*node, NodeGenerator::Direction::Any);
        reachedEnd = false;
    }
    else {
       next();
    }
}

bool TreeTraverser::operator!=(TreeTraverser & other) {
    return reachedEnd != other.reachedEnd;
}

TreeTraverser & TreeTraverser::operator++() {
    if (reachedEnd) {
        throw std::runtime_error{"no more nodes in tree to iterate"};
    }
    ++nodeIter;
    next();
    return *this;
}

void TreeTraverser::next() {
    if (nodeIter.reachedEnd) {
        for (; progress != std::end(state->skeletonState->nodesByNodeID); ++progress) {
            if (nodeIter.queuedNodes.find(progress->second) == std::end(nodeIter.queuedNodes)) {
                nodeIter.queuedNodes.emplace(progress->second);
                nodeIter.queue = {progress->second};
                nodeIter.reachedEnd = false;
                break;
            }
        }
        reachedEnd = nodeIter.reachedEnd && progress == std::end(state->skeletonState->nodesByNodeID);
    }
}

nodeListElement & TreeTraverser::operator*() {
    return *nodeIter;
}
