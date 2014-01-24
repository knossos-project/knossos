#ifndef DOCUMENTATION_H
#define DOCUMENTATION_H

#include <QDialog>
#include <QtHelp>
#include <QTextBrowser>

class HelpBrowser : public QTextBrowser {
public:
    HelpBrowser(QHelpEngine *helpEngine, QWidget *parent = 0);
    QVariant loadResource(int type, const QUrl &url);
private:
    QHelpEngine *helpEngine;
};

class DocumentationWidget : public QDialog
{
    Q_OBJECT
public:
    explicit DocumentationWidget(QWidget *parent = 0);

protected:
    QSplitter *splitter;
signals:
    
public slots:
    
};

#endif // DOCUMENTATION_H
