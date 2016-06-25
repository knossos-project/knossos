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
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

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
