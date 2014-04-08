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

enum {
    NODECOMBO_100, NODECOMBO_1000, NODECOMBO_5000, NODECOMBO_ALL
};

enum {
    DISPLAY_ALL
};

extern stateInfo *state;

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
    mainLayout->addWidget(splitter);
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
    QObject::connect(activeTreeTable, &TreeTable::itemSelectionChanged, this, &ToolsTreeviewTab::activeTreeSelectionChanged);
    QObject::connect(activeTreeTable, &TreeTable::nodesUpdateSignal, this, &ToolsTreeviewTab::recreateNodesTable);

    QObject::connect(treeTable, &TreeTable::customContextMenuRequested, this, &ToolsTreeviewTab::contextMenu);
    QObject::connect(treeTable, &TreeTable::deleteTreesSignal, this, &ToolsTreeviewTab::deleteTreesAction);
    QObject::connect(treeTable, &TreeTable::itemChanged, this, &ToolsTreeviewTab::treeItemChanged);
    QObject::connect(treeTable, &TreeTable::itemDoubleClicked, this, &ToolsTreeviewTab::treeItemDoubleClicked);
    QObject::connect(treeTable, &TreeTable::itemSelectionChanged, this, &ToolsTreeviewTab::treeSelectionChanged);
    QObject::connect(treeTable, &TreeTable::nodesUpdateSignal, this, &ToolsTreeviewTab::recreateNodesTable);

    QObject::connect(activeNodeTable, &NodeTable::customContextMenuRequested, this, &ToolsTreeviewTab::contextMenu);
    QObject::connect(activeNodeTable, &NodeTable::deleteNodesSignal, this, &ToolsTreeviewTab::deleteNodesAction);
    QObject::connect(activeNodeTable, &NodeTable::itemChanged, this, &ToolsTreeviewTab::actNodeItemChanged);
    QObject::connect(activeNodeTable, &NodeTable::itemDoubleClicked, this, &ToolsTreeviewTab::nodeItemDoubleClicked);
    QObject::connect(activeNodeTable, &NodeTable::itemSelectionChanged, this, &ToolsTreeviewTab::activeNodeSelectionChanged);

    QObject::connect(nodeTable, &NodeTable::customContextMenuRequested, this, &ToolsTreeviewTab::contextMenu);
    QObject::connect(nodeTable, &NodeTable::deleteNodesSignal, this, &ToolsTreeviewTab::deleteNodesAction);
    QObject::connect(nodeTable, &NodeTable::itemChanged, this, &ToolsTreeviewTab::nodeItemChanged);
    QObject::connect(nodeTable, &NodeTable::itemDoubleClicked, this, &ToolsTreeviewTab::nodeItemDoubleClicked);
    QObject::connect(nodeTable, &NodeTable::itemSelectionChanged, this, &ToolsTreeviewTab::nodeSelectionChanged);
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
        Skeletonizer::mergeTrees(CHANGE_MANUAL, treeID1, treeID2, true);
        treesMerged(treeID1, treeID2);
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
        qDebug("no trees to delete");
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
            emit deleteSelectedTreesSignal();
            treesDeleted();
        }
    }
}

void ToolsTreeviewTab::setTreeCommentAction() {
    bool applied = false;
    auto text = QInputDialog::getText(this, "Edit Tree Comment", "new tree comment", QLineEdit::Normal, treeCommentBuffer, &applied);
    if (applied) {
        auto treeCommentBuffer = text;
        if (activeTreeTable->hasFocus()) {
            Skeletonizer::addTreeComment(CHANGE_MANUAL, state->skeletonState->activeTree->treeID, treeCommentBuffer.toLocal8Bit().data());
            setText(activeTreeTable, activeTreeTable->item(0, TreeTable::TREE_COMMENT), treeCommentBuffer);
            int row = getActiveTreeRow();
            if (row != -1) {
                setText(treeTable, treeTable->item(row, TreeTable::TREE_COMMENT), treeCommentBuffer);
            }
        } else {
            for (auto tree : state->skeletonState->selectedTrees) {
                Skeletonizer::addTreeComment(CHANGE_MANUAL, tree->treeID, treeCommentBuffer);
            }
            recreateTreesTable();
        }
    }
}

