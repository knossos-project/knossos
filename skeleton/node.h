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

#ifndef NODE_H
#define NODE_H

#include "property_query.h"
#include "widgets/viewports/viewportbase.h"

#include <QVariantHash>

#include <cstddef>

class segmentListElement;
class treeListElement;
class Synapse;

class nodeListElement : public PropertyQuery {
public:

    static constexpr double MAX_RADIUS_SETTING = 1000000;
    std::uint64_t nodeID;
    float radius;
    Coordinate position;
    int createdInMag;
    ViewportType createdInVp;
    uint64_t timestamp;
    std::list<nodeListElement>::iterator iterator;
    treeListElement * correspondingTree = nullptr;
    Synapse * correspondingSynapse = nullptr;

    std::list<segmentListElement> segments;
    // circumsphere radius - max. of length of all segments and node radius.
    //Used for frustum culling
    float circRadius{radius};
    bool isBranchNode{false};
    bool isSynapticNode{false}; //pre- or postSynapse
    bool selected{false};

    nodeListElement(const decltype(nodeID) nodeID, const decltype(radius) radius, const decltype(position) & position, const decltype(createdInMag) inMag
                    , const decltype(createdInVp) inVP, const decltype(timestamp) ms, const decltype(properties) & properties, decltype(*correspondingTree) & tree);
    bool operator==(const nodeListElement & other) const;

    QList<segmentListElement *> *getSegments();
};

class segmentListElement {
public:
    segmentListElement(nodeListElement & source, nodeListElement & target, const bool forward = true) : source{source}, target{target}, forward(forward) {}

    nodeListElement & source;
    nodeListElement & target;
    const bool forward;
    float length{0.0};
    //reference to the segment inside the target node
    std::list<segmentListElement>::iterator sisterSegment;
};
#endif//NODE_H
