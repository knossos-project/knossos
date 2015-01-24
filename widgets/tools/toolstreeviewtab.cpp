#include "toolstreeviewtab.h"

#include <QColorDialog>
#include <QInputDialog>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QToolTip>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QComboBox>
#include <QKeyEvent>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDropEvent>

#include "skeletonizer.h"

template <typename Table, typename Func>
void applySelection(Table table, Func func) {
    QItemSelection selectedItems;
    std::size_t startIndex;
    bool blockSelection = false;
    int row;
    for (row = 0; row < table->rowCount(); ++row) {
        const auto & elem = table->item(row, 0)->data(Qt::DisplayRole);
        if (!blockSelection && func(elem)) {//start block selection
            blockSelection = true;
            startIndex = row;
        }
        if (blockSelection && !func(elem)) {//end block selection
            blockSelection = false;
            selectedItems.select(table->model()->index(startIndex, 0), table->model()->index(row-1, table->model()->columnCount()-1));
        }
    }
    //finish last blockselection – if any
    if (blockSelection) {
        selectedItems.select(table->model()->index(startIndex, 0), table->model()->index(row-1, table->model()->columnCount()-1));
    }

    //apply accumulated selections
    table->selectionProtection = true;
    table->selectionModel()->select(selectedItems, QItemSelectionModel::ClearAndSelect);
    table->selectionProtection = false;
}

enum {
    NODECOMBO_100, NODECOMBO_1000, NODECOMBO_5000, NODECOMBO_ALL
};

enum {
    DISPLAY_ALL
};