void ToolsTreeviewTab::contextMenu(QPoint pos) {
    if (activeNodeTable->hasFocus()) {
        QMenu nodeContextMenu;
        QObject::connect(nodeContextMenu.addAction("Jump to"), &QAction::triggered, this, &ToolsTreeviewTab::activateFirstSelectedNode);
        QObject::connect(nodeContextMenu.addAction("Split component from tree"), &QAction::triggered, this, &ToolsTreeviewTab::splitComponentAction);
        QObject::connect(nodeContextMenu.addAction(QIcon(":/images/icons/user-trash.png"), "delete active node"), &QAction::triggered, this, &ToolsTreeviewTab::deleteNodesAction);

        nodeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedNodes.size() == 1);//jump to
        nodeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedNodes.size() == 1);//split connected components
        nodeContextMenu.actions().at(2)->setEnabled(state->skeletonState->selectedNodes.size() == 1);//del active node
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        nodeContextMenu.exec(activeNodeTable->viewport()->mapToGlobal(pos));
    } else if (nodeTable->hasFocus()) {
        QMenu nodeContextMenu;
        QObject::connect(nodeContextMenu.addAction("Set as active node"), &QAction::triggered, this, &ToolsTreeviewTab::activateFirstSelectedNode);
        QObject::connect(nodeContextMenu.addAction("Split component from tree"), &QAction::triggered, this, &ToolsTreeviewTab::splitComponentAction);
        QObject::connect(nodeContextMenu.addAction("(Un)link nodes"), &QAction::triggered, this, &ToolsTreeviewTab::linkNodesAction);
        QObject::connect(nodeContextMenu.addAction("Set comment for node(s)"), &QAction::triggered, this, &ToolsTreeviewTab::setNodeCommentAction);
        QObject::connect(nodeContextMenu.addAction("Set radius for node(s)"), &QAction::triggered, this, &ToolsTreeviewTab::setNodeRadiusAction);
        QObject::connect(nodeContextMenu.addAction(QIcon(":/images/icons/user-trash.png"), "(DEL)ete node(s)"), &QAction::triggered, this, &ToolsTreeviewTab::deleteNodesAction);

        nodeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedNodes.size() == 1);//active node
        nodeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedNodes.size() == 1);//split connected components
        nodeContextMenu.actions().at(2)->setEnabled(state->skeletonState->selectedNodes.size() == 2);//link nodes needs two selected nodes
        nodeContextMenu.actions().at(3)->setEnabled(state->skeletonState->selectedNodes.size() > 0);
        nodeContextMenu.actions().at(4)->setEnabled(state->skeletonState->selectedNodes.size() > 0);
        nodeContextMenu.actions().at(5)->setEnabled(state->skeletonState->selectedNodes.size() > 0);
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        nodeContextMenu.exec(nodeTable->viewport()->mapToGlobal(pos));
    } else if (activeTreeTable->hasFocus()) {
        QMenu treeContextMenu;
        QObject::connect(treeContextMenu.addAction("Set comment for tree"), &QAction::triggered, this, &ToolsTreeviewTab::setTreeCommentAction);
        QObject::connect(treeContextMenu.addAction("Move selected node(s) to this tree"), &QAction::triggered, this, &ToolsTreeviewTab::moveNodesAction);
        QObject::connect(treeContextMenu.addAction("Restore default color"), &QAction::triggered, this, &ToolsTreeviewTab::restoreColorAction);
        QObject::connect(treeContextMenu.addAction(QIcon(":/images/icons/user-trash.png"), "(DEL)ete tree"), &QAction::triggered, this, &ToolsTreeviewTab::deleteTreesAction);

        treeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        treeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedTrees.size() == 1 && state->skeletonState->selectedNodes.size() > 0);//move nodes
        treeContextMenu.actions().at(2)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        treeContextMenu.actions().at(3)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        treeContextMenu.exec(activeTreeTable->viewport()->mapToGlobal(pos));
    } else if (treeTable->hasFocus()) {
        QMenu treeContextMenu;
        QObject::connect(treeContextMenu.addAction("Set as active tree"), &QAction::triggered, this, &ToolsTreeviewTab::activateFirstSelectedTree);
        QObject::connect(treeContextMenu.addAction("Move selected node(s) to this tree"), &QAction::triggered, this, &ToolsTreeviewTab::moveNodesAction);
        QObject::connect(treeContextMenu.addAction("Merge trees"), &QAction::triggered, this, &ToolsTreeviewTab::mergeTreesAction);
        QObject::connect(treeContextMenu.addAction("Set comment for tree(s)"), &QAction::triggered, this, &ToolsTreeviewTab::setTreeCommentAction);
        QObject::connect(treeContextMenu.addAction("Restore default color"), &QAction::triggered, this, &ToolsTreeviewTab::restoreColorAction);
        QObject::connect(treeContextMenu.addAction(QIcon(":/images/icons/user-trash.png"), "Delete tree(s)"), &QAction::triggered, this, &ToolsTreeviewTab::deleteTreesAction);

        treeContextMenu.actions().at(0)->setEnabled(state->skeletonState->selectedTrees.size() == 1);//set as active tree action
        treeContextMenu.actions().at(1)->setEnabled(state->skeletonState->selectedTrees.size() == 1 && state->skeletonState->selectedNodes.size() > 0);//move nodes
        treeContextMenu.actions().at(2)->setEnabled(state->skeletonState->selectedTrees.size() == 2);//merge trees action
        treeContextMenu.actions().at(3)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        treeContextMenu.actions().at(4)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        treeContextMenu.actions().at(5)->setEnabled(state->skeletonState->selectedTrees.size() > 0);
        //display the context menu at pos in screen coordinates instead of widget coordinates of the content of the currently focused table
        treeContextMenu.exec(treeTable->viewport()->mapToGlobal(pos));
    }
}

