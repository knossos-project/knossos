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

#include "treelistdecorator.h"

#include "scriptengine/proxies/skeletonproxy.h"
#include "skeleton/tree.h"

TreeListDecorator::TreeListDecorator(QObject *parent) :
    QObject(parent)
{
}

int TreeListDecorator::tree_id(treeListElement *self) {
    return self->treeID;
}

QString TreeListDecorator::comment(treeListElement *self) {
    return self->getComment();
}

QColor TreeListDecorator::color(treeListElement *self) {
    return self->color;
}

QList<nodeListElement *> *TreeListDecorator::nodes(treeListElement *self) {
    return self->getNodes();
}

nodeListElement *TreeListDecorator::first_node(treeListElement *self) {
    return self->nodes.empty() ? nullptr : &self->nodes.front();
}

QString TreeListDecorator::static_Tree_help() {
    return QString("A read-only class representing a tree of the KNOSSOS skeleton. Access to attributes only via getter."
                   "\n tree_id() : returns the id of the current tree"
                   "\n first_node() : returns the first node object of the current tree"
                   "\n comment() : returns the comment of the current tree"
                   "\n color() : returns the color object of the current tree"
                   "\n nodes() : returns a list of node objects of the current tree");
}
