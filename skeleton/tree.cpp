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
 *  or contact knossosteam@gmail.com
 */

#include "tree.h"

#include "node.h"

#include <QString>

treeListElement::treeListElement(const decltype(treeID) id, const decltype(PropertyQuery::properties) & properties) : PropertyQuery{properties}, treeID{id} {}

QList<nodeListElement *> *treeListElement::getNodes() {
    QList<nodeListElement *> * nodes = new QList<nodeListElement *>();

    for (auto & node : this->nodes) {
        nodes->append(&node);
    }

    return nodes;
}

QList<segmentListElement *> *treeListElement::getSegments() {
    QList<segmentListElement *> *complete_segments = new QList<segmentListElement *>();

    for(auto & node : nodes) {
        for(auto & segment : node.segments) {
            if(!complete_segments->contains(&segment)) {
                complete_segments->append(&segment);
            }
        }
    }
    return complete_segments;
}
