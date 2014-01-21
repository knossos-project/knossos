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
#include "toolstreeviewtab.h"

#define TREE_ID      0
#define TREE_R       1
#define TREE_G       2
#define TREE_B       3
#define TREE_A       4
#define TREE_COMMENT 5
#define TREE_COLS    6
#define NODE_ID      0
#define NODE_COMMENT 1
#define NODE_X       2
#define NODE_Y       3
#define NODE_Z       4
#define NODE_RADIUS  5
#define NODE_COLS    6

#define NODECOMBO_100 0
#define NODECOMBO_1000 1
#define NODECOMBO_5000 2
#define NODECOMBO_ALL 3
#define DISPLAY_ALL 0

extern stateInfo *state;
stateInfo *kState;

TreeTable::TreeTable(QWidget *parent) : QTableWidget(parent), droppedOnTreeID(0), changeByCode(true) {}

void TreeTable::setItem(int row, int column, QTableWidgetItem *item) {
    if(item != NULL) {
        changeByCode = true;
        QTableWidget::setItem(row, column, item);
        changeByCode = false;
    }
}

void TreeTable::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Delete) {
        deleteTreesAction();
    }
}

void TreeTable::dropEvent(QDropEvent *event) {
    QTableWidgetItem *droppedOnItem = itemAt(event->pos());
    droppedOnTreeID = item(droppedOnItem->row(), 0)->text().toInt();
    qDebug("dropped on tree %i", droppedOnTreeID);
    if(droppedOnItem == NULL or kState->skeletonState->selectedNodes.size() == 0) {
        return;
    }

    QMessageBox prompt;
    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
    prompt.setIcon(QMessageBox::Question);
    prompt.setWindowTitle("Cofirmation required");
    prompt.setText(QString("Do you really want to add selected node(s) to tree %1?").arg(droppedOnTreeID));
    QPushButton *confirmButton = prompt.addButton("Yes", QMessageBox::ActionRole);
    prompt.addButton("Cancel", QMessageBox::ActionRole);
    prompt.exec();
    if(prompt.clickedButton() == confirmButton) {
        std::vector<nodeListElement *>::iterator iter;
        for(iter = kState->skeletonState->selectedNodes.begin();
            iter != kState->skeletonState->selectedNodes.end(); ++iter) {
            Skeletonizer::moveNodeToTree((*iter), droppedOnTreeID);
        }
    }
    kState->skeletonState->selectedNodes.clear();
}

void TreeTable::deleteTreesAction() {
    if(kState->skeletonState->selectedTrees.size() == 0) {
        return;
    }
    QMessageBox prompt;
    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
    prompt.setIcon(QMessageBox::Question);
    prompt.setWindowTitle("Cofirmation required");
    prompt.setText("Do you really want to delete selected tree(s)?");
    QPushButton *confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
    prompt.addButton("Cancel", QMessageBox::ActionRole);
    prompt.exec();
    if(prompt.clickedButton() == confirmButton) {
        emit deleteSelectedTreesSignal();
        emit treesDeletedSignal(selectionModel()->selectedIndexes());
    }
    kState->skeletonState->selectedTrees.clear();
}
// node table
NodeTable::NodeTable(QWidget *parent) : QTableWidget(parent), changeByCode(true) {}

void NodeTable::setItem(int row, int column, QTableWidgetItem *item) {
    changeByCode = true;
    QTableWidget::setItem(row, column, item);
    changeByCode = false;
}

void NodeTable::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Delete) {
        deleteNodesAction();
    }
}

void NodeTable::deleteNodesAction() {
    if(kState->skeletonState->selectedNodes.size() == 0) {
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
        std::vector<nodeListElement *>::iterator iter;
        for(iter = kState->skeletonState->selectedNodes.begin();
            iter != kState->skeletonState->selectedNodes.end(); ++iter) {
            if((*iter) == kState->skeletonState->activeNode) {
                emit delActiveNodeSignal();
            }
            else {
                Skeletonizer::delNode(CHANGE_MANUAL, 0, *iter, true);
            }
        }
        emit nodesDeletedSignal(selectionModel()->selectedIndexes());
    }
    kState->skeletonState->selectedNodes.clear();
}

