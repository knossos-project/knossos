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

#include "node.h"

#include <coordinate.h>

nodeListElement::nodeListElement(const decltype(nodeID) nodeID, const decltype(radius) radius, const decltype(position) & position, const decltype(createdInMag) inMag
                                 , const decltype(createdInVp) inVP, const decltype(timestamp) ms, const decltype(properties) & properties, decltype(*correspondingTree) & tree)
        : PropertyQuery{properties}, nodeID{nodeID}, radius{radius}, position{position}, createdInMag{inMag}, createdInVp{inVP}, timestamp{ms}, correspondingTree{&tree} {}

bool nodeListElement::operator==(const nodeListElement & other) const {
    return nodeID == other.nodeID;
}

QList<segmentListElement *> *nodeListElement::getSegments() {
    QList<segmentListElement *> * segmentList = new QList<segmentListElement *>();
    for (auto & segment : segments) {
        segmentList->append(&segment);
    }
    return segmentList;
}