// treeview
ToolsTreeviewTab::ToolsTreeviewTab(QWidget *parent) :
    QWidget(parent), selectedNodes("selected nodes")
    , radiusBuffer(state->skeletonState->defaultNodeRadius), draggedNodeID(0), displayedNodes(1000)
{
    treeSearchField = new QLineEdit();
    treeSearchField->setPlaceholderText("search tree");
    treeSearchField->setFocusPolicy(Qt::ClickFocus);
    nodeSearchField = new QLineEdit();
    nodeSearchField->setPlaceholderText("search node");
    nodeSearchField->setFocusPolicy(Qt::ClickFocus);
    treeRegExCheck = new QCheckBox("RegEx");
    treeRegExCheck->setToolTip("search by regular expression");
    nodeRegExCheck = new QCheckBox("RegEx");
    nodeRegExCheck->setToolTip("search by regular expression");

    nodesOfSelectedTreesRadio = new QRadioButton("nodes of selected trees");
    nodesOfSelectedTreesRadio->setToolTip("Select the tree(s) by single left click");
    allNodesRadio = new QRadioButton("all nodes");
    allNodesRadio->setChecked(true);
    branchNodesChckBx = new QCheckBox("... with branch mark");
    commentNodesChckBx = new QCheckBox("... with comments");

    displayedNodesTable = new QLabel("Displayed Nodes:");
    displayedNodesCombo = new QComboBox();
    displayedNodesCombo->addItem("1000");
    displayedNodesCombo->addItem("2000");
    displayedNodesCombo->addItem("5000");
    displayedNodesCombo->setCurrentIndex(0); // 1000 elements default
    displayedNodesCombo->addItem("all (slow)");


    // tables

    // active tree table
    activeTreeTable = new TreeTable(this);
    activeTreeTable->setColumnCount(TreeTable::TREE_COLS);
    activeTreeTable->setMaximumHeight(54);
    activeTreeTable->verticalHeader()->setVisible(false);
    activeTreeTable->setToolTip("The active tree");
    activeTreeTable->setHorizontalHeaderItem(TreeTable::TREE_ID, new QTableWidgetItem("Tree ID"));
    activeTreeTable->setHorizontalHeaderItem(TreeTable::TREE_COLOR, new QTableWidgetItem("Color"));
    activeTreeTable->setHorizontalHeaderItem(TreeTable::TREE_COMMENT, new QTableWidgetItem("Comment"));
    activeTreeTable->horizontalHeader()->setStretchLastSection(true);
    activeTreeTable->resizeColumnsToContents();
    activeTreeTable->setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    activeTreeTable->setDragDropMode(QAbstractItemView::DropOnly);//enables acceptDrops
    activeTreeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    activeTreeTable->setStyleSheet("QTableWidget::item {selection-background-color: #00FF00; selection-color: #000000}");

    // tree table
    treeTable = new TreeTable(this);
    treeTable->setColumnCount(TreeTable::TREE_COLS);
    treeTable->setAlternatingRowColors(true);
    treeTable->verticalHeader()->setVisible(false);
    treeTable->setHorizontalHeaderItem(TreeTable::TREE_ID, new QTableWidgetItem("Tree ID"));
    treeTable->setHorizontalHeaderItem(TreeTable::TREE_COLOR, new QTableWidgetItem("Color"));
    treeTable->setHorizontalHeaderItem(TreeTable::TREE_COMMENT, new QTableWidgetItem("Comment"));
    treeTable->horizontalHeader()->setStretchLastSection(true);
    treeTable->resizeColumnsToContents();
    treeTable->setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    treeTable->setDragDropMode(QAbstractItemView::DropOnly);//enables acceptDrops
    treeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    treeTable->setStyleSheet("QTableWidget::item {selection-background-color: #00FF00; selection-color: #000000}");


    // active node table
    activeNodeTable = new NodeTable(this);
    activeNodeTable->setColumnCount(NodeTable::NODE_COLS);
    activeNodeTable->setMaximumHeight(70);
    activeNodeTable->verticalHeader()->setVisible(false);
    activeNodeTable->setToolTip("The active node");
    activeNodeTable->setHorizontalHeaderItem(NodeTable::NODE_ID, new QTableWidgetItem("Node ID"));
    activeNodeTable->setHorizontalHeaderItem(NodeTable::NODE_COMMENT, new QTableWidgetItem("Comment"));
    activeNodeTable->setHorizontalHeaderItem(NodeTable::NODE_X, new QTableWidgetItem("x"));
    activeNodeTable->setHorizontalHeaderItem(NodeTable::NODE_Y, new QTableWidgetItem("y"));
    activeNodeTable->setHorizontalHeaderItem(NodeTable::NODE_Z, new QTableWidgetItem("z"));
    activeNodeTable->setHorizontalHeaderItem(NodeTable::NODE_RADIUS, new QTableWidgetItem("Radius"));
    activeNodeTable->horizontalHeader()->setSectionResizeMode(NodeTable::NODE_COMMENT, QHeaderView::Stretch);
    activeNodeTable->resizeColumnsToContents();
    activeNodeTable->setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    activeNodeTable->setDragDropMode(QAbstractItemView::DragOnly);//enables dragging
    activeNodeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    activeNodeTable->setStyleSheet("QTableWidget::item {selection-background-color: #00FF00; selection-color: #000000}");


    // node table
    nodeTable = new NodeTable(this);
    nodeTable->setColumnCount(NodeTable::NODE_COLS);
    nodeTable->setAlternatingRowColors(true);
    nodeTable->verticalHeader()->setVisible(false);
    nodeTable->setHorizontalHeaderItem(NodeTable::NODE_ID, new QTableWidgetItem("Node ID"));
    nodeTable->setHorizontalHeaderItem(NodeTable::NODE_COMMENT, new QTableWidgetItem("Comment"));
    nodeTable->setHorizontalHeaderItem(NodeTable::NODE_X, new QTableWidgetItem("x"));
    nodeTable->setHorizontalHeaderItem(NodeTable::NODE_Y, new QTableWidgetItem("y"));
    nodeTable->setHorizontalHeaderItem(NodeTable::NODE_Z, new QTableWidgetItem("z"));
    nodeTable->setHorizontalHeaderItem(NodeTable::NODE_RADIUS, new QTableWidgetItem("Radius"));
    nodeTable->horizontalHeader()->setSectionResizeMode(NodeTable::NODE_COMMENT, QHeaderView::Stretch);
    nodeTable->resizeColumnsToContents();
    nodeTable->setContextMenuPolicy(Qt::CustomContextMenu);//enables signal for custom context menu
    nodeTable->setDragDropMode(QAbstractItemView::DragOnly);//enables dragging
    nodeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    nodeTable->setStyleSheet("QTableWidget::item {selection-background-color: #00FF00; selection-color: #000000}");

    for (auto && verticalHeader : {treeTable->verticalHeader(), nodeTable->verticalHeader()}) {
        verticalHeader->sectionResizeMode(QHeaderView::Fixed);
        verticalHeader->setDefaultSectionSize(verticalHeader->minimumSectionSize());
    }

    bottomHLayout.addWidget(&treeCountLabel, 0, Qt::AlignLeft);
    bottomHLayout.addWidget(&nodeCountLabel, 0, Qt::AlignRight);

    mainLayout = new QVBoxLayout();
    QVBoxLayout *vLayout = new QVBoxLayout();
    QHBoxLayout *hLayout = new QHBoxLayout();

    treeSide = new QWidget();
    vLayout = new QVBoxLayout();
    hLayout = new QHBoxLayout();
    hLayout->addWidget(treeSearchField);
    hLayout->addWidget(treeRegExCheck);
    vLayout->addLayout(hLayout);
    vLayout->addWidget(activeTreeTable);
    vLayout->addWidget(treeTable);
    treeSide->setLayout(vLayout);

    nodeSide = new QWidget();
    vLayout = new QVBoxLayout();
    hLayout = new QHBoxLayout();
    hLayout->addWidget(nodeSearchField);
    hLayout->addWidget(nodeRegExCheck);
    vLayout->addLayout(hLayout);
    QLabel *showLabel = new QLabel("Show...");
    vLayout->addWidget(showLabel);
    vLayout->addWidget(allNodesRadio);

    hLayout = new QHBoxLayout();
    hLayout->addWidget(nodesOfSelectedTreesRadio);
    hLayout->addWidget(&selectedNodes);

    vLayout->addLayout(hLayout);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(branchNodesChckBx);
    hLayout->addWidget(commentNodesChckBx);
    vLayout->addLayout(hLayout);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(displayedNodesTable);
    hLayout->addWidget(displayedNodesCombo);
    vLayout->addLayout(hLayout);
    vLayout->addWidget(activeNodeTable);
    vLayout->addWidget(nodeTable);
    nodeSide->setLayout(vLayout);

    splitter = new QSplitter(this);
    splitter->addWidget(treeSide);
    splitter->addWidget(nodeSide);
    mainLayout->addWidget(splitter, 1);
    mainLayout->addLayout(&bottomHLayout);
    QList<int> list;
    list.append(310);
    list.append(390);
    splitter->setSizes(list);

    setLayout(mainLayout);

    // search events
    QObject::connect(treeSearchField, &QLineEdit::editingFinished, this, &ToolsTreeviewTab::treeSearchChanged);
    QObject::connect(nodeSearchField, &QLineEdit::editingFinished, this, &ToolsTreeviewTab::nodeSearchChanged);
    // display events
    QObject::connect(allNodesRadio, &QRadioButton::clicked, this, &ToolsTreeviewTab::recreateNodesTable);
    QObject::connect(nodesOfSelectedTreesRadio, &QRadioButton::clicked, this, &ToolsTreeviewTab::recreateNodesTable);
    QObject::connect(&selectedNodes, &QRadioButton::clicked, this, &ToolsTreeviewTab::recreateNodesTable);//special :D

    QObject::connect(branchNodesChckBx, &QCheckBox::clicked, this, &ToolsTreeviewTab::recreateNodesTable);
    QObject::connect(commentNodesChckBx, &QCheckBox::clicked, this, &ToolsTreeviewTab::recreateNodesTable);
    // displayed nodes
    QObject::connect(displayedNodesCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ToolsTreeviewTab::displayedNodesChanged);

    // table click events
    QObject::connect(activeTreeTable, &TreeTable::customContextMenuRequested, this, &ToolsTreeviewTab::contextMenu);
    QObject::connect(activeTreeTable, &TreeTable::deleteTreesSignal, this, &ToolsTreeviewTab::deleteTreesAction);
    QObject::connect(activeTreeTable, &TreeTable::itemChanged, this, &ToolsTreeviewTab::actTreeItemChanged);
    QObject::connect(activeTreeTable, &TreeTable::itemDoubleClicked, this, &ToolsTreeviewTab::treeItemDoubleClicked);
    QObject::connect(activeTreeTable, &TreeTable::itemSelectionChanged, this, &ToolsTreeviewTab::activeTreeTableSelectionChanged);

    QObject::connect(treeTable, &TreeTable::customContextMenuRequested, this, &ToolsTreeviewTab::contextMenu);
    QObject::connect(treeTable, &TreeTable::deleteTreesSignal, this, &ToolsTreeviewTab::deleteTreesAction);
    QObject::connect(treeTable, &TreeTable::itemChanged, this, &ToolsTreeviewTab::treeItemChanged);
    QObject::connect(treeTable, &TreeTable::itemDoubleClicked, this, &ToolsTreeviewTab::treeItemDoubleClicked);
    QObject::connect(treeTable, &TreeTable::itemSelectionChanged, this, &ToolsTreeviewTab::treeTableSelectionChanged);

    QObject::connect(activeNodeTable, &NodeTable::customContextMenuRequested, this, &ToolsTreeviewTab::contextMenu);
    QObject::connect(activeNodeTable, &NodeTable::deleteNodesSignal, this, &ToolsTreeviewTab::deleteNodesAction);
    QObject::connect(activeNodeTable, &NodeTable::itemChanged, this, &ToolsTreeviewTab::actNodeItemChanged);
    QObject::connect(activeNodeTable, &NodeTable::itemDoubleClicked, this, &ToolsTreeviewTab::nodeItemDoubleClicked);
    QObject::connect(activeNodeTable, &NodeTable::itemSelectionChanged, this, &ToolsTreeviewTab::activeNodeTableSelectionChanged);

    QObject::connect(nodeTable, &NodeTable::customContextMenuRequested, this, &ToolsTreeviewTab::contextMenu);
    QObject::connect(nodeTable, &NodeTable::deleteNodesSignal, this, &ToolsTreeviewTab::deleteNodesAction);
    QObject::connect(nodeTable, &NodeTable::itemChanged, this, &ToolsTreeviewTab::nodeItemChanged);
    QObject::connect(nodeTable, &NodeTable::itemDoubleClicked, this, &ToolsTreeviewTab::nodeItemDoubleClicked);
    QObject::connect(nodeTable, &NodeTable::itemSelectionChanged, this, &ToolsTreeviewTab::nodeTableSelectionChanged);
    QObject::connect(nodeTable->horizontalHeader(), &QHeaderView::sectionClicked, this, &ToolsTreeviewTab::sortComments);

    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::branchPoppedSignal, this, &ToolsTreeviewTab::branchPopped);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::branchPushedSignal, this, &ToolsTreeviewTab::branchPushed);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeSelectionChangedSignal, [this](){
        if (selectedNodes.isChecked()) {
            recreateNodesTable();
        } else {
            applyNodeSelection();
        }
    });
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeAddedSignal, this, &ToolsTreeviewTab::nodeAdded);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeChangedSignal, this, &ToolsTreeviewTab::nodeEdited);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeRemovedSignal, this, &ToolsTreeviewTab::resetData);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeAddedSignal, this, &ToolsTreeviewTab::treeAddedOrChanged);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeChangedSignal, this, &ToolsTreeviewTab::treeAddedOrChanged);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeRemovedSignal, this, &ToolsTreeviewTab::resetData);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treesMerged, this, &ToolsTreeviewTab::treesMerged);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::treeSelectionChangedSignal, this, &ToolsTreeviewTab::applyTreeSelection);
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::resetData, this, &ToolsTreeviewTab::resetData);
}

