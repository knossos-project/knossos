#ifndef CHEATSHEET_H
#define CHEATSHEET_H

#include "annotation/annotation.h"

#include <QDockWidget>
#include <QTextBrowser>

class Cheatsheet : public QDockWidget {
    Q_OBJECT
    QTextBrowser browser;
public:
    explicit Cheatsheet(QWidget * parent = nullptr);
    void load(const AnnotationMode mode);

signals:
    void anchorClicked(const QUrl & link);
};

#endif // CHEATSHEET_H
