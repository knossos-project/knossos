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

#ifndef NODELISTDECORATOR_H
#define NODELISTDECORATOR_H

#include "coordinate.h"
#include "skeleton/node.h"
#include "skeleton/tree.h"

#include <QObject>

class NodeListDecorator : public QObject
{
    template<typename, std::size_t> friend class Coord;
    Q_OBJECT
public:
    explicit NodeListDecorator(QObject *parent = 0);

signals:

public slots:

    quint64 node_id(nodeListElement *self);
    QList<segmentListElement *> *segments(nodeListElement *self);
    bool is_branch_node(nodeListElement *self);
    QString comment(nodeListElement *self);
    int time(nodeListElement *self);
    float radius(nodeListElement *self);
    treeListElement *parent_tree(nodeListElement *self);
    Coordinate coordinate(nodeListElement *self);
    int mag(nodeListElement *self);
    int viewport(nodeListElement *self);
    bool selected(nodeListElement *self);
    QString static_Node_help();
};

#endif // NODELISTDECORATOR_H
