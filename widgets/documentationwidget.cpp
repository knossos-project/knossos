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

    setWindowTitle("Documentation");
    splitter = new QSplitter();
    splitter->addWidget(contentWidget);
    splitter->addWidget(helpBrowser);
    helpBrowser->setSource(QUrl("qthelp://max-planck-institut-for-medical-research.knossos.4.0/doc/documentation.html"));
    QList<int> sizeList;
    sizeList.append(223);
    sizeList.append(788);
    splitter->setSizes(sizeList);
    contentWidget->setModel(contentModel);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(splitter);
    setLayout(layout);

    connect(contentWidget, SIGNAL(linkActivated(const QUrl &)), helpBrowser, SLOT(setSource(const QUrl &)));
}


