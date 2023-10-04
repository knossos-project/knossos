/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2018
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

#pragma once

#include <boost/optional.hpp>

#include <cstdint>

void updateToken(struct Dataset & layer);

void parseGoogleJson(struct Dataset & info);
void createChangeStack(const struct Dataset & dataset);

bool bminvalid(const bool erase, const std::uint64_t srcSvx, const std::uint64_t dstSvx);
void retrieveMeshes(const std::uint64_t soid, const int assert = 0);
void bmmergesplit(const std::uint64_t src_soid, const std::uint64_t dst_soid, const bool undo = false);
bool bmUndoable();
void bmundo();

class QColor brainmapsMeshColor(const std::uint64_t treeID);

#include "coordinate.h"

void brainmapsBranchPoint(const boost::optional<class nodeListElement &> cursorPos, std::uint64_t subobjID, const Coordinate & globalCoord);

void setSplit(const Coordinate & p, const std::uint64_t id);
void splitMe();
void splitMe2();
void splitHightlightToSelection();

void anchorSetNoteRequest(const std::uint64_t anchorid, const QString & note);
void anchorRequest(const std::uint64_t anchorid, const bool add);
void selectAnchorObjects();
void explodeRequest(const std::uint64_t svId);