void ToolsTreeviewTab::deleteNodesAction() {
    if (activeNodeTable->hasFocus()) {
        QMessageBox prompt;
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Question);
        prompt.setWindowTitle("Cofirmation required");
        prompt.setText("Do you really want to delete the active node?");
        QPushButton *confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            emit delActiveNodeSignal();//skeletonizer
            recreateNodesTable();//removes active node from nodeTable
            nodeActivated();//removes active node from activeNodeTable
        }
    } else {
        if(state->skeletonState->selectedNodes.size() == 0) {
            qDebug("no nodes");
            return;
        }
        QMessageBox prompt;
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Question);
        prompt.setWindowTitle("Cofirmation required");
        prompt.setText("Do you really want to deleted selected node(s)?");
        QPushButton *confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            emit deleteSelectedNodesSignal();//skeletonizer
            recreateNodesTable();//removes nodes from nodeTable
            nodeActivated();//updates active node
        }
    }
}

void ToolsTreeviewTab::setNodeRadiusAction() {
    bool applied = false;
    auto value = QInputDialog::getDouble(this, "Edit Node Radii", "new node radius", radiusBuffer, 0, 100000, 1, &applied);
    if (applied) {
        radiusBuffer = value;

        if (activeNodeTable->hasFocus()) {
            Skeletonizer::editNode(CHANGE_MANUAL, 0, state->skeletonState->activeNode, radiusBuffer,
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
                Skeletonizer::editNode(CHANGE_MANUAL, 0, node, radiusBuffer,
                                       node->position.x, node->position.y, node->position.z, node->createdInMag);
            }
            recreateNodesTable();
        }
    }
}

void ToolsTreeviewTab::linkNodesAction() {
    const auto node0 = state->skeletonState->selectedNodes[0]->nodeID;
    const auto node1 = state->skeletonState->selectedNodes[1]->nodeID;
    //segments are only stored and searched in one direction so we have to search for both
    if (Skeletonizer::findSegmentByNodeIDs(node0, node1) != nullptr) {
        emit delSegmentSignal(CHANGE_MANUAL, node0, node1, nullptr, true);
    } else if (Skeletonizer::findSegmentByNodeIDs(node1, node0) != nullptr) {
        emit delSegmentSignal(CHANGE_MANUAL, node1, node0, nullptr, true);
    } else{
        emit addSegmentSignal(CHANGE_MANUAL, node0, node1, true);
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
        TreeListElement *prevActive = state->skeletonState->activeTree;
        Skeletonizer::splitConnectedComponent(CHANGE_MANUAL, state->skeletonState->selectedNodes.front()->nodeID, true);
        if(prevActive != state->skeletonState->activeTree) { // if the active tree changes, the splitting was successful
            // when a tree is split, a new tree has been created and made active
            treeActivated();//add new tree to active tree table
            insertTree(state->skeletonState->activeTree, treeTable);//add new tree to tree table
            emit updateAnnotationLabelsSignal();//tree count changed
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
        for (auto * const node : state->skeletonState->selectedNodes) {
            Skeletonizer::moveNodeToTree(node, state->skeletonState->selectedTrees.front()->treeID);
        }
    }
    recreateNodesTable();
}

void ToolsTreeviewTab::setNodeCommentAction() {
    bool applied = false;
    auto text = QInputDialog::getText(this, "Edit Node Comment", "new node comment", QLineEdit::Normal, nodeCommentBuffer, &applied);
    if (applied) {
        nodeCommentBuffer = text;

        if (activeNodeTable->hasFocus()) {
            if (nodeCommentBuffer.length() > 0) {
                if(state->skeletonState->activeNode->comment == nullptr) {
                    Skeletonizer::addComment(CHANGE_MANUAL, nodeCommentBuffer.toLocal8Bit().data(),
                                             state->skeletonState->activeNode, 0, true);
                } else {
                    Skeletonizer::editComment(CHANGE_MANUAL,
                                              state->skeletonState->activeNode->comment, 0,
                                              nodeCommentBuffer.toLocal8Bit().data(),
                                              state->skeletonState->activeNode, 0, true);
                }
            } else if(state->skeletonState->activeNode->comment != nullptr) {
                Skeletonizer::delComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment,
                                         state->skeletonState->activeNode->nodeID, true);
            }
            setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_COMMENT), nodeCommentBuffer);
            int row = getActiveNodeRow();
            if (row != -1) {
                setText(nodeTable, nodeTable->item(row, NodeTable::NODE_COMMENT), nodeCommentBuffer);
            }
        } else {// nodeTable focused
            for (auto node : state->skeletonState->selectedNodes) {
                if (node == state->skeletonState->activeNode) {
                    setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_COMMENT), nodeCommentBuffer);
                }
                if(nodeCommentBuffer.length() > 0) {
                    if (node->comment == nullptr) {
                        Skeletonizer::addComment(CHANGE_MANUAL, nodeCommentBuffer.toLocal8Bit().data(), node, 0, true);
                    } else {
                        Skeletonizer::editComment(CHANGE_MANUAL,
                                                  node->comment, 0, nodeCommentBuffer.toLocal8Bit().data(), node, 0, true);
                    }
                } else if (node-> comment != nullptr) {
                    Skeletonizer::delComment(CHANGE_MANUAL, node->comment, node->nodeID, true);
                }
            }
            recreateNodesTable();
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

    if(displayedNodes == nodeTable->rowCount()) {
        return;
    }
    else {
        recreateNodesTable();
    }

    emit updateAnnotationLabelsSignal();
}