void ToolsTreeviewTab::updateLabels() {
    treeCountLabel.setText(QString("%1 Trees").arg(state->skeletonState->treeElements));
    nodeCountLabel.setText(QString("%1 Nodes (%2 currently listed)").arg(state->skeletonState->totalNodeElements).arg(nodeTable->rowCount()));
}

void ToolsTreeviewTab::mergeTreesAction() {
    QMessageBox prompt;
    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
    prompt.setIcon(QMessageBox::Question);
    prompt.setWindowTitle("Cofirmation required");
    prompt.setText("Do you really want to merge selected trees?");
    QPushButton *confirmButton = prompt.addButton("Merge", QMessageBox::ActionRole);
    prompt.addButton("Cancel", QMessageBox::ActionRole);
    prompt.exec();
    if (prompt.clickedButton() == confirmButton) {
        int treeID1 = state->skeletonState->selectedTrees[0]->treeID;
        int treeID2 = state->skeletonState->selectedTrees[1]->treeID;
        Skeletonizer::singleton().mergeTrees(treeID1, treeID2);
    }
}

void ToolsTreeviewTab::restoreColorAction() {
    for (auto * tree : state->skeletonState->selectedTrees) {
        Skeletonizer::restoreDefaultTreeColor(tree);
    }
    recreateTreesTable();
}

void ToolsTreeviewTab::deleteTreesAction() {
    if(state->skeletonState->selectedTrees.size() == 0) {
        qDebug() << "no trees to delete";
    } else {
        QMessageBox prompt;
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Question);
        prompt.setWindowTitle("Cofirmation required");
        prompt.setText("Do you really want to delete selected tree(s)?");
        const QPushButton * const confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            Skeletonizer::singleton().deleteSelectedTrees();
        }
    }
}

void ToolsTreeviewTab::setTreeCommentAction() {
    bool applied = false;
    auto text = QInputDialog::getText(this, "Edit Tree Comment", "new tree comment", QLineEdit::Normal, treeCommentBuffer, &applied);
    if (applied) {
        auto treeCommentBuffer = text;
        if (activeTreeTable->hasFocus()) {
            Skeletonizer::singleton().addTreeComment(state->skeletonState->activeTree->treeID, treeCommentBuffer.toLocal8Bit().data());
            setText(activeTreeTable, activeTreeTable->item(0, TreeTable::TREE_COMMENT), treeCommentBuffer);
            int row = getActiveTreeRow();
            if (row != -1) {
                setText(treeTable, treeTable->item(row, TreeTable::TREE_COMMENT), treeCommentBuffer);
            }
        } else {
            for (auto tree : state->skeletonState->selectedTrees) {
                Skeletonizer::singleton().addTreeComment(tree->treeID, treeCommentBuffer);
            }
        }
    }
}