// treeview
ToolsTreeviewTab::ToolsTreeviewTab(QWidget *parent) :
    QWidget(parent), draggedNodeID(0), radiusBuffer(state->skeletonState->defaultNodeRadius), displayedNodes(1000)
{
    kState = state;

    treeSearchField = new QLineEdit();
    treeSearchField->setPlaceholderText("search tree");
    nodeSearchField = new QLineEdit();
    nodeSearchField->setPlaceholderText("search node");
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
    QTableWidgetItem *IDCol, *commentCol, *rCol, *gCol, *bCol, *aCol,
                     *radiusCol, *xCol, *yCol, *zCol;
    // active tree table
    activeTreeTable = new TreeTable(this);
    activeTreeTable->setColumnCount(TREE_COLS);
    activeTreeTable->setRowCount(1);
    activeTreeTable->setFixedHeight(54);
    activeTreeTable->verticalHeader()->setVisible(false);
    activeTreeTable->setToolTip("The active tree");
    IDCol = new QTableWidgetItem("Tree ID");
    rCol = new QTableWidgetItem("red");
    gCol = new QTableWidgetItem("green");
    bCol = new QTableWidgetItem("blue");
    aCol = new QTableWidgetItem("alpha");
    commentCol = new QTableWidgetItem("Comment");
    activeTreeTable->setHorizontalHeaderItem(TREE_ID, IDCol);
    activeTreeTable->setHorizontalHeaderItem(TREE_R, rCol);
    activeTreeTable->setHorizontalHeaderItem(TREE_G, gCol);
    activeTreeTable->setHorizontalHeaderItem(TREE_B, bCol);
    activeTreeTable->setHorizontalHeaderItem(TREE_A, aCol);
    activeTreeTable->setHorizontalHeaderItem(TREE_COMMENT, commentCol);
    activeTreeTable->horizontalHeader()->setStretchLastSection(true);
    activeTreeTable->resizeColumnsToContents();
    activeTreeTable->setContextMenuPolicy(Qt::CustomContextMenu);
    activeTreeTable->setDragDropMode(QAbstractItemView::DropOnly);
    QTableWidgetItem *item;
    item = new QTableWidgetItem();
    activeTreeTable->setItem(0, TREE_ID, item);
    item = new QTableWidgetItem();
    activeTreeTable->setItem(0, TREE_R, item);
    item = new QTableWidgetItem();
    activeTreeTable->setItem(0, TREE_G, item);
    item = new QTableWidgetItem();
    activeTreeTable->setItem(0, TREE_B, item);
    item = new QTableWidgetItem();
    activeTreeTable->setItem(0, TREE_A, item);
    item = new QTableWidgetItem();
    activeTreeTable->setItem(0, TREE_COMMENT, item);

    // tree table
    treeTable = new TreeTable(this);
    treeTable->setColumnCount(TREE_COLS);
    treeTable->setAlternatingRowColors(true);
    treeTable->verticalHeader()->setVisible(false);
    IDCol = new QTableWidgetItem("Tree ID");
    rCol = new QTableWidgetItem("red");
    gCol = new QTableWidgetItem("green");
    bCol = new QTableWidgetItem("blue");
    aCol = new QTableWidgetItem("alpha");
    commentCol = new QTableWidgetItem("Comment");
    treeTable->setHorizontalHeaderItem(TREE_ID, IDCol);
    treeTable->setHorizontalHeaderItem(TREE_R, rCol);
    treeTable->setHorizontalHeaderItem(TREE_G, gCol);
    treeTable->setHorizontalHeaderItem(TREE_B, bCol);
    treeTable->setHorizontalHeaderItem(TREE_A, aCol);
    treeTable->setHorizontalHeaderItem(TREE_COMMENT, commentCol);
    treeTable->horizontalHeader()->setStretchLastSection(true);
    treeTable->resizeColumnsToContents();
    treeTable->setContextMenuPolicy(Qt::CustomContextMenu);
    treeTable->setDragDropMode(QAbstractItemView::DropOnly);


    // active node table
    activeNodeTable = new NodeTable(this);
    activeNodeTable->setColumnCount(NODE_COLS);
    activeNodeTable->setRowCount(1);
    activeNodeTable->setFixedHeight(54);
    activeNodeTable->verticalHeader()->setVisible(false);
    activeNodeTable->setToolTip("The active node");
    IDCol = new QTableWidgetItem("Node ID");
    commentCol = new QTableWidgetItem("Comment");
    xCol = new QTableWidgetItem("x");
    yCol = new QTableWidgetItem("y");
    zCol = new QTableWidgetItem("z");
    radiusCol = new QTableWidgetItem("Radius");
    activeNodeTable->setHorizontalHeaderItem(NODE_ID, IDCol);
    activeNodeTable->setHorizontalHeaderItem(NODE_COMMENT, commentCol);
    activeNodeTable->setHorizontalHeaderItem(NODE_X, xCol);
    activeNodeTable->setHorizontalHeaderItem(NODE_Y, yCol);
    activeNodeTable->setHorizontalHeaderItem(NODE_Z, zCol);
    activeNodeTable->setHorizontalHeaderItem(NODE_RADIUS, radiusCol);
    activeNodeTable->horizontalHeader()->setSectionResizeMode(NODE_COMMENT, QHeaderView::Stretch);
    activeNodeTable->resizeColumnsToContents();
    activeNodeTable->setContextMenuPolicy(Qt::CustomContextMenu);
    activeNodeTable->setDragEnabled(true);
    activeNodeTable->setDragDropMode(QAbstractItemView::DragOnly);
    item = new QTableWidgetItem();
    activeNodeTable->setItem(0, NODE_ID, item);
    item = new QTableWidgetItem();
    activeNodeTable->setItem(0, NODE_COMMENT, item);
    item = new QTableWidgetItem();
    activeNodeTable->setItem(0, NODE_X, item);
    item = new QTableWidgetItem();
    activeNodeTable->setItem(0, NODE_Y, item);
    item = new QTableWidgetItem();
    activeNodeTable->setItem(0, NODE_Z, item);
    item = new QTableWidgetItem();
    activeNodeTable->setItem(0, NODE_RADIUS, item);


    // node table
    nodeTable = new NodeTable(this);
    nodeTable->setColumnCount(NODE_COLS);
    nodeTable->setAlternatingRowColors(true);
    nodeTable->verticalHeader()->setVisible(false);
    IDCol = new QTableWidgetItem("Node ID");
    commentCol = new QTableWidgetItem("Comment");
    xCol = new QTableWidgetItem("x");
    yCol = new QTableWidgetItem("y");
    zCol = new QTableWidgetItem("z");
    radiusCol = new QTableWidgetItem("Radius");
    nodeTable->setHorizontalHeaderItem(NODE_ID, IDCol);
    nodeTable->setHorizontalHeaderItem(NODE_COMMENT, commentCol);
    nodeTable->setHorizontalHeaderItem(NODE_X, xCol);
    nodeTable->setHorizontalHeaderItem(NODE_Y, yCol);
    nodeTable->setHorizontalHeaderItem(NODE_Z, zCol);
    nodeTable->setHorizontalHeaderItem(NODE_RADIUS, radiusCol);
    nodeTable->horizontalHeader()->setSectionResizeMode(NODE_COMMENT, QHeaderView::Stretch);
    nodeTable->resizeColumnsToContents();
    nodeTable->setContextMenuPolicy(Qt::CustomContextMenu);
    nodeTable->setDragEnabled(true);
    nodeTable->setDragDropMode(QAbstractItemView::DragOnly);
    createTreesContextMenu();
    createNodesContextMenu();
    createContextMenuDialogs();

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
    hLayout = new QHBoxLayout();
    hLayout->addWidget(nodesOfSelectedTreesRadio);
    hLayout->addWidget(allNodesRadio);
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
    connect(treeSearchField, SIGNAL(editingFinished()), this, SLOT(treeSearchChanged()));
    connect(nodeSearchField, SIGNAL(editingFinished()), this, SLOT(nodeSearchChanged()));
    // display events
    connect(nodesOfSelectedTreesRadio, SIGNAL(clicked()), this, SLOT(updateNodesTable()));
    connect(allNodesRadio, SIGNAL(clicked()), this, SLOT(updateNodesTable()));
    connect(branchNodesChckBx, SIGNAL(clicked()), this, SLOT(updateNodesTable()));
    connect(commentNodesChckBx, SIGNAL(clicked()), this,SLOT(updateNodesTable()));
    // displayed nodes
    connect(displayedNodesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(displayedNodesChanged(int)));
    // table click events

    connect(activeTreeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(actTreeItemChanged(QTableWidgetItem*)));
    connect(activeTreeTable, SIGNAL(itemSelectionChanged()), this, SLOT(activeTreeSelected()));

    connect(treeTable, SIGNAL(updateTreeview()), this, SLOT(update()));
    connect(treeTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(treeContextMenuCalled(QPoint)));
    connect(treeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(treeItemChanged(QTableWidgetItem*)));
    connect(treeTable, SIGNAL(itemSelectionChanged()), this, SLOT(treeItemSelected()));
    connect(treeTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(treeItemDoubleClicked(QTableWidgetItem*)));
    connect(treeTable, SIGNAL(treesDeletedSignal(QModelIndexList)), this, SLOT(treesDeleted(QModelIndexList)));

    connect(activeNodeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(actNodeItemChanged(QTableWidgetItem*)));

    connect(nodeTable, SIGNAL(updateNodesTable()), this, SLOT(updateNodesTable()));
    connect(nodeTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(nodeContextMenuCalled(QPoint)));
    connect(nodeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(nodeItemChanged(QTableWidgetItem*)));
    connect(nodeTable, SIGNAL(itemSelectionChanged()), this, SLOT(nodeItemSelected()));
    connect(nodeTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(nodeItemDoubleClicked(QTableWidgetItem*)));
    connect(nodeTable, SIGNAL(nodesDeletedSignal(QModelIndexList)), this, SLOT(nodesDeleted(QModelIndexList)));
}

void ToolsTreeviewTab::createTreesContextMenu() {
    treeContextMenu = new QMenu();
    treeContextMenu->addAction("Set as active tree", this, SLOT(setActiveTreeAction()));
    treeContextMenu->addAction("Set comment for tree(s)", this, SLOT(setTreeCommentAction()));
    treeContextMenu->addAction("Merge trees", this, SLOT(mergeTreesAction()));
    treeContextMenu->addAction("Move selected node(s) to this tree", this, SLOT(moveNodesAction()));
    treeContextMenu->addAction("Restore default color", this, SLOT(restoreColorAction()));
    treeContextMenu->addAction(QIcon(":/images/icons/user-trash.png"), "Delete tree(s)", treeTable, SLOT(deleteTreesAction()));
}

void ToolsTreeviewTab::createNodesContextMenu() {
    nodeContextMenu = new QMenu();
    nodeContextMenu->addAction("Set comment for node(s)...", this, SLOT(setNodeCommentAction()));
    nodeContextMenu->addAction("Set radius for node(s)...", this, SLOT(setNodeRadiusAction()));
    nodeContextMenu->addAction("Link nodes... ", this, SLOT(linkNodesAction()));
    nodeContextMenu->addAction("Split component from tree", this, SLOT(splitComponentAction()));
    nodeContextMenu->addAction(QIcon(":/images/icons/user-trash.png"), "delete node(s)", nodeTable, SLOT(deleteNodesAction()));

}

