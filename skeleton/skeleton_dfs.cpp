#include "skeleton/skeleton_dfs.h"

#include "skeleton/skeletonizer.h"
#include "stateInfo.h"

NodeGenerator::NodeGenerator(nodeListElement & node, const Direction direction) : direction(direction), reachedEnd{false} {
    queuedNodes.emplace(&node, &node);
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
                queuedNodes.emplace(&neighbor, &node);
                queue.emplace_back(&neighbor);
            } else if (queuedNodes.at(&node) != &neighbor) {// not just a backward segment
                auto * cycleBegin = queuedNodes.at(&neighbor);// previous node with that neighbor
                cycle = {cycleBegin, &neighbor, &node};
                auto * predecessor = queuedNodes.at(&node);
                for (; queuedNodes.at(predecessor) != predecessor && predecessor != cycleBegin; predecessor = queuedNodes.at(predecessor)) {
                    cycle.insert(predecessor);
                }
                break;
            }
        }
    }

    if (queue.empty()) {
        reachedEnd = true;
        if (queuedNodes.size() != node.correspondingTree->nodes.size()) {
            for (auto & node : node.correspondingTree->nodes) {
                if (queuedNodes.find(&node) == std::end(queuedNodes)) {
                    queuedNodes.emplace(&node, &node);
                    queue.emplace_back(&node);
                    reachedEnd = false;
                    break;
                }
            }
        }
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
                nodeIter.queuedNodes.emplace(progress->second, progress->second);
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
