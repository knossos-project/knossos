#include "documentationwidget.h"

#include <QFile>
#include <QHelpEngine>

HelpBrowser::HelpBrowser(QHelpEngine *helpEngine, QWidget *parent)
    : QTextBrowser(parent), helpEngine(helpEngine) {

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

QVariant HelpBrowser::loadResource(int type, const QUrl &url) {
    if (url.scheme() == "qthelp")
        return QVariant(helpEngine->fileData(url));
    else
        return QTextBrowser::loadResource(type, url);
}

DocumentationWidget::DocumentationWidget(QWidget *parent) : QDialog(parent) {
    //extract the docs from the ressource to appdata and read it from there
    //as QHelpEngine doesn’t support reading from ressource files directly
    const auto docDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/doc";
    QDir(docDir).mkpath(".");//create doc folder
    const auto collectionFilePath = docDir + "/collection.qhc";
    const auto docFilePath = docDir + "/doc.qch";
    //delete first to update because copy does not overwrite
    QFile::remove(collectionFilePath);
    QFile::remove(docFilePath);
    QFile::copy(":/doc/collection.qhc", collectionFilePath);
    QFile::copy(":/doc/doc.qch", docFilePath);

    QHelpEngine *helpEngine = new QHelpEngine(collectionFilePath);
    helpEngine->setupData();

    HelpBrowser *helpBrowser = new HelpBrowser(helpEngine);

    QHelpContentWidget *contentWidget = helpEngine->contentWidget();
    contentWidget->setModel(helpEngine->contentModel());

    setWindowIcon(QIcon(":/resources/icons/edit-select-all.png"));
    setWindowTitle("Documentation");
    splitter = new QSplitter();
    splitter->addWidget(contentWidget);
    splitter->addWidget(helpBrowser);
    QList<int> sizeList;
    sizeList.append(223);
    sizeList.append(788);
    splitter->setSizes(sizeList);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(splitter);
    setLayout(layout);

    connect(contentWidget, SIGNAL(linkActivated(const QUrl &)), helpBrowser, SLOT(setSource(const QUrl &)));
}