void ToolsTreeviewTab::createContextMenuDialogs() {
    // for tree table
    treeCommentEditDialog = new QDialog();
    treeCommentField = new QLineEdit();
    treeCommentField->setPlaceholderText("Enter comment here");
    treeApplyButton = new QPushButton("Apply");
    treeCancelButton = new QPushButton("Cancel");
    treeCommentLayout = new QVBoxLayout();
    treeCommentLayout->addWidget(treeCommentField);
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(treeApplyButton);
    hLayout->addWidget(treeCancelButton);
    treeCommentLayout->addLayout(hLayout);
    treeCommentEditDialog->setLayout(treeCommentLayout);

    connect(treeCommentField, SIGNAL(textChanged(QString)), this, SLOT(updateTreeCommentBuffer(QString)));
    connect(treeApplyButton, SIGNAL(clicked()), this, SLOT(editTreeComments()));
    connect(treeCancelButton, SIGNAL(clicked()), treeCommentEditDialog, SLOT(hide()));

    // build node action dialogs
    nodeCommentEditDialog = new QDialog();
    nodeCommentEditDialog->setWindowTitle("Edit node comments");
    nodeCommentField = new QLineEdit();
    nodeCommentField->setPlaceholderText("Enter comment here");
    nodeCommentApplyButton = new QPushButton("Apply");
    nodeCommentCancelButton = new QPushButton("Cancel");
    nodeCommentLayout = new QVBoxLayout();
    nodeCommentLayout->addWidget(nodeCommentField);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(nodeCommentApplyButton);
    hLayout->addWidget(nodeCommentCancelButton);
    nodeCommentLayout->addLayout(hLayout);
    nodeCommentEditDialog->setLayout(nodeCommentLayout);

    nodeRadiusEditDialog = new QDialog();
    nodeRadiusEditDialog->setWindowTitle("Edit node radii");
    radiusLabel = new QLabel("new radius:");
    radiusSpin = new QDoubleSpinBox();
    radiusSpin->setMaximum(100000);
    nodeRadiusApplyButton = new QPushButton("Apply");
    nodeRadiusCancelButton = new QPushButton("Cancel");
    nodeRadiusLayout = new QVBoxLayout();
    hLayout = new QHBoxLayout();
    hLayout->addWidget(radiusLabel);
    hLayout->addWidget(radiusSpin);
    nodeRadiusLayout->addLayout(hLayout);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(nodeRadiusApplyButton);
    hLayout->addWidget(nodeRadiusCancelButton);
    nodeRadiusLayout->addLayout(hLayout);
    nodeRadiusEditDialog->setLayout(nodeRadiusLayout);

    moveNodesDialog = new QDialog();
    moveNodesDialog->setWindowTitle("Move nodes");
    newTreeLabel = new QLabel("New Tree:");
    newTreeIDSpin = new QSpinBox();
    //newTreeIDSpin->setMaximum(state->skeletonState->greatestTreeID);
    moveNodesButton = new QPushButton("Move Nodes");
    moveNodesCancelButton = new QPushButton("Cancel");
    QVBoxLayout *moveNodesLayout = new QVBoxLayout();
    hLayout = new QHBoxLayout();
    hLayout->addWidget(newTreeLabel);
    hLayout->addWidget(newTreeIDSpin);
    moveNodesLayout->addLayout(hLayout);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(moveNodesButton);
    hLayout->addWidget(moveNodesCancelButton);
    moveNodesLayout->addLayout(hLayout);
    moveNodesDialog->setLayout(moveNodesLayout);

    connect(nodeCommentField, SIGNAL(textChanged(QString)), this, SLOT(updateNodeCommentBuffer(QString)));
    connect(nodeCommentApplyButton, SIGNAL(clicked()), this, SLOT(editNodeComments()));
    connect(nodeCommentCancelButton, SIGNAL(clicked()), nodeCommentEditDialog, SLOT(hide()));
    connect(radiusSpin, SIGNAL(valueChanged(double)), this, SLOT(updateNodeRadiusBuffer(double)));
    connect(nodeRadiusApplyButton, SIGNAL(clicked()), this, SLOT(editNodeRadii()));
    connect(nodeRadiusCancelButton, SIGNAL(clicked()), nodeRadiusEditDialog, SLOT(hide()));
    connect(moveNodesButton, SIGNAL(clicked()), this, SLOT(moveNodesClicked()));
    connect(moveNodesCancelButton, SIGNAL(clicked()), moveNodesDialog, SLOT(hide()));
}


// tree context menu
void ToolsTreeviewTab::treeContextMenuCalled(QPoint pos) {
    treeContextMenu->move(treeTable->mapToGlobal(pos));
    treeContextMenu->show();
}

void ToolsTreeviewTab::setActiveTreeAction() {
    if(state->skeletonState->selectedTrees.size() != 1) {
        QMessageBox prompt;
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Information);
        prompt.setWindowTitle("Information");
        prompt.setText("Choose exactly one tree for activating.");
        prompt.exec();
    }
    else {
        Skeletonizer::setActiveTreeByID(state->skeletonState->selectedTrees.at(0)->treeID);
        treeActivated();
    }
    state->skeletonState->selectedTrees.clear();
}

void ToolsTreeviewTab::mergeTreesAction() {
    QMessageBox prompt;
    if(state->skeletonState->selectedTrees.size() != 2) {
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Information);
        prompt.setWindowTitle("Information");
        prompt.setText("Choose exactly two trees for merging.");
        prompt.exec();
    }
    else {
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Question);
        prompt.setWindowTitle("Cofirmation required");
        prompt.setText("Do you really want to merge selected trees?");
        QPushButton *confirmButton = prompt.addButton("Merge", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            int treeID1 = state->skeletonState->selectedTrees[0]->treeID;
            int treeID2 = state->skeletonState->selectedTrees[1]->treeID;
            Skeletonizer::mergeTrees(CHANGE_MANUAL, treeID1, treeID2, true);
            treesMerged(treeID1, treeID2);
        }
    }
    state->skeletonState->selectedTrees.clear();
}

void ToolsTreeviewTab::restoreColorAction() {
    if(state->skeletonState->selectedTrees.size() == 0) {
        return;
    }
    std::vector<treeListElement *>::iterator iter;
    for(iter = state->skeletonState->selectedTrees.begin();
        iter != state->skeletonState->selectedTrees.end(); ++iter) {
        Skeletonizer::restoreDefaultTreeColor(*iter);
    }
    updateTreesTable();
    state->skeletonState->selectedTrees.clear();
}

void ToolsTreeviewTab::setTreeCommentAction() {
    if(state->skeletonState->selectedTrees.size() == 0) {
        return;
    }
    treeCommentField->setText(treeCommentBuffer);
    treeCommentEditDialog->show();
}

void ToolsTreeviewTab::updateTreeCommentBuffer(QString comment) {
    treeCommentBuffer = comment;
}

void ToolsTreeviewTab::editTreeComments() {
    if(state->skeletonState->selectedTrees.size() == 0) {
        return;
    }
    std::vector<treeListElement *>::iterator iter;
    for(iter = state->skeletonState->selectedTrees.begin();
        iter != state->skeletonState->selectedTrees.end(); ++iter) {
        Skeletonizer::addTreeComment(CHANGE_MANUAL, (*iter)->treeID, treeCommentBuffer.toLocal8Bit().data());
        if((*iter) == state->skeletonState->activeTree) {
            setText(activeTreeTable, activeTreeTable->item(0, TREE_COMMENT), treeCommentBuffer);
        }
    }
    treeCommentEditDialog->hide();
    updateTreesTable();
}

// node context menu
void ToolsTreeviewTab::nodeContextMenuCalled(QPoint pos) {
    nodeContextMenu->move(nodeTable->mapToGlobal(pos));
    nodeContextMenu->show();
}

void ToolsTreeviewTab::setNodeRadiusAction() {
    if(state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    radiusSpin->setValue(radiusBuffer);
    nodeRadiusEditDialog->show();
}

void ToolsTreeviewTab::linkNodesAction() {
    if(state->skeletonState->selectedNodes.size() != 2) {
        QMessageBox prompt;
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Information);
        prompt.setWindowTitle("Information");
        prompt.setText("Please select exactly two nodes to link with each other.");
        prompt.exec();
        return;
    }
    emit addSegmentSignal(CHANGE_MANUAL,
                          state->skeletonState->selectedNodes[0]->nodeID,
                          state->skeletonState->selectedNodes[1]->nodeID,
                          true);
}

void ToolsTreeviewTab::updateNodeRadiusBuffer(double value) {
    radiusBuffer = value;
}

