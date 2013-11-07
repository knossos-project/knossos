#include <QHelpEngineCore>

#include "documentationwidget.h"

HelpBrowser::HelpBrowser(QHelpEngine *helpEngine, QWidget *parent)
    : QTextBrowser(parent), helpEngine(helpEngine) {}

QVariant HelpBrowser::loadResource(int type, const QUrl &url) {
    if (url.scheme() == "qthelp")
        return QVariant(helpEngine->fileData(url));
    else
        return QTextBrowser::loadResource(type, url);
}

DocumentationWidget::DocumentationWidget(QWidget *parent) :
    QDialog(parent)
{
    QHelpEngine *helpEngine = new QHelpEngine("doc/collection.qhc");
    helpEngine->setupData();

    HelpBrowser *helpBrowser = new HelpBrowser(helpEngine);

    QHelpContentModel *contentModel = helpEngine->contentModel();
    QHelpContentWidget *contentWidget = helpEngine->contentWidget();

    splitter = new QSplitter();
    splitter->addWidget(contentWidget);
    splitter->addWidget(helpBrowser);

    contentWidget->setModel(contentModel);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(splitter);
    setLayout(layout);

    connect(contentWidget, SIGNAL(linkActivated(const QUrl &)), helpBrowser, SLOT(setSource(const QUrl &)));
}