void ToolsTreeviewTab::actTreeItemChanged(QTableWidgetItem *item) {
    if(activeTreeTable->selectionProtection) {
        return;
    }
    if (item->column() == TreeTable::TREE_COMMENT) {
        Skeletonizer::addTreeComment(CHANGE_MANUAL, state->skeletonState->activeTree->treeID, item->text().toUtf8().data());
        recreateTreesTable();
        treeActivated();
    }
}

void ToolsTreeviewTab::treeItemChanged(QTableWidgetItem* item) {
//    if(treeTable->changeByCode) {
//        return;
//    }
    QTableWidgetItem *idItem = treeTable->item(item->row(), TreeTable::TREE_ID);
    if(idItem == nullptr) {
        return;
    }
    TreeListElement *selectedTree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
    if(selectedTree == nullptr) {
        return;
    }

    switch(item->column()) {
    case TreeTable::TREE_COMMENT: {
            Skeletonizer::addTreeComment(CHANGE_MANUAL, selectedTree->treeID, item->text().toLocal8Bit().data());
            // check if tree will still be displayed in list
            if(treeSearchField->text().isEmpty() == false) {
                if(matchesSearchString(treeSearchField->text(),
                                       QString(selectedTree->comment),
                                       treeRegExCheck->isChecked()) == false) {
                    for(int i = 0; i < treeTable->rowCount(); ++i) {
                        if(treeTable->item(i, TreeTable::TREE_ID)->text().toInt() == selectedTree->treeID) {
                            treeTable->removeRow(i);
                            break;
                        }
                    }
                }
            }
            // update active tree table if necessary
            if(selectedTree == state->skeletonState->activeTree) {
                if(activeTreeTable->item(0, TreeTable::TREE_COMMENT)) {
                    setText(activeTreeTable, activeTreeTable->item(0, TreeTable::TREE_COMMENT), item->text());
                }
            }
            break;
        }
    default:
        break;
    }
}

void ToolsTreeviewTab::actNodeItemChanged(QTableWidgetItem *item) {
    if(activeNodeTable->selectionProtection) {
        return;
    }
    NodeListElement *activeNode = state->skeletonState->activeNode;
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
    case NodeTable::NODE_COMMENT:
        if(activeNode->comment) {
            if(activeRow != -1) {
                setText(nodeTable, nodeTable->item(activeRow, NodeTable::NODE_COMMENT), item->text());
            }
            if(item->text().length() == 0) {
                Skeletonizer::delComment(CHANGE_MANUAL, activeNode->comment, 0, true);
                if(activeRow == -1) {
                    break;
                }
                if(commentNodesChckBx->isChecked() or nodeSearchField->text().isEmpty() == false) {
                    // since node has no comment anymore, it should be filtered out
                    if(activeRow != -1) {
                        nodeTable->removeRow(activeRow);
                    }
                }
                break;
            }
            Skeletonizer::editComment(CHANGE_MANUAL, activeNode->comment, 0,
                                      item->text().toLocal8Bit().data(), activeNode, 0, true);
            if(activeRow != -1) {
                if(nodeSearchField->text().isEmpty() == false) {
                    if(matchesSearchString(nodeSearchField->text(),
                                           QString(activeNode->comment->content),
                                           nodeRegExCheck->isChecked()) == false) {
                        nodeTable->removeRow(activeRow);
                    }
                }
            }
        }
        else {
            // node will not need to be filtered out. It wasn't filtered out before, either.
            if(item->text().length() == 0) {
                break;
            }
            Skeletonizer::addComment(CHANGE_MANUAL, item->text().toLocal8Bit().data(), activeNode, 0, true);
            if(activeRow != -1) {
                setText(nodeTable, nodeTable->item(activeRow, NodeTable::NODE_COMMENT), item->text());
            }
        }
        break;
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
    emit updateAnnotationLabelsSignal();
}

