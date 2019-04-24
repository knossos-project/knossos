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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#ifndef SKELETONVIEW_H
#define SKELETONVIEW_H

#include "widgets/Spoiler.h"
#include "widgets/UserOrientableSplitter.h"

#include <QAbstractListModel>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

template<typename ConcreteModel>
class AbstractSkeletonModel : public QAbstractListModel {
public:
    bool selectionProtection{false};
    bool selectionFromModel{false};
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
    virtual int rowCount(const QModelIndex & = QModelIndex{}) const override;
};

class TreeModel : public AbstractSkeletonModel<TreeModel> {
    Q_OBJECT
    friend class AbstractSkeletonModel<TreeModel>;
    const std::vector<QString> header = {"ID", ""/*color*/, "Show", "#", "Comment", "Properties"};
    const std::vector<Qt::ItemFlags> flagModifier = {Qt::ItemIsDropEnabled, 0, Qt::ItemIsUserCheckable, 0, Qt::ItemIsEditable, 0};
public:
    std::vector<std::reference_wrapper<class treeListElement>> cache;
    enum SynapseDisplayModes {
        Hide     = 0,
        Show     = 1,
        ShowOnly = 2
    };
    SynapseDisplayModes mode = SynapseDisplayModes::Hide;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    virtual bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) override;
    void recreate();
signals:
    void moveNodes(const QModelIndex &);
};

class NodeModel : public AbstractSkeletonModel<NodeModel> {
    friend class AbstractSkeletonModel<NodeModel>;
    const std::vector<QString> header = {"ID", "x", "y", "z", "Radius", "Comment", "Properties"};
    const std::vector<Qt::ItemFlags> flagModifier = {Qt::ItemIsDragEnabled, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable, 0};
public:
    std::vector<std::reference_wrapper<class nodeListElement>> cache;
    enum FilterMode {
        All = 0,
        InSelectedTree = 1 << 1,
        Selected       = 1 << 2,
        Branch         = 1 << 3,
        Comment        = 1 << 4,
        Synapse        = 1 << 5
    };
    QFlags<FilterMode> mode = FilterMode::InSelectedTree;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    void recreate(const bool matchAll);
};

class NodeView : public QTreeView {
    QSortFilterProxyModel & proxy;
    NodeModel & source;
    virtual void mousePressEvent(QMouseEvent * event) override;
public:
    NodeView(QSortFilterProxyModel & proxy, NodeModel & source) : proxy{proxy}, source{source} {}
};

class SkeletonView : public QWidget {
    friend class AnnotationWidget;

    QVBoxLayout mainLayout;

    QColorDialog colorDialog{this};

    UserOrientableSplitter splitter;

    QWidget treeDummyWidget;
    QVBoxLayout treeLayout;
    QComboBox treeFilterCombo;
    // tree comment filter
    QHBoxLayout treeCommentLayout;
    QLineEdit treeCommentFilter;
    QCheckBox treeRegex{"Regex"};

    TreeModel treeModel;
    QSortFilterProxyModel treeSortAndCommentFilterProxy;
    int treeSortSectionIndex{-1};
    QTreeView treeView;
    QLabel treeCountLabel;

    QWidget nodeDummyWidget;
    QVBoxLayout nodeLayout;
    // node comment filter
    QHBoxLayout nodeCommentLayout;
    QLineEdit nodeCommentFilter;
    QCheckBox nodeRegex{"Regex"};
    // node filter spoiler
    Spoiler nodeFilterSpoiler{"Filter options"};
    QVBoxLayout nodeFilterSpoilerLayout;
    QGroupBox nodeFilterGroupBox{"Filter"};
    QHBoxLayout nodeGroupBoxLayout;
    QButtonGroup nodeFilterButtonGroup;
    QComboBox nodeFilterModeCombo;
    QCheckBox nodeFfilterInSelectedTreeCheckbox{"In selected tree"};
    QCheckBox nodeFilterSelectedCheckbox{"Selected"};
    QCheckBox nodeFilterBranchCheckbox{"Branch node"};
    QCheckBox nodeFilterCommentCheckbox{"Comment node"};
    QCheckBox nodeFilterSynapseCheckbox{"Synapse node"};

    NodeModel nodeModel;
    QSortFilterProxyModel nodeSortAndCommentFilterProxy;
    int nodeSortSectionIndex{-1};
    NodeView nodeView;
    QLabel nodeCountLabel;

    QMenu treeContextMenu{&treeView};
    QMenu nodeContextMenu{&nodeView};

    // advanced tree actions
    QAction *moveNodesAction;
    QAction *mergeAction;
    // advanced node actions
    QAction *swapSynapseDirectionAction;
    QAction *extractComponentAction;
    QAction *linkAction;

    Spoiler commandsBox{"Commands"};
    QGridLayout commandsLayout;
    QLabel defaultRadiusLabel{tr("Default node radius:")};
    QDoubleSpinBox defaultRadiusSpin;

    QLabel lockingLabel{"<strong>Locking</strong>"};
    QLabel lockedNodeLabel{tr("Locked to nothing at the moment.")};
    QLineEdit commentConditionEdit{"seed"};
    QLabel lockingRadiusLabel{tr("Locking radius:")};
    QSpinBox lockingRadiusSpin;
    QCheckBox lockToActiveCheckbox{tr("Lock to active node")};
    QPushButton disableCurrentLockingButton{tr("Disable current locking")};

public:
    explicit SkeletonView(QWidget * const parent = nullptr);
    void jumpToNextNode(const bool forward) const;
    void jumpToNextTree(const bool forward) const;
    void reverseSynapseDirection();
};

#endif//SKELETONVIEW_H
