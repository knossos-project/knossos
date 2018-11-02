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

#include "segmentlistdecorator.h"

#include "skeleton/node.h"

SegmentListDecorator::SegmentListDecorator(QObject *parent) : QObject(parent) {}

nodeListElement * SegmentListDecorator::source(segmentListElement *self) {
    return &self->source;
}

nodeListElement * SegmentListDecorator::target(segmentListElement *self) {
    return &self->target;
}

quint64 SegmentListDecorator::source_id(segmentListElement *self) {
    return self->source.nodeID;
}

quint64 SegmentListDecorator::target_id(segmentListElement *self) {
    return self->target.nodeID;
}

QString SegmentListDecorator::static_Segment_help() {
    return QString("the read-only class representing a connection between tree nodes of the KNOSSOS skeleton. Access to attributes only via getter and setter." \
                   "source() : returns the source node of the segment" \
                   "source_id() : returns the source node id of the segment" \
                   "target() : returns the target node of the segment" \
                   "target_id() : returns the target node id of the segment");
}
