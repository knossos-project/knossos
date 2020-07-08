/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2020
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "datasetbrowser.h"

#include "datasetloadwidget.h"
#include "mainwindow.h"
#include "stateInfo.h"

DatasetBrowser::DatasetBrowser(QWidget * parent) : DialogVisibilityNotify(DATASET_BROWSER, parent) {
    layout.addWidget(&treeview);
    layout.addWidget(&listview);
    setLayout(&layout);

    const auto path = dirModel.myComputer().toString();

    dirModel.setRootPath(path);
    dirModel.setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);

    treeview.setModel(&dirModel);
    treeview.setRootIndex(dirModel.index(path));

    QObject::connect(treeview.selectionModel(), &QItemSelectionModel::currentChanged, [this](auto index){
        const auto path = dirModel.fileInfo(index).absoluteFilePath();
        // recreate model because it displays directories if they they were root at one point
        // https://bugreports.qt.io/browse/QTBUG-74471
        fileModel = std::make_unique<QFileSystemModel>();
        fileModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
        fileModel->setNameFilterDisables(false);
        fileModel->setNameFilters({"*.conf"});
        listview.setModel(fileModel.get());
        listview.setRootIndex(fileModel->setRootPath(path));
    });
    QObject::connect(&listview, &QTreeView::doubleClicked, [this](auto index){
        const auto path = fileModel->fileInfo(index).absoluteFilePath();
        state->mainWindow->widgetContainer.datasetLoadWidget.loadDataset(boost::none, QUrl::fromLocalFile(path));
    });
}