void ToolsTreeviewTab::nodeItemChanged(QTableWidgetItem* item) {
    if(nodeTable->selectionProtection) {
        return;
    }
    QTableWidgetItem *idItem = nodeTable->item(item->row(), NodeTable::NODE_ID);
    if(idItem == nullptr) {
        return;
    }
    NodeListElement *selectedNode = Skeletonizer::findNodeByNodeID(idItem->text().toInt());
    if(selectedNode == nullptr) {
        return;
    }
    switch(item->column()) {
    case NodeTable::NODE_COMMENT:
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NodeTable::NODE_COMMENT)) {
                setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_COMMENT), item->text());
            }
        }
        if(selectedNode->comment) {
            if(item->text().length() == 0) {
                Skeletonizer::delComment(CHANGE_MANUAL, selectedNode->comment, 0, true);
                if(commentNodesChckBx->isChecked() or nodeSearchField->text().isEmpty() == false) {
                    // since node has no comment anymore, it should be filtered out
                    nodeTable->removeRow(item->row());
                }
                break;
            }
            Skeletonizer::editComment(CHANGE_MANUAL, selectedNode->comment, 0,
                                      item->text().toLocal8Bit().data(), selectedNode, 0, true);
            if(nodeSearchField->text().isEmpty() == false) {
                if(matchesSearchString(nodeSearchField->text(),
                                       QString(selectedNode->comment->content),
                                       nodeRegExCheck->isChecked()) == false) {
                    nodeTable->removeRow(item->row());
                }
            }
        }
        else {
            // node will not need to be filtered out. It wasn't filtered out before, either.
            if(item->text().length() == 0) {
                break;
            }
            Skeletonizer::addComment(CHANGE_MANUAL, item->text().toLocal8Bit().data(), selectedNode, 0, true);
            if(selectedNode == state->skeletonState->activeNode) {
                if(activeNodeTable->item(0, NodeTable::NODE_COMMENT)) {
                    setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_COMMENT), item->text());
                }
            }
        }
        break;
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
    emit updateAnnotationLabelsSignal();
}

void ToolsTreeviewTab::activeTreeSelectionChanged() {
    if (activeTreeTable->selectionProtection) {
        return;
    }

    treeTable->selectionProtection = true;
    treeTable->clearSelection();
    treeTable->selectionProtection = false;

    state->skeletonState->selectedTrees.clear();
    QModelIndexList selected = activeTreeTable->selectionModel()->selectedRows();
    if (selected.size() == 1) {
        state->skeletonState->selectedTrees.push_back(state->skeletonState->activeTree);
        //select active tree also in treeTable
        treeTable->selectionProtection = true;
        treeTable->selectRow(getActiveTreeRow());
        treeTable->selectionProtection = false;

        if(nodesOfSelectedTreesRadio->isChecked()) {
            recreateNodesTable();
        }
    }
}

