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

#include "nodelistdecorator.h"

NodeListDecorator::NodeListDecorator(QObject *parent) : QObject(parent) {}

quint64 NodeListDecorator::node_id(nodeListElement *self) {
    return self->nodeID;
}

bool NodeListDecorator::is_branch_node(nodeListElement *self) {
    return self->isBranchNode;
}

QList<segmentListElement *> *NodeListDecorator::segments(nodeListElement *self) {
    return self->getSegments();
}

QString NodeListDecorator::comment(nodeListElement *self) {
    return self->getComment();
}

int NodeListDecorator::time(nodeListElement *self) {
    return self->timestamp;
}

float NodeListDecorator::radius(nodeListElement *self) {
    return self->radius;
}

treeListElement *NodeListDecorator::parent_tree(nodeListElement *self) {
    return self->correspondingTree;
}

Coordinate NodeListDecorator::coordinate(nodeListElement *self) {
    return self->position;
}

int NodeListDecorator::mag(nodeListElement *self) {
    return self->createdInMag;
}

int NodeListDecorator::viewport(nodeListElement *self) {
    return self->createdInVp;
}

bool NodeListDecorator::selected(nodeListElement *self) {
    return self->selected;
}

QString NodeListDecorator::static_Node_help() {
    return QString("A read-only class representing a tree node of the KNOSSOS skeleton. Access to attributes only via getter." \
            "\n node_id() : returns the id of the current node" \
            "\n radius() : returns the radius of the current node" \
            "\n parent_tree() : returns the parent tree object of the current node" \
            "\n Coordinate() : returns the Coordinate object of the current node " \
            "\n comment() : returns the comment of the current node" \
            "\n is_branch_node() : returns a bool value indicating if the node is a branch node or not\n" \
            "\n viewport() : returns the viewport in which the node was created\n" \
            "\n mag() : returns the magnification in which the node was created\n" \
            "\n time() : returns the timestamp when the node was created");
}