void ToolsTreeviewTab::contextMenu(QPoint pos) {
    if (activeNodeTable->hasFocus()) {
        QMenu nodeContextMenu;
        QObject::connect(nodeContextMenu.addAction("Split component from tree"), &QAction::triggered, this, &ToolsTreeviewTab::splitComponentAction);
        QObject::connect(nodeContextMenu.addAction(QIcon(":/resources/icons/user-trash.png"), "(DEL)ete active node"), &QAction::triggered, this, &ToolsTreeviewTab::deleteNodesAction);

        nodeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedNodes.size() == 1);//split connected components
        nodeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedNodes.size() == 1);//del active node
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        nodeContextMenu.exec(activeNodeTable->viewport()->mapToGlobal(pos));
    } else if (nodeTable->hasFocus()) {
        QMenu nodeContextMenu;
        QObject::connect(nodeContextMenu.addAction("Split component from tree"), &QAction::triggered, this, &ToolsTreeviewTab::splitComponentAction);
        QObject::connect(nodeContextMenu.addAction("(Un)link nodes"), &QAction::triggered, this, &ToolsTreeviewTab::linkNodesAction);
        QObject::connect(nodeContextMenu.addAction("Set comment for node(s)"), &QAction::triggered, this, &ToolsTreeviewTab::setNodeCommentAction);
        QObject::connect(nodeContextMenu.addAction("Set radius for node(s)"), &QAction::triggered, this, &ToolsTreeviewTab::setNodeRadiusAction);
        QObject::connect(nodeContextMenu.addAction(QIcon(":/resources/icons/user-trash.png"), "(DEL)ete node(s)"), &QAction::triggered, this, &ToolsTreeviewTab::deleteNodesAction);

        nodeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedNodes.size() == 1);//split connected components
        nodeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedNodes.size() == 2);//link nodes needs two selected nodes
        nodeContextMenu.actions().at(2)->setEnabled(state->skeletonState->selectedNodes.size() > 0);
        nodeContextMenu.actions().at(3)->setEnabled(state->skeletonState->selectedNodes.size() > 0);
        nodeContextMenu.actions().at(4)->setEnabled(state->skeletonState->selectedNodes.size() > 0);
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        nodeContextMenu.exec(nodeTable->viewport()->mapToGlobal(pos));
    } else if (activeTreeTable->hasFocus()) {
        QMenu treeContextMenu;
        QObject::connect(treeContextMenu.addAction("Set comment for tree"), &QAction::triggered, this, &ToolsTreeviewTab::setTreeCommentAction);
        QObject::connect(treeContextMenu.addAction("Move selected node(s) to this tree"), &QAction::triggered, this, &ToolsTreeviewTab::moveNodesAction);
        QObject::connect(treeContextMenu.addAction("Restore default color"), &QAction::triggered, this, &ToolsTreeviewTab::restoreColorAction);
        QObject::connect(treeContextMenu.addAction(QIcon(":/resources/icons/user-trash.png"), "(DEL)ete tree"), &QAction::triggered, this, &ToolsTreeviewTab::deleteTreesAction);

        treeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        treeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedTrees.size() == 1 && state->skeletonState->selectedNodes.size() > 0);//move nodes
        treeContextMenu.actions().at(2)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        treeContextMenu.actions().at(3)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        treeContextMenu.exec(activeTreeTable->viewport()->mapToGlobal(pos));
    } else if (treeTable->hasFocus()) {
        QMenu treeContextMenu;
        QObject::connect(treeContextMenu.addAction("Move selected node(s) to this tree"), &QAction::triggered, this, &ToolsTreeviewTab::moveNodesAction);
        QObject::connect(treeContextMenu.addAction("Merge trees"), &QAction::triggered, this, &ToolsTreeviewTab::mergeTreesAction);
        QObject::connect(treeContextMenu.addAction("Set comment for tree(s)"), &QAction::triggered, this, &ToolsTreeviewTab::setTreeCommentAction);
        QObject::connect(treeContextMenu.addAction("Restore default color"), &QAction::triggered, this, &ToolsTreeviewTab::restoreColorAction);
        QObject::connect(treeContextMenu.addAction(QIcon(":/resources/icons/user-trash.png"), "Delete tree(s)"), &QAction::triggered, this, &ToolsTreeviewTab::deleteTreesAction);

        treeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedTrees.size() == 1 && state->skeletonState->selectedNodes.size() > 0);//move nodes
        treeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedTrees.size() == 2);//merge trees action
        treeContextMenu.actions().at(2)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        treeContextMenu.actions().at(3)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        treeContextMenu.actions().at(4)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        treeContextMenu.exec(treeTable->viewport()->mapToGlobal(pos));
    }
}

void ToolsTreeviewTab::deleteNodesAction() {
    bool deleteNodes = true;
    if (nodeTable->hasFocus()) {
        if(state->skeletonState->selectedNodes.size() == 0) {
            qDebug() << "no nodes";
            return;
        }
        if(state->skeletonState->selectedNodes.size() != 1) {
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Question);
            prompt.setWindowTitle("Cofirmation required");
            prompt.setText("Do you really want to delet selected node(s)?");
            QPushButton *confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
            prompt.addButton("Cancel", QMessageBox::ActionRole);
            prompt.exec();
            deleteNodes = prompt.clickedButton() == confirmButton;
        }
    }
    if (deleteNodes) {
        Skeletonizer::singleton().deleteSelectedNodes();
    }
}

void ToolsTreeviewTab::setNodeRadiusAction() {
    bool applied = false;
    auto value = QInputDialog::getDouble(this, "Edit Node Radii", "new node radius", radiusBuffer, 0, 100000, 1, &applied);
    if (applied) {
        radiusBuffer = value;

        if (activeNodeTable->hasFocus()) {
            Skeletonizer::singleton().editNode(0, state->skeletonState->activeNode, radiusBuffer,
                                   state->skeletonState->activeNode->position.x,
                                   state->skeletonState->activeNode->position.y,
                                   state->skeletonState->activeNode->position.z,
                                   state->skeletonState->activeNode->createdInMag);
            setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_RADIUS), QString::number(radiusBuffer));
            int row = getActiveNodeRow();
            if (row != -1) {
                setText(nodeTable, nodeTable->item(row, NodeTable::NODE_RADIUS), QString::number(radiusBuffer));
            }
        } else {
            for (auto node : state->skeletonState->selectedNodes) {
                if(node == state->skeletonState->activeNode) {
                    setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_RADIUS), QString::number(radiusBuffer));
                }
                Skeletonizer::singleton().editNode(0, node, radiusBuffer,
                                       node->position.x, node->position.y, node->position.z, node->createdInMag);
            }
            recreateNodesTable();
        }
    }
}

void ToolsTreeviewTab::linkNodesAction() {
    const auto node0 = state->skeletonState->selectedNodes[0];
    const auto node1 = state->skeletonState->selectedNodes[1];
    auto & skel = Skeletonizer::singleton();
    //segments are only stored and searched in one direction so we have to search for both
    if (Skeletonizer::findSegmentByNodeIDs(node0->nodeID, node1->nodeID) != nullptr) {
        skel.delSegment(node0->nodeID, node1->nodeID, nullptr);
    } else if (Skeletonizer::findSegmentByNodeIDs(node1->nodeID, node0->nodeID) != nullptr) {
        skel.delSegment(node1->nodeID, node0->nodeID, nullptr);
    } else if (!skel.simpleTracing || Skeletonizer::singleton().areConnected(*node0, *node1)) {
        //nodes are not already linked
        skel.addSegment(node0->nodeID, node1->nodeID);
    }
}

void ToolsTreeviewTab::splitComponentAction() {
    QMessageBox prompt;
    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
    prompt.setIcon(QMessageBox::Question);
    prompt.setWindowTitle("Cofirmation required");
    prompt.setText("Do you really want to split the connected component from the tree?");
    QPushButton *confirmButton = prompt.addButton("Split", QMessageBox::ActionRole);
    prompt.addButton("Cancel", QMessageBox::ActionRole);
    prompt.exec();
    if(prompt.clickedButton() == confirmButton) {
        treeListElement *prevActive = state->skeletonState->activeTree;
        Skeletonizer::splitConnectedComponent(state->skeletonState->selectedNodes.front()->nodeID);
        if(prevActive != state->skeletonState->activeTree) { // if the active tree changes, the splitting was successful
            // when a tree is split, a new tree has been created and made active
            treeActivated();//add new tree to active tree table
            insertTree(state->skeletonState->activeTree, treeTable);//add new tree to tree table
            updateLabels();
        }
    }
}

void ToolsTreeviewTab::moveNodesAction() {
    QMessageBox prompt;
    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
    prompt.setIcon(QMessageBox::Question);
    prompt.setWindowTitle("Confirmation Requested");
    prompt.setText(QString("Do you really want to move selected nodes to tree %1?").
                            arg(state->skeletonState->selectedTrees.front()->treeID));
    QPushButton *confirmButton = prompt.addButton("Move", QMessageBox::ActionRole);
    prompt.addButton("Cancel", QMessageBox::ActionRole);
    prompt.exec();
    if(prompt.clickedButton() == confirmButton) {
        Skeletonizer::singleton().moveSelectedNodesToTree(state->skeletonState->selectedTrees.front()->treeID);
    }
}

