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

#pragma once

#include "widgets/DialogVisibilityNotify.h"

#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QListView>
#include <QTreeView>

#include <memory>

class DatasetBrowser : public DialogVisibilityNotify {
    QTreeView treeview;
    QListView listview;
    QHBoxLayout layout;
    QFileSystemModel dirModel;
    std::unique_ptr<QFileSystemModel> fileModel;
public:
    DatasetBrowser(class QWidget * parent);
};