void ToolsTreeviewTab::editNodeRadii() {
    if(state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    std::vector<nodeListElement *>::iterator iter;
    for(iter = state->skeletonState->selectedNodes.begin();
        iter != state->skeletonState->selectedNodes.end(); ++iter) {
        if((*iter) == state->skeletonState->activeNode) {
            setText(activeNodeTable, activeNodeTable->item(0, NODE_RADIUS), QString::number(radiusBuffer));
        }
        Skeletonizer::editNode(CHANGE_MANUAL, 0, *iter, radiusBuffer,
                               (*iter)->position.x, (*iter)->position.y, (*iter)->position.z, (*iter)->createdInMag);
    }
    nodeRadiusEditDialog->hide();
    updateNodesTable();
}

void ToolsTreeviewTab::moveNodesClicked() {
    int treeID = newTreeIDSpin->value();
    treeListElement *tree = Skeletonizer::findTreeByTreeID(treeID);
    if(tree == NULL) {
        QMessageBox prompt;
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Information);
        prompt.setWindowTitle("Information");
        prompt.setText(QString("Tree with ID %1 does not exist.").arg(treeID));
        prompt.exec();
    }
    else {
        std::vector<nodeListElement *>::iterator iter;
        for(iter = kState->skeletonState->selectedNodes.begin();
            iter != kState->skeletonState->selectedNodes.end(); ++iter) {
            Skeletonizer::moveNodeToTree((*iter), treeID);
        }
        moveNodesDialog->hide();
    }
    kState->skeletonState->selectedNodes.clear();
}

void ToolsTreeviewTab::splitComponentAction() {
    QMessageBox prompt;
    if(state->skeletonState->selectedNodes.size() != 1) {
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Information);
        prompt.setWindowTitle("Information");
        prompt.setText("Choose exactly one node whose component should be split from the tree.");
        prompt.exec();
    }
    else {
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Question);
        prompt.setWindowTitle("Cofirmation required");
        prompt.setText("Do you really want to split the connected component from the tree?");
        QPushButton *confirmButton = prompt.addButton("Split", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            treeListElement *prevActive = state->skeletonState->activeTree;
            Skeletonizer::splitConnectedComponent(CHANGE_MANUAL, state->skeletonState->selectedNodes[0]->nodeID, true);
            state->skeletonState->selectedNodes.clear();
            if(prevActive != state->skeletonState->activeTree) { // if the active tree changes, the splitting was successful
                treeComponentSplit();
            }
        }
    }
}

void ToolsTreeviewTab::moveNodesAction() {
    QMessageBox prompt;
    if(state->skeletonState->selectedNodes.size() == 0
            or state->skeletonState->selectedTrees.size() != 1) {
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Information);
        prompt.setWindowTitle("Information");
        prompt.setText("Choose at least one node to move to exactly one tree.");
        prompt.exec();
    }
    else {
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Question);
        prompt.setWindowTitle("Confirmation Requested");
        prompt.setText(QString("Do you really want to move selected nodes to tree %1?").
                                arg(state->skeletonState->selectedTrees[0]->treeID));
        QPushButton *confirmButton = prompt.addButton("Move", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            for(uint i = 0; i < state->skeletonState->selectedNodes.size(); ++i) {
                Skeletonizer::moveNodeToTree(state->skeletonState->selectedNodes[i],
                                             state->skeletonState->selectedTrees[0]->treeID);
            }
        }
    }
    state->skeletonState->selectedNodes.clear();
    state->skeletonState->selectedTrees.clear();
}

void ToolsTreeviewTab::setNodeCommentAction() {
    if(state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    nodeCommentField->setText(nodeCommentBuffer);
    nodeCommentEditDialog->show();
}

void ToolsTreeviewTab::updateNodeCommentBuffer(QString comment) {
    nodeCommentBuffer = comment;
}

void ToolsTreeviewTab::editNodeComments() {
    if(state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    std::vector<nodeListElement *>::iterator iter;
    for(iter = state->skeletonState->selectedNodes.begin();
        iter != state->skeletonState->selectedNodes.end(); ++iter) {
        if((*iter) == state->skeletonState->activeNode) {
            setText(activeNodeTable, activeNodeTable->item(0, NODE_COMMENT), nodeCommentBuffer);
        }
        if(nodeCommentBuffer.length() > 0) {
            if((*iter)->comment == NULL) {
                Skeletonizer::addComment(CHANGE_MANUAL, nodeCommentBuffer.toLocal8Bit().data(), *iter, 0, true);
            }
            else {
                Skeletonizer::editComment(CHANGE_MANUAL,
                                          (*iter)->comment, 0, nodeCommentBuffer.toLocal8Bit().data(), *iter, 0, true);
            }
        }
        else {
            if((*iter)-> comment != NULL) {
                Skeletonizer::delComment(CHANGE_MANUAL, (*iter)->comment, (*iter)->nodeID, true);
            }
        }
    }
    nodeCommentEditDialog->hide();
    updateNodesTable();
}


void ToolsTreeviewTab::treeSearchChanged() {
    if(treeRegExCheck->isChecked() and treeSearchField->text().length() > 0) {
        QRegularExpression regex(treeSearchField->text());
        if(regex.isValid() == false) {
            QToolTip::showText(treeSearchField->mapToGlobal(treeSearchField->pos()), "Invalid regular expression.");
            return;
        }
    }
    updateTreesTable();
}
void ToolsTreeviewTab::nodeSearchChanged() {
    if(nodeRegExCheck->isChecked() and nodeSearchField->text().length() > 0) {
        QRegularExpression regex(nodeSearchField->text());
        if(regex.isValid() == false) {
            QToolTip::showText(nodeSearchField->mapToGlobal(nodeSearchField->pos()), "Invalid regular expression.");
            return;
        }
    }
    updateNodesTable();
}

void ToolsTreeviewTab::displayedNodesChanged(int index) {
    switch(index) {
    case NODECOMBO_100:
    case NODECOMBO_1000:
    case NODECOMBO_5000:
        displayedNodes = displayedNodesCombo->currentText().toInt();
        break;
    case NODECOMBO_ALL:
        displayedNodes = 0;
        break;
    default:
        break;
    }

    if(displayedNodes == nodeTable->rowCount()) {
        return;
    }
    else if(displayedNodes != DISPLAY_ALL and displayedNodes < nodeTable->rowCount()) {
        nodeTable->setRowCount(displayedNodes);
    }
    else {
        updateNodesTable();
    }
}

void ToolsTreeviewTab::actTreeItemChanged(QTableWidgetItem *item) {
    if(activeTreeTable->changeByCode) {
        return;
    }
    treeListElement *activeTree = state->skeletonState->activeTree;
    if(activeTree == NULL) {
        return;
    }
    int activeRow = -1;
    for(int i = 0; i < treeTable->rowCount(); ++i) {
        if(treeTable->item(i, TREE_ID)->text().toInt() == activeTree->treeID) {
            activeRow = i;
            break;
        }
    }

    float value;
    switch(item->column()) {
    case TREE_R:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            activeTree->color.r = value;
            if(activeRow != -1) {
                setText(treeTable, treeTable->item(activeRow, TREE_R), QString::number(activeTree->color.r));
                updateTreeColorCells(treeTable, activeRow);
            }
            updateTreeColorCells(activeTreeTable, item->row());
        }
        else {
            setText(activeTreeTable, item, QString::number(activeTree->color.r));
        }
        break;
    case TREE_G:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            activeTree->color.g = value;
            if(activeRow != -1) {
                setText(treeTable, treeTable->item(activeRow, TREE_G), QString::number(activeTree->color.g));
                updateTreeColorCells(treeTable, activeRow);
            }
            updateTreeColorCells(activeTreeTable, item->row());
        }
        else {
            setText(activeTreeTable, item, QString::number(activeTree->color.g));
        }
        break;
    case TREE_B:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            activeTree->color.b = value;
            if(activeRow != -1) {
                setText(treeTable, treeTable->item(activeRow, TREE_B), QString::number(activeTree->color.b));
                updateTreeColorCells(treeTable, activeRow);
            }
            updateTreeColorCells(activeTreeTable, item->row());
        }
        else {
            setText(activeTreeTable, item, QString::number(activeTree->color.b));
        }
        break;
    case TREE_A:
        value = item->text().toFloat();
        activeTree->color.a = value;
        if(value >= 0 and value <= 1.) {
            if(activeRow != -1) {
                setText(treeTable, treeTable->item(activeRow, TREE_A), QString::number(activeTree->color.a));
                updateTreeColorCells(treeTable, activeRow);
            }
            updateTreeColorCells(activeTreeTable, item->row());
        }
        else {
            setText(activeTreeTable, item, QString::number(activeTree->color.a));
        }
        break;
    case TREE_COMMENT: {
            QString prevComment(activeTree->comment);
            if(activeRow != -1) {
                setText(treeTable, treeTable->item(activeRow, TREE_COMMENT), item->text());
            }
            Skeletonizer::addTreeComment(CHANGE_MANUAL, activeTree->treeID, item->text().toLocal8Bit().data());
            // check if tree will be displayed in list
            if(treeSearchField->text().isEmpty() == false) {
                if(matchesSearchString(treeSearchField->text(),
                                       QString(activeTree->comment),
                                       nodeRegExCheck->isChecked()) == false) { // new comment does not match search string
                    for(int i = 0; i < treeTable->rowCount(); ++i) {
                        if(treeTable->item(i, TREE_ID)->text().toInt() == activeTree->treeID) {
                            treeTable->removeRow(i);
                            break;
                        }
                    }
                }
                else { // new comment matches search string
                    if(prevComment.compare(item->text(), Qt::CaseInsensitive) != 0) {
                        //different from previous comment. So we know, the tree is not in table, at the moment
                        treeTable->insertRow(0);
                        insertTree(activeTree, treeTable);
                    }
                }
            }
            else { // update active tree's comment in tree table
                for(int i = 0; i < treeTable->rowCount(); ++i) {
                    if(treeTable->item(i, TREE_ID)->text().toInt() == activeTree->treeID) {
                        setText(treeTable, treeTable->item(i, TREE_COMMENT), activeTree->comment);
                        break;
                    }
                }
            }
            break;
        }
    default:
        break;
    }
}

