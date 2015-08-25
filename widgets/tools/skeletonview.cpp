#include "skeletonview.h"

#include "model_helper.h"
#include "session.h"
#include "skeleton/node.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"

#include <QCheckBox>
#include <QInputDialog>
#include <QMessageBox>

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
    auto * tree = Skeletonizer::singleton().treesOrdered[index.row()];

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
    if (!index.isValid()) {
        return false;
    }
    auto * tree = Skeletonizer::singleton().treesOrdered[index.row()];

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
    auto * node = Skeletonizer::singleton().nodesOrdered[index.row()];

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
    if (state->skeletonState->firstTree == nullptr || !index.isValid() || !(role == Qt::DisplayRole || role == Qt::EditRole)) {
        return false;
    }
    auto * node = Skeletonizer::singleton().nodesOrdered[index.row()];

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

template<typename T>
std::vector<T*> ordered();
template<>
std::vector<nodeListElement*> ordered() {
    return Skeletonizer::singleton().nodesOrdered;
}
template<>
std::vector<treeListElement*> ordered() {
    return Skeletonizer::singleton().treesOrdered;
}

template<typename T>
auto selectElems(const bool & selectionProtection) {
    return [&selectionProtection](const QItemSelection & selected, const QItemSelection & deselected){
        if (!selectionProtection) {
            auto indices = selected.indexes();
            indices.append(deselected.indexes());
            QSet<T*> elems;
            for (const auto & modelIndex : indices) {
                if (modelIndex.column() == 0) {
                    elems.insert(ordered<T>()[modelIndex.row()]);
                }
            }
            Skeletonizer::singleton().toggleSelection(elems);
        }
    };
}

template<typename T, typename U>
auto updateSelection(QTreeView & view, U & model) {
    const auto selectedIndices = blockSelection(model, ordered<T>());
    model.selectionProtection = true;
    view.selectionModel()->select(selectedIndices, QItemSelectionModel::ClearAndSelect);
    model.selectionProtection = false;

    if (!selectedIndices.indexes().isEmpty()) {// scroll to first selected entry
        view.scrollTo(selectedIndices.indexes().front());
    }
}

