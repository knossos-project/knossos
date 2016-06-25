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
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef SKELETON_DFS
#define SKELETON_DFS

#include "skeletonizer.h"

#include <unordered_set>

struct NodeGenerator {
    std::vector<nodeListElement *> queue;
    std::unordered_set<const nodeListElement *> queuedNodes;
    std::vector<nodeListElement *> visitedNodes;
    bool reachedEnd{true};
    bool reachedStart{true};
    NodeGenerator() {}
    NodeGenerator(nodeListElement & node) : reachedEnd{false} {
        queuedNodes.emplace(&node);
        queue.emplace_back(&node);
        visitedNodes.emplace_back(&node);
    }
    bool operator!=(NodeGenerator & other) {
        return reachedEnd != other.reachedEnd;
    }
    NodeGenerator & operator++() {
        if (queue.empty()) {
            throw std::runtime_error{"no more nodes to iterate"};
        }

        auto & node = *queue.back();
        if (visitedNodes.empty() || queue.back() != visitedNodes.back()) {
            visitedNodes.emplace_back(&node);
        }
        queue.pop_back();
        for (auto & segment : node.segments) {
            auto & neighbor = segment.forward ? segment.target : segment.source;
            if (queuedNodes.find(&neighbor) == std::end(queuedNodes)) {
                queuedNodes.emplace(&neighbor);
                queue.emplace_back(&neighbor);
            }
        }

        if (queue.empty()) {
            reachedEnd = true;
        } else {
            visitedNodes.emplace_back(queue.back());
        }
        reachedStart = false;
        return *this;
    }

    NodeGenerator & operator--() {
        if (reachedStart) {
            throw std::runtime_error{"no more nodes to iterate"};
        }
        if (queue.empty() || queue.back() != visitedNodes.back()) {
            queue.emplace_back(visitedNodes.back());
        }
        if (!visitedNodes.empty()) {
            visitedNodes.pop_back();
        }
        if (visitedNodes.empty()) {
            reachedStart = true;
        }
        reachedEnd = false;
        return *this;
    }

    nodeListElement & operator*() {
        return *queue.back();
    }
};

struct TreeTraverser {
    bool reachedEnd{true};
    NodeGenerator nodeIter;
    std::list<treeListElement> * trees;
    decltype(state->skeletonState->nodesByNodeID)::iterator progress;
    TreeTraverser() {}
    TreeTraverser(std::list<treeListElement> & trees, nodeListElement * node = nullptr) : trees{&trees} {
                progress = std::begin(state->skeletonState->nodesByNodeID);
        if (node != nullptr) {
            nodeIter = NodeGenerator(*node);
            reachedEnd = false;
        }
        else {
           next();
        }
    }
    bool operator!=(TreeTraverser & other) {
        return reachedEnd != other.reachedEnd;
    }
    TreeTraverser & operator++() {
        if (reachedEnd) {
            throw std::runtime_error{"no more nodes in tree to iterate"};
        }
        ++nodeIter;
        next();
        return *this;
    }

    TreeTraverser & operator--() {
        if (reachedStart()) {
            throw std::runtime_error{"no more nodes to iterate, sir!"};
        }
        --nodeIter;
        if (reachedEnd) { // previously empty queue might not be empty anymore.
            reachedEnd = nodeIter.reachedEnd;
        }
        return *this;
    }

    void next() {
        if (nodeIter.reachedEnd) {
            for (; progress != std::end(state->skeletonState->nodesByNodeID); ++progress) {
                if (nodeIter.queuedNodes.find(progress->second) == std::end(nodeIter.queuedNodes)) {
                    nodeIter.queuedNodes.emplace(progress->second);
                    nodeIter.queue = {progress->second};
                    nodeIter.visitedNodes.push_back(progress->second);
                    nodeIter.reachedEnd = false;
                    nodeIter.reachedStart = false;
                    break;
                }
            }
            reachedEnd = nodeIter.reachedEnd && progress == std::end(state->skeletonState->nodesByNodeID);
        }
    }

    bool reachedStart() {
        return nodeIter.reachedStart;
    }

    nodeListElement & operator*() {
        return *nodeIter;
    }
};

#endif