void ToolsTreeviewTab::treeItemChanged(QTableWidgetItem* item) {
//    if(treeTable->changeByCode) {
//        return;
//    }
    QTableWidgetItem *idItem = treeTable->item(item->row(), TREE_ID);
    if(idItem == NULL) {
        return;
    }
    treeListElement *selectedTree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
    if(selectedTree == NULL) {
        return;
    }
    float value;
    switch(item->column()) {
    case TREE_R:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            if(selectedTree == state->skeletonState->activeTree) {
                if(activeTreeTable->item(0, TREE_R)) {
                    setText(activeTreeTable, activeTreeTable->item(0, TREE_R), item->text());
                    updateTreeColorCells(activeTreeTable, 0);
                }
            }
            selectedTree->color.r = value;
            updateTreeColorCells(treeTable, item->row());
        }
        else {
            setText(treeTable, item, QString::number(selectedTree->color.r));
        }
        break;
    case TREE_G:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            if(selectedTree == state->skeletonState->activeTree) {
                if(activeTreeTable->item(0, TREE_G)) {
                    setText(activeTreeTable, activeTreeTable->item(0, TREE_G), item->text());
                    updateTreeColorCells(activeTreeTable, 0);
                }
            }
            selectedTree->color.g = value;
            updateTreeColorCells(treeTable, item->row());
        }
        else {
            setText(treeTable, item, QString::number(selectedTree->color.g));
        }
        break;
    case TREE_B:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            if(selectedTree == state->skeletonState->activeTree) {
                if(activeTreeTable->item(0, TREE_B)) {
                    setText(activeTreeTable, activeTreeTable->item(0, TREE_B), item->text());
                    updateTreeColorCells(activeTreeTable, 0);
                }
            }
            selectedTree->color.b = value;
            updateTreeColorCells(treeTable, item->row());
        }
        else {
            setText(treeTable, item, QString::number(selectedTree->color.b));
        }
        break;
    case TREE_A:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            if(selectedTree == state->skeletonState->activeTree) {
                if(activeTreeTable->item(0, TREE_A)) {
                    setText(activeTreeTable, activeTreeTable->item(0, TREE_A), item->text());
                    updateTreeColorCells(activeTreeTable, 0);
                }
            }
            selectedTree->color.a = value;
            updateTreeColorCells(treeTable, item->row());
        }
        else {
            setText(treeTable, item, QString::number(selectedTree->color.a));
        }
        break;
    case TREE_COMMENT: {
            Skeletonizer::addTreeComment(CHANGE_MANUAL, selectedTree->treeID, item->text().toLocal8Bit().data());
            // check if tree will still be displayed in list
            if(treeSearchField->text().isEmpty() == false) {
                if(matchesSearchString(treeSearchField->text(),
                                       QString(selectedTree->comment),
                                       nodeRegExCheck->isChecked()) == false) {
                    for(int i = 0; i < treeTable->rowCount(); ++i) {
                        if(treeTable->item(i, TREE_ID)->text().toInt() == selectedTree->treeID) {
                            treeTable->removeRow(i);
                            break;
                        }
                    }
                }
            }
            // update active tree table if necessary
            if(selectedTree == state->skeletonState->activeTree) {
                if(activeTreeTable->item(0, TREE_COMMENT)) {
                    setText(activeTreeTable, activeTreeTable->item(0, TREE_COMMENT), item->text());
                }
            }
            break;
        }
    default:
        break;
    }
    state->skeletonState->selectedTrees.clear();
    emit updateToolsSignal();
}

void ToolsTreeviewTab::actNodeItemChanged(QTableWidgetItem *item) {
    if(activeNodeTable->changeByCode) {
        return;
    }
    nodeListElement *activeNode = state->skeletonState->activeNode;
    if(activeNode == NULL) {
        return;
    }
    int activeRow = -1;
    for(int i = 0; i < nodeTable->rowCount(); ++i) {
        if((uint)nodeTable->item(i, NODE_ID)->text().toInt() == activeNode->nodeID) {
            activeRow = i;
            break;
        }
    }

    switch(item->column()) {
    case NODE_COMMENT:
        if(activeNode->comment) {
            if(activeRow != -1) {
                setText(nodeTable, nodeTable->item(activeRow, NODE_COMMENT), item->text());
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
                setText(nodeTable, nodeTable->item(activeRow, NODE_COMMENT), item->text());
            }
        }
        break;
    case NODE_X:
        if(item->text().toInt() < 1) { // out of bounds
            setText(activeNodeTable, item, QString::number(activeNode->position.x + 1));
        } else {
            activeNode->position.x = item->text().toInt() - 1;
        }
        if(activeRow != -1) {
            setText(nodeTable, nodeTable->item(activeRow, NODE_X), item->text());
        }
        break;
    case NODE_Y:
        if(item->text().toInt() < 1) { // out of bounds
            setText(activeNodeTable, item, QString::number(activeNode->position.y + 1));
        } else {
            activeNode->position.y = item->text().toInt() - 1;
        }
        if(activeRow != -1) {
            setText(nodeTable, nodeTable->item(activeRow, NODE_Y), item->text());
        }
        break;
    case NODE_Z:
        if(item->text().toInt() < 1) { // out of bounds
            setText(activeNodeTable, item, QString::number(activeNode->position.z + 1));
        } else {
            activeNode->position.z = item->text().toInt() - 1;
        }
        if(activeRow != -1) {
            setText(nodeTable, nodeTable->item(activeRow, NODE_Z), item->text());
        }
        break;
    case NODE_RADIUS:
        activeNode->radius = item->text().toFloat();
        if(activeRow != -1) {
            setText(nodeTable, nodeTable->item(activeRow, NODE_RADIUS), item->text());
        }
        break;
    default:
        break;
    }
    state->skeletonState->selectedNodes.clear();
    emit updateToolsSignal();
}

