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

#include "commentstab.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/GuiConstants.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QHeaderView>
#include <QInputDialog>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

void CommentsModel::fill() {
    beginResetModel();
    for (int i = 1; i < 10; ++i) {
        CommentSetting::comments.emplace_back(QString("%1").arg(i%10));
    }
    endResetModel();
}

QVariant CommentsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return header[section];
    } else {
        return QVariant();  //return invalid QVariant
    }
}

QVariant CommentsModel::data(const QModelIndex & index, int role) const {
    if (index.isValid()) {
        const auto & comment = CommentSetting::comments[index.row()];
        if (index.column() == 2 && role == Qt::BackgroundRole) {
            return comment.color;
        } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch (index.column()) {
            case 0: return comment.shortcut;
            case 1: return comment.text;
            case 3: return comment.nodeRadius;
            }
        }
        return QVariant(); //return invalid QVariant
    }
    return QVariant(); //return invalid QVariant
}

bool CommentsModel::setData(const QModelIndex & index, const QVariant & value, int role) {
    if (index.isValid()) {
        auto & comment = CommentSetting::comments[index.row()];

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch (index.column()) {
            case 1:
                comment.text = value.toString();
                state->viewer->mainWindow.updateCommentShortcut(index.row(), comment.text);
                break;
            case 3: {
                bool success = false;
                float radius = value.toFloat(&success);
                comment.nodeRadius = success ? radius : comment.nodeRadius;
                break;
            }
            default:
                return false;
            }
        }
        return true;
    }
    return true;
}

int CommentsModel::columnCount(const QModelIndex &) const {
    return header.size();
}

int CommentsModel::rowCount(const QModelIndex &) const {
    return CommentSetting::comments.size();
}

Qt::ItemFlags CommentsModel::flags(const QModelIndex & index) const {
    auto flags = QAbstractItemModel::flags(index);
    return (index.column() == 0) ? flags | Qt::ItemNeverHasChildren : flags | Qt::ItemIsEditable;
}

void CommentsTab::itemDoubleClicked(const QModelIndex &index) {
    auto & comment = CommentSetting::comments[index.row()];
    if (index.column() == 2) {
        const auto color = state->viewer->suspend([this, &comment]{
            return QColorDialog::getColor(comment.color, this, "Select comment color", QColorDialog::ShowAlphaChannel);
        });
        if (color.isValid() == QColorDialog::Accepted) {
            comment.color = color;
        }
    }
}


CommentsTab::CommentsTab(QWidget *parent) : QWidget(parent) {
    auto mainlayout = new QVBoxLayout();

    auto checkboxLayout = new QHBoxLayout();
    checkboxLayout->addWidget(&useCommentColorCheckbox);
    checkboxLayout->addWidget(&useCommentRadiusCheckbox);
    checkboxLayout->addWidget(&appendCommentCheckbox);
    QObject::connect(&useCommentColorCheckbox, &QCheckBox::stateChanged, [this](bool checked) { CommentSetting::useCommentNodeColor = checked; commentsTable.header()->setSectionHidden(2, !checked); });
    QObject::connect(&useCommentRadiusCheckbox, &QCheckBox::stateChanged, [this](bool checked) { CommentSetting::useCommentNodeRadius = checked; commentsTable.header()->setSectionHidden(3, !checked); });
    QObject::connect(&appendCommentCheckbox, &QCheckBox::stateChanged, [](bool checked) { CommentSetting::appendComment = checked;});

    commentsTable.setModel(&commentModel);
    commentsTable.setAllColumnsShowFocus(true);
    commentsTable.setUniformRowHeights(true);//perf hint from doc
    commentsTable.setRootIsDecorated(false);
    commentsTable.header()->setStretchLastSection(false);
    commentsTable.header()->setSectionResizeMode(1, QHeaderView::Stretch);
    commentsTable.setSelectionMode(QAbstractItemView::ExtendedSelection);
    for (const auto & index : {0, 2, 3}) {
        commentsTable.resizeColumnToContents(index);
    }
    QObject::connect(&commentsTable, &QTreeView::doubleClicked, this, &CommentsTab::itemDoubleClicked);

    mainlayout->addLayout(checkboxLayout);
    mainlayout->addWidget(&commentsTable);
    setLayout(mainlayout);

    commentModel.fill();
}

CommentsTab::~CommentsTab() {}

void CommentsTab::loadSettings() {
    QSettings settings;
    settings.beginGroup(COMMENTS_TAB);
    for(int i = 0; i < 10; ++i) {
        CommentSetting::comments[i].text = settings.value(QString("comment%1").arg(i+1)).toString();
        CommentSetting::comments[i].color = settings.value(QString("color%1").arg(i+1), QColor(255, 255, 0, 255)).value<QColor>();
        CommentSetting::comments[i].nodeRadius = settings.value(QString("radius%1").arg(i+1), 1.5).toFloat();
        state->viewer->mainWindow.updateCommentShortcut(i, CommentSetting::comments[i].text);
    }
    useCommentColorCheckbox.setChecked(settings.value(CUSTOM_COMMENT_NODECOLOR, true).toBool());
    useCommentRadiusCheckbox.setChecked(settings.value(CUSTOM_COMMENT_NODERADIUS, false).toBool());
    useCommentRadiusCheckbox.stateChanged(useCommentRadiusCheckbox.isChecked());
    appendCommentCheckbox.setChecked(settings.value(CUSTOM_COMMENT_APPEND, false).toBool());
    settings.endGroup();
}

void CommentsTab::saveSettings() {
    QSettings settings;
    settings.beginGroup(COMMENTS_TAB);
    for(int i = 0; i < 10; ++i) {
        settings.setValue(QString("comment%1").arg(i+1), CommentSetting::comments[i].text);
        settings.setValue(QString("color%1").arg(i+1), CommentSetting::comments[i].color);
        settings.setValue(QString("radius%1").arg(i+1), CommentSetting::comments[i].nodeRadius);
    }

    settings.setValue(CUSTOM_COMMENT_NODECOLOR, useCommentColorCheckbox.isChecked());
    settings.setValue(CUSTOM_COMMENT_NODERADIUS, useCommentRadiusCheckbox.isChecked());
    settings.setValue(CUSTOM_COMMENT_APPEND, appendCommentCheckbox.isChecked());
    settings.endGroup();
}
