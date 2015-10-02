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