void ToolsTreeviewTab::treeSelectionChanged() {
    if (treeTable->selectionProtection) {
        return;
    }

    activeTreeTable->selectionProtection = true;
    activeTreeTable->clearSelection();
    activeTreeTable->selectionProtection = false;

    state->skeletonState->selectedTrees.clear();
    QModelIndexList selected = treeTable->selectionModel()->selectedRows();
    foreach(QModelIndex index, selected) {
        TreeListElement * const tree = Skeletonizer::findTreeByTreeID(index.data().toInt());
        if (tree) {
            state->skeletonState->selectedTrees.push_back(tree);
            //select active tree also in activeTreeTable
            if (tree == state->skeletonState->activeTree) {
                activeTreeTable->selectionProtection = true;
                activeTreeTable->selectRow(0);
                activeTreeTable->selectionProtection = false;
            }
        }
    }
    if (nodesOfSelectedTreesRadio->isChecked()) {
        recreateNodesTable();
    }
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

void ToolsTreeviewTab::activeNodeSelectionChanged() {
    if (activeNodeTable->selectionProtection) {
        return;
    }

    nodeTable->selectionProtection = true;
    nodeTable->clearSelection();
    nodeTable->selectionProtection = false;

    for (auto & selectedNode : state->skeletonState->selectedNodes) {
        selectedNode->selected = false;
    }
    state->skeletonState->selectedNodes.clear();

    if (activeNodeTable->selectionModel()->selectedRows().size() == 1) {
        if (state->skeletonState->activeNode) {
            state->skeletonState->activeNode->selected = true;
            state->skeletonState->selectedNodes.push_back(state->skeletonState->activeNode);
        }
        //selection of active node in nodeTable
        nodeTable->selectionProtection = true;
        nodeTable->selectRow(getActiveNodeRow());
        nodeTable->selectionProtection = false;
    }
    //(de)select of active node means 0 or 1 selected node, so this is no performance problem
    if (selectedNodes.isChecked()) {
        recreateNodesTable();
    }
}

void ToolsTreeviewTab::nodeSelectionChanged() {
    if (nodeTable->selectionProtection) {
        return;
    }

    activeNodeTable->selectionProtection = true;
    activeNodeTable->clearSelection();
    activeNodeTable->selectionProtection = false;

    for (auto & selectedNode : state->skeletonState->selectedNodes) {
        selectedNode->selected = false;
    }
    state->skeletonState->selectedNodes.clear();

    QModelIndexList selected = nodeTable->selectionModel()->selectedRows();
    foreach(QModelIndex index, selected) {
        NodeListElement * const node = Skeletonizer::findNodeByNodeID(index.data().toInt());
        //select node
        if (node != nullptr) {
            node->selected = true;
            state->skeletonState->selectedNodes.push_back(node);
        }
        //select active node also in activeNodeTable
        if (node == state->skeletonState->activeNode) {
            activeNodeTable->selectionProtection = true;
            activeNodeTable->selectRow(0);
            activeNodeTable->selectionProtection = false;
        }
    }
    nodeTable->setDragEnabled(false);//enable multi-selection on previously unselected elements
}

void ToolsTreeviewTab::activateFirstSelectedNode() {
    if (state->skeletonState->selectedNodes.size() == 1) {
        emit setActiveNodeSignal(CHANGE_MANUAL, nullptr, state->skeletonState->selectedNodes.front()->nodeID);
        emit JumpToActiveNodeSignal();
        nodeActivated();
    }
}

void ToolsTreeviewTab::activateFirstSelectedTree() {
    if (state->skeletonState->selectedTrees.size() == 1) {
        Skeletonizer::setActiveTreeByID(state->skeletonState->selectedTrees.front()->treeID);
        treeActivated();
    }
}

void ToolsTreeviewTab::nodeItemDoubleClicked(QTableWidgetItem * item) {
    if (item->column() == NodeTable::NODE_ID) {
        activateFirstSelectedNode();
    }
}

void ToolsTreeviewTab::updateTreeColorCell(TreeTable *table, int row) {
    QTableWidgetItem * const idItem = table->item(row, TreeTable::TREE_ID);
    TreeListElement * const tree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
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
            qDebug("invalid regex");
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

    QItemSelection selectedItems = treeTable->selectionModel()->selection();
    std::size_t startIndex;
    bool blockSelection = false;

    size_t treeIndex = 0;
    for (TreeListElement * currentTree = state->skeletonState->firstTree; currentTree != nullptr; currentTree = currentTree->next) {
        // filter comment for search string match
        if(treeSearchField->text().length() > 0) {
            if(strlen(currentTree->comment) == 0) {
                continue;
            }
            if(matchesSearchString(treeSearchField->text(), currentTree->comment, treeRegExCheck->isChecked()) == false) {
                continue;
            }
        }
        //check selection
        if (std::find(std::begin(state->skeletonState->selectedTrees), std::end(state->skeletonState->selectedTrees), currentTree)
                != std::end(state->skeletonState->selectedTrees)) {//tree is selected
            if (!blockSelection) {
                blockSelection = true;
                startIndex = treeIndex;
            }
        } else {
            if (blockSelection) {
                selectedItems.select(treeTable->model()->index(startIndex, 0), treeTable->model()->index(treeIndex-1, treeTable->model()->columnCount()-1));
                blockSelection = false;
            }
        }

        treeTable->setRow(treeIndex, QString::number(currentTree->treeID)
                , QColor(currentTree->color.r*255, currentTree->color.g*255, currentTree->color.b*255, 0.6*255)
                , currentTree->comment == nullptr ? "" : currentTree->comment);

        treeIndex++;//this is here so it doesn’t get incremented on continue
    }

    if (blockSelection) {
        selectedItems.select(treeTable->model()->index(startIndex, 0), treeTable->model()->index(treeIndex-1, treeTable->model()->columnCount()-1));
    }

    treeTable->selectionProtection = true;
    treeTable->selectionModel()->select(selectedItems, QItemSelectionModel::Select);
    treeTable->selectionProtection = false;

    treeTable->setRowCount(treeIndex);
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

    QItemSelection selectedItems = nodeTable->selectionModel()->selection();
    std::size_t startIndex;
    bool blockSelection = false;

    int nodeIndex = 0;
    for (TreeListElement * currentTree = state->skeletonState->firstTree; currentTree != nullptr; currentTree = currentTree->next) {
        for (NodeListElement * node = currentTree->firstNode; node != nullptr; node = node->next) {
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
                if (std::find(std::begin(state->skeletonState->selectedTrees), std::end(state->skeletonState->selectedTrees), node->correspondingTree)
                        == std::end(state->skeletonState->selectedTrees)) {// node not in one of the selected trees
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

            if (!blockSelection && node->selected) {//start block selection
                blockSelection = true;
                startIndex = nodeIndex;
            }
            if (blockSelection && !node->selected) {//end block selection
                selectedItems.select(nodeTable->model()->index(startIndex, 0), nodeTable->model()->index(nodeIndex-1, nodeTable->model()->columnCount()-1));
                blockSelection = false;
            }

            ++nodeIndex;
        }
    }
    //finish last blockselection – if any
    if (blockSelection) {
        selectedItems.select(nodeTable->model()->index(startIndex, 0), nodeTable->model()->index(nodeIndex-1, nodeTable->model()->columnCount()-1));
    }

    //apply accumulated selections
    nodeTable->selectionProtection = true;
    nodeTable->selectionModel()->select(selectedItems, QItemSelectionModel::Select);
    nodeTable->selectionProtection = false;

    nodeTable->setRowCount(nodeIndex);
    nodeTable->resizeToFit();

    emit updateAnnotationLabelsSignal();
}

void ToolsTreeviewTab::treeActivated() {
    //purge
    activeTreeTable->selectionProtection = true;
    activeTreeTable->clearContents();
    activeTreeTable->setRowCount(0);
    activeTreeTable->selectionProtection = false;
    //add active tree if present
    if (state->skeletonState->activeTree != nullptr) {
        insertTree(state->skeletonState->activeTree, activeTreeTable);
        //find if active tree is selected
        const auto & trees = state->skeletonState->selectedTrees;
        if (std::find(std::begin(trees), std::end(trees), state->skeletonState->activeTree) != std::end(trees)) {
            activeTreeTable->selectionProtection = true;
            activeTreeTable->selectRow(0);
            activeTreeTable->selectionProtection = false;
        }
    }
}

void ToolsTreeviewTab::treeAdded(TreeListElement *) {
    //no mess here, lazily recreate tables
    recreateTreesTable();
    treeActivated();
    emit updateAnnotationLabelsSignal();
}

void ToolsTreeviewTab::treesDeleted() {
    //remove tree
    if (activeTreeTable->hasFocus()) {
        const int activeID = activeTreeTable->item(0, TreeTable::TREE_ID)->text().toInt(); // for deletion in tree table

        for (int i = 0; i < treeTable->rowCount(); ++i) {
            if (treeTable->item(i, TreeTable::TREE_ID)->text().toInt() == activeID) {
                treeTable->removeRow(i);
                break;
            }
        }
    } else { // tree table is focused
        for (int i = treeTable->rowCount() - 1; i >= 0; --i) {
            const int treeID = treeTable->item(i, TreeTable::TREE_ID)->text().toInt();
            if (Skeletonizer::findTreeByTreeID(treeID) == nullptr) {
                treeTable->removeRow(i);
            }
        }
    }

    // remove all nodes of the tree
    int nodeRows = nodeTable->rowCount();
    QVector<int> rowsToDel;
    for (int i = 0; i < nodeRows; ++i) {
        if (Skeletonizer::findNodeByNodeID(nodeTable->item(i, NodeTable::NODE_ID)->text().toInt()) == nullptr) {
            rowsToDel.push_back(i);
        }
    }
    std::sort(rowsToDel.begin(), rowsToDel.end());
    for (int i = rowsToDel.size() - 1; i >= 0; --i) {
        nodeTable->removeRow(rowsToDel[i]);
    }

    //handle active node/tree table
    nodeActivated();//also activates tree

    emit updateAnnotationLabelsSignal(); // update the tree and node count
}

void ToolsTreeviewTab::treesMerged(int, int treeID2) {
    // remove second tree from tree table
    int rowCount = treeTable->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        if (treeTable->item(i, TreeTable::TREE_ID)->text().toInt() == treeID2) {
            treeTable->removeRow(i);//the signal emmited here will also trigger recreatation of selected trees
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
    emit updateAnnotationLabelsSignal();//tree count changed
}

// update active node table
void ToolsTreeviewTab::nodeActivated() {
    //purge
    activeNodeTable->selectionProtection = true;
    activeNodeTable->clearContents();
    activeNodeTable->setRowCount(0);
    activeNodeTable->selectionProtection = false;
    //add active node if present
    if (state->skeletonState->activeNode != nullptr) {
        insertNode(state->skeletonState->activeNode, activeNodeTable);
        activeNodeTable->resizeToFit();
        //select
        if (state->skeletonState->activeNode->selected) {
            activeNodeTable->selectionProtection = true;
            activeNodeTable->selectRow(0);
            activeNodeTable->selectionProtection = false;
        }
    }
    treeActivated(); // update active tree table in case of tree switch
}

void ToolsTreeviewTab::nodeAdded() {
    if(state->skeletonState->activeNode == nullptr) {
        return;
    }
    insertNode(state->skeletonState->activeNode, nodeTable);
    nodeActivated();
    if (state->skeletonState->totalNodeElements == 1) {
        nodeTable->resizeToFit();//we don’t want to slow down insertion of nodes
    }
    emit updateAnnotationLabelsSignal();
}

void ToolsTreeviewTab::branchPushed() {
    if(branchNodesChckBx->isChecked()) {
        // the active node has become branch point, add it to node table
        insertNode(state->skeletonState->activeNode, nodeTable);
        nodeTable->resizeToFit();
        emit updateAnnotationLabelsSignal();
    }
}

void ToolsTreeviewTab::branchPopped() {
    if(branchNodesChckBx->isChecked()) {
        // active node is no branch point anymore, remove from node table
        for(int i = 0; i < nodeTable->rowCount(); ++i) {
            if (nodeTable->item(i, NodeTable::NODE_ID)->text().toUInt() == state->skeletonState->activeNode->nodeID) {
                nodeTable->removeRow(i);
                break;
            }
        }
        emit updateAnnotationLabelsSignal();
    }
}

void ToolsTreeviewTab::nodeCommentChanged(NodeListElement *node) {
    if (node == state->skeletonState->activeNode) {
        const QString cmt = node->comment == nullptr ? "" : node->comment->content;
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_COMMENT), cmt);
    }
    for (int i = 0; i < nodeTable->rowCount(); ++i) {
        if (nodeTable->item(i, NodeTable::NODE_ID)->text().toUInt() == node->nodeID) {
            const QString cmt = node->comment == nullptr ? "" : node->comment->content;
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_COMMENT), cmt);
            break;
        }
    }
}

