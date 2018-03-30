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

#include "commentsetting.h"

bool CommentSetting::useCommentNodeColor;
bool CommentSetting::useCommentNodeRadius;
bool CommentSetting::appendComment;
std::vector<CommentSetting> CommentSetting::comments;

QColor CommentSetting::getColor(const QString & comment) {
    for(const auto & item : comments) {
        if(!item.text.isEmpty() && comment.contains(item.text, Qt::CaseInsensitive)) {
            return item.color;
        }
    }
    return Qt::yellow;
}

float CommentSetting::getRadius(const QString & comment) {
    for(const auto & item : comments) {
        if(!item.text.isEmpty() && comment.contains(item.text, Qt::CaseInsensitive)) {
            return item.nodeRadius;
        }
    }
    return 0;
}
