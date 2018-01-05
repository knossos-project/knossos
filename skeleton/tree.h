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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef TREE_H
#define TREE_H

#include "mesh/mesh.h"
#include "node.h"
#include "property_query.h"

#include <QColor>
#include <QHash>
#include <QList>

#include <list>
#include <memory>

class segmentListElement;

class treeListElement : public PropertyQuery {
public:
    std::uint64_t treeID;
    std::list<treeListElement>::iterator iterator;

    std::list<nodeListElement> nodes;
    std::unique_ptr<Mesh> mesh;

    bool render{true};
    bool isSynapticCleft{false};
    Synapse * correspondingSynapse;
    QColor color;
    bool selected{false};
    bool colorSetManually{false};

    QHash<uint64_t, int> subobjectCount;

    treeListElement(const decltype(treeID) id, const decltype(PropertyQuery::properties) & properties);

    QList<nodeListElement *> *getNodes();
    QList<segmentListElement *> *getSegments();
};

#endif// TREE_H
