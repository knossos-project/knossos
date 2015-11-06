#ifndef DOCUMENTATIONWIDGET_H
#define DOCUMENTATIONWIDGET_H

#include <QDialog>
#include <QHelpEngine>
#include <QSplitter>
#include <QTextBrowser>
#include <QVBoxLayout>

class HelpBrowser : public QTextBrowser {
    QHelpEngine & helpEngine;
public:
    HelpBrowser(QWidget & parent, QHelpEngine & helpEngine);
    QVariant loadResource(int type, const QUrl &url);
};

class DocumentationWidget : public QDialog {
    Q_OBJECT
    QHelpEngine helpEngine;

    QVBoxLayout layout;
    QSplitter splitter;
    HelpBrowser helpBrowser;
public:
    explicit DocumentationWidget(QWidget *parent = 0);
};

#endif// DOCUMENTATIONWIDGET_H