void ToolsTreeviewTab::setNodeCommentAction() {
    bool applied = false;
    auto text = QInputDialog::getText(this, "Edit Node Comment", "new node comment", QLineEdit::Normal, nodeCommentBuffer, &applied);
    if (applied) {
        nodeCommentBuffer = text;

        if (activeNodeTable->hasFocus()) {
            if (nodeCommentBuffer.length() > 0) {
                Skeletonizer::singleton().setComment(nodeCommentBuffer, state->skeletonState->activeNode, 0);
            } else if(state->skeletonState->activeNode->comment != nullptr) {
                Skeletonizer::singleton().delComment(state->skeletonState->activeNode->comment, state->skeletonState->activeNode->nodeID);
            }
        } else {// nodeTable focused
            for (auto node : state->skeletonState->selectedNodes) {
                if(nodeCommentBuffer.length() > 0) {
                    Skeletonizer::singleton().setComment(nodeCommentBuffer.toLocal8Bit().data(), node, 0);
                } else if (node->comment != nullptr) {
                    Skeletonizer::singleton().delComment(node->comment, node->nodeID);
                }
            }
        }
    }
}

void ToolsTreeviewTab::treeSearchChanged() {
    if(treeRegExCheck->isChecked() and treeSearchField->text().length() > 0) {
        QRegularExpression regex(treeSearchField->text());
        if(regex.isValid() == false) {
            QToolTip::showText(treeSearchField->mapToGlobal(treeSearchField->pos()), "Invalid regular expression.");
            return;
        }
    }
    recreateTreesTable();
}
void ToolsTreeviewTab::nodeSearchChanged() {
    if(nodeRegExCheck->isChecked() and nodeSearchField->text().length() > 0) {
        QRegularExpression regex(nodeSearchField->text());
        if(regex.isValid() == false) {
            QToolTip::showText(nodeSearchField->mapToGlobal(nodeSearchField->pos()), "Invalid regular expression.");
            return;
        }
    }
    recreateNodesTable();
}

void ToolsTreeviewTab::displayedNodesChanged(int index) {
    switch(index) {
    case NODECOMBO_100:
    case NODECOMBO_1000:
    case NODECOMBO_5000:
        displayedNodes = displayedNodesCombo->currentText().toInt();
        break;
    case NODECOMBO_ALL:
        displayedNodes = DISPLAY_ALL;
        break;
    default:
        break;
    }

    if (displayedNodes == nodeTable->rowCount()) {
        return;
    } else {
        recreateNodesTable();
    }
}

void ToolsTreeviewTab::actTreeItemChanged(QTableWidgetItem *item) {
    if(activeTreeTable->selectionProtection) {
        return;
    }
    if (item->column() == TreeTable::TREE_COMMENT) {
        Skeletonizer::singleton().addTreeComment(state->skeletonState->activeTree->treeID, item->text());
    }
}

void ToolsTreeviewTab::treeItemChanged(QTableWidgetItem* item) {
    if (treeTable->selectionProtection) {
        return;
    }
    const auto treeId = treeTable->item(item->row(), TreeTable::TREE_ID)->text().toInt();
    if (item->column() == TreeTable::TREE_COMMENT) {
        Skeletonizer::singleton().addTreeComment(treeId, item->text());
    }
}

void ToolsTreeviewTab::actNodeItemChanged(QTableWidgetItem *item) {
    if(activeNodeTable->selectionProtection) {
        return;
    }
    nodeListElement *activeNode = state->skeletonState->activeNode;
    if(activeNode == nullptr) {
        return;
    }
    int activeRow = -1;
    for(int i = 0; i < nodeTable->rowCount(); ++i) {
        if((uint)nodeTable->item(i, NodeTable::NODE_ID)->text().toInt() == activeNode->nodeID) {
            activeRow = i;
            break;
        }
    }

    switch(item->column()) {
    case NodeTable::NODE_COMMENT: {
        auto matches = nodeSearchField->text().isEmpty() || matchesSearchString(nodeSearchField->text(), item->text(), nodeRegExCheck->isChecked());
        if(activeNode->comment) {
            if(item->text().isEmpty()) {
                Skeletonizer::singleton().delComment(activeNode->comment, 0);
                matches &= !commentNodesChckBx->isChecked();//no cmt anymore but cmts wanted
            }
            else {
                Skeletonizer::singleton().editComment(activeNode->comment, 0, item->text(), activeNode, 0);
            }
        }
        else if(item->text().isEmpty() == false) {
            Skeletonizer::singleton().addComment(item->text(), activeNode, 0);
        }
        if (activeRow != -1) {//remove not matching
            if (matches) {
                setText(nodeTable, nodeTable->item(activeRow, NodeTable::NODE_COMMENT), item->text());
            } else {
                nodeTable->selectionProtection = true;
                nodeTable->removeRow(activeRow);
                nodeTable->selectionProtection = false;
            }
        } else if (matches) {//add matching
            recreateNodesTable();
        }
        break;
    }
    case NodeTable::NODE_X:
        if(item->text().toInt() < 1) { // out of bounds
            setText(activeNodeTable, item, QString::number(activeNode->position.x + 1));
        } else {
            activeNode->position.x = item->text().toInt() - 1;
        }
        if(activeRow != -1) {
            setText(nodeTable, nodeTable->item(activeRow, NodeTable::NODE_X), item->text());
        }
        break;
    case NodeTable::NODE_Y:
        if(item->text().toInt() < 1) { // out of bounds
            setText(activeNodeTable, item, QString::number(activeNode->position.y + 1));
        } else {
            activeNode->position.y = item->text().toInt() - 1;
        }
        if(activeRow != -1) {
            setText(nodeTable, nodeTable->item(activeRow, NodeTable::NODE_Y), item->text());
        }
        break;
    case NodeTable::NODE_Z:
        if(item->text().toInt() < 1) { // out of bounds
            setText(activeNodeTable, item, QString::number(activeNode->position.z + 1));
        } else {
            activeNode->position.z = item->text().toInt() - 1;
        }
        if(activeRow != -1) {
            setText(nodeTable, nodeTable->item(activeRow, NodeTable::NODE_Z), item->text());
        }
        break;
    case NodeTable::NODE_RADIUS:
        activeNode->radius = item->text().toFloat();
        if(activeRow != -1) {
            setText(nodeTable, nodeTable->item(activeRow, NodeTable::NODE_RADIUS), item->text());
        }
        break;
    default:
        break;
    }
    updateLabels();
}