void ToolsTreeviewTab::nodeRadiusChanged(NodeListElement *node) {
    if (node == state->skeletonState->activeNode) {
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_RADIUS), QString::number(state->skeletonState->activeNode->radius));
    }
    for (int i = 0; i < nodeTable->rowCount(); ++i) {
        if (nodeTable->item(i, NodeTable::NODE_ID)->text().toUInt() == node->nodeID) {
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_RADIUS), QString::number(state->skeletonState->activeNode->radius));
            break;
        }
    }
}

void ToolsTreeviewTab::nodePositionChanged(NodeListElement *node) {
    if (node == state->skeletonState->activeNode) {
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_X), QString::number(node->position.x + 1));
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_Y), QString::number(node->position.y + 1));
        setText(activeNodeTable, activeNodeTable->item(0, NodeTable::NODE_Z), QString::number(node->position.z + 1));
    }
    for (int i = 0; i < nodeTable->rowCount(); ++i) {
        if (nodeTable->item(i, NodeTable::NODE_ID)->text().toUInt() == node->nodeID) {
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_X), QString::number(node->position.x + 1));
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_Y), QString::number(node->position.y + 1));
            setText(nodeTable, nodeTable->item(i, NodeTable::NODE_Z), QString::number(node->position.z + 1));
            break;
        }
    }
}

void ToolsTreeviewTab::update() {
    recreateTreesTable();
    recreateNodesTable();
    nodeActivated();//also activates tree
    emit updateAnnotationLabelsSignal();
}

void ToolsTreeviewTab::insertTree(TreeListElement *tree, TreeTable *table) {
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

void ToolsTreeviewTab::insertNode(NodeListElement *node, NodeTable *table) {
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
            if(std::find(std::begin(state->skeletonState->selectedTrees), std::end(state->skeletonState->selectedTrees), node->correspondingTree)
                    == std::end(state->skeletonState->selectedTrees)) {// node not in one of the selected trees
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
