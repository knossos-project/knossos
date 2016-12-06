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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef COMMENTSETTING_H
#define COMMENTSETTING_H

#include <QColor>

#include <vector>

class CommentSetting {
    friend class CommentsModel;
    QString shortcut;

public:
    QString text;
    QColor color;
    float nodeRadius;

    static bool useCommentNodeRadius;
    static bool useCommentNodeColor;
    static bool appendComment;
    static std::vector<CommentSetting> comments;

    explicit CommentSetting(const QString shortcut, const QString text = "", const QColor color = QColor(255, 255, 0, 255), const float nodeRadius = 1.5);

    static QColor getColor(const QString comment);
    static float getRadius(const QString comment);
};

#endif // COMMENTSETTING_H