void ToolsTreeviewTab::nodeItemChanged(QTableWidgetItem* item) {
    if(nodeTable->changeByCode) {
        return;
    }
    QTableWidgetItem *idItem = nodeTable->item(item->row(), NODE_ID);
    if(idItem == NULL) {
        return;
    }
    nodeListElement *selectedNode = Skeletonizer::findNodeByNodeID(idItem->text().toInt());
    if(selectedNode == NULL) {
        return;
    }
    switch(item->column()) {
    case NODE_COMMENT:
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NODE_COMMENT)) {
                setText(activeNodeTable, activeNodeTable->item(0, NODE_COMMENT), item->text());
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
                if(activeNodeTable->item(0, NODE_COMMENT)) {
                    setText(activeNodeTable, activeNodeTable->item(0, NODE_COMMENT), item->text());
                }
            }
        }
        break;
    case NODE_X:
        if(item->text().toInt() < 1) { // out of bounds
            setText(nodeTable, item, QString::number(selectedNode->position.x + 1));
        } else {
            selectedNode->position.x = item->text().toInt() - 1;
        }
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NODE_X)) {
                setText(activeNodeTable, activeNodeTable->item(0, NODE_X), item->text());
            }
        }
        break;
    case NODE_Y:
        if(item->text().toInt() < 1) { // out of bounds
            setText(nodeTable, item, QString::number(selectedNode->position.y + 1));
        } else {
            selectedNode->position.y = item->text().toInt() - 1;
        }
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NODE_Y)) {
                setText(activeNodeTable, activeNodeTable->item(0, NODE_Y), item->text());
            }
        }
        break;
    case NODE_Z:
        if(item->text().toInt() < 1) { // out of bounds
            setText(nodeTable, item, QString::number(selectedNode->position.z + 1));
        } else {
            selectedNode->position.z = item->text().toInt() - 1;
        }
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NODE_Z)) {
                setText(activeNodeTable, activeNodeTable->item(0, NODE_Z), item->text());
            }
        }
        break;
    case NODE_RADIUS:
        selectedNode->radius = item->text().toFloat();
        if(selectedNode == state->skeletonState->activeNode) {
            if(activeNodeTable->item(0, NODE_RADIUS)) {
                setText(activeNodeTable, activeNodeTable->item(0, NODE_RADIUS), item->text());
            }
        }
        break;
    default:
        break;
    }
    state->skeletonState->selectedNodes.clear();
    emit updateToolsSignal();
}

void ToolsTreeviewTab::activeTreeSelected() {
    state->skeletonState->selectedTrees.clear();
    QModelIndexList selected = activeTreeTable->selectionModel()->selectedIndexes();
    if(selected.size() == 1) {
        state->skeletonState->selectedTrees.push_back(state->skeletonState->activeTree);
        if(nodesOfSelectedTreesRadio->isChecked()) {
            updateNodesTable();
        }
    }
}

void ToolsTreeviewTab::treeItemSelected() {
    state->skeletonState->selectedTrees.clear();
    QModelIndexList selected = treeTable->selectionModel()->selectedIndexes();
    treeListElement *tree;
    foreach(QModelIndex index, selected) {
        tree = Skeletonizer::findTreeByTreeID(index.data().toInt());
        if(tree) {
            state->skeletonState->selectedTrees.push_back(tree);
        }
    }
    if(nodesOfSelectedTreesRadio->isChecked()) {
        updateNodesTable();
    }
}

void ToolsTreeviewTab::treeItemDoubleClicked(QTableWidgetItem* item) {
    if(item->column() != 0) {
        return;
    }
    if(state->skeletonState->selectedTrees.size() != 1) {
        return;
    }
    Skeletonizer::setActiveTreeByID(state->skeletonState->selectedTrees.at(0)->treeID);
    treeActivated();
    state->skeletonState->selectedTrees.clear();
}

void ToolsTreeviewTab::nodeItemSelected() {
    state->skeletonState->selectedNodes.clear();
    QModelIndexList selected = nodeTable->selectionModel()->selectedIndexes();
    nodeListElement *node;
    foreach(QModelIndex index, selected) {
        node = Skeletonizer::findNodeByNodeID(index.data().toInt());
        if(node) {

            state->skeletonState->selectedNodes.push_back(node);
        }
    }
}
void ToolsTreeviewTab::nodeItemDoubleClicked(QTableWidgetItem*) {
    if(state->skeletonState->selectedNodes.size() != 1) {
        return;
    }
    emit setActiveNodeSignal(CHANGE_MANUAL, NULL, state->skeletonState->selectedNodes.at(0)->nodeID);
    emit JumpToActiveNodeSignal();
    nodeActivated();
    state->skeletonState->selectedNodes.clear();
}