template<typename Func>
void question(QWidget * const parent, Func func, const QString & acceptButtonText, const QString & text, const QString & extraText, const bool condition = true) {
    if (condition) {
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
}

template<typename... Args>
void deleteAction(QMenu & menu, QTreeView & view, QString text, Args &&... args) {
    auto * deleteAction = menu.addAction(QIcon(":/resources/icons/user-trash.png"), text);
    QObject::connect(deleteAction, &QAction::triggered, args...);
    view.addAction(deleteAction);
    deleteAction->setShortcut(Qt::Key_Delete);
    deleteAction->setShortcutContext(Qt::WidgetShortcut);
}

SkeletonView::SkeletonView(QWidget * const parent) : QWidget(parent) {
    treeView.setModel(&treeModel);
    treeView.setUniformRowHeights(true);//perf hint from doc
    treeView.setSelectionMode(QAbstractItemView::ExtendedSelection);
    nodeView.setModel(&nodeModel);
    nodeView.setUniformRowHeights(true);//perf hint from doc
    nodeView.setSelectionMode(QAbstractItemView::ExtendedSelection);

    splitter.addWidget(&treeView);
    splitter.addWidget(&nodeView);
    splitter.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);//size policy of QTreeView is also Expanding
    bottomHLayout.addWidget(&treeCountLabel);
    bottomHLayout.addWidget(&nodeCountLabel);
    mainLayout.addWidget(&splitter);
    mainLayout.addLayout(&bottomHLayout);
    setLayout(&mainLayout);

    static auto treeRecreate = [&, this](){
        treeView.selectionModel()->reset();
        treeModel.recreate();
        updateSelection<treeListElement>(treeView, treeModel);
    };
    static auto nodeRecreate = [&, this](){
        nodeView.selectionModel()->reset();
        nodeModel.recreate();
        updateSelection<nodeListElement>(nodeView, nodeModel);
    };

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeAddedSignal, &treeModel, &TreeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeChangedSignal, &treeModel, &TreeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeRemovedSignal, &treeModel, &TreeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treesMerged, &treeModel, &TreeModel::recreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeSelectionChangedSignal, [this](){updateSelection<treeListElement>(treeView, treeModel);});

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::branchPoppedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::branchPushedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeSelectionChangedSignal, [this](){updateSelection<nodeListElement>(nodeView, nodeModel);});

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeAddedSignal, &nodeModel, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeChangedSignal, nodeRecreate);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeRemovedSignal, &nodeModel, nodeRecreate);

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::resetData, [&, this](){
        treeRecreate();
        nodeRecreate();
    });

    QObject::connect(treeView.selectionModel(), &QItemSelectionModel::selectionChanged, selectElems<treeListElement>(treeModel.selectionProtection));
    QObject::connect(nodeView.selectionModel(), &QItemSelectionModel::selectionChanged, selectElems<nodeListElement>(nodeModel.selectionProtection));

    treeView.setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    QObject::connect(&treeView, &QTreeView::customContextMenuRequested, [this](const QPoint & pos){
        treeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedTrees.size() == 1 && state->skeletonState->selectedNodes.size() > 0);//move nodes
        treeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedTrees.size() >= 2);//merge trees action
        treeContextMenu.actions().at(2)->setEnabled(state->skeletonState->selectedTrees.size() > 0);//show
        treeContextMenu.actions().at(3)->setEnabled(state->skeletonState->selectedTrees.size() > 0);//hide
        treeContextMenu.actions().at(4)->setEnabled(state->skeletonState->selectedTrees.size() > 0);//restore default color
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        treeContextMenu.exec(treeView.viewport()->mapToGlobal(pos));
    });
    nodeView.setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    QObject::connect(&nodeView, &QTreeView::customContextMenuRequested, [this](const QPoint & pos){
        nodeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedNodes.size() == 1);//split connected components
        nodeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedNodes.size() == 2);//link nodes needs two selected nodes
        nodeContextMenu.actions().at(2)->setEnabled(state->skeletonState->selectedNodes.size() > 0);//set comment
        nodeContextMenu.actions().at(3)->setEnabled(state->skeletonState->selectedNodes.size() > 0);//set radius
        nodeContextMenu.actions().at(4)->setEnabled(state->skeletonState->selectedNodes.size() > 0);//delete
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        nodeContextMenu.exec(nodeView.viewport()->mapToGlobal(pos));
    });


    QObject::connect(treeContextMenu.addAction("Move selected nodes to this tree"), &QAction::triggered, [this](){
        const auto treeID = state->skeletonState->selectedTrees.front()->treeID;
        const auto text = tr("Do you really want to move selected nodes to tree %1?").arg(treeID);
        question(this, [treeID](){ Skeletonizer::singleton().moveSelectedNodesToTree(treeID); }, tr("Move"), text, tr(""));
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
        }, tr("Merge"), tr("Do you really want to merge all selected trees?"), tr(""));
    });
    QObject::connect(treeContextMenu.addAction("Set &comment for trees"), &QAction::triggered, [this](){
        bool applied = false;
        static auto prevComment = QString("");
        auto comment = QInputDialog::getText(this, "Edit Tree Comment", "new tree comment", QLineEdit::Normal, prevComment, &applied);
        if (applied) {
            prevComment = comment;
            Skeletonizer::singleton().addTreeCommentToSelectedTrees(comment);
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
        question(this, [](){ Skeletonizer::singleton().deleteSelectedTrees(); }, tr("Delete"), tr("Delete the selected trees?"), tr(""), !state->skeletonState->selectedTrees.empty());
    });


    QObject::connect(nodeContextMenu.addAction("&Extract connected component"), &QAction::triggered, [this](){
        auto res = QMessageBox::Ok;
        static bool askExtractConnectedComponent = true;
        if (askExtractConnectedComponent) {
            const auto msg = tr("Do you really want to extract all nodes in this component into a new tree?");

            QMessageBox msgBox;
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
                Skeletonizer::singleton().addTreeComment(state->skeletonState->firstTree->treeID, msg);
            }
        }
    });
    QObject::connect(nodeContextMenu.addAction("&Link/Unlink nodes"), &QAction::triggered, [this](){
        Skeletonizer::singleton().toggleConnectionOfFirstPairOfSelectedNodes(this);
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
        if (state->skeletonState->selectedNodes.size() == 1) {//don’t ask for one node
            Skeletonizer::singleton().deleteSelectedNodes();
        } else {
            question(this, [](){ Skeletonizer::singleton().deleteSelectedNodes(); }, tr("Delete"), tr("Delete the selected nodes?"), tr(""));
        }
    });
}
