#include "skeletontab.h"

#include <skeleton/node.h>
#include <skeleton/skeletonizer.h>
#include <skeleton/tree.h>

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

int TreeModel::rowCount(const QModelIndex &) const {
    return state->skeletonState->treeElements;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const {
    auto * tree = state->skeletonState->firstTree.get();
    for (int i = 0; i < index.row(); ++i, tree = tree->next.get());

    if (index.column() == 2 && role == Qt::BackgroundRole) {
        return QColor(tree->color.r * 255, tree->color.g * 255, tree->color.b * 255, tree->color.a * 255);
    } else if (index.column() == 4 && role == Qt::CheckStateRole) {
        return tree->render ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return tree->treeID;
        case 1: return tree->comment;
        case 3: return static_cast<quint64>(tree->size);
        }
    }
    return QVariant();//return invalid QVariant
}

bool TreeModel::setData(const QModelIndex & index, const QVariant & value, int role) {
    if (index.isValid()) {
        return false;
    }
    auto * tree = state->skeletonState->firstTree.get();
    for (int i = 0; i < index.row(); ++i, tree = tree->next.get());

    if (index.column() == 4 && role == Qt::CheckStateRole) {
        tree->render = value.toBool();
    } else if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == 2) {
        //TODO
    } else if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == 1) {
        strcpy(tree->comment, value.toString().toUtf8().data());
    } else {
        return false;
    }
    return true;
}

int NodeModel::rowCount(const QModelIndex &) const {
    return state->skeletonState->totalNodeElements;
}

QVariant NodeModel::data(const QModelIndex &index, int role) const {
    if (state->skeletonState->firstTree == nullptr) {
        return QVariant();//return invalid QVariant
    }
    auto * node = state->skeletonState->firstTree->firstNode.get();
    for (int i = 0; i < index.row(); ++i, node = node->next.get());

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return static_cast<quint64>(node->nodeID);
        case 1: return node->comment != nullptr ? node->comment->content : "";
        case 2: return node->position.x;
        case 3: return node->position.y;
        case 4: return node->position.z;
        case 5: return node->radius;
        }
    }
    return QVariant();//return invalid QVariant
}

bool NodeModel::setData(const QModelIndex & index, const QVariant & value, int role) {
    if (state->skeletonState->firstTree == nullptr || index.isValid() || !(role == Qt::DisplayRole || role == Qt::EditRole)) {
        return false;
    }
    auto * node = state->skeletonState->firstTree->firstNode.get();
    for (int i = 0; i < index.row(); ++i, node = node->next.get());

    if (index.column() == 1) {
        strcpy(node->comment->content, value.toString().toUtf8().data());
    } else if (index.column() == 2) {
        node->position.x = value.toInt();
    } else if (index.column() == 3) {
        node->position.y = value.toInt();
    } else if (index.column() == 4) {
        node->position.z = value.toInt();
    } else if (index.column() == 4) {
        node->radius = value.toFloat();
    } else {
        return false;
    }
    return true;
}

void TreeModel::recreate() {
    beginResetModel();
    endResetModel();
}

void NodeModel::recreate() {
    beginResetModel();
    endResetModel();
}

SkeletonTab::SkeletonTab(QWidget * const parent) : QWidget(parent) {
    treeView.setModel(&treeModel);
    treeView.setUniformRowHeights(true);//perf hint from doc
    nodeView.setModel(&nodeModel);
    nodeView.setUniformRowHeights(true);//perf hint from doc

    splitter.addWidget(&treeView);
    splitter.addWidget(&nodeView);
    splitter.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);//size policy of QTreeView is also Expanding
    bottomHLayout.addWidget(&treeCountLabel);
    bottomHLayout.addWidget(&nodeCountLabel);
    mainLayout.addWidget(&splitter);
    mainLayout.addLayout(&bottomHLayout);
    setLayout(&mainLayout);

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeAddedSignal, &treeModel, &TreeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeChangedSignal, &treeModel, &TreeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeRemovedSignal, &treeModel, &TreeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treesMerged, &treeModel, &TreeModel::recreate);
//    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeSelectionChangedSignal, &treeModel, &TreeModel::recreate);

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::branchPoppedSignal, &nodeModel, &NodeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::branchPushedSignal, &nodeModel, &NodeModel::recreate);
//    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeSelectionChangedSignal, &nodeModel, &NodeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeAddedSignal, &nodeModel, &NodeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeChangedSignal, &nodeModel, &NodeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeRemovedSignal, &nodeModel, &NodeModel::recreate);

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::resetData, [this](){
        treeModel.recreate();
        nodeModel.recreate();
    });
}
