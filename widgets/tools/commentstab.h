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

#ifndef COMMENTSTAB_H
#define COMMENTSTAB_H

#include "commentsetting.h"

#include <QAbstractListModel>
#include <QCheckBox>
#include <QShortcut>
#include <QTreeView>
#include <QWidget>

class CommentsModel : public QAbstractListModel {
Q_OBJECT
    friend class CommentsTab;

protected:
    const std::vector<QString> header{"Shortcut", "Comment", "Color", "Node radius"};

public:
    void fill();
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;
};


class CommentsTab : public QWidget
{
    Q_OBJECT
    QCheckBox useCommentColorCheckbox{"Use custom comment color"};
    QCheckBox useCommentRadiusCheckbox{"Use custom comment radius"};
    QCheckBox appendCommentCheckbox{"Append comments"};
    CommentsModel commentModel;
    QTreeView commentsTable;

    void itemDoubleClicked(const QModelIndex &index);
public:
    void saveSettings();
    void loadSettings();
    explicit CommentsTab(QWidget *parent = nullptr);
    ~CommentsTab() override;

signals:

public slots:
};

#endif // COMMENTSTAB_H
