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

#pragma once

#include <QColor>

#include <utility>
#include <vector>

class CommentSetting {
    friend class CommentsModel;
    QString shortcut;

public:
    QString text;
    QColor color;
    float nodeRadius;

    static inline bool useCommentNodeRadius{false};
    static inline bool useCommentNodeColor{false};
    static inline bool appendComment{false};
    static std::vector<CommentSetting> comments;

    explicit CommentSetting(QString shortcut, QString text = "", QColor color = QColor(255, 255, 0, 255), const float nodeRadius = 1.5) :
        shortcut(shortcut), text(text), color(color), nodeRadius(nodeRadius) {}

    static QColor getColor(const QString & comment);
    static float getRadius(const QString & comment);
};

inline decltype(CommentSetting::comments) CommentSetting::comments = {};