void ToolsTreeviewTab::nodeItemChanged(QTableWidgetItem* item) {
    if(nodeTable->selectionProtection) {
        return;
    }
    QTableWidgetItem *idItem = nodeTable->item(item->row(), NodeTable::NODE_ID);
    if(idItem == nullptr) {
        return;
    }
    nodeListElement *selectedNode = Skeletonizer::findNodeByNodeID(idItem->text().toUInt());
    if(selectedNode == nullptr) {
        return;
    }
    switch(item->column()) {
    case NodeTable::NODE_COMMENT: {
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NodeTable::NODE_COMMENT)) {
                setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_COMMENT), item->text());
            }
        }
        auto matches = nodeSearchField->text().isEmpty() || matchesSearchString(nodeSearchField->text(), item->text(), nodeRegExCheck->isChecked());
        if(selectedNode->comment) {
            if (item->text().isEmpty()) {
                Skeletonizer::singleton().delComment(selectedNode->comment, 0);
                matches &= !commentNodesChckBx->isChecked();//no cmt anymore but cmts wanted
            } else {
                Skeletonizer::singleton().editComment(selectedNode->comment, 0, item->text(), selectedNode, 0);
            }
        } else {
            Skeletonizer::singleton().addComment(item->text(), selectedNode, 0);
        }
        if (!matches) {
            nodeTable->selectionProtection = true;
            nodeTable->removeRow(item->row());
            nodeTable->selectionProtection = false;
        }
        break;
    }
    case NodeTable::NODE_X:
        if(item->text().toInt() < 1) { // out of bounds
            setText(nodeTable, item, QString::number(selectedNode->position.x + 1));
        } else {
            selectedNode->position.x = item->text().toInt() - 1;
        }
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NodeTable::NODE_X)) {
                setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_X), item->text());
            }
        }
        break;
    case NodeTable::NODE_Y:
        if(item->text().toInt() < 1) { // out of bounds
            setText(nodeTable, item, QString::number(selectedNode->position.y + 1));
        } else {
            selectedNode->position.y = item->text().toInt() - 1;
        }
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NodeTable::NODE_Y)) {
                setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_Y), item->text());
            }
        }
        break;
    case NodeTable::NODE_Z:
        if(item->text().toInt() < 1) { // out of bounds
            setText(nodeTable, item, QString::number(selectedNode->position.z + 1));
        } else {
            selectedNode->position.z = item->text().toInt() - 1;
        }
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NodeTable::NODE_Z)) {
                setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_Z), item->text());
            }
        }
        break;
    case NodeTable::NODE_RADIUS:
        selectedNode->radius = item->text().toFloat();
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NodeTable::NODE_RADIUS)) {
                setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_RADIUS), item->text());
            }
        }
        break;
    default:
        break;
    }
    updateLabels();
}

void ToolsTreeviewTab::activeTreeTableSelectionChanged() {
    if (activeTreeTable->selectionProtection) {
        return;
    }

    if (activeTreeTable->selectionModel()->selectedRows().size() == 1) {
        Skeletonizer::singleton().setActiveTreeByID(state->skeletonState->activeTree->treeID);
    }
}

void ToolsTreeviewTab::treeTableSelectionChanged() {
    if (treeTable->selectionProtection) {
        return;
    }

    QModelIndexList selected = treeTable->selectionModel()->selectedRows();
    std::vector<treeListElement*> selectedTrees;
    foreach(QModelIndex index, selected) {
        treeListElement * const tree = Skeletonizer::findTreeByTreeID(index.data().toInt());
        selectedTrees.emplace_back(tree);
    }
    Skeletonizer::singleton().selectTrees(selectedTrees);
}

void ToolsTreeviewTab::treeItemDoubleClicked(QTableWidgetItem* item) {
    if (item->column() == TreeTable::TREE_ID) {// user wants to activate tree
        activateFirstSelectedTree();
    } else if (item->column() == TreeTable::TREE_COLOR) {
        auto initial_color = state->skeletonState->selectedTrees.front()->color;
        auto color = QColorDialog::getColor(QColor(255.f*initial_color.r, 255.f*initial_color.g, 255.f*initial_color.b, 255.f*initial_color.a), this, "Select Tree Color", QColorDialog::ShowAlphaChannel);
        if (color.isValid() == QColorDialog::Accepted) {
            state->skeletonState->selectedTrees.front()->color.r = color.redF();
            state->skeletonState->selectedTrees.front()->color.g = color.greenF();
            state->skeletonState->selectedTrees.front()->color.b = color.blueF();
            state->skeletonState->selectedTrees.front()->color.a = color.alphaF();
            updateTreeColorCell(treeTable, treeTable->selectionModel()->selectedRows().front().row());
            if (state->skeletonState->selectedTrees.front() == state->skeletonState->activeTree) {
                updateTreeColorCell(activeTreeTable, 0);
            }
        }
    }
}

void ToolsTreeviewTab::activeNodeTableSelectionChanged() {
    if (activeNodeTable->selectionProtection) {
        return;
    }

    if (activeNodeTable->selectionModel()->selectedRows().size() == 1) {
        Skeletonizer::singleton().setActiveNode(state->skeletonState->activeNode, 0);
    }
}

void ToolsTreeviewTab::nodeTableSelectionChanged() {
    if (nodeTable->selectionProtection) {
        return;
    }

    QModelIndexList selected = nodeTable->selectionModel()->selectedRows();
    std::vector<nodeListElement*> selectedNodes;
    foreach(QModelIndex index, selected) {
        nodeListElement * const node = Skeletonizer::findNodeByNodeID(index.data().toUInt());
        selectedNodes.emplace_back(node);
    }
    Skeletonizer::singleton().selectNodes(selectedNodes);

    nodeTable->setDragEnabled(false);//enable multi-selection on previously unselected elements
}

void ToolsTreeviewTab::activateFirstSelectedNode() {
    if (state->skeletonState->selectedNodes.size() == 1) {
        Skeletonizer::singleton().setActiveNode(nullptr, state->skeletonState->selectedNodes.front()->nodeID);
        Skeletonizer::singleton().jumpToActiveNode();
    }
}

void ToolsTreeviewTab::activateFirstSelectedTree() {
    if (state->skeletonState->selectedTrees.size() == 1) {
        Skeletonizer::singleton().setActiveTreeByID(state->skeletonState->selectedTrees.front()->treeID);
    }
}

void ToolsTreeviewTab::nodeItemDoubleClicked(QTableWidgetItem * item) {
    if (item->column() == NodeTable::NODE_ID) {
        activateFirstSelectedNode();
    }
}

void ToolsTreeviewTab::sortComments(const int header_index) {
    if (header_index == NodeTable::NODE_COMMENT) {
        static bool sortAscending = true;
        nodeTable->sortByColumn(1, sortAscending ? Qt::AscendingOrder : Qt::DescendingOrder);
        sortAscending = !sortAscending;
    }
}

