﻿/*
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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "skeletonview.h"

#include "action_helper.h"
#include "gui_wrapper.h"
#include "model_helper.h"
#include "session.h"
#include "skeleton/node.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "stateInfo.h"
#include "viewer.h"// Viewer::suspend

#include <QApplication>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegExpValidator>
#include <QSignalBlocker>

template<typename Func>
void question(Func func, const QString & acceptButtonText, const QString & text, const QString & extraText = "") {
    QMessageBox prompt{QApplication::activeWindow()};
    prompt.setIcon(QMessageBox::Question);
    prompt.setText(text);
    prompt.setInformativeText(extraText);
    const auto & confirmButton = *prompt.addButton(acceptButtonText, QMessageBox::AcceptRole);
    prompt.addButton(QMessageBox::Cancel);
    prompt.exec();
    if (prompt.clickedButton() == &confirmButton) {
        func();
    }
}

template<typename ConcreteModel>
int AbstractSkeletonModel<ConcreteModel>::columnCount(const QModelIndex &) const {
    return static_cast<ConcreteModel const * const>(this)->header.size();
}
template<typename ConcreteModel>
QVariant AbstractSkeletonModel<ConcreteModel>::headerData(int section, Qt::Orientation orientation, int role) const {
    return (orientation == Qt::Horizontal && role == Qt::DisplayRole) ? static_cast<ConcreteModel const * const>(this)->header[section] : QVariant();
}
template<typename ConcreteModel>
Qt::ItemFlags AbstractSkeletonModel<ConcreteModel>::flags(const QModelIndex &index) const {
    return QAbstractItemModel::flags(index) | Qt::ItemNeverHasChildren | static_cast<ConcreteModel const * const>(this)->flagModifier[index.column()];
}
template<typename ConcreteModel>
int AbstractSkeletonModel<ConcreteModel>::rowCount(const QModelIndex &) const {
    return static_cast<ConcreteModel const * const>(this)->cache.size();
}

template class AbstractSkeletonModel<TreeModel>;//please clang, should actually be implicitly instantiated in here anyway

QString propertyStringWithoutComment(const QVariantHash & properties) {
    QString propertiesString("");
    for (auto it = std::cbegin(properties); it != std::cend(properties); ++it) {
        if (it.key() != "comment") {
            propertiesString += it.key() + ": " + it.value().toString() + "; ";
        }
    }
    return propertiesString;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const {
    const auto & tree = cache[index.row()].get();
    if (&tree == state->skeletonState->activeTree && index.column() == 0 && role == Qt::DecorationRole) {
        return QPixmap(":/resources/icons/active-arrow.png").scaled(QSize(8, 8), Qt::KeepAspectRatio);
    } else if (index.column() == 1 && (role == Qt::BackgroundRole || role == Qt::DecorationRole || role == Qt::UserRole)) { // background role not visible for selected row under gnome…
        return tree.color;
    } else if (index.column() == 2 && (role == Qt::CheckStateRole || role == Qt::UserRole)) {
        return tree.render ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::UserRole) {
        switch (index.column()) {
        case 0: return static_cast<quint64>(tree.treeID);
        case 3: return static_cast<quint64>(tree.nodes.size());
        case 4: return tree.getComment();
        case 5:
            auto treeProperties = propertyStringWithoutComment(tree.properties);
            if(tree.mesh != nullptr) {
                treeProperties.prepend("Mesh");
            }
            return treeProperties;
        }
    }
    return QVariant();//return invalid QVariant
}

bool TreeModel::setData(const QModelIndex & index, const QVariant & value, int role) {
    if (!index.isValid()) {
        return false;
    }
    auto & tree = cache[index.row()].get();

    if (index.column() == 2 && role == Qt::CheckStateRole) {
        tree.render = value.toBool();
    } else if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == 1) {
        Skeletonizer::singleton().setColor(tree, value.value<QColor>());
    } else if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == 4) {
        const QString comment{value.toString()};
        Skeletonizer::singleton().setComment(tree, comment);
    } else {
        return false;
    }
    return true;
}

QVariant NodeModel::data(const QModelIndex &index, int role) const {
    if (state->skeletonState->trees.empty()) {
        return QVariant();//return invalid QVariant
    }
    const auto & node = cache[index.row()].get();

    if (&node == state->skeletonState->activeNode && index.column() == 0 && role == Qt::DecorationRole) {
        return QPixmap(":/resources/icons/active-arrow.png").scaled(QSize(8, 8), Qt::KeepAspectRatio);
    }
    if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::UserRole) {
        switch (index.column()) {
        case 0: return static_cast<quint64>(node.nodeID);
        case 1: return node.position.x + 1;
        case 2: return node.position.y + 1;
        case 3: return node.position.z + 1;
        case 4: return node.radius;
        case 5: return node.getComment();
        case 6:
            auto nodeProperties = propertyStringWithoutComment(node.properties);
            if(node.isSynapticNode) {
                if(node.correspondingSynapse->getPreSynapse() == &node) {
                    nodeProperties.prepend("PreSynapse");
                } else if(node.correspondingSynapse->getPostSynapse() == &node){
                    nodeProperties.prepend("PostSynapse");
                }
            }
            return nodeProperties;
        }
    }
    return QVariant();//return invalid QVariant
}

bool NodeModel::setData(const QModelIndex & index, const QVariant & value, int role) {
    if (state->skeletonState->trees.empty() || !index.isValid() || !(role == Qt::DisplayRole || role == Qt::EditRole)) {
        return false;
    }
    auto & node = cache[index.row()].get();

    if (index.column() == 1) {
        const Coordinate position{value.toInt() - 1, node.position.y, node.position.z};
        Skeletonizer::singleton().editNode(0, &node, node.radius, position, node.createdInMag);
    } else if (index.column() == 2) {
        const Coordinate position{node.position.x, value.toInt() - 1, node.position.z};
        Skeletonizer::singleton().editNode(0, &node, node.radius, position, node.createdInMag);
    } else if (index.column() == 3) {
        const Coordinate position{node.position.x, node.position.y, value.toInt() - 1};
        Skeletonizer::singleton().editNode(0, &node, node.radius, position, node.createdInMag);
    } else if (index.column() == 4) {
        const float radius{value.toFloat()};
        Skeletonizer::singleton().editNode(0, &node, radius, node.position, node.createdInMag);
    } else if (index.column() == 5) {
        const QString comment{value.toString()};
        Skeletonizer::singleton().setComment(node, comment);
    } else {
        return false;
    }
    return true;
}

bool TreeModel::dropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex & parent) {
    if (parent.isValid()) {
        emit moveNodes(parent);
        return true;
    }
    return false;
}

void TreeModel::recreate() {
    beginResetModel();
    cache.clear();
    for (auto && tree : state->skeletonState->trees) {
        if ((mode == SynapseDisplayModes::Hide && tree.isSynapticCleft == false)
                || (mode == SynapseDisplayModes::Show)
                || (mode == SynapseDisplayModes::ShowOnly && tree.isSynapticCleft)) {
            cache.emplace_back(tree);
        }
    }
    endResetModel();
}

void NodeModel::recreate(const bool matchAll = true) {
    beginResetModel();
    cache.clear();
    for (auto && tree : state->skeletonState->trees)
    for (auto && node : tree.nodes) {
        cache.emplace_back(node);
    }

    if(mode.testFlag(FilterMode::All) == false) {
        decltype(cache) hits;
        for (auto && node : cache) {// show node if for all criteria: either criterion not demanded or fulfilled
            const auto oneMatched = (mode.testFlag(FilterMode::Selected) && node.get().selected)
                    || (mode.testFlag(FilterMode::InSelectedTree) && node.get().correspondingTree->selected)
                    || (mode.testFlag(FilterMode::Branch) && node.get().isBranchNode)
                    || (mode.testFlag(FilterMode::Comment) && node.get().getComment().isEmpty() == false)
                    || (mode.testFlag(FilterMode::Synapse) && node.get().isSynapticNode);
            const auto allMatched = (!mode.testFlag(FilterMode::Selected) || node.get().selected)
                    && (!mode.testFlag(FilterMode::InSelectedTree) || node.get().correspondingTree->selected)
                    && (!mode.testFlag(FilterMode::Branch) || node.get().isBranchNode)
                    && (!mode.testFlag(FilterMode::Comment) || node.get().getComment().isEmpty() == false)
                    && (!mode.testFlag(FilterMode::Synapse) || node.get().isSynapticNode);
            if ((!matchAll && oneMatched) || (matchAll && allMatched)) {
                hits.emplace_back(node);
            } else if (matchAll && mode.testFlag(FilterMode::Selected) && node.get().selected) {
                selectionFromModel = true;
                Skeletonizer::singleton().toggleNodeSelection({&node.get()});
                selectionFromModel = false;
            }
        }
        cache = hits;
    }
    endResetModel();
}

void NodeView::mousePressEvent(QMouseEvent * event) {
    if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_TracingAdvanced)) {
        const auto index = proxy.mapToSource(indexAt(event->pos()));
        if (index.isValid()) {//enable drag’n’drop only for selected items to retain rubberband selection
            const auto selected = source.cache[index.row()].get().selected && !event->modifiers().testFlag(Qt::ControlModifier) && !event->modifiers().testFlag(Qt::ShiftModifier);
            setDragDropMode(selected ? QAbstractItemView::DragOnly : QAbstractItemView::NoDragDrop);
        }
    }
    QTreeView::mousePressEvent(event);
}

template<typename T, typename View, typename Model, typename Proxy>
auto selectElems(View & view, Model & model, Proxy & proxy) {
    return [&view, &model, &proxy](const QItemSelection & selected, const QItemSelection & deselected){
        if (!model.selectionProtection) {
            const auto & proxySelected = proxy.mapSelectionToSource(selected);
            const auto & proxyDeselected = proxy.mapSelectionToSource(deselected);
            auto collectElems = [&model](auto && indices){
                QSet<T*> elems;
                for (const auto & modelIndex : indices) {
                    if (modelIndex.column() == 0) {
                        elems.insert(&model.cache[modelIndex.row()].get());
                    }
                }
                return elems;
            };
            model.selectionFromModel = true;
            const auto onlyOneNodeSelectedPreviously = state->skeletonState->selectedNodes.size() == 1;
            const auto firstSelectionInTable = !selected.empty() && (view.selectionModel()->selection() == selected);
            if (onlyOneNodeSelectedPreviously && firstSelectionInTable) {
                Skeletonizer::singleton().select(collectElems(proxySelected.indexes()));
            } else {
                auto indices = proxySelected.indexes();
                indices.append(proxyDeselected.indexes());
                Skeletonizer::singleton().toggleSelection(collectElems(indices));
            }
        }
    };
}

template<typename Model, typename Proxy>
auto updateSelection(QTreeView & view, Model & model, Proxy & proxy) {
    auto isSelectedFunc = [&model, &proxy, &selectionModel = *view.selectionModel()](const int rowIndex){
        return selectionModel.isSelected(proxy.mapFromSource(model.index(rowIndex, 0)));
    };
    const auto selection = deltaBlockSelection(model, model.cache, isSelectedFunc);
    const auto selectedIndices = proxy.mapSelectionFromSource(selection);

    model.selectionProtection = true;
    if (!selectedIndices.isEmpty()) {// selecting an empty index range apparently clears
        view.selectionModel()->select(selectedIndices, QItemSelectionModel::SelectCurrent);
    }
    model.selectionProtection = false;

    auto * active = Skeletonizer::singleton().active<typename decltype(model.cache)::value_type::type>();
    if (!view.currentIndex().isValid() && active != nullptr) {// find active elem and set as current index
        const auto activeIt = std::find_if(std::begin(model.cache), std::end(model.cache), [&active](const auto & elem){
            return &elem.get() == active;
        });
        if (activeIt != std::end(model.cache)) {
            const std::size_t activeIndex = std::distance(std::begin(model.cache), activeIt);
            const auto modelIndex = proxy.mapFromSource(model.index(activeIndex, 0));
            view.selectionModel()->setCurrentIndex(modelIndex, QItemSelectionModel::NoUpdate);
        }
    }
    if (!selectedIndices.indexes().isEmpty()) {// scroll to first selected entry
        view.scrollTo(selectedIndices.indexes().front());
    }
}

SkeletonView::SkeletonView(QWidget * const parent) : QWidget{parent}
        , nodeView{nodeSortAndCommentFilterProxy, nodeModel} {
    auto setupTable = [](auto & table, auto & model, auto & sortIndex){
        table.setModel(&model);
        table.setAllColumnsShowFocus(true);
        table.setUniformRowHeights(true);//perf hint from doc
        table.setRootIsDecorated(false);//remove padding to the left of each cell’s content
        table.setSelectionMode(QAbstractItemView::ExtendedSelection);
        table.setSortingEnabled(true);
        table.sortByColumn(sortIndex = 0, Qt::SortOrder::AscendingOrder);
    };

    treeFilterCombo.addItems({tr("Hide synapses"), tr("Show synapses"), tr("Show only synapses")});
    treeFilterCombo.setMinimumWidth(170);
    treeFilterCombo.setMaximumWidth(treeFilterCombo.minimumWidth());

    treeCommentFilter.setPlaceholderText("Tree comment");
    treeSortAndCommentFilterProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
    treeSortAndCommentFilterProxy.setFilterKeyColumn(4);
    treeSortAndCommentFilterProxy.setSourceModel(&treeModel);

    setupTable(treeView, treeSortAndCommentFilterProxy, treeSortSectionIndex);
    treeView.setDragDropMode(QAbstractItemView::DropOnly);
    treeView.setDropIndicatorShown(true);
    for (auto index : {0, 1, 2}) {
        treeView.resizeColumnToContents(index);
    }

    nodeFilterModeCombo.addItems({tr("Match all"), tr("Match at least one")});
    nodeGroupBoxLayout.addWidget(&nodeFilterModeCombo);
    nodeFilterButtonGroup.setExclusive(false);
    for (auto option : { std::make_pair(&nodeFfilterInSelectedTreeCheckbox, NodeModel::FilterMode::InSelectedTree),
                         {&nodeFilterSelectedCheckbox, NodeModel::FilterMode::Selected},
                         {&nodeFilterBranchCheckbox, NodeModel::FilterMode::Branch},
                         {&nodeFilterCommentCheckbox, NodeModel::FilterMode::Comment},
                         {&nodeFilterSynapseCheckbox, NodeModel::FilterMode::Synapse}} ) {
        nodeFilterButtonGroup.addButton(option.first, option.second);
        option.first->setChecked(nodeModel.mode.testFlag(option.second));
        nodeGroupBoxLayout.addWidget(option.first);
    }

    nodeFilterGroupBox.setLayout(&nodeGroupBoxLayout);
    nodeFilterGroupBox.setCheckable(true);
    const bool showAllNodes = nodeModel.mode == NodeModel::FilterMode::All;
    nodeFilterGroupBox.setChecked(!showAllNodes);

    nodeFilterSpoilerLayout.addWidget(&nodeFilterGroupBox);
    nodeFilterSpoiler.setContentLayout(nodeFilterSpoilerLayout);

    nodeCommentFilter.setPlaceholderText("Node comment");

    nodeSortAndCommentFilterProxy.setSourceModel(&nodeModel);
    nodeSortAndCommentFilterProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
    nodeSortAndCommentFilterProxy.setFilterKeyColumn(5);
    setupTable(nodeView, nodeSortAndCommentFilterProxy, nodeSortSectionIndex);

    colorDialog.setOption(QColorDialog::ShowAlphaChannel);

    treeCommentLayout.addWidget(&treeCommentFilter);
    treeCommentLayout.addWidget(&treeRegex);
    treeLayout.addWidget(&treeFilterCombo);
    treeLayout.addLayout(&treeCommentLayout);
    treeLayout.addWidget(&treeView);
    treeLayout.addWidget(&treeCountLabel);
    treeLayout.setContentsMargins({});
    treeDummyWidget.setLayout(&treeLayout);
    splitter.setOrientation(Qt::Vertical);
    splitter.addWidget(&treeDummyWidget);

    nodeCommentLayout.addWidget(&nodeCommentFilter);
    nodeCommentLayout.addWidget(&nodeRegex);
    nodeLayout.addLayout(&nodeCommentLayout);
    nodeLayout.addWidget(&nodeFilterSpoiler);
    nodeLayout.addWidget(&nodeView);
    nodeLayout.addWidget(&nodeCountLabel);
    nodeLayout.setContentsMargins({});
    nodeDummyWidget.setLayout(&nodeLayout);
    splitter.addWidget(&nodeDummyWidget);

    splitter.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);//size policy of QTreeView is also Expanding

    defaultRadiusSpin.setMaximum(nodeListElement::MAX_RADIUS_SETTING);
    defaultRadiusSpin.setValue(Skeletonizer::singleton().skeletonState.defaultNodeRadius);
    defaultRadiusSpin.setSuffix("px");
    lockingRadiusSpin.setMaximum(100000);
    lockingRadiusSpin.setValue(Skeletonizer::singleton().skeletonState.lockRadius);
    lockingRadiusSpin.setSuffix("px");

    commentConditionEdit.setPlaceholderText("Comment condition for locking");
    lockedNodeLabel.setEnabled(false);
    commentConditionEdit.setEnabled(false);
    lockingRadiusLabel.setEnabled(false);
    lockingRadiusSpin.setEnabled(false);
    disableCurrentLockingButton.setEnabled(false);

    commandsLayout.setSpacing(3);
    int row = 0;
    commandsLayout.addWidget(&defaultRadiusLabel, row, 0, 1, 1);
    commandsLayout.addWidget(&defaultRadiusSpin, row++, 1, 1, 1);
    commandsLayout.addWidget(&lockingLabel, row++, 0, 1, 1);
    commandsLayout.addWidget(&lockedNodeLabel, row++, 0, 1, 2);
    commandsLayout.addWidget(&lockToActiveCheckbox, row, 0, 1, 1, Qt::AlignLeft);
    commandsLayout.addWidget(&commentConditionEdit, row++, 1, 1, 1);
    commandsLayout.addWidget(&lockingRadiusLabel, row, 0, 1, 1);
    commandsLayout.addWidget(&lockingRadiusSpin, row++, 1, 1, 1);
    commandsLayout.addWidget(&disableCurrentLockingButton, row++, 1, 1, 1, Qt::AlignRight);
    commandsBox.setContentLayout(commandsLayout);

    mainLayout.addWidget(&splitter);
    mainLayout.addWidget(&commandsBox);
    setLayout(&mainLayout);
    auto & skeleton = Skeletonizer::singleton().skeletonState;
    connect(&defaultRadiusSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [&skeleton](const double value) { skeleton.defaultNodeRadius = value; });
    connect(&lockingRadiusSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&skeleton](const int value) { skeleton.lockRadius = value; });
    connect(&commentConditionEdit, &QLineEdit::textChanged, [&skeleton](const QString comment) { skeleton.lockingComment = comment; });
    connect(&lockToActiveCheckbox, &QPushButton::clicked, [&skeleton,this](const bool checked) {
        skeleton.lockPositions = checked;
        if (checked) {
            if(skeleton.activeNode && skeleton.activeNode->getComment().contains(commentConditionEdit.text(), Qt::CaseInsensitive)) {
                Skeletonizer::singleton().lockPosition(skeleton.activeNode->position);
            }
        } else {
            Skeletonizer::singleton().unlockPosition();
        }
        lockedNodeLabel.setEnabled(checked);
        commentConditionEdit.setEnabled(checked);
        lockingRadiusLabel.setEnabled(checked);
        lockingRadiusSpin.setEnabled(checked);
        disableCurrentLockingButton.setEnabled(checked);
    });
    connect(&disableCurrentLockingButton, &QPushButton::clicked, []() { Skeletonizer::singleton().unlockPosition(); });
    connect(&Skeletonizer::singleton(), &Skeletonizer::lockingChanged, [&skeleton,this] () { lockToActiveCheckbox.setChecked(skeleton.lockPositions); lockToActiveCheckbox.clicked(); });
    connect(&Skeletonizer::singleton(), &Skeletonizer::lockedToNode, [this](const std::uint64_t nodeID) { lockedNodeLabel.setText(tr("Locked to node %1").arg(nodeID)); });
    connect(&Skeletonizer::singleton(), &Skeletonizer::unlockedNode, [this]() { lockedNodeLabel.setText(tr("Locked to nothing at the moment")); });

    static auto updateTreeSelection = [this](){
        updateSelection(treeView, treeModel, treeSortAndCommentFilterProxy);
        const auto all = state->skeletonState->trees.size();
        const auto shown = static_cast<std::size_t>(treeView.model()->rowCount());
        const auto selected = state->skeletonState->selectedTrees.size();
        treeCountLabel.setText(tr("%1 trees").arg(all) + (all != shown ? tr(", %2 shown").arg(shown) : "") + (selected != 0 ? tr(", %3 selected").arg(selected) : ""));
    };
    static auto updateNodeSelection = [this](){
        updateSelection(nodeView, nodeModel, nodeSortAndCommentFilterProxy);
        const auto all = state->skeletonState->nodesByNodeID.size();
        const auto shown = static_cast<std::size_t>(nodeView.model()->rowCount());
        const auto selected = state->skeletonState->selectedNodes.size();
        nodeCountLabel.setText(tr("%1 nodes").arg(all) + (all != shown ? tr(", %2 shown").arg(shown) : "") + (selected != 0 ? tr(", %3 selected").arg(selected) : ""));
    };
    static auto treeRecreate = [&, this](){
        treeModel.recreate();
        updateTreeSelection();
    };
    static auto nodeRecreate = [&, this](){
        nodeModel.recreate(nodeFilterModeCombo.currentIndex() == 0);
        updateNodeSelection();
    };
    static auto allRecreate = [&](){
        treeRecreate();
        nodeRecreate();
    };
    // populate count labels
    updateTreeSelection();
    updateNodeSelection();

    QObject::connect(&treeFilterCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int index) {
        treeModel.mode = static_cast<TreeModel::SynapseDisplayModes>(index);
        treeRecreate();
    });

    QObject::connect(&nodeFilterGroupBox, &QGroupBox::clicked, [this](const bool checked) {
        nodeModel.mode = NodeModel::FilterMode::All;
        if (checked) {
            for (auto * checkbox : nodeFilterButtonGroup.buttons()) {
                if (checkbox->isChecked()) {
                    nodeModel.mode |= QFlags<NodeModel::FilterMode>(nodeFilterButtonGroup.id(checkbox));
                }
            }
        }
        nodeRecreate();
    });
    QObject::connect(&nodeFilterModeCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [](const int) { nodeRecreate(); });
    QObject::connect(&nodeFilterButtonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this](const int id) {
        const auto flag = QFlags<NodeModel::FilterMode>(id);
        nodeModel.mode = nodeFilterButtonGroup.button(id)->isChecked() ? nodeModel.mode | flag : nodeModel.mode & ~flag;
        nodeRecreate();
    });

    const auto treeIndex = [this](const auto & tree){
        const auto it = std::find_if(std::cbegin(treeModel.cache), std::cend(treeModel.cache), [&tree](const auto & elem){
            return elem.get().treeID == tree.treeID;
        });
        return static_cast<int>(std::distance(std::cbegin(treeModel.cache), it));
    };
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeAddedSignal, allRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeChangedSignal, [this, treeIndex](const auto & tree){
        const auto index = treeIndex(tree);
        treeModel.dataChanged(treeModel.index(index, 0), treeModel.index(index, treeModel.columnCount() - 1));
    });
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeRemovedSignal, allRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treesMerged, treeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeSelectionChangedSignal, [this](){
        if (!treeModel.selectionFromModel) {
            treeRecreate();// show active node from vp selection
        } else {
            updateTreeSelection();
        }
        treeModel.selectionFromModel = false;
        if (nodeModel.mode.testFlag(NodeModel::FilterMode::InSelectedTree)) {
            nodeRecreate();
        }
    });

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::branchPoppedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::branchPushedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeSelectionChangedSignal, [this](){
        if (!nodeModel.selectionFromModel) {
            nodeRecreate();// show active node from vp selection
        } else {
            updateNodeSelection();
        }
        nodeModel.selectionFromModel = false;
    });

    const auto nodeIndex = [this](const auto & node){
        const auto it = std::find_if(std::cbegin(nodeModel.cache), std::cend(nodeModel.cache), [&node](const auto & elem){
            return elem.get().nodeID == node.nodeID;
        });
        return static_cast<int>(std::distance(std::cbegin(nodeModel.cache), it));
    };
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeAddedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeChangedSignal, [this, nodeIndex](const auto & node){
        const auto index = nodeIndex(node);
        nodeModel.dataChanged(nodeModel.index(index, 0), nodeModel.index(index, nodeModel.columnCount() - 1));
    });
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeRemovedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::jumpedToNodeSignal, [this, nodeIndex](const auto & node){
        const auto modelIndex = nodeSortAndCommentFilterProxy.mapFromSource(nodeModel.index(nodeIndex(node), 0));
        if (modelIndex.isValid()) {
            nodeView.scrollTo(modelIndex);
        }
    });

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::resetData, allRecreate);

    static auto filter = [](auto & regex, auto & model, const QString & filterText) {
        if (regex.isChecked()) {
            model.setFilterRegExp(filterText);
        } else {
            model.setFilterFixedString(filterText);
        }
    };
    static auto treeFilter = [this](const QString & filterText) {
        filter(treeRegex, treeSortAndCommentFilterProxy, filterText);
        updateTreeSelection();
    };
    static auto nodeFilter = [this](const QString & filterText) {
        filter(nodeRegex, nodeSortAndCommentFilterProxy, filterText);
        updateNodeSelection();
    };
    QObject::connect(&treeCommentFilter, &QLineEdit::textChanged, treeFilter);
    QObject::connect(&treeRegex, &QCheckBox::clicked, [this](){ return treeFilter(treeCommentFilter.text()); });
    QObject::connect(&nodeCommentFilter, &QLineEdit::textChanged, nodeFilter);
    QObject::connect(&nodeRegex, &QCheckBox::clicked, [this](){ return nodeFilter(nodeCommentFilter.text()); });

    QObject::connect(&treeModel, &TreeModel::moveNodes, [this](const QModelIndex & parent){
        const auto index = parent.row();//already is a source model index
        const auto droppedOnTreeID = treeModel.cache[index].get().treeID;
        const auto text = tr("Do you really want to move selected nodes to tree %1?").arg(droppedOnTreeID);
        question([droppedOnTreeID](){Skeletonizer::singleton().moveSelectedNodesToTree(droppedOnTreeID);}, tr("Move"), text);
    });

    QObject::connect(treeView.selectionModel(), &QItemSelectionModel::selectionChanged, selectElems<treeListElement>(treeView, treeModel, treeSortAndCommentFilterProxy));
    QObject::connect(nodeView.selectionModel(), &QItemSelectionModel::selectionChanged, selectElems<nodeListElement>(nodeView, nodeModel, nodeSortAndCommentFilterProxy));

    QObject::connect(treeView.header(), &QHeaderView::sortIndicatorChanged, threeWaySorting(treeView, treeSortSectionIndex));
    QObject::connect(nodeView.header(), &QHeaderView::sortIndicatorChanged, threeWaySorting(nodeView, nodeSortSectionIndex));

    QObject::connect(&treeView, &QTreeView::doubleClicked, [this](const QModelIndex & index){
        if (index.column() == 1) {
            colorDialog.setCurrentColor(treeView.model()->data(index, Qt::BackgroundRole).value<QColor>());
            if (state->viewer->suspend([this]{ return colorDialog.exec(); }) == QColorDialog::Accepted) {
                treeView.model()->setData(index, colorDialog.currentColor());
            }
        }
    });

    treeView.setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    QObject::connect(&treeView, &QTreeView::customContextMenuRequested, [this](const QPoint & pos){
        int i = 0, copyActionIndex, deleteActionIndex;

        bool containsMesh{false};
        for(const auto & selectedTree : state->skeletonState->selectedTrees) {
            if(selectedTree->mesh != nullptr) {
                containsMesh = true;
                break;
            }
        }
        const auto & selectedTrees = state->skeletonState->selectedTrees;
        const auto nodeEditing = Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing);
        const auto tracingAdvanced = Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_TracingAdvanced);
        mergeAction->setVisible(tracingAdvanced);
        moveNodesAction->setVisible(tracingAdvanced);

        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() == 1);//show
        treeContextMenu.actions().at(i++)->setVisible(selectedTrees.size() == 1 && selectedTrees.front()->isSynapticCleft);//seperator
        treeContextMenu.actions().at(i++)->setVisible(tracingAdvanced
                                                      && selectedTrees.size() == 1
                                                      && selectedTrees.front()->isSynapticCleft
                                                      && selectedTrees.front()->properties.contains("preSynapse")); //jump to preSynapse
        treeContextMenu.actions().at(i++)->setVisible(tracingAdvanced
                                                      && selectedTrees.size() == 1
                                                      && selectedTrees.front()->isSynapticCleft
                                                      && selectedTrees.front()->properties.contains("postSynapse")); //jump to postSynapse
        treeContextMenu.actions().at(i++)->setVisible(tracingAdvanced
                                                      && selectedTrees.size() == 1
                                                      && selectedTrees.front()->isSynapticCleft
                                                      && selectedTrees.front()->properties.contains("preSynapse")
                                                      && selectedTrees.front()->properties.contains("postSynapse")); //reverse synapse direction
        ++i;// separator
        treeContextMenu.actions().at(copyActionIndex = i++)->setEnabled(selectedTrees.size() > 0);//copy selected contents
        ++i;// separator
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() == 1 && state->skeletonState->selectedNodes.size() > 0);//move nodes
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() >= 2);//merge trees action
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() > 0);//set comment
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() > 0);//show
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() > 0);//hide
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() > 0);//restore default color
        treeContextMenu.actions().at(i++)->setVisible(selectedTrees.size() > 0 && containsMesh);//remove mesh
        treeContextMenu.actions().at(deleteActionIndex = i++)->setEnabled(nodeEditing && selectedTrees.size() > 0);//delete
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        treeContextMenu.exec(treeView.viewport()->mapToGlobal(pos));
        // make shortcuted actions always available for when ctx menu isn’t shown
        treeContextMenu.actions().at(copyActionIndex)->setEnabled(true);
        treeContextMenu.actions().at(deleteActionIndex)->setEnabled(true);
    });
    nodeView.setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    QObject::connect(&nodeView, &QTreeView::customContextMenuRequested, [this](const QPoint & pos){
        int i = 0, copyActionIndex, deleteActionIndex;
        const auto & selectedNodes = state->skeletonState->selectedNodes;
        const auto nodeEditing = Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing);
        const auto tracingAdvanced = Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_TracingAdvanced);
        const auto synapseNode = selectedNodes.size() == 1 && selectedNodes.front()->isSynapticNode;
        extractComponentAction->setVisible(tracingAdvanced);
        linkAction->setVisible(tracingAdvanced);
        nodeContextMenu.actions().at(i++)->setEnabled(selectedNodes.size() == 1);//jump to node
        nodeContextMenu.actions().at(i++)->setEnabled(!state->skeletonState->nodesByNodeID.empty());//jump to node with id
        nodeContextMenu.actions().at(i++)->setEnabled(selectedNodes.size() > 0);//select containing trees
        nodeContextMenu.actions().at(i++)->setVisible(tracingAdvanced && synapseNode);// synapse separator
        nodeContextMenu.actions().at(i++)->setVisible(tracingAdvanced && synapseNode);//jump to corresponding cleft
        nodeContextMenu.actions().at(i++)->setVisible(tracingAdvanced
                                                      && (synapseNode || (selectedNodes.size() == 2 && selectedNodes.front()->isSynapticNode
                                                                          && selectedNodes.front()->correspondingSynapse == selectedNodes[1]->correspondingSynapse))); // reverse synapse direction
        ++i;// separator
        nodeContextMenu.actions().at(copyActionIndex = i++)->setEnabled(selectedNodes.size() > 0);//copy selected contents
        ++i;// separator
        nodeContextMenu.actions().at(i++)->setEnabled(tracingAdvanced && selectedNodes.size() == 1);//split connected components
        nodeContextMenu.actions().at(i++)->setEnabled(tracingAdvanced && selectedNodes.size() == 2);//link nodes needs two selected nodes
        nodeContextMenu.actions().at(i++)->setEnabled(nodeEditing && selectedNodes.size() > 0);//set comment
        nodeContextMenu.actions().at(i++)->setEnabled(nodeEditing && selectedNodes.size() > 0);//set radius
        nodeContextMenu.actions().at(deleteActionIndex = i++)->setEnabled(nodeEditing && selectedNodes.size() > 0);//delete
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        nodeContextMenu.exec(nodeView.viewport()->mapToGlobal(pos));
        // make shortcuted actions always available for when ctx menu isn’t shown
        nodeContextMenu.actions().at(copyActionIndex)->setEnabled(true);
        nodeContextMenu.actions().at(deleteActionIndex)->setEnabled(true);
    });

    QObject::connect(treeContextMenu.addAction("&Jump to first node"), &QAction::triggered, [](){
        const auto * tree = state->skeletonState->selectedTrees.front();
        if (!tree->nodes.empty()) {
            Skeletonizer::singleton().jumpToNode(tree->nodes.front());
        }
    });
    addDisabledSeparator(treeContextMenu);
    QObject::connect(treeContextMenu.addAction("Jump to preSynapse"), &QAction::triggered, [](){
        const auto * tree = state->skeletonState->selectedTrees.front();
        assert(tree->properties.contains("preSynapse"));
        Skeletonizer::singleton().jumpToNode(*state->skeletonState->nodesByNodeID[tree->properties["preSynapse"].toLongLong()]);
    });
    QObject::connect(treeContextMenu.addAction("Jump to postSynapse"), &QAction::triggered, [](){
        const auto * tree = state->skeletonState->selectedTrees.front();
        assert(tree->properties.contains("preSynapse"));
        Skeletonizer::singleton().jumpToNode(*state->skeletonState->nodesByNodeID[tree->properties["postSynapse"].toLongLong()]);
    });
    QObject::connect(treeContextMenu.addAction("Reverse synapse direction"), &QAction::triggered, this, &SkeletonView::reverseSynapseDirection);
    addDisabledSeparator(treeContextMenu);
    copyAction(treeContextMenu, treeView);
    addDisabledSeparator(treeContextMenu);
    moveNodesAction = treeContextMenu.addAction("Move selected nodes to this tree");
    QObject::connect(moveNodesAction, &QAction::triggered, [](){
        const auto treeID = state->skeletonState->selectedTrees.front()->treeID;
        const auto text = tr("Do you really want to move selected nodes to tree %1?").arg(treeID);
        question([treeID](){ Skeletonizer::singleton().moveSelectedNodesToTree(treeID); }, tr("Move"), text);
    });
    mergeAction = treeContextMenu.addAction("&Merge trees");
    QObject::connect(mergeAction, &QAction::triggered, [](){
        question([](){
            auto initiallySelectedTrees = state->skeletonState->selectedTrees;//HACK mergeTrees clears the selection by setting the merge result active
            while (initiallySelectedTrees.size() >= 2) {//2 at a time
                const auto treeID1 = initiallySelectedTrees[0]->treeID;
                const auto treeID2 = initiallySelectedTrees[1]->treeID;
                initiallySelectedTrees.erase(std::begin(initiallySelectedTrees)+1);//HACK mergeTrees deletes tree2
                Skeletonizer::singleton().mergeTrees(treeID1, treeID2);
            }
        }, tr("Merge"), tr("Do you really want to merge all selected trees?"));
    });
    QObject::connect(treeContextMenu.addAction("Set &comment for trees"), &QAction::triggered, [this](){
        bool applied = false;
        static auto prevComment = QString("");
        auto comment = QInputDialog::getText(this, "Edit tree comment", "New tree comment", QLineEdit::Normal, prevComment, &applied);
        if (applied) {
            prevComment = comment;
            Skeletonizer::singleton().bulkOperation(state->skeletonState->selectedTrees, [comment](auto & tree){
                Skeletonizer::singleton().setComment(tree, comment);
            });
        }
    });
    QObject::connect(treeContextMenu.addAction("Show selected trees"), &QAction::triggered, [](){
        for (auto * tree : state->skeletonState->selectedTrees) {
            tree->render = true;
        }
    });
    QObject::connect(treeContextMenu.addAction("Hide selected trees"), &QAction::triggered, [](){
        for (auto * tree : state->skeletonState->selectedTrees) {
            tree->render = false;
        }});
    QObject::connect(treeContextMenu.addAction("Restore default color"), &QAction::triggered, [](){
        Skeletonizer::singleton().bulkOperation(state->skeletonState->selectedTrees, [](auto & tree){
            Skeletonizer::singleton().restoreDefaultTreeColor(tree);
        });
    });
    QObject::connect(treeContextMenu.addAction("&Remove meshes from trees"), &QAction::triggered, [](){
        Skeletonizer::singleton().bulkOperation(state->skeletonState->selectedTrees, [](auto & tree){
            if(tree.mesh) {
                Skeletonizer::singleton().deleteMeshOfTree(tree);
            }
        });
    });
    deleteAction(treeContextMenu, treeView, tr("&Delete trees"), [](){// this is also a shortcut and needs checks here
        if (Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing) && !state->skeletonState->selectedTrees.empty()) {
            question([](){ Skeletonizer::singleton().deleteSelectedTrees(); }, tr("Delete"), tr("Delete selected trees?"));
        }
    });

    QObject::connect(nodeContextMenu.addAction("&Jump to node"), &QAction::triggered, [](){
        Skeletonizer::singleton().jumpToNode(*state->skeletonState->selectedNodes.front());
    });
    QObject::connect(nodeContextMenu.addAction("Jump to node with &ID"), &QAction::triggered, [this](){
        auto validatedInput = [](QWidget * parent, const QString && labelText, const QValidator && validator){
            QDialog dialog(parent);
            QFormLayout flayout;
            QLineEdit edit;
            edit.setPlaceholderText("node ID");
            edit.setValidator(&validator);
            QLabel warningText;
            QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
            buttonBox.button(QDialogButtonBox::Ok)->setEnabled(false);
            flayout.addRow(labelText, &edit);
            flayout.addRow(&warningText);
            flayout.addRow(&buttonBox);
            dialog.setLayout(&flayout);
            edit.setFocus();
            QObject::connect(&edit, &QLineEdit::textChanged, [&warningText, &buttonBox](const QString & text){
                const bool nodeExists{Skeletonizer::singleton().findNodeByNodeID(text.toLongLong()) != nullptr};
                warningText.setText(!nodeExists ? "<font color='red'>The node with the specified ID does not exist.</font>" : "");
                buttonBox.button(QDialogButtonBox::Ok)->setEnabled(nodeExists);
            });
            QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
            const bool done{dialog.exec() == QDialog::Accepted};
            return boost::make_optional(done, Skeletonizer::singleton().findNodeByNodeID(edit.text().toLongLong()));
        };
        if (auto node = validatedInput(this, "Please enter node ID", QRegExpValidator{QRegExp{R"regex(\d+)regex"}})) {
            Skeletonizer::singleton().setActive(*node.get());
            Skeletonizer::singleton().jumpToNode(*node.get());
        }
    });

    QObject::connect(nodeContextMenu.addAction("Select containing trees"), &QAction::triggered, []() {
        Skeletonizer::singleton().inferTreeSelectionFromNodeSelection();
    });

    addDisabledSeparator(nodeContextMenu);
    QObject::connect(nodeContextMenu.addAction("&Jump to corresponding cleft"), &QAction::triggered, [](){
        const auto & correspondingSynapse = state->skeletonState->selectedNodes.front()->correspondingSynapse;
        Skeletonizer::singleton().jumpToNode(correspondingSynapse->getCleft()->nodes.front());
    });
    swapSynapseDirectionAction = nodeContextMenu.addAction("Reverse synapse direction");
    swapSynapseDirectionAction->setShortcut(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_C);
    swapSynapseDirectionAction->setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect(swapSynapseDirectionAction, &QAction::triggered, this, &SkeletonView::reverseSynapseDirection);
    addDisabledSeparator(nodeContextMenu);
    copyAction(nodeContextMenu, nodeView);
    addDisabledSeparator(nodeContextMenu);
    extractComponentAction = nodeContextMenu.addAction("&Extract connected component");
    QObject::connect(extractComponentAction, &QAction::triggered, [](){
        auto res = QMessageBox::Ok;
        static bool askExtractConnectedComponent = true;
        if (askExtractConnectedComponent) {
            const auto msg = tr("Do you really want to extract all nodes in this component into a new tree?");

            QMessageBox msgBox{QApplication::activeWindow()};
            msgBox.setIcon(QMessageBox::Question);
            msgBox.setText(msg);
            msgBox.addButton(QMessageBox::Ok);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            QCheckBox dontShowCheckBox(tr("Hide message until restart"));
            msgBox.addButton(&dontShowCheckBox, QMessageBox::ActionRole);
            QSignalBlocker blocker{dontShowCheckBox};// even ActionRole doesn’t seem to prevent the msg box from closing
            res = static_cast<QMessageBox::StandardButton>(msgBox.exec());
            askExtractConnectedComponent = dontShowCheckBox.checkState() == Qt::Unchecked;
        }
        if (res == QMessageBox::Ok) {
            const auto treeID = state->skeletonState->selectedTrees.front()->treeID;
            const bool extracted = Skeletonizer::singleton().extractConnectedComponent(state->skeletonState->selectedNodes.front()->nodeID);
            if (!extracted) {
                QMessageBox box{QApplication::activeWindow()};
                box.setIcon(QMessageBox::Information);
                box.setText(tr("Nothing to extract"));
                box.setInformativeText(tr("The component spans an entire tree."));
                box.exec();
            } else {
                auto msg = tr("Extracted from %1").arg(treeID);
                if (Skeletonizer::singleton().findTreeByTreeID(treeID) == nullptr) {
                    msg = tr("Extracted from deleted %1").arg(treeID);
                }
                Skeletonizer::singleton().setComment(state->skeletonState->trees.back(), msg);
            }
        }
    });
    linkAction = nodeContextMenu.addAction("&Link/Unlink nodes");
    QObject::connect(linkAction, &QAction::triggered, [](){
        checkedToggleNodeLink(*state->skeletonState->selectedNodes[0], *state->skeletonState->selectedNodes[1]);
    });
    QObject::connect(nodeContextMenu.addAction("Set &comment for nodes"), &QAction::triggered, [this](){
        bool applied = false;
        static auto prevComment = QString("");
        const auto comment = state->viewer->suspend([this, &applied]{
            return QInputDialog::getText(this, "Edit node comment", "New node comment", QLineEdit::Normal, prevComment, &applied);
        });
        if (applied) {
            Skeletonizer::singleton().bulkOperation(state->skeletonState->selectedNodes, [comment](auto & node){
                Skeletonizer::singleton().setComment(node, comment);
            });
            prevComment = comment;// save for the next time the dialog is opened
        }
    });
    QObject::connect(nodeContextMenu.addAction("Set &radius for nodes"), &QAction::triggered, [this](){
        bool applied = false;
        static auto prevRadius = Skeletonizer::singleton().skeletonState.defaultNodeRadius;
        const auto radius = state->viewer->suspend([this, &applied]{
            return QInputDialog::getDouble(this, "Edit node radii", "New node radius", prevRadius, 0, nodeListElement::MAX_RADIUS_SETTING, 1, &applied);
        });
        if (applied) {
            Skeletonizer::singleton().bulkOperation(state->skeletonState->selectedNodes, [radius](auto & node){
                Skeletonizer::singleton().editNode(0, &node, radius, node.position, node.createdInMag);
            });
            prevRadius = radius;// save for the next time the dialog is opened
        }
    });
    deleteAction(nodeContextMenu, nodeView, tr("&Delete nodes"), [](){// this is also a shortcut and needs checks here
        if (Session::singleton().annotationMode.testFlag(AnnotationMode::NodeEditing) && !state->skeletonState->selectedNodes.empty()) {
            if (state->skeletonState->selectedNodes.size() == 1) {//don’t ask for one node
                Skeletonizer::singleton().deleteSelectedNodes();
            } else {
                question([](){ Skeletonizer::singleton().deleteSelectedNodes(); }, tr("Delete"), tr("Delete the selected nodes?"));
            }
        }
    });

    treeContextMenu.setDefaultAction(treeContextMenu.actions().front());
    QObject::connect(&treeView, &QAbstractItemView::doubleClicked, [this](const QModelIndex & index){
        if (index.column() == 0) {
            treeContextMenu.defaultAction()->trigger();
        }
    });
    nodeContextMenu.setDefaultAction(nodeContextMenu.actions().front());
    QObject::connect(&nodeView, &QAbstractItemView::doubleClicked, [this](const QModelIndex & index){
        if (index.column() == 0) {
            nodeContextMenu.defaultAction()->trigger();
        }
    });
}

auto jumoTo = [](const auto & view, const auto & model, const auto & proxy, const bool forward, auto func) {
    auto current = view.currentIndex();
    auto next = forward ? view.indexBelow(current) : view.indexAbove(current);
    next = !next.isValid() ? current : next;// cap over- and underflow
    const auto firstOrLast = (forward ? view.model()->index(0, 0) : view.model()->index(view.model()->rowCount() - 1, 0));
    next = !next.isValid() ? firstOrLast : next;// jump into table
    if (next.isValid()) {// no current index will fail here
        auto & elem = model.cache[static_cast<std::size_t>(proxy.mapToSource(next).row())].get();// mapping to source always succeeds
        Skeletonizer::singleton().setActive(elem);
        func(elem);
    }
};

void SkeletonView::jumpToNextNode(const bool forward) const {
    jumoTo(nodeView, nodeModel, nodeSortAndCommentFilterProxy, forward, [](auto && elem){
        Skeletonizer::singleton().jumpToNode(elem);
    });
}

void SkeletonView::jumpToNextTree(const bool forward) const {
    jumoTo(treeView, treeModel, treeSortAndCommentFilterProxy, forward, [](auto && elem){
        if (elem.nodes.size() > 0) {
            Skeletonizer::singleton().setActive(elem.nodes.front());
            Skeletonizer::singleton().jumpToNode(elem.nodes.front());
        }
    });
}

void SkeletonView::reverseSynapseDirection() {
    const auto & selectedNodes = state->skeletonState->selectedNodes;
    const auto & selectedTrees = state->skeletonState->selectedTrees;
    if((selectedNodes.size() == 1 && selectedNodes.front()->isSynapticNode && selectedNodes.front()->correspondingSynapse != nullptr) ||
       (selectedNodes.size() == 2 && selectedNodes.front()->isSynapticNode && selectedNodes[1]->correspondingSynapse == selectedNodes.front()->correspondingSynapse)) {
        selectedNodes.front()->correspondingSynapse->toggleDirection();
    } else if (selectedTrees.size() == 1 && selectedTrees.front()->isSynapticCleft) {
        selectedTrees.front()->correspondingSynapse->toggleDirection();
    } else {
        // if two are selected, they should belong to the same synapse.
        QMessageBox info{QApplication::activeWindow()};
        info.setIcon(QMessageBox::Information);
        info.setText(tr("Select one or two corresponding synapse nodes first"));
        info.exec();
        return;
    }
}
