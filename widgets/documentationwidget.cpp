#include "documentationwidget.h"

#include <QDir>
#include <QFile>
#include <QHelpContentWidget>
#include <QIcon>
#include <QStandardPaths>

HelpBrowser::HelpBrowser(QWidget & parent, QHelpEngine & helpEngine) : QTextBrowser(&parent), helpEngine(helpEngine) {
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

QVariant HelpBrowser::loadResource(int type, const QUrl &url) {
    if (url.scheme() == "qthelp") {
        return QVariant(helpEngine.fileData(url));
    } else {
        return QTextBrowser::loadResource(type, url);
    }
}

QString pathOfExtractedHelp() {
    //extract the docs from the ressource to appdata and read it from there
    //as QHelpEngine doesn’t support reading from ressource files directly
    const auto docDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/doc";
    QDir(docDir).mkpath(".");//create doc folder
    const auto collectionFilePath = docDir + "/collection.qhc";
    const auto docFilePath = docDir + "/doc.qch";
    //delete first to update because copy does not overwrite
    QFile::remove(collectionFilePath);
    QFile::remove(docFilePath);
    QFile::copy(":/resources/doc/collection.qhc", collectionFilePath);
    QFile::copy(":/resources/doc/doc.qch", docFilePath);

    return collectionFilePath;
}

DocumentationWidget::DocumentationWidget(QWidget *parent) : QDialog(parent), helpEngine(pathOfExtractedHelp()), helpBrowser(*this, helpEngine) {
    helpEngine.setupData();

    setWindowIcon(QIcon(":/resources/icons/edit-select-all.png"));
    setWindowTitle("Documentation");

    splitter.addWidget(helpEngine.contentWidget());
    splitter.addWidget(&helpBrowser);

    splitter.setSizes({223, 788});

    layout.addWidget(&splitter);
    setLayout(&layout);

    resize(1024, 640);

    QObject::connect(helpEngine.contentWidget(), &QHelpContentWidget::linkActivated, &helpBrowser, &HelpBrowser::setSource);
}
