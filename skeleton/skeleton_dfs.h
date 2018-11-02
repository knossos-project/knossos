/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef SKELETON_DFS
#define SKELETON_DFS

#include "skeleton/node.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>

struct NodeGenerator {
    enum class Direction { Forward, Backward, Any };
    std::vector<nodeListElement *> queue;
    std::unordered_set<const nodeListElement *> queuedNodes;
    Direction direction{Direction::Any};
    bool reachedEnd{true};
    NodeGenerator() = default;
    NodeGenerator(nodeListElement & node, const Direction direction);
    bool operator!=(NodeGenerator & other);
    NodeGenerator & operator++();
    nodeListElement & operator*();
};

struct TreeTraverser {
    bool reachedEnd{true};
    NodeGenerator nodeIter;
    std::list<treeListElement> * trees;
    std::unordered_map<decltype(nodeListElement::nodeID), nodeListElement *>::iterator progress;
    TreeTraverser() = default;
    TreeTraverser(std::list<treeListElement> & trees, nodeListElement * node = nullptr);
    bool operator!=(TreeTraverser & other);
    TreeTraverser & operator++();
    nodeListElement & operator*();
    void next();
};

#endif
