/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef ACTION_HELPER
#define ACTION_HELPER

#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include <QMenu>
#include <QObject>
#include <QTreeView>

static QAction * widgetShortcut(QWidget & parent, QAction & action, QKeySequence keySequence) {
    parent.addAction(&action);
    action.setShortcut(keySequence);
    action.setShortcutContext(Qt::WidgetShortcut);// local to the table
    return &action;
}

static void copyAction(QMenu & menu, QTreeView & view) {
    auto & action = *menu.addAction(QIcon(":/resources/icons/edit-copy.png"), QObject::tr("Copy selected contents"));
    QObject::connect(widgetShortcut(view, action, QKeySequence::Copy), &QAction::triggered, [&view]{
        QString content;
        for (const auto & rowIndex : view.selectionModel()->selectedRows()) {
            for (int column{0}; column < view.model()->columnCount(QModelIndex{}); ++column) {
                content += view.model()->data(view.model()->index(rowIndex.row(), column), Qt::UserRole).toString() + '\t';
            }
            content.chop(1);// remove last tab
            content += '\n';
        }
        QGuiApplication::clipboard()->setText(content);
    });
}

template<typename... Args>
void deleteAction(QMenu & menu, QTreeView & view, QString text, Args &&... args) {
    auto & deleteAction = *menu.addAction(QIcon(":/resources/icons/menubar/trash.png"), text);
    QObject::connect(widgetShortcut(view, deleteAction, Qt::Key_Delete), &QAction::triggered, args...);
}

static void addDisabledSeparator(QMenu & contextMenu) {
    contextMenu.addSeparator();
    contextMenu.actions().back()->setEnabled(false);// disable separator for ctx menu to not vanish on click
}

#endif//ACTION_HELPER
