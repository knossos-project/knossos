#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QObject>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QVector>

struct Rule {
    QRegExp pattern;
    QTextCharFormat format;
};


class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument *document);
    QVector<Rule> rules;
signals:

public slots:
    void highlightBlock(const QString &text);


};

#endif // HIGHLIGHTER_H