void ToolsTreeviewTab::updateTreeColorCell(TreeTable *table, int row) {
    QTableWidgetItem * const idItem = table->item(row, TreeTable::TREE_ID);
    treeListElement * const tree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
    if (tree != nullptr) {
        QColor treeColor = QColor(tree->color.r*255, tree->color.g*255, tree->color.b*255, tree->color.a*255);
        QTableWidgetItem * const colorItem = table->item(row, TreeTable::TREE_COLOR);
        if (colorItem != nullptr) {
            colorItem->setBackgroundColor(treeColor);
        }
    }
}

bool ToolsTreeviewTab::matchesSearchString(QString searchString, QString string, bool useRegEx) {
    if(useRegEx) {
        QRegularExpression regex(searchString);
        if(regex.isValid() == false) {
            qDebug() << "invalid regex";
            return false;
        }
        return regex.match(string).hasMatch();
    }
    return string.contains(searchString, Qt::CaseInsensitive);
}

void ToolsTreeviewTab::setText(TreeTable *table, QTableWidgetItem *item, QString text) {
    if(item != nullptr) {
        table->selectionProtection = true;
        item->setText(text);
        table->selectionProtection = false;
    }
}

void ToolsTreeviewTab::setText(NodeTable *table, QTableWidgetItem *item, QString text) {
    if(item != nullptr) {
        table->selectionProtection = true;
        item->setText(text);
        table->selectionProtection = false;
    }
}

void ToolsTreeviewTab::recreateTreesTable() {
    treeTable->selectionProtection = true;
    treeTable->clearContents();
    treeTable->setRowCount(state->skeletonState->treeElements);
    treeTable->selectionProtection = false;

    size_t treeIndex = 0;
    for (treeListElement * currentTree = state->skeletonState->firstTree; currentTree != nullptr; currentTree = currentTree->next) {
        // filter comment for search string match
        if(treeSearchField->text().length() > 0) {
            if(strlen(currentTree->comment) == 0) {
                continue;
            }
            if(matchesSearchString(treeSearchField->text(), currentTree->comment, treeRegExCheck->isChecked()) == false) {
                continue;
            }
        }

        treeTable->setRow(treeIndex, QString::number(currentTree->treeID)
                , QColor(currentTree->color.r*255, currentTree->color.g*255, currentTree->color.b*255, 0.6*255)
                , currentTree->comment == nullptr ? "" : currentTree->comment);

        treeIndex++;//this is here so it doesn’t get incremented on continue
    }

    treeTable->setRowCount(treeIndex);

    applyTreeSelection();
}

void ToolsTreeviewTab::applyTreeSelection() {
    auto func = [](const QVariant & cell){
        return Skeletonizer::singleton().findTreeByTreeID(cell.toUInt())->selected;
    };

    if (nodesOfSelectedTreesRadio->isChecked()) {
        recreateNodesTable();
    }
    if (state->skeletonState->selectedTrees.size() == 1) {
        nodeActivated();
    }
    applySelection(treeTable, func);
    treeActivated();
}

void ToolsTreeviewTab::recreateNodesTable() {
    nodeTable->selectionProtection = true;
    nodeTable->clearContents();
    if(displayedNodes > state->skeletonState->totalNodeElements or displayedNodes == DISPLAY_ALL) {
        nodeTable->setRowCount(state->skeletonState->totalNodeElements);
    } else {
        nodeTable->setRowCount(displayedNodes);
    }
    nodeTable->selectionProtection = false;

    int nodeIndex = 0;
    for (treeListElement * currentTree = state->skeletonState->firstTree; currentTree != nullptr; currentTree = currentTree->next) {
        for (nodeListElement * node = currentTree->firstNode; node != nullptr; node = node->next) {
            // cap node list elements
            if (displayedNodes != DISPLAY_ALL && nodeIndex >= displayedNodes) {
                break;
            }
            // filter for comment search string
            if (nodeSearchField->text().length() > 0) {
                if(node->comment == nullptr || !matchesSearchString(nodeSearchField->text(), QString(node->comment->content), nodeRegExCheck->isChecked())) {
                    continue;
                }
            }
            // filter for nodes of selected trees
            if (nodesOfSelectedTreesRadio->isChecked()) {
                if (node->correspondingTree->selected == false) {// node not in one of the selected trees
                    continue;
                }
            }
            if ((selectedNodes.isChecked() && !node->selected)
                    || (branchNodesChckBx->isChecked() && !node->isBranchNode)
                    || (commentNodesChckBx->isChecked() && node->comment == nullptr)) {
                continue;
            }

            nodeTable->setRow(nodeIndex, QString::number(node->nodeID), node->comment == nullptr ? "" : node->comment->content
                    , QString::number(node->position.x + 1), QString::number(node->position.y + 1), QString::number(node->position.z + 1)
                    , QString::number(node->radius));

            ++nodeIndex;
        }
    }

    nodeTable->setRowCount(nodeIndex);
    nodeTable->resizeToFit();

    applyNodeSelection();

    updateLabels();
}

void ToolsTreeviewTab::applyNodeSelection() {
    auto func = [](const QVariant & cell){
        return Skeletonizer::singleton().findNodeByNodeID(cell.toUInt())->selected;
    };
    applySelection(nodeTable, func);
    nodeActivated();
}

void ToolsTreeviewTab::treeActivated() {
    //purge
    activeTreeTable->selectionProtection = true;
    activeTreeTable->clearContents();
    activeTreeTable->setRowCount(0);
    activeTreeTable->selectionProtection = false;
    //add active tree if present and only selected tree
    if (state->skeletonState->activeTree != nullptr && state->skeletonState->selectedTrees.size() == 1) {
        insertTree(state->skeletonState->activeTree, activeTreeTable);
        activeTreeTable->selectionProtection = true;
        activeTreeTable->selectRow(0);
        activeTreeTable->selectionProtection = false;

        treeTable->selectionProtection = true;
        treeTable->clearSelection();
        treeTable->setSelectionMode(QAbstractItemView::SingleSelection);
        treeTable->selectRow(getActiveTreeRow());
        treeTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        treeTable->selectionProtection = false;

        const auto height = activeTreeTable->horizontalHeader()->sizeHint().height() + 2 + activeTreeTable->rowHeight(0);
        activeTreeTable->setMinimumHeight(height);
        activeTreeTable->setMaximumHeight(height);
    }
}

void ToolsTreeviewTab::treeAddedOrChanged() {
    //no mess here, lazily recreate tables
    recreateTreesTable();
    treeActivated();
    updateLabels();
}

void ToolsTreeviewTab::treesMerged(int, int treeID2) {
    // remove second tree from tree table
    int rowCount = treeTable->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        if (treeTable->item(i, TreeTable::TREE_ID)->text().toInt() == treeID2) {
            treeTable->selectionProtection = true;
            treeTable->removeRow(i);
            treeTable->selectionProtection = false;
            break;
        }
    }
    // if second tree was active before, update active tree table too
    QTableWidgetItem *item = activeTreeTable->item(0, TreeTable::TREE_ID);
    if (item) {
        if (item->text().toInt() != state->skeletonState->activeTree->treeID) {
            treeActivated();//new active tree
        }
    }
    updateLabels();//tree count changed
}

