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
