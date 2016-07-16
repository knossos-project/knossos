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

#include "skeletonview.h"

#include "model_helper.h"
#include "session.h"
#include "skeleton/node.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "viewer.h"//state->viewerState->renderInterval

#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegExpValidator>

template<typename Func>
void question(QWidget * const parent, Func func, const QString & acceptButtonText, const QString & text, const QString & extraText = "") {
    QMessageBox prompt(parent);
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

QVariant TreeModel::data(const QModelIndex &index, int role) const {
    const auto & tree = cache[index.row()].get();
    if (&tree == state->skeletonState->activeTree && index.column() == 0 && role == Qt::DecorationRole) {
        return QPixmap(":/resources/icons/active-arrow.png").scaled(QSize(8, 8), Qt::KeepAspectRatio);
    } else if (index.column() == 1 && (role == Qt::BackgroundRole || role == Qt::DecorationRole)) { // background role not visible for selected row under gnome…
        return tree.color;
    } else if (index.column() == 2 && role == Qt::CheckStateRole) {
        return tree.render ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return tree.treeID;
        case 3: return static_cast<quint64>(tree.nodes.size());
        case 4: return tree.getComment();
        case 5:
            QString propertyString("");
            for (auto it = std::cbegin(tree.properties); it != std::cend(tree.properties); ++it) {
                if (it.key() != "comment") {
                    propertyString += it.key() + ": " + it.value().toString() + "; ";
                }
            }
            return propertyString;
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
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return static_cast<quint64>(node.nodeID);
        case 1: return node.position.x + 1;
        case 2: return node.position.y + 1;
        case 3: return node.position.z + 1;
        case 4: return node.radius;
        case 5: return node.getComment();
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
        if ((mode.testFlag(FilterMode::SynapticClefts) && tree.isSynapticCleft)
                || (mode.testFlag(FilterMode::Default) && !tree.isSynapticCleft)) {
            tree.render = true;
            cache.emplace_back(tree);
        } else {
            tree.render = false;
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
        if (matchAll == false) {
            for (auto && node : cache) {
                if ((mode.testFlag(FilterMode::Selected) && node.get().selected)
                        || (mode.testFlag(FilterMode::InSelectedTree) && node.get().correspondingTree->selected)
                        || (mode.testFlag(FilterMode::Branch) && node.get().isBranchNode)
                        || (mode.testFlag(FilterMode::Comment) && node.get().getComment().isEmpty() == false)
                        || (mode.testFlag(FilterMode::Synapse) && node.get().isSynapticNode)) {
                    hits.emplace_back(node);
                }
            }
        } else {
            for (auto && node : cache) { // show node if for all criteria: either criterion not demanded or fulfilled.
                if ((!mode.testFlag(FilterMode::Selected) || node.get().selected)
                        && (!mode.testFlag(FilterMode::InSelectedTree) || node.get().correspondingTree->selected)
                        && (!mode.testFlag(FilterMode::Branch) || node.get().isBranchNode)
                        && (!mode.testFlag(FilterMode::Comment) || node.get().getComment().isEmpty() == false)
                        && (!mode.testFlag(FilterMode::Synapse) || node.get().isSynapticNode)) {
                    hits.emplace_back(node);
                }
            }
        }
        cache = hits;
    }
    endResetModel();
}

void NodeView::mousePressEvent(QMouseEvent * event) {
    const auto index = proxy.mapToSource(indexAt(event->pos()));
    if (index.isValid()) {//enable drag’n’drop only for selected items to retain rubberband selection
        const auto selected = source.cache[index.row()].get().selected && !event->modifiers().testFlag(Qt::ControlModifier) && !event->modifiers().testFlag(Qt::ShiftModifier);
        setDragDropMode(selected ? QAbstractItemView::DragOnly : QAbstractItemView::NoDragDrop);
    }
    QTreeView::mousePressEvent(event);
}

template<typename T, typename Model, typename Proxy>
auto selectElems(Model & model, Proxy & proxy) {
    return [&model, &proxy](const QItemSelection & selected, const QItemSelection & deselected){
        if (!model.selectionProtection) {
            const auto & proxySelected = proxy.mapSelectionToSource(selected);
            const auto & proxyDeselected = proxy.mapSelectionToSource(deselected);
            auto indices = proxySelected.indexes();
            indices.append(proxyDeselected.indexes());
            QSet<T*> elems;
            for (const auto & modelIndex : indices) {
                if (modelIndex.column() == 0) {
                    elems.insert(&model.cache[modelIndex.row()].get());
                }
            }
            model.selectionFromModel = true;
            Skeletonizer::singleton().toggleSelection(elems);
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

template<typename... Args>
void deleteAction(QMenu & menu, QTreeView & view, QString text, Args &&... args) {
    auto * deleteAction = menu.addAction(QIcon(":/resources/icons/user-trash.png"), text);
    QObject::connect(deleteAction, &QAction::triggered, args...);
    view.addAction(deleteAction);
    deleteAction->setShortcut(Qt::Key_Delete);
    deleteAction->setShortcutContext(Qt::WidgetShortcut);
}

SkeletonView::SkeletonView(QWidget * const parent) : QWidget{parent}
        , nodeView{nodeSortAndCommentFilterProxy, nodeModel} {
    auto setupTable = [this](auto & table, auto & model, auto & sortIndex){
        table.setModel(&model);
        table.setAllColumnsShowFocus(true);
        table.setUniformRowHeights(true);//perf hint from doc
        table.setRootIsDecorated(false);//remove padding to the left of each cell’s content
        table.setSelectionMode(QAbstractItemView::ExtendedSelection);
        table.setSortingEnabled(true);
        table.sortByColumn(sortIndex = 0, Qt::SortOrder::AscendingOrder);
    };

    treeCommentFilter.setPlaceholderText("tree comment");
    treeSortAndCommentFilterProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
    treeSortAndCommentFilterProxy.setFilterKeyColumn(4);
    treeSortAndCommentFilterProxy.setSourceModel(&treeModel);
    setupTable(treeView, treeSortAndCommentFilterProxy, treeSortSectionIndex);
    treeView.setDragDropMode(QAbstractItemView::DropOnly);
    treeView.setDropIndicatorShown(true);
    for (auto index : {0, 1, 2}) {
        treeView.resizeColumnToContents(index);
    }

    filterTreeButtonGroup.setExclusive(false);
    for (auto option : { std::make_pair(&filterTreeDisplayAll, TreeModel::FilterMode::Default),
                            {&filterTreeDisplaySynapse, TreeModel::FilterMode::SynapticClefts}}) {
        filterTreeButtonGroup.addButton(option.first);
        filterTreeButtonGroup.setId(option.first, option.second);
        option.first->setChecked(treeModel.mode.testFlag(option.second));
        filterTreeLayout.addWidget(option.first);
    }

    filterModeCombo.addItems({"match all", "match at least one"});
    filterLayout.addWidget(&filterModeCombo);
    filterButtonGroup.setExclusive(false);
    for (auto option : { std::make_pair(&filterInSelectedTreeCheckbox, NodeModel::FilterMode::InSelectedTree),
                         {&filterSelectedCheckbox, NodeModel::FilterMode::Selected},
                         {&filterBranchCheckbox, NodeModel::FilterMode::Branch},
                         {&filterCommentCheckbox, NodeModel::FilterMode::Comment},
                         {&filterSynapseCheckbox, NodeModel::FilterMode::Synapse}} ) {
        filterButtonGroup.addButton(option.first);
        filterButtonGroup.setId(option.first, option.second);
        option.first->setChecked(nodeModel.mode.testFlag(option.second));
        filterLayout.addWidget(option.first);
    }

    filterGroupBox.setLayout(&filterLayout);
    filterGroupBox.setCheckable(true);
    const bool showAllNodes = nodeModel.mode == NodeModel::FilterMode::All;
    filterGroupBox.setChecked(!showAllNodes);

    filterSpoilerLayout.addWidget(&filterGroupBox);
    filterSpoiler.setContentLayout(filterSpoilerLayout);

    nodeCommentFilter.setPlaceholderText("node comment");

    nodeSortAndCommentFilterProxy.setSourceModel(&nodeModel);
    nodeSortAndCommentFilterProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
    nodeSortAndCommentFilterProxy.setFilterKeyColumn(5);
    setupTable(nodeView, nodeSortAndCommentFilterProxy, nodeSortSectionIndex);

    filterOptions.addWidget(&filterSpoiler);

    treeOptionsLayout.addWidget(&treeCommentFilter);
    treeOptionsLayout.addWidget(&treeRegex);
    treeLayout.addLayout(&filterTreeLayout);
    treeLayout.addLayout(&treeOptionsLayout);
    treeLayout.addWidget(&treeView);
    treeLayout.addWidget(&treeCountLabel);
    treeLayout.setContentsMargins({});
    treeDummyWidget.setLayout(&treeLayout);
    splitter.setOrientation(Qt::Vertical);
    splitter.addWidget(&treeDummyWidget);

    nodeOptionsLayout.addWidget(&nodeCommentFilter);
    nodeOptionsLayout.addWidget(&nodeRegex);
    nodeLayout.addLayout(&filterOptions);
    nodeLayout.addLayout(&nodeOptionsLayout);
    nodeLayout.addWidget(&nodeView);
    nodeLayout.addWidget(&nodeCountLabel);
    nodeLayout.setContentsMargins({});
    nodeDummyWidget.setLayout(&nodeLayout);
    splitter.addWidget(&nodeDummyWidget);

    splitter.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);//size policy of QTreeView is also Expanding

    defaultRadiusSpin.setRange(0.01, 100000);
    defaultRadiusSpin.setValue(Skeletonizer::singleton().skeletonState.defaultNodeRadius);
    defaultRadiusSpin.setSuffix("px");
    lockingRadiusSpin.setMaximum(100000);
    lockingRadiusSpin.setValue(Skeletonizer::singleton().skeletonState.lockRadius);
    lockingRadiusSpin.setSuffix("px");

    commentConditionEdit.setPlaceholderText("comment condition for locking");
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
        nodeModel.recreate(filterModeCombo.currentIndex() == 0);
        updateNodeSelection();
    };
    static auto allRecreate = [&](){
        treeRecreate();
        nodeRecreate();
    };
    // populate count labels
    updateTreeSelection();
    updateNodeSelection();

    QObject::connect(&filterGroupBox, &QGroupBox::clicked, [this](const bool checked) {
        nodeModel.mode = NodeModel::FilterMode::All;
        if (checked) {
            for (auto * checkbox : filterButtonGroup.buttons()) {
                if (checkbox->isChecked()) {
                    nodeModel.mode |= QFlags<NodeModel::FilterMode>(filterButtonGroup.id(checkbox));
                }
            }
        }
        nodeRecreate();
    });
    QObject::connect(&filterModeCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int) { nodeRecreate(); });
    QObject::connect(&filterButtonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this](const int id) {
        const auto flag = QFlags<NodeModel::FilterMode>(id);
        nodeModel.mode = filterButtonGroup.button(id)->isChecked() ? nodeModel.mode | flag : nodeModel.mode & ~flag;
        nodeRecreate();
    });
    QObject::connect(&filterTreeButtonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this](const int id) {
        const auto flag = QFlags<TreeModel::FilterMode>(id);
        treeModel.mode = filterTreeButtonGroup.button(id)->isChecked() ? treeModel.mode | flag : treeModel.mode & ~flag;
        treeRecreate();
    });

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeAddedSignal, allRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeChangedSignal, treeRecreate);
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

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeAddedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeChangedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeRemovedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::jumpedToNodeSignal, [this](const nodeListElement & node){
        const auto it = std::find_if(std::cbegin(nodeModel.cache), std::cend(nodeModel.cache), [node](const std::reference_wrapper<nodeListElement> & elem){
            return elem.get().nodeID == node.nodeID;
        });
        const auto index = std::distance(std::cbegin(nodeModel.cache), it);
        const auto modelIndex = nodeSortAndCommentFilterProxy.mapFromSource(nodeModel.index(index, 0));
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
    QObject::connect(&treeCommentFilter, &QLineEdit::textEdited, [this](const QString & filterText) {
        filter(treeRegex, treeSortAndCommentFilterProxy, filterText);
        updateTreeSelection();
    });

    QObject::connect(&nodeCommentFilter, &QLineEdit::textEdited, [this](const QString & filterText) {
        filter(nodeRegex, nodeSortAndCommentFilterProxy, filterText);
        updateNodeSelection();
    });

    QObject::connect(&treeModel, &TreeModel::moveNodes, [this](const QModelIndex & parent){
        const auto index = parent.row();//already is a source model index
        const auto droppedOnTreeID = treeModel.cache[index].get().treeID;
        const auto text = tr("Do you really want to move selected nodes to tree %1?").arg(droppedOnTreeID);
        question(this, [droppedOnTreeID](){Skeletonizer::singleton().moveSelectedNodesToTree(droppedOnTreeID);}, tr("Move"), text);
    });

    QObject::connect(treeView.selectionModel(), &QItemSelectionModel::selectionChanged, selectElems<treeListElement>(treeModel, treeSortAndCommentFilterProxy));
    QObject::connect(nodeView.selectionModel(), &QItemSelectionModel::selectionChanged, selectElems<nodeListElement>(nodeModel, nodeSortAndCommentFilterProxy));

    QObject::connect(treeView.header(), &QHeaderView::sortIndicatorChanged, threeWaySorting(treeView, treeSortSectionIndex));
    QObject::connect(nodeView.header(), &QHeaderView::sortIndicatorChanged, threeWaySorting(nodeView, nodeSortSectionIndex));

    QObject::connect(&treeView, &QTreeView::doubleClicked, [this](const QModelIndex & index){
        if (index.column() == 1) {
            colorDialog.setCurrentColor(treeView.model()->data(index, Qt::BackgroundRole).value<QColor>());
            state->viewerState->renderInterval = SLOW;
            if (colorDialog.exec() == QColorDialog::Accepted) {
                treeView.model()->setData(index, colorDialog.currentColor());
            }
            state->viewerState->renderInterval = FAST;
        }
    });

    treeView.setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    QObject::connect(&treeView, &QTreeView::customContextMenuRequested, [this](const QPoint & pos){
        int i = 0;

        const auto & selectedTrees = state->skeletonState->selectedTrees;
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() == 1);//show
        treeContextMenu.actions().at(i++)->setVisible(selectedTrees.size() == 1
                                                      && selectedTrees.front()->isSynapticCleft
                                                      && selectedTrees.front()->properties.contains("preSynapse")); //jump to preSynapse
        treeContextMenu.actions().at(i++)->setVisible(selectedTrees.size() == 1
                                                      && selectedTrees.front()->isSynapticCleft
                                                      && selectedTrees.front()->properties.contains("postSynapse")); //jump to postSynapse
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() == 1 && state->skeletonState->selectedNodes.size() > 0);//move nodes
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() >= 2);//merge trees action
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() > 0);//set comment
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() > 0);//show
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() > 0);//hide
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() > 0);//restore default color
        treeContextMenu.actions().at(i++)->setEnabled(selectedTrees.size() > 0);//delete
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        treeContextMenu.exec(treeView.viewport()->mapToGlobal(pos));
    });
    nodeView.setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    QObject::connect(&nodeView, &QTreeView::customContextMenuRequested, [this](const QPoint & pos){
        int i = 0;
        const auto & selectedNodes = state->skeletonState->selectedNodes;
        nodeContextMenu.actions().at(i++)->setEnabled(selectedNodes.size() == 1);//jump to node
        nodeContextMenu.actions().at(i++)->setEnabled(!state->skeletonState->nodesByNodeID.empty());//jump to node with id
        nodeContextMenu.actions().at(i++)->setEnabled(selectedNodes.size() == 1);//split connected components
        nodeContextMenu.actions().at(i++)->setEnabled(selectedNodes.size() == 2);//link nodes needs two selected nodes
        nodeContextMenu.actions().at(i++)->setEnabled(selectedNodes.size() == 2
                                                      && selectedNodes.front()->isSynapticNode
                                                      && selectedNodes.back()->isSynapticNode
                                                      && selectedNodes.front()->correspondingSynapse
                                                            == selectedNodes.back()->correspondingSynapse);//swap synaptic nodes
        nodeContextMenu.actions().at(i++)->setEnabled(selectedNodes.size() > 0);//set comment
        nodeContextMenu.actions().at(i++)->setEnabled(selectedNodes.size() > 0);//set radius
        nodeContextMenu.actions().at(i++)->setEnabled(selectedNodes.size() > 0);//delete
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        nodeContextMenu.exec(nodeView.viewport()->mapToGlobal(pos));
    });


    QObject::connect(treeContextMenu.addAction("&Jump to first node"), &QAction::triggered, [this](){
        const auto * tree = state->skeletonState->selectedTrees.front();
        if (!tree->nodes.empty()) {
            Skeletonizer::singleton().jumpToNode(tree->nodes.front());
        }
    });
    QObject::connect(treeContextMenu.addAction("Jump to preSynapse"), &QAction::triggered, [this](){
        const auto * tree = state->skeletonState->selectedTrees.front();
        if(tree->properties.contains("preSynapse"))
            Skeletonizer::singleton().jumpToNode(*state->skeletonState->nodesByNodeID[tree->properties["preSynapse"].toLongLong()]);
    });
    QObject::connect(treeContextMenu.addAction("Jump to postSynapse"), &QAction::triggered, [this](){
        const auto * tree = state->skeletonState->selectedTrees.front();
        if(tree->properties.contains("preSynapse"))
            Skeletonizer::singleton().jumpToNode(*state->skeletonState->nodesByNodeID[tree->properties["postSynapse"].toLongLong()]);
    });
    QObject::connect(treeContextMenu.addAction("Move selected nodes to this tree"), &QAction::triggered, [this](){
        const auto treeID = state->skeletonState->selectedTrees.front()->treeID;
        const auto text = tr("Do you really want to move selected nodes to tree %1?").arg(treeID);
        question(this, [treeID](){ Skeletonizer::singleton().moveSelectedNodesToTree(treeID); }, tr("Move"), text);
    });
    QObject::connect(treeContextMenu.addAction("&Merge trees"), &QAction::triggered, [this](){
        question(this, [](){
            auto initiallySelectedTrees = state->skeletonState->selectedTrees;//HACK mergeTrees clears the selection by setting the merge result active
            while (initiallySelectedTrees.size() >= 2) {//2 at a time
                const int treeID1 = initiallySelectedTrees[0]->treeID;
                const int treeID2 = initiallySelectedTrees[1]->treeID;
                initiallySelectedTrees.erase(std::begin(initiallySelectedTrees)+1);//HACK mergeTrees deletes tree2
                Skeletonizer::singleton().mergeTrees(treeID1, treeID2);
            }
        }, tr("Merge"), tr("Do you really want to merge all selected trees?"));
    });
    QObject::connect(treeContextMenu.addAction("Set &comment for trees"), &QAction::triggered, [this](){
        bool applied = false;
        static auto prevComment = QString("");
        auto comment = QInputDialog::getText(this, "Edit Tree Comment", "new tree comment", QLineEdit::Normal, prevComment, &applied);
        if (applied) {
            prevComment = comment;
            Skeletonizer::singleton().setCommentOfSelectedTrees(comment);
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
        for (auto * tree : state->skeletonState->selectedTrees) {
            Skeletonizer::singleton().restoreDefaultTreeColor(*tree);
        }
    });
    deleteAction(treeContextMenu, treeView, tr("&Delete trees"), [this](){
        if (!state->skeletonState->selectedTrees.empty()) {
            question(this, [](){ Skeletonizer::singleton().deleteSelectedTrees(); }, tr("Delete"), tr("Delete the selected trees?"));
        }
    });

    QObject::connect(nodeContextMenu.addAction("&Jump to node"), &QAction::triggered, [this](){
        Skeletonizer::singleton().jumpToNode(*state->skeletonState->selectedNodes.front());
    });
    QObject::connect(nodeContextMenu.addAction("&Jump to node with ID"), &QAction::triggered, [this](){
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
                warningText.setText(!nodeExists ? "<font color='red'>The node with the specified ID does not exist.</color>" : "");
                buttonBox.button(QDialogButtonBox::Ok)->setEnabled(nodeExists);
            });
            QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
            const bool done{dialog.exec() == QDialog::Accepted};
            return boost::make_optional(done, Skeletonizer::singleton().findNodeByNodeID(edit.text().toLongLong()));
        };
        if (auto node = validatedInput(this, "Please enter node ID", QRegExpValidator{QRegExp{"\\d+"}})) {
            Skeletonizer::singleton().setActive(*node.get());
            Skeletonizer::singleton().jumpToNode(*node.get());
        }
    });
    QObject::connect(nodeContextMenu.addAction("&Extract connected component"), &QAction::triggered, [this](){
        auto res = QMessageBox::Ok;
        static bool askExtractConnectedComponent = true;
        if (askExtractConnectedComponent) {
            const auto msg = tr("Do you really want to extract all nodes in this component into a new tree?");

            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Question);
            msgBox.setText(msg);
            msgBox.addButton(QMessageBox::Ok);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            QCheckBox dontShowCheckBox(tr("Hide message until restart"));
            dontShowCheckBox.blockSignals(true);
            msgBox.addButton(&dontShowCheckBox, QMessageBox::ResetRole);
            res = static_cast<QMessageBox::StandardButton>(msgBox.exec());
            askExtractConnectedComponent = dontShowCheckBox.checkState() == Qt::Unchecked;
        }
        if (res == QMessageBox::Ok) {
            const auto treeID = state->skeletonState->selectedTrees.front()->treeID;
            const bool extracted = Skeletonizer::singleton().extractConnectedComponent(state->skeletonState->selectedNodes.front()->nodeID);
            if (!extracted) {
                QMessageBox::information(this, "Nothing to extract", "The component spans an entire tree.");
            } else {
                auto msg = tr("Extracted from %1").arg(treeID);
                if (Skeletonizer::singleton().findTreeByTreeID(treeID) == nullptr) {
                    msg = tr("Extracted from deleted %1").arg(treeID);
                }
                Skeletonizer::singleton().setComment(state->skeletonState->trees.back(), msg);
            }
        }
    });
    QObject::connect(nodeContextMenu.addAction("&Link/Unlink nodes"), &QAction::triggered, [this](){
        Skeletonizer::singleton().toggleConnectionOfFirstPairOfSelectedNodes(this);
    });
    QObject::connect(nodeContextMenu.addAction("Swap synaptic nodes"), &QAction::triggered, [this](){
        const auto & selectedNodes = state->skeletonState->selectedNodes;
        if(selectedNodes.size() == 2
                && selectedNodes.front()->isSynapticNode
                && selectedNodes.back()->isSynapticNode
                && selectedNodes.front()->correspondingSynapse
                        == selectedNodes.back()->correspondingSynapse) {
            auto & synapse = state->skeletonState->selectedNodes.front()->correspondingSynapse;
            std::swap(synapse->synapticCleft->properties["preSynapse"], synapse->synapticCleft->properties["postSynapse"]);
            std::swap(synapse->postSynapse, synapse->preSynapse);
            allRecreate();
        }
    });
    QObject::connect(nodeContextMenu.addAction("Set &comment for nodes"), &QAction::triggered, [this](){
        bool applied = false;
        static auto prevComment = QString("");
        auto comment = QInputDialog::getText(this, "Edit Node Comment", "new node comment", QLineEdit::Normal, prevComment, &applied);
        if (applied) {
            prevComment = comment;
            for (auto node : state->skeletonState->selectedNodes) {
                Skeletonizer::singleton().setComment(*node,comment);
            }
        }
    });
    QObject::connect(nodeContextMenu.addAction("Set &radius for nodes"), &QAction::triggered, [this](){
        bool applied = false;
        static auto prevRadius = Skeletonizer::singleton().skeletonState.defaultNodeRadius;
        auto radius = QInputDialog::getDouble(this, "Edit Node Radii", "new node radius", prevRadius, 0, 100000, 1, &applied);
        if (applied) {
            prevRadius = radius;
            for (auto * node : state->skeletonState->selectedNodes) {
                Skeletonizer::singleton().editNode(0, node, radius, node->position, node->createdInMag);
            }
        }
    });
    deleteAction(nodeContextMenu, nodeView, tr("&Delete nodes"), [this](){
        if (!state->skeletonState->selectedNodes.empty()) {
            if (state->skeletonState->selectedNodes.size() == 1) {//don’t ask for one node
                Skeletonizer::singleton().deleteSelectedNodes();
            } else {
                question(this, [](){ Skeletonizer::singleton().deleteSelectedNodes(); }, tr("Delete"), tr("Delete the selected nodes?"));
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

QString SkeletonView::getFilterComment() const {
    return nodeCommentFilter.text();
}