// update active node table
void ToolsTreeviewTab::nodeActivated() {
    //purge
    activeNodeTable->selectionProtection = true;
    activeNodeTable->clearContents();
    activeNodeTable->setRowCount(0);
    activeNodeTable->selectionProtection = false;

    //add active node if present and only selected node
    if (state->skeletonState->activeNode != nullptr && state->skeletonState->selectedNodes.size() == 1) {
        insertNode(state->skeletonState->activeNode, activeNodeTable);

        activeNodeTable->resizeToFit();
        activeNodeTable->selectionProtection = true;
        activeNodeTable->selectRow(0);
        activeNodeTable->selectionProtection = false;

        nodeTable->selectionProtection = true;
        nodeTable->clearSelection();
        nodeTable->setSelectionMode(QAbstractItemView::SingleSelection);
        nodeTable->selectRow(getActiveNodeRow());
        nodeTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        nodeTable->selectionProtection = false;

        const auto height = activeNodeTable->horizontalHeader()->sizeHint().height() + 2 + activeNodeTable->rowHeight(0);
        activeNodeTable->setMinimumHeight(height);
        activeNodeTable->setMaximumHeight(height);
    }

    treeActivated(); // update active tree table in case of tree switch
}

void ToolsTreeviewTab::nodeAdded(const nodeListElement & node) {
    insertNode(&node, nodeTable);
    if (state->skeletonState->totalNodeElements == 1) {
        nodeTable->resizeToFit();//we don’t want to slow down insertion of nodes
    }
    updateLabels();
}

void ToolsTreeviewTab::branchPushed() {
    if(branchNodesChckBx->isChecked()) {
        // the active node has become branch point, add it to node table
        insertNode(state->skeletonState->activeNode, nodeTable);
        nodeTable->resizeToFit();
    }
    updateLabels();
}

void ToolsTreeviewTab::branchPopped() {
    if(branchNodesChckBx->isChecked()) {
        // active node is no branch point anymore, remove from node table
        for(int i = 0; i < nodeTable->rowCount(); ++i) {
            if (nodeTable->item(i, NodeTable::NODE_ID)->text().toUInt() == state->skeletonState->activeNode->nodeID) {
                nodeTable->selectionProtection = true;
                nodeTable->removeRow(i);
                nodeTable->selectionProtection = false;
                break;
            }
        }
    }
    updateLabels();
}

void ToolsTreeviewTab::nodeEdited(const nodeListElement & node) {
    if (node.nodeID == state->skeletonState->activeNode->nodeID) {
        const QString cmt = node.comment == nullptr ? "" : node.comment->content;
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_COMMENT), cmt);
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_RADIUS), QString::number(state->skeletonState->activeNode->radius));
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_X), QString::number(node.position.x + 1));
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_Y), QString::number(node.position.y + 1));
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_Z), QString::number(node.position.z + 1));
    }
    for (int i = 0; i < nodeTable->rowCount(); ++i) {
        if (nodeTable->item(i, NodeTable::NODE_ID)->text().toUInt() == node.nodeID) {
            const QString cmt = node.comment == nullptr ? "" : node.comment->content;
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_COMMENT), cmt);
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_RADIUS), QString::number(state->skeletonState->activeNode->radius));
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_X), QString::number(node.position.x + 1));
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_Y), QString::number(node.position.y + 1));
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_Z), QString::number(node.position.z + 1));
            break;
        }
    }
}

void ToolsTreeviewTab::resetData() {
    recreateTreesTable();
    recreateNodesTable();
    //active tree and node table get updated during the selection application
    updateLabels();
}

void ToolsTreeviewTab::insertTree(treeListElement *tree, TreeTable *table) {
    if (tree == nullptr) {
        return;
    }

    if (table == treeTable && treeSearchField->text().length() > 0) {
        if (tree->comment == nullptr) {
            return;
        }
        if (!matchesSearchString(treeSearchField->text(), tree->comment, treeRegExCheck->isChecked())) {
            return;
        }
    }

    table->insertRow(0);
    table->setRow(0, QString::number(tree->treeID)
            , QColor(tree->color.r*255, tree->color.g*255, tree->color.b*255, 0.6*255)
            , tree->comment == nullptr ? "" : tree->comment);
}

void ToolsTreeviewTab::insertNode(const nodeListElement *node, NodeTable *table) {
    if(node == nullptr) {
        return;
    }

    if(table == nodeTable) { // first check if node will be filtered out
        if (nodeSearchField->text().length() > 0) {
            if (node->comment == nullptr || !matchesSearchString(nodeSearchField->text(), QString(node->comment->content), nodeRegExCheck->isChecked()) ) {
                return;
            }
        }
        // filter for nodes of selected trees
        if (nodesOfSelectedTreesRadio->isChecked() and state->skeletonState->selectedTrees.size() > 0) {
            if(node->correspondingTree->selected == false) {// node not in one of the selected trees
                return;
            }
        }
        if ((selectedNodes.isChecked() && !node->selected)
                || (branchNodesChckBx->isChecked() && !node->isBranchNode)
                || (commentNodesChckBx->isChecked() && node->comment == nullptr)) {
            return;
        }
    }

    // throw away oldest row if displayed nodes maximum is reached
    if(table == nodeTable && displayedNodes != DISPLAY_ALL && nodeTable->rowCount() >= displayedNodes) {
        table->removeRow(nodeTable->rowCount() - 1);
    }

    int position = 0;//insert on first position if node is firstnode or on activeNodeTable
    if (table == nodeTable) {
        for (int i = 0; i < nodeTable->rowCount(); ++i) {//subsequent nodes are added after the first node of their tree
            if (nodeTable->item(i, 0)->text().toUInt() == node->correspondingTree->firstNode->nodeID) {
                position = i+1;//we want to add one row after this
                break;
            }
        }
    }

    table->insertRow(position);
    table->setRow(position, QString::number(node->nodeID), node->comment == nullptr ? "" : node->comment->content
            , QString::number(node->position.x + 1), QString::number(node->position.y + 1), QString::number(node->position.z + 1)
            , QString::number(node->radius));
}

int ToolsTreeviewTab::getActiveTreeRow() {
    int activeRow = -1;
    for(int i = 0; i < treeTable->rowCount(); ++i) {
        if(treeTable->item(i, TreeTable::TREE_ID)->text().toInt() == state->skeletonState->activeTree->treeID) {
            activeRow = i;
            break;
        }
    }
    return activeRow;
}

int ToolsTreeviewTab::getActiveNodeRow() {
    int activeRow = -1;
    for(int i = 0; i < nodeTable->rowCount(); ++i) {
        if(nodeTable->item(i, NodeTable::NODE_ID)->text().toUInt() == state->skeletonState->activeNode->nodeID) {
            activeRow = i;
            break;
        }
    }
    return activeRow;
}