void ToolsTreeviewTab::updateTreeColorCells(TreeTable *table, int row) {
    QTableWidgetItem *idItem = table->item(row, TREE_ID);
    treeListElement *tree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
    if(tree == NULL) {
        return;
    }
    QColor treeColor = QColor(tree->color.r*255, tree->color.g*255, tree->color.b*255, 153);
    QTableWidgetItem *colorItem = table->item(row, TREE_R);
    if(colorItem) {
        colorItem->setBackgroundColor(treeColor);
    }
    colorItem = table->item(row, TREE_G);
    if(colorItem) {
        colorItem->setBackgroundColor(treeColor);
    }
    colorItem = table->item(row, TREE_B);
    if(colorItem) {
        colorItem->setBackgroundColor(treeColor);
    }
    colorItem = table->item(row, TREE_A);
    if(colorItem) {
        colorItem->setBackgroundColor(treeColor);
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
    if(item != NULL) {
        table->changeByCode = true;
        item->setText(text);
        table->changeByCode = false;
    }
}

void ToolsTreeviewTab::setText(NodeTable *table, QTableWidgetItem *item, QString text) {
    if(item != NULL) {
        table->changeByCode = true;
        item->setText(text);
        table->changeByCode = false;
    }
}

void ToolsTreeviewTab::updateTreesTable() {
    treeTable->clearContents();
    treeTable->setRowCount(state->skeletonState->treeElements);

    treeListElement *currentTree = state->skeletonState->firstTree;
    if(currentTree == NULL) { 
        return;
    }

    QTableWidgetItem *item;
    uint treeIndex = 0;
    while(currentTree) {
        // filter comment for search string match
        if(treeSearchField->text().length() > 0) {
            if(strlen(currentTree->comment) == 0) {
                currentTree = currentTree->next;
                continue;
            }
            if(matchesSearchString(treeSearchField->text(), currentTree->comment, treeRegExCheck->isChecked()) == false) {
                currentTree = currentTree->next;
                continue;
            }
        }
        // tree id
        item = new QTableWidgetItem(QString::number(currentTree->treeID));
        Qt::ItemFlags flags = item->flags();
        flags |= Qt::ItemIsSelectable;
        flags &= ~Qt::ItemIsEditable;
        item->setFlags(flags);
        treeTable->setItem(treeIndex, TREE_ID, item);

        // tree color
        QColor treeColor = QColor(currentTree->color.r*255,
                                  currentTree->color.g*255,
                                  currentTree->color.b*255,
                                  0.6*255);
        item = new QTableWidgetItem(QString::number(currentTree->color.r, 'g', 2)); // precision of 2
        item->setBackgroundColor(treeColor);
        treeTable->setItem(treeIndex, TREE_R, item);

        item = new QTableWidgetItem(QString::number(currentTree->color.g, 'g', 2));
        item->setBackgroundColor(treeColor);
        treeTable->setItem(treeIndex, TREE_G, item);

        item = new QTableWidgetItem(QString::number(currentTree->color.b, 'g', 2));
        item->setBackgroundColor(treeColor);
        treeTable->setItem(treeIndex, TREE_B, item);

        item = new QTableWidgetItem(QString::number(currentTree->color.a, 'g', 2));
        item->setBackgroundColor(treeColor);
        treeTable->setItem(treeIndex, TREE_A, item);

        // tree comment
        item = new QTableWidgetItem("");
        if(currentTree->comment) {
            setText(treeTable, item, QString(currentTree->comment));
        }
        treeTable->setItem(treeIndex, TREE_COMMENT, item);

        treeIndex++;
        currentTree = currentTree->next;
    }

    // resize every column to size of content, to make them small,
    // omit comment column, because it might become large
    treeTable->resizeColumnToContents(TREE_R);
    treeTable->resizeColumnToContents(TREE_G);
    treeTable->resizeColumnToContents(TREE_B);
    treeTable->resizeColumnToContents(TREE_A);
    treeTable->setRowCount(treeIndex);
}

void ToolsTreeviewTab::updateNodesTable() {
    nodeTable->changeByCode = true;
    nodeTable->clearContents();

    if(displayedNodes > state->skeletonState->totalNodeElements or displayedNodes == DISPLAY_ALL) {
        nodeTable->setRowCount(state->skeletonState->totalNodeElements);
    }
    else {
        nodeTable->setRowCount(displayedNodes);
    }

    treeListElement *currentTree = state->skeletonState->firstTree;
    if(currentTree == NULL) {
        return;
    }
    QTableWidgetItem *item;
    uint nodeIndex = 0;
    nodeListElement *node;
    while(currentTree) {
        node = currentTree->firstNode;
        while(node and nodeIndex <= displayedNodes or (node and displayedNodes == DISPLAY_ALL)) {
            // filter for comment search string
            if(nodeSearchField->text().length() > 0) {
                if(node->comment == NULL) {
                    node = node->next;
                    continue;
                }
                if(matchesSearchString(nodeSearchField->text(),
                                       QString(node->comment->content),
                                       nodeRegExCheck->isChecked()) == false) {
                    node = node->next;
                    continue;
                }
            }
            // filter for nodes of selected trees
            if(nodesOfSelectedTreesRadio->isChecked() and state->skeletonState->selectedTrees.size() > 0) {
                if(std::find(state->skeletonState->selectedTrees.begin(),
                             state->skeletonState->selectedTrees.end(),
                             node->correspondingTree)
                   == state->skeletonState->selectedTrees.end()) {
                    // node not in one of the selected trees
                    node = node->next;
                    continue;
                }
            }
            // filter for branch nodes
            if(branchNodesChckBx->isChecked()) {
                if(node->isBranchNode == false) {
                    node = node->next;
                    continue;
                }
            }
            // filter for comment nodes
            if(commentNodesChckBx->isChecked()) {
                if(node->comment == NULL) {
                    node = node->next;
                    continue;
                }
            }
            item = new QTableWidgetItem(QString::number(node->nodeID));
            Qt::ItemFlags flags = item->flags();
            flags |= Qt::ItemIsSelectable;
            flags &= ~Qt::ItemIsEditable;
            item->setFlags(flags);
            nodeTable->setItem(nodeIndex, NODE_ID, item);

            item = new QTableWidgetItem("");
            if(node->comment) {
                setText(nodeTable, item, node->comment->content);
            }
            nodeTable->setItem(nodeIndex, NODE_COMMENT, item);

            item = new QTableWidgetItem(QString::number(node->position.x + 1));
            nodeTable->setItem(nodeIndex, NODE_X, item);

            item = new QTableWidgetItem(QString::number(node->position.y + 1));
            nodeTable->setItem(nodeIndex, NODE_Y, item);

            item = new QTableWidgetItem(QString::number(node->position.z + 1));
            nodeTable->setItem(nodeIndex, NODE_Z, item);

            item = new QTableWidgetItem(QString::number(node->radius));
            nodeTable->setItem(nodeIndex, NODE_RADIUS, item);

            nodeIndex++;
            node = node->next;
        }
        currentTree = currentTree->next;
    }

    // resize every column to size of content, to make them small,
    // omit comment column, because it might become large
    nodeTable->resizeColumnToContents(NODE_RADIUS);
    nodeTable->resizeColumnToContents(NODE_X);
    nodeTable->resizeColumnToContents(NODE_Y);
    nodeTable->resizeColumnToContents(NODE_Z);
    if(state->skeletonState->totalNodeElements > 0) {
        nodeTable->setRowCount(nodeIndex);
    }

    emit updateListedNodesSignal(nodeTable->rowCount());
    nodeTable->changeByCode = false;
}

void ToolsTreeviewTab::treeActivated() {
    activeTreeTable->clearContents();
    if(state->skeletonState->activeTree == NULL) {
        return;
    }
    insertTree(state->skeletonState->activeTree, activeTreeTable);
    activeTreeTable->setRowCount(1);
    emit updateToolsSignal();
}

void ToolsTreeviewTab::treeAdded(treeListElement *tree) {
    if(tree == NULL) {
        return;
    }
    if(tree->treeID == state->skeletonState->activeTree->treeID) {
        treeActivated();
    }
    insertTree(tree, treeTable);
    emit updateToolsSignal();
}

void ToolsTreeviewTab::treesDeleted(QModelIndexList selected) {
    qSort(selected.begin(), selected.end());
    for(int i = selected.size() - 1; i >= 0; --i) {
        treeTable->removeRow(selected[i].row());
    }

    // might have been the displayed active tree
    if(activeTreeTable->item(0, TREE_ID) != NULL) {
        if(state->skeletonState->activeTree == NULL) {
            activeTreeTable->clearContents();
        }
        else if(state->skeletonState->activeTree->treeID != activeTreeTable->item(0, TREE_ID)->text().toInt()) {
            treeActivated();
        }
    }
    if(activeNodeTable->item(0, NODE_ID) != NULL) { // displayed active node might have been in the deleted tree
        if(state->skeletonState->activeNode == NULL) {
            activeNodeTable->clearContents();
        }
        else if(state->skeletonState->activeNode->nodeID != (uint)activeNodeTable->item(0, NODE_ID)->text().toInt()) {
            nodeActivated();
        }
    }
    // remove all nodes of the tree
    int nodeRows = nodeTable->rowCount();
    QVector<int> rowsToDel;
    for(int i = 0; i < nodeRows; ++i) {
        if(Skeletonizer::findNodeByNodeID(nodeTable->item(i, NODE_ID)->text().toInt()) == NULL) {
            rowsToDel.push_back(i);
        }
    }
    qSort(rowsToDel.begin(), rowsToDel.end());
    for(int i = rowsToDel.size() - 1; i >= 0; --i) {
        nodeTable->removeRow(rowsToDel[i]);
    }

    emit updateToolsSignal(); // update the tree and node count
}

void ToolsTreeviewTab::treesMerged(int treeID1, int treeID2) {
    // remove second tree from tree table
    int rowCount = treeTable->rowCount();
    for(int i = 0; i < rowCount; ++i) {
        if(treeTable->item(i, TREE_ID)->text().toInt() == treeID2) {
            treeTable->removeRow(i);
            break;
        }
    }
    // if second tree was active before, update active tree table too
    QTableWidgetItem *item = activeTreeTable->item(0, TREE_ID);
    if(item) {
        if(state->skeletonState->activeTree == NULL) {
            activeTreeTable->clearContents();
        }
        else if(item->text().toInt() != state->skeletonState->activeTree->treeID) {
            treeActivated();
        }
    }
    updateToolsSignal();
}

void ToolsTreeviewTab::treeComponentSplit() {
    // when a tree is split, a new tree has been created and made active
    treeActivated();
    insertTree(state->skeletonState->activeTree, treeTable);
    updateToolsSignal();
}

// update node table
void ToolsTreeviewTab::nodeActivated() {
    activeNodeTable->clearContents();
    if(state->skeletonState->activeNode == NULL) {
        if(state->skeletonState->activeTree == NULL) {
            treeActivated();
        }
        return;
    }
    insertNode(state->skeletonState->activeNode, activeNodeTable);
    activeNodeTable->setRowCount(1);
    treeActivated(); // update active tree table in case of tree switch
    emit updateToolsSignal();
}

void ToolsTreeviewTab::nodeAdded() {
    if(state->skeletonState->activeNode == NULL) {
        return;
    }
    nodeActivated();
    insertNode(state->skeletonState->activeNode, nodeTable);
}

void ToolsTreeviewTab::nodesDeleted(QModelIndexList selected) {
    // nodes have to be deleted in reverse order or the row numbers will shift
    qSort(selected.begin(), selected.end());
    for(int i = selected.size() - 1; i >= 0; --i) {
        nodeTable->removeRow(selected[i].row());
    }

    // update active node table, if active node was deleted
    if(Skeletonizer::findNodeByNodeID(activeNodeTable->item(0, NODE_ID)->text().toInt()) == false) {
        nodeActivated(); // also activates tree
    }
}

void ToolsTreeviewTab::branchPushed() {
    if(branchNodesChckBx->isChecked()) {
        // the active node has become branch point, add it to node table
        insertNode(state->skeletonState->activeNode, nodeTable);
    }
    updateToolsSignal();
}

void ToolsTreeviewTab::branchPopped() {
    if(branchNodesChckBx->isChecked()) {
        // active node is no branch point anymore, remove from node table
        for(int i = 0; i < nodeTable->rowCount(); ++i) {
            if((uint)nodeTable->item(i, NODE_ID)->text().toInt() == state->skeletonState->activeNode->nodeID) {
                nodeTable->removeRow(i);
                break;
            }
        }
    }
    nodeActivated();
    updateToolsSignal();
}

void ToolsTreeviewTab::nodeCommentChanged(nodeListElement *node) {
    if(node == state->skeletonState->activeNode) {
        if(node->comment) {
            setText(activeNodeTable, activeNodeTable->item(0, NODE_COMMENT), node->comment->content);
        }
        else {
            setText(activeNodeTable, activeNodeTable->item(0, NODE_COMMENT), "");
        }
        for(int i = 0; i < nodeTable->rowCount(); ++i) {
            if(nodeTable->item(i, NODE_ID)->text().toInt() == node->nodeID) {
                if(node->comment) {
                    setText(nodeTable, nodeTable->item(i, NODE_COMMENT), QString(node->comment->content));
                }
                else {
                    setText(nodeTable, nodeTable->item(i, NODE_COMMENT), "");
                }
                break;
            }
        }
    }
}

void ToolsTreeviewTab::nodeRadiusChanged(nodeListElement *node) {
    if(node == state->skeletonState->activeNode) {
        setText(activeNodeTable, activeNodeTable->item(0, NODE_RADIUS), QString::number(state->skeletonState->activeNode->radius));
    }
    for(int i = 0; i < nodeTable->rowCount(); ++i) {
        if(nodeTable->item(i, NODE_ID)->text().toInt() == node->nodeID) {
            setText(nodeTable, nodeTable->item(i, NODE_RADIUS), QString::number(state->skeletonState->activeNode->radius));
            break;
        }
    }
}

void ToolsTreeviewTab::nodePositionChanged(nodeListElement *node) {
    if(node == state->skeletonState->activeNode) {
        setText(activeNodeTable, activeNodeTable->item(0, NODE_X), QString::number(node->position.x + 1));
        setText(activeNodeTable, activeNodeTable->item(0, NODE_Y), QString::number(node->position.y + 1));
        setText(activeNodeTable, activeNodeTable->item(0, NODE_Z), QString::number(node->position.z + 1));

        for(int i = 0; i < nodeTable->rowCount(); ++i) {
            if(nodeTable->item(i, NODE_ID)->text().toInt() == node->nodeID) {
                setText(nodeTable, nodeTable->item(i, NODE_X), QString::number(node->position.x + 1));
                setText(nodeTable, nodeTable->item(i, NODE_Y), QString::number(node->position.y + 1));
                setText(nodeTable, nodeTable->item(i, NODE_Z), QString::number(node->position.z + 1));
                break;
            }
        }
    }
}

void ToolsTreeviewTab::update() {
    updateTreesTable();
    updateNodesTable();
    nodeActivated();
    emit updateToolsSignal();
}

void ToolsTreeviewTab::insertTree(treeListElement *tree, TreeTable *table) {
    if(tree != NULL) {
        if(table == treeTable) {
            if(treeSearchField->text().length() > 0) {
                if(strlen(tree->comment) == 0) {
                    return;
                }
                if(matchesSearchString(treeSearchField->text(), tree->comment, treeRegExCheck->isChecked()) == false) {
                    return;
                }
            }
        }

        QTableWidgetItem *item;
        QColor treeColor = QColor(tree->color.r*255,
                                  tree->color.g*255,
                                  tree->color.b*255,
                                  0.6*255);
        table->insertRow(0);

        item = new QTableWidgetItem(QString::number(tree->treeID));
        if(table == treeTable) {
            Qt::ItemFlags flags = item->flags();
            flags |= Qt::ItemIsSelectable;
            flags &= ~Qt::ItemIsEditable;
            item->setFlags(flags);
        }
        table->setItem(0, TREE_ID, item);
        item = new QTableWidgetItem(QString::number(tree->color.r));
        item->setBackgroundColor(treeColor);
        table->setItem(0, TREE_R, item);
        item = new QTableWidgetItem(QString::number(tree->color.g));
        item->setBackgroundColor(treeColor);
        table->setItem(0, TREE_G, item);
        item = new QTableWidgetItem(QString::number(tree->color.b));
        item->setBackgroundColor(treeColor);
        table->setItem(0, TREE_B, item);
        item = new QTableWidgetItem(QString::number(tree->color.a));
        item->setBackgroundColor(treeColor);
        table->setItem(0, TREE_A, item);
        item = new QTableWidgetItem(QString(tree->comment));
        table->setItem(0, TREE_COMMENT, item);
    }
}

void ToolsTreeviewTab::insertNode(nodeListElement *node, NodeTable *table) {
    if(node == NULL) {
        return;
    }

    if(table == nodeTable) { // first check if node will be filtered out
        if(nodeSearchField->text().length() > 0) {
            if(node->comment == NULL) {
                return;
            }
            if(matchesSearchString(nodeSearchField->text(),
                                   QString(node->comment->content),
                                   nodeRegExCheck->isChecked()) == false) {
                return;
            }
        }
        // filter for nodes of selected trees
        if(nodesOfSelectedTreesRadio->isChecked() and state->skeletonState->selectedTrees.size() > 0) {
            if(std::find(state->skeletonState->selectedTrees.begin(),
                         state->skeletonState->selectedTrees.end(),
                         node->correspondingTree)
               == state->skeletonState->selectedTrees.end()) {
                // node not in one of the selected trees
                return;
            }
        }
        // filter for branch nodes
        if(branchNodesChckBx->isChecked()) {
            if(node->isBranchNode == false) {
                return;
            }
        }
        // filter for comment nodes
        if(commentNodesChckBx->isChecked()) {
            if(node->comment == NULL) {
                return;
            }
        }
    }

    // throw away oldest row if displayed nodes maximum is reached
    if(table == nodeTable and displayedNodes != DISPLAY_ALL and nodeTable->rowCount() >= displayedNodes) {
        table->removeRow(nodeTable->rowCount() - 1);
    }

    QTableWidgetItem *item;
    table->insertRow(0);

    item = new QTableWidgetItem(QString::number(node->nodeID));
    if(table == nodeTable) {
        Qt::ItemFlags flags = item->flags();
        flags |= Qt::ItemIsSelectable;
        flags &= ~Qt::ItemIsEditable;
        item->setFlags(flags);
    }
    table->setItem(0, NODE_ID, item);
    item = new QTableWidgetItem("");
    if(node->comment) {
        setText(table, item, node->comment->content);
    }
    table->setItem(0, NODE_COMMENT, item);
    item = new QTableWidgetItem(QString::number(node->position.x + 1));
    table->setItem(0, NODE_X, item);
    item = new QTableWidgetItem(QString::number(node->position.y + 1));
    table->setItem(0, NODE_Y, item);
    item = new QTableWidgetItem(QString::number(node->position.z + 1));
    table->setItem(0, NODE_Z, item);
    item = new QTableWidgetItem(QString::number(node->radius));
    table->setItem(0, NODE_RADIUS, item);
    if(table == nodeTable) {
        emit updateListedNodesSignal(nodeTable->rowCount());
    }
}
