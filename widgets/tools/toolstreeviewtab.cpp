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
#include "patch.h"
#include "toolstreeviewtab.h"

#define TREE_ID      0
#define TREE_COLOR   1
#define TREE_COMMENT 2
#define TREE_COLS    3
#define NODE_ID      0
#define NODE_COMMENT 1
#define NODE_X       2
#define NODE_Y       3
#define NODE_Z       4
#define NODE_RADIUS  5
#define NODE_COLS    6

#define PATCH_ID      0
#define PATCH_COMMENT 1
#define PATCH_VISIBLE 2
#define PATCH_COLS    3

#define NODECOMBO_100 0
#define NODECOMBO_1000 1
#define NODECOMBO_5000 2
#define NODECOMBO_ALL 3
#define DISPLAY_ALL 0

extern stateInfo *state;
stateInfo *kState;

// -- table implementations

KTable::KTable(QWidget *parent) : QTableWidget(parent), changeByCode(true) {}

void KTable::setItem(int row, int column, QTableWidgetItem *item) {
    if(item != NULL) {
        changeByCode = true;
        QTableWidget::setItem(row, column, item);
        changeByCode = false;
    }
}

void KTable::focusInEvent(QFocusEvent *) {
    emit focused(this);
}

void KTable::keyPressEvent(QKeyEvent *event) {
    switch(event->key()) {
    case Qt::Key_Delete:
        emit deleteSignal();
        break;
    }
}

TreeTable::TreeTable(QWidget *parent) : KTable(parent), droppedOnTreeID(0) {}

void TreeTable::dropEvent(QDropEvent *event) {
    QTableWidgetItem *droppedOnItem = itemAt(event->pos());
    droppedOnTreeID = item(droppedOnItem->row(), 0)->text().toInt();
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
    for(uint i = 0; i < kState->skeletonState->selectedNodes.size(); ++i) {
        kState->skeletonState->selectedNodes[i]->selected = false;
    }
    kState->skeletonState->selectedNodes.clear();
}

// -- end table implementations


// treeview
ToolsTreeviewTab::ToolsTreeviewTab(QWidget *parent) :
    QWidget(parent), focusedTable(NULL), radiusBuffer(state->skeletonState->defaultNodeRadius),
    draggedNodeID(0), displayedNodes(1000)
{
    kState = state;

    treeSearchField = new QLineEdit();
    treeSearchField->setPlaceholderText("search tree");
    nodeSearchField = new QLineEdit();
    nodeSearchField->setPlaceholderText("search node");
    patchSearchField = new QLineEdit();
    patchSearchField->setPlaceholderText("search patch");
    treeRegExCheck = new QCheckBox("RegEx");
    treeRegExCheck->setToolTip("search by regular expression");
    nodeRegExCheck = new QCheckBox("RegEx");
    nodeRegExCheck->setToolTip("search by regular expression");
    patchRegExCheck = new QCheckBox("RegEx");
    patchRegExCheck->setToolTip("search by regular expression");
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
    QTableWidgetItem *IDCol, *commentCol, *colorCol, // tree table cols
                     *radiusCol, *xCol, *yCol, *zCol, // node table cols
                     *visibleCol;
    // active tree table
    activeTreeTable = new TreeTable(this);
    activeTreeTable->setColumnCount(TREE_COLS);
    activeTreeTable->setFixedHeight(54);
    activeTreeTable->verticalHeader()->setVisible(false);
    activeTreeTable->setToolTip("The active tree");
    IDCol = new QTableWidgetItem("Tree ID");
    colorCol = new QTableWidgetItem("Color");
    commentCol = new QTableWidgetItem("Comment");
    activeTreeTable->setHorizontalHeaderItem(TREE_ID, IDCol);
    activeTreeTable->setHorizontalHeaderItem(TREE_COLOR, colorCol);
    activeTreeTable->setHorizontalHeaderItem(TREE_COMMENT, commentCol);
    activeTreeTable->horizontalHeader()->setStretchLastSection(true);
    activeTreeTable->resizeColumnsToContents();
    activeTreeTable->setContextMenuPolicy(Qt::CustomContextMenu);
    activeTreeTable->setDragDropMode(QAbstractItemView::DropOnly);

    // tree table
    treeTable = new TreeTable(this);
    treeTable->setColumnCount(TREE_COLS);
    treeTable->setAlternatingRowColors(true);
    treeTable->verticalHeader()->setVisible(false);
    IDCol = new QTableWidgetItem("Tree ID");
    colorCol = new QTableWidgetItem("Color");
    commentCol = new QTableWidgetItem("Comment");
    treeTable->setHorizontalHeaderItem(TREE_ID, IDCol);
    treeTable->setHorizontalHeaderItem(TREE_COLOR, colorCol);
    treeTable->setHorizontalHeaderItem(TREE_COMMENT, commentCol);
    treeTable->horizontalHeader()->setStretchLastSection(true);
    treeTable->resizeColumnsToContents();
    treeTable->setContextMenuPolicy(Qt::CustomContextMenu);
    treeTable->setDragDropMode(QAbstractItemView::DropOnly);


    // active node table
    activeNodeTable = new KTable(this);
    activeNodeTable->setColumnCount(NODE_COLS);
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


    // node table
    nodeTable = new KTable(this);
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


    // active patch table
    activePatchTable = new KTable(this);
    activePatchTable->setColumnCount(PATCH_COLS);
    activePatchTable->setAlternatingRowColors(true);
    activePatchTable->verticalHeader()->setVisible(false);
    activePatchTable->setFixedHeight(54);
    IDCol = new QTableWidgetItem("Patch ID");
    commentCol = new QTableWidgetItem("Comment");
    visibleCol = new QTableWidgetItem("show/hide");
    activePatchTable->setHorizontalHeaderItem(PATCH_ID, IDCol);
    activePatchTable->setHorizontalHeaderItem(PATCH_COMMENT, commentCol);
    activePatchTable->setHorizontalHeaderItem(PATCH_VISIBLE, visibleCol);
    activePatchTable->horizontalHeader()->setSectionResizeMode(PATCH_COMMENT, QHeaderView::Stretch);
    activePatchTable->resizeColumnsToContents();
    activePatchTable->setContextMenuPolicy(Qt::CustomContextMenu);

    // patch table
    patchTable = new KTable(this);
    patchTable->setColumnCount(PATCH_COLS);
    patchTable->setAlternatingRowColors(true);
    patchTable->verticalHeader()->setVisible(false);
    IDCol = new QTableWidgetItem("Patch ID");
    commentCol = new QTableWidgetItem("Comment");
    visibleCol = new QTableWidgetItem("show/hide");
    patchTable->setHorizontalHeaderItem(PATCH_ID, IDCol);
    patchTable->setHorizontalHeaderItem(PATCH_COMMENT, commentCol);
    patchTable->setHorizontalHeaderItem(PATCH_VISIBLE, visibleCol);
    patchTable->horizontalHeader()->setSectionResizeMode(PATCH_COMMENT, QHeaderView::Stretch);
    patchTable->resizeColumnsToContents();
    patchTable->setContextMenuPolicy(Qt::CustomContextMenu);

    createContextMenus();
    createContextMenuDialogs();

    mainLayout = new QVBoxLayout();
    QVBoxLayout *vLayout;
    QHBoxLayout *hLayout;

    QWidget *treeSide = new QWidget();
    vLayout = new QVBoxLayout();
    hLayout = new QHBoxLayout();
    hLayout->addWidget(treeSearchField);
    hLayout->addWidget(treeRegExCheck);
    vLayout->addLayout(hLayout);
    vLayout->addWidget(activeTreeTable);
    vLayout->addWidget(treeTable);
    treeSide->setLayout(vLayout);

    QWidget *nodeSide = new QWidget();
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

    QWidget *patchSide = new QWidget();
    vLayout = new QVBoxLayout();
    hLayout = new QHBoxLayout();
    hLayout->addWidget(patchSearchField);
    hLayout->addWidget(patchRegExCheck);
    vLayout->addLayout(hLayout);
    vLayout->addWidget(activePatchTable);
    vLayout->addWidget(patchTable);
    patchSide->setLayout(vLayout);

    splitter = new QSplitter(this);
    splitter->addWidget(treeSide);
    splitter->addWidget(patchSide);
    splitter->addWidget(nodeSide);
    mainLayout->addWidget(splitter);
    QList<int> list;
    list.append(310);
    list.append(390);
    list.append(200);
    splitter->setSizes(list);

    setLayout(mainLayout);

    // search events
    connect(treeSearchField, SIGNAL(editingFinished()), this, SLOT(treeSearchChanged()));
    connect(nodeSearchField, SIGNAL(editingFinished()), this, SLOT(nodeSearchChanged()));
    connect(patchSearchField, SIGNAL(editingFinished()), this, SLOT(patchSearchChanged()));
    // display events
    connect(nodesOfSelectedTreesRadio, SIGNAL(clicked()), this, SLOT(updateNodesTable()));
    connect(allNodesRadio, SIGNAL(clicked()), this, SLOT(updateNodesTable()));
    connect(branchNodesChckBx, SIGNAL(clicked()), this, SLOT(updateNodesTable()));
    connect(commentNodesChckBx, SIGNAL(clicked()), this,SLOT(updateNodesTable()));
    // displayed nodes
    connect(displayedNodesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(displayedNodesChanged(int)));
    // table click events

    connect(activeTreeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(actTreeItemChanged(QTableWidgetItem*)));
    connect(activeTreeTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemsSelected()));
    connect(activeTreeTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(itemDoubleClicked(QTableWidgetItem*)));
    connect(activeTreeTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuCalled(QPoint)));
    connect(activeTreeTable, SIGNAL(focused(KTable*)), this, SLOT(setFocused(KTable*)));
    connect(activeTreeTable, SIGNAL(deleteSignal()), this, SLOT(deleteAction()));

    connect(treeTable, SIGNAL(focused(KTable*)), this, SLOT(setFocused(KTable*)));
    connect(treeTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuCalled(QPoint)));
    connect(treeTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(itemDoubleClicked(QTableWidgetItem*)));
    connect(treeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(treeItemChanged(QTableWidgetItem*)));
    connect(treeTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemsSelected()));
    connect(treeTable, SIGNAL(deleteSignal()), this, SLOT(deleteAction()));

    connect(activeNodeTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemsSelected()));
    connect(activeNodeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(actNodeItemChanged(QTableWidgetItem*)));
    connect(activeNodeTable, SIGNAL(focused(KTable*)), this, SLOT(setFocused(KTable*)));
    connect(activeNodeTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuCalled(QPoint)));
    connect(activeNodeTable, SIGNAL(deleteSignal()), this, SLOT(deleteAction()));

    connect(nodeTable, SIGNAL(focused(KTable*)), this, SLOT(setFocused(KTable*)));
    connect(nodeTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuCalled(QPoint)));
    connect(nodeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(nodeItemChanged(QTableWidgetItem*)));
    connect(nodeTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemsSelected()));
    connect(nodeTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(itemDoubleClicked(QTableWidgetItem*)));
    connect(nodeTable, SIGNAL(deleteSignal()), this, SLOT(deleteAction()));

    connect(activePatchTable, SIGNAL(focused(KTable*)), this, SLOT(setFocused(KTable*)));
    connect(activePatchTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuCalled(QPoint)));
    connect(activePatchTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemsSelected()));
    connect(activePatchTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(itemDoubleClicked(QTableWidgetItem*)));
    connect(activePatchTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(patchItemChanged(QTableWidgetItem*)));
    connect(activePatchTable, SIGNAL(deleteSignal()), this, SLOT(deleteAction()));
    connect(patchTable, SIGNAL(focused(KTable*)), this, SLOT(setFocused(KTable*)));
    connect(patchTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemsSelected()));
    connect(patchTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(itemDoubleClicked(QTableWidgetItem*)));
    connect(patchTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(patchItemChanged(QTableWidgetItem*)));
    connect(patchTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuCalled(QPoint)));
    connect(patchTable, SIGNAL(deleteSignal()), this, SLOT(deleteAction()));
}

void ToolsTreeviewTab::createContextMenus() {
    treeContextMenu = new QMenu();
    treeContextMenu->addAction("Set as active tree", this, SLOT(setActiveTreeAction()));
    treeContextMenu->addAction("Set comment for tree(s)", this, SLOT(setTreeCommentAction()));
    treeContextMenu->addAction("Merge trees", this, SLOT(mergeTreesAction()));
    treeContextMenu->addAction("Move selected node(s) to this tree", this, SLOT(moveNodesAction()));
    treeContextMenu->addAction("Restore default color", this, SLOT(restoreColorAction()));
    treeContextMenu->addAction(QIcon(":/images/icons/user-trash.png"), "Delete tree(s)", this, SLOT(deleteAction()));

    nodeContextMenu = new QMenu();
    nodeContextMenu->addAction("Set comment for node(s)...", this, SLOT(setNodeCommentAction()));
    nodeContextMenu->addAction("Set radius for node(s)...", this, SLOT(setNodeRadiusAction()));
    nodeContextMenu->addAction("(Un)link nodes... ", this, SLOT(linkNodesAction()));
    nodeContextMenu->addAction("Split component from tree", this, SLOT(splitComponentAction()));
    nodeContextMenu->addAction(QIcon(":/images/icons/user-trash.png"), "delete node(s)", this, SLOT(deleteAction()));

    patchContextMenu = new QMenu();
    patchContextMenu->addAction("Create new patch", this, SLOT(addPatchAction()));
}

void ToolsTreeviewTab::createContextMenuDialogs() {
    // for tree table
    treeCommentEditDialog = new QDialog();
    treeCommentEditDialog->setWindowTitle("Edit Tree Comment");
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

    treeColorEditDialog = new QDialog();
    treeColorEditDialog->setWindowTitle("Change Tree Color");
    treeColorEditDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    rLabel = new QLabel("R:"); gLabel = new QLabel("G:"); bLabel = new QLabel("B:"); aLabel = new QLabel("A:");
    rSpin = new QDoubleSpinBox(); gSpin = new QDoubleSpinBox(); bSpin = new QDoubleSpinBox(); aSpin = new QDoubleSpinBox();
    rSpin->setMaximum(1.); gSpin->setMaximum(1.); bSpin->setMaximum(1.); aSpin->setMaximum(1.);
    treeColorApplyButton = new QPushButton("Apply");
    treeColorCancelButton = new QPushButton("Cancel");
    QVBoxLayout *vLayout = new QVBoxLayout();
    hLayout = new QHBoxLayout();
    hLayout->addWidget(rLabel); hLayout->addWidget(rSpin);
    hLayout->addWidget(gLabel); hLayout->addWidget(gSpin);
    hLayout->addWidget(bLabel); hLayout->addWidget(bSpin);
    hLayout->addWidget(aLabel); hLayout->addWidget(aSpin);
    vLayout->addLayout(hLayout);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(treeColorApplyButton);
    hLayout->addWidget(treeColorCancelButton);
    vLayout->addLayout(hLayout);
    treeColorEditDialog->setLayout(vLayout);
    connect(treeColorApplyButton, SIGNAL(clicked()), this, SLOT(editTreeColor()));
    connect(treeColorCancelButton, SIGNAL(clicked()), treeColorEditDialog, SLOT(hide()));

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

void ToolsTreeviewTab::setFocused(KTable *table) {
    focusedTable = table;
}

void ToolsTreeviewTab::contextMenuCalled(QPoint pos) {
    if(focusedTable == activeTreeTable) {
        treeContextMenu->actions().at(0)->setEnabled(false); // set as active tree action
        treeContextMenu->actions().at(2)->setEnabled(false); // merge trees action
        treeContextMenu->move(activeTreeTable->mapToGlobal(pos));
        treeContextMenu->show();
    }
    else if(focusedTable == treeTable) {
        treeContextMenu->actions().at(0)->setEnabled(true);
        treeContextMenu->actions().at(2)->setEnabled(true);
        treeContextMenu->move(treeTable->mapToGlobal(pos));
        treeContextMenu->show();
    }
    else if(focusedTable == activeNodeTable) {
        nodeContextMenu->actions().at(2)->setEnabled(false); // link nodes needs two selected nodes
        nodeContextMenu->move(activeNodeTable->mapToGlobal(pos));
        nodeContextMenu->show();
    }
    else if(focusedTable == nodeTable) {
        nodeContextMenu->actions().at(2)->setEnabled(true);
        nodeContextMenu->move(nodeTable->mapToGlobal(pos));
        nodeContextMenu->show();
    }
    else if(focusedTable == activePatchTable) {
        patchContextMenu->move(activePatchTable->mapToGlobal(pos));
        patchContextMenu->show();
    }
    else if(focusedTable == patchTable) {
        patchContextMenu->move(patchTable->mapToGlobal(pos));
        patchContextMenu->show();
    }
}

void ToolsTreeviewTab::itemDoubleClicked(QTableWidgetItem* item) {
    if(focusedTable == activeTreeTable) {
        if(item->column() == TREE_COLOR) {
            if(state->skeletonState->activeTree == NULL) {
                qDebug("No active tree!");
                activeTreeChanged();
                return;
            }
            state->skeletonState->selectedTrees.clear();
            state->skeletonState->selectedTrees.push_back(state->skeletonState->activeTree);
            treeColorEditRow = -1;
            for(int i = 0; i < treeTable->rowCount(); ++i) {
                if(treeTable->item(i, TREE_ID)->text().toInt() == state->skeletonState->activeTree->treeID) {
                    treeColorEditRow = item->row();
                    break;
                }
            }

            rSpin->setValue(state->skeletonState->activeTree->color.r);
            gSpin->setValue(state->skeletonState->activeTree->color.g);
            bSpin->setValue(state->skeletonState->activeTree->color.b);
            aSpin->setValue(state->skeletonState->activeTree->color.a);
            treeColorEditDialog->exec(); // don't use show. We have to block other events for this to safely finish
        }
    }
    if(focusedTable == treeTable) {
        if(item->column() == TREE_COLOR) {
            QTableWidgetItem *idItem = treeTable->item(item->row(), TREE_ID);
            treeListElement *tree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
            if(tree == NULL) {
                return;
            }
            state->skeletonState->selectedTrees.clear();
            state->skeletonState->selectedTrees.push_back(tree);
            treeColorEditRow = item->row();
            rSpin->setValue(tree->color.r);
            gSpin->setValue(tree->color.g);
            bSpin->setValue(tree->color.b);
            aSpin->setValue(tree->color.a);
            treeColorEditDialog->exec(); // don't use show. We have to block other events for this to safely finish
        }
        else if(item->column() == TREE_ID) {
            // user wants to activate tree
            if(state->skeletonState->selectedTrees.size() != 1) {
                return;
            }
            Skeletonizer::setActiveTreeByID(state->skeletonState->selectedTrees.at(0)->treeID);
            activeTreeChanged();
            state->skeletonState->selectedTrees.clear();
        }
    }

    else if(focusedTable == nodeTable) {
        if(state->skeletonState->selectedNodes.size() != 1) {
            return;
        }
        emit setActiveNodeSignal(CHANGE_MANUAL, NULL, state->skeletonState->selectedNodes.at(0)->nodeID);
        emit jumpToActiveNodeSignal();
        activeNodeChanged();
        for(uint i = 0; i < state->skeletonState->selectedNodes.size(); ++i) {
            state->skeletonState->selectedNodes[i]->selected = false;
        }
        state->skeletonState->selectedNodes.clear();
    }

    else if(focusedTable == patchTable) {
        switch(item->column()) {
        case PATCH_ID:
            if(setActivePatchSignal(NULL, patchTable->item(item->row(), PATCH_ID)->text().toInt())) {
                activePatchChanged();
                emit jumpToActivePatchSignal();
            }
            state->skeletonState->selectedPatches.clear();
            break;
        case PATCH_VISIBLE:
            QTableWidgetItem *idItem = patchTable->item(item->row(), PATCH_ID);
            Patch *patch = Patch::getPatchWithID(idItem->text().toInt());
            if(patch == NULL) {
                qDebug("could not find patch with id %i", idItem->text().toInt());
                patchTable->removeRow(item->row());
                return;
            }
            if(item->text() == "show") {
                item->setText("hide");
                patch->visible = false;
            }
            else {
                item->setText("show");
                patch->visible = true;
            }
            if(patch == Patch::activePatch) {
                activePatchTable->item(0, PATCH_VISIBLE)->setText(item->text());
            }
            break;
        }
    }
    else if(focusedTable == activePatchTable) {
        switch(item->column()) {
        case PATCH_ID:
            emit jumpToActivePatchSignal();
            state->skeletonState->selectedPatches.clear();
            break;
        case PATCH_VISIBLE:
            if(Patch::activePatch == NULL) {
                qDebug("no active patch.");
                activePatchTable->clearContents();
                return;
            }
            if(item->text() == "show") {
                item->setText("hide");
                Patch::activePatch->visible = false;
            }
            else {
                item->setText("show");
                Patch::activePatch->visible = true;
            }
            for(int i = 0; i < patchTable->rowCount(); ++i) {
                if(patchTable->item(i, PATCH_ID)->text().toInt() == Patch::activePatch->patchID) {
                    patchTable->item(i, PATCH_VISIBLE)->setText(item->text());
                }
            }
            break;
        }
    }
}

void ToolsTreeviewTab::itemsSelected() {
    QModelIndexList selected = focusedTable->selectionModel()->selectedIndexes();

    if(focusedTable == activeTreeTable) {
        treeTable->clearSelection();
        state->skeletonState->selectedTrees.clear();
        if(state->skeletonState->activeTree) {
            state->skeletonState->selectedTrees.push_back(state->skeletonState->activeTree);
            if(nodesOfSelectedTreesRadio->isChecked()) {
                updateNodesTable();
            }
        }
    }

    else if(focusedTable == treeTable) {
        activeTreeTable->clearSelection();
        state->skeletonState->selectedTrees.clear();
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

    else if(focusedTable == activeNodeTable) {
        nodeTable->clearSelection();
        for(uint i = 0; i < state->skeletonState->selectedNodes.size(); ++i) {
            state->skeletonState->selectedNodes[i]->selected = false;
        }
        state->skeletonState->selectedNodes.clear();
        if(state->skeletonState->activeNode) {
            state->skeletonState->activeNode->selected = true;
            state->skeletonState->selectedNodes.push_back(state->skeletonState->activeNode);
        }
    }
    else if(focusedTable == nodeTable) {
        activeNodeTable->clearSelection();
        for(uint i = 0; i < state->skeletonState->selectedNodes.size(); ++i) {
            state->skeletonState->selectedNodes[i]->selected = false;
        }
        state->skeletonState->selectedNodes.clear();
        nodeListElement *node;
        foreach(QModelIndex index, selected) {
            node = Skeletonizer::findNodeByNodeID(index.data().toInt());
            if(node) {
                state->skeletonState->selectedNodes.push_back(node);
                node->selected = true;
            }
        }
    }
    else if(focusedTable == patchTable) {
        state->skeletonState->selectedPatches.clear();
        patchListElement *patch;
        foreach(QModelIndex index, selected) {
            patch = Skeletonizer::findPatchByPatchID(index.data().toInt());
            if(patch) {
                state->skeletonState->selectedPatches.push_back(patch);
            }
        }
    }
}

void ToolsTreeviewTab::deleteAction() {
    QMessageBox info, prompt;
    QPushButton *confirmButton;
    info.setWindowFlags(Qt::WindowStaysOnTopHint);
    info.setIcon(QMessageBox::Information);
    info.setWindowTitle("Information");
    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
    prompt.setIcon(QMessageBox::Question);
    prompt.setWindowTitle("Cofirmation required");

    if(focusedTable == activeTreeTable) {
        if(state->skeletonState->activeTree == NULL) {
            return;
        }
        prompt.setText("Do you really want to delete active tree?");
        confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            state->skeletonState->selectedTrees.clear();
            state->skeletonState->selectedTrees.push_back(state->skeletonState->activeTree);
            if(deleteSelectedTreesSignal()) {
                treesDeleted();
            }
        }
        state->skeletonState->selectedTrees.clear();
    }
    else if(focusedTable == treeTable) {
        if(state->skeletonState->selectedTrees.size() == 0) {
            info.setText("Select at least one tree for deletion.");
            info.exec();
            return;
        }
        prompt.setText("Do you really want to delete selected tree(s)?");
        confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            if(deleteSelectedTreesSignal()) {
                treesDeleted();
            }
        }
        state->skeletonState->selectedTrees.clear();
    }

    else if(focusedTable == activeNodeTable) {
        if(state->skeletonState->activeNode == NULL) {
            return;
        }
        prompt.setText("Do you really want to delete the active node?");
        confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            state->skeletonState->selectedNodes.clear();
            state->skeletonState->selectedNodes.push_back(state->skeletonState->activeNode);
            if(deleteSelectedNodesSignal()) {
                nodesDeleted();
            }
        }
        return;
    }
    else if(focusedTable == nodeTable) {
        if(state->skeletonState->selectedNodes.size() == 0) {
            info.setText("Select at least one node for deletion.");
            info.exec();
            return;
        }
        prompt.setText("Do you really want to deleted selected node(s)?");
        confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            if(deleteSelectedNodesSignal()) {
                nodesDeleted();
            }
        }
        state->skeletonState->selectedNodes.clear();
    }
    else if(focusedTable == activePatchTable) {
        if(Patch::activePatch == NULL) {
            return;
        }
        prompt.setText("Do you really want to delete the active patch?");
        confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
        prompt.addButton("Cancel", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            state->skeletonState->selectedPatches.clear();
            patchListElement *patchEl = Skeletonizer::findPatchByPatchID(Patch::activePatch->patchID);
            if(patchEl) {
                state->skeletonState->selectedPatches.push_back(patchEl);
                if(deleteSelectedPatchesSignal()) {
                    patchesDeleted();
                }
            }
        }
        return;
    }
    else if(focusedTable == patchTable) {
        if(state->skeletonState->selectedPatches.size() == 0) {
            info.setText("Select at least one patch for deletion.");
            info.exec();
            return;
        }
        prompt.setText("Do you really want to delete selected patch(es)?");
        confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
        prompt.exec();
        if(prompt.clickedButton() == confirmButton) {
            if(deleteSelectedPatchesSignal()) {
                patchesDeleted();
            }
        }
    }
}

// tree actions

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
        Skeletonizer::setActiveTreeByID(state->skeletonState->selectedTrees[0]->treeID);
        activeTreeChanged();
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
            treesMerged(treeID2);
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
    if(focusedTable == treeTable and state->skeletonState->selectedTrees.size() == 0) {
        return;
    }
    treeCommentField->setText(treeCommentBuffer);
    treeCommentEditDialog->exec();
}

void ToolsTreeviewTab::updateTreeCommentBuffer(QString comment) {
    treeCommentBuffer = comment;
}

void ToolsTreeviewTab::editTreeComments() {
    if(focusedTable == activeTreeTable) {
        Skeletonizer::addTreeComment(CHANGE_MANUAL, state->skeletonState->activeTree->treeID,
                                     treeCommentBuffer.toLocal8Bit().data());
        setText(activeTreeTable, activeTreeTable->item(0, TREE_COMMENT), treeCommentBuffer);
        int row = getActiveTreeRow();
        if(row != -1) {
            setText(treeTable, treeTable->item(row, TREE_COMMENT), treeCommentBuffer);
        }
        treeCommentEditDialog->hide();
        return;
    }

    if(state->skeletonState->selectedTrees.size() == 0) {
        QMessageBox prompt;
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Information);
        prompt.setWindowTitle("Information");
        prompt.setText("Please select at least one tree to edit its comment.");
        prompt.exec();
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

void ToolsTreeviewTab::editTreeColor() {
    if(state->skeletonState->selectedTrees.size() != 1) {
        qDebug("Failed to edit tree color. No tree selected.");
    }
    else {
        state->skeletonState->selectedTrees[0]->color.r = rSpin->value();
        state->skeletonState->selectedTrees[0]->color.g = gSpin->value();
        state->skeletonState->selectedTrees[0]->color.b = bSpin->value();
        state->skeletonState->selectedTrees[0]->color.a = aSpin->value();
        if(treeColorEditRow != -1) {
            updateTreeColorCell(treeTable, treeColorEditRow);
        }
        if(state->skeletonState->selectedTrees[0] == state->skeletonState->activeTree) {
            updateTreeColorCell(activeTreeTable, 0);
        }
        state->skeletonState->selectedTrees.clear();
    }
    treeColorEditDialog->hide();
}

// node actions

void ToolsTreeviewTab::setNodeRadiusAction() {
    if(state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    radiusSpin->setValue(radiusBuffer);
    nodeRadiusEditDialog->exec();
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
    if(Skeletonizer::findSegmentByNodeIDs(state->skeletonState->selectedNodes[0]->nodeID,
                                          state->skeletonState->selectedNodes[1]->nodeID)) {
        emit delSegmentSignal(CHANGE_MANUAL, state->skeletonState->selectedNodes[0]->nodeID,
                              state->skeletonState->selectedNodes[1]->nodeID, NULL, true);
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
    if(focusedTable == activeNodeTable) {
        Skeletonizer::editNode(CHANGE_MANUAL, 0, state->skeletonState->activeNode, radiusBuffer,
                               state->skeletonState->activeNode->position.x,
                               state->skeletonState->activeNode->position.y,
                               state->skeletonState->activeNode->position.z,
                               state->skeletonState->activeNode->createdInMag);
        setText(activeNodeTable, activeNodeTable->item(0, NODE_RADIUS), QString::number(radiusBuffer));
        int row = getActiveNodeRow();
        if(row != -1) {
            setText(nodeTable, nodeTable->item(row, NODE_RADIUS), QString::number(radiusBuffer));
        }
        nodeRadiusEditDialog->hide();
    }
    else {
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
    for(uint i = 0; i < state->skeletonState->selectedNodes.size(); ++i) {
        state->skeletonState->selectedNodes[i]->selected = false;
    }
    state->skeletonState->selectedNodes.clear();
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
            for(uint i = 0; i < state->skeletonState->selectedNodes.size(); ++i) {
                state->skeletonState->selectedNodes[i]->selected = false;
            }
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
    for(uint i = 0; i < state->skeletonState->selectedNodes.size(); ++i) {
        state->skeletonState->selectedNodes[i]->selected = false;
    }
    state->skeletonState->selectedNodes.clear();
    state->skeletonState->selectedTrees.clear();
}

void ToolsTreeviewTab::setNodeCommentAction() {
    if(state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    nodeCommentField->setText(nodeCommentBuffer);
    nodeCommentEditDialog->exec();
}

void ToolsTreeviewTab::updateNodeCommentBuffer(QString comment) {
    nodeCommentBuffer = comment;
}

void ToolsTreeviewTab::editNodeComments() {
    if(focusedTable == activeNodeTable) {
        if(nodeCommentBuffer.length() > 0) {
            if(state->skeletonState->activeNode->comment == NULL) {
                Skeletonizer::addComment(CHANGE_MANUAL, nodeCommentBuffer.toLocal8Bit().data(),
                                         state->skeletonState->activeNode, 0, true);
            }
            else {
                Skeletonizer::editComment(CHANGE_MANUAL,
                                          state->skeletonState->activeNode->comment, 0,
                                          nodeCommentBuffer.toLocal8Bit().data(),
                                          state->skeletonState->activeNode, 0, true);
            }
        }
        else {
            if(state->skeletonState->activeNode->comment != NULL) {
                Skeletonizer::delComment(CHANGE_MANUAL, state->skeletonState->activeNode->comment,
                                         state->skeletonState->activeNode->nodeID, true);
            }
        }
        setText(activeNodeTable, activeNodeTable->item(0, NODE_COMMENT), nodeCommentBuffer);
        int row = getActiveNodeRow();
        if(row != -1) {
            setText(nodeTable, nodeTable->item(row, NODE_COMMENT), nodeCommentBuffer);
        }
        nodeCommentEditDialog->hide();
        return;
    }
    // nodeTable focused
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

// patch actions
void ToolsTreeviewTab::addPatchAction() {
    if(newPatchSignal()) {
        patchAdded();
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

void ToolsTreeviewTab::patchSearchChanged() {
    if(patchRegExCheck->isChecked() and patchSearchField->text().length() > 0) {
        QRegularExpression regex(patchSearchField->text());
        if(regex.isValid() == false) {
            QToolTip::showText(patchSearchField->mapToGlobal(patchSearchField->pos()), "Invalid regular expression.");
        }
    }
    updatePatchesTable();
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

    switch(item->column()) {
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
    if(treeTable->changeByCode) {
        return;
    }
    QTableWidgetItem *idItem = treeTable->item(item->row(), TREE_ID);
    if(idItem == NULL) {
        return;
    }
    treeListElement *selectedTree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
    if(selectedTree == NULL) {
        return;
    }

    switch(item->column()) {
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
    for(uint i = 0; i < state->skeletonState->selectedNodes.size(); ++i) {
        state->skeletonState->selectedNodes[i]->selected = false;
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
    for(uint i = 0; i < state->skeletonState->selectedNodes.size(); ++i) {
        state->skeletonState->selectedNodes[i]->selected = false;
    }
    state->skeletonState->selectedNodes.clear();
    emit updateToolsSignal();
}

void ToolsTreeviewTab::patchItemChanged(QTableWidgetItem *item) {
    KTable *table = (KTable *)item->tableWidget();
    if(table->changeByCode) {
        return;
    }

    QTableWidgetItem *idItem = table->item(item->row(), PATCH_ID);
    Patch *patch;
    if((patch = Patch::getPatchWithID(idItem->text().toInt())) == NULL) {
        QMessageBox prompt;
        prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
        prompt.setIcon(QMessageBox::Information);
        prompt.setWindowTitle("Information");
        prompt.setText(QString("Patch with ID %1 could not be found.").arg(idItem->text()));
        prompt.exec();
        table->removeRow(item->row());
    }
    if(item->column() == PATCH_COMMENT) {
        if(patch == Patch::activePatch) {
            if(table == patchTable) {
                setText(activePatchTable, activePatchTable->item(0, PATCH_COMMENT), item->text());
                //check if patch is now filtered out by comment search
                if(patch->comment.isEmpty() == false) {
                    patch->comment = item->text();
                    if(item->text().length() == 0) {
                        if(patchSearchField->text().isEmpty() == false) {
                            patchTable->removeRow(item->row()); // filter out
                        }
                    }
                    else {
                        if(patchSearchField->text().isEmpty() == false) {
                            if(matchesSearchString(patchSearchField->text(),
                                                   patch->comment, patchRegExCheck->isChecked()) == false) {
                                patchTable->removeRow(item->row());
                            }
                        }
                    }
                }
                else { // comment was empty before and not filtered out, so it won't be filtered out after this, either
                    patch->comment = item->text();
                }
            }
            else { // activePatchTable
                patch->comment = item->text();
                int i;
                for(i = 0; i < patchTable->rowCount(); ++i) {
                    if((uint)patchTable->item(i, PATCH_ID)->text().toInt() == Patch::activePatch->patchID) {
                        setText(patchTable, patchTable->item(i, PATCH_COMMENT), item->text());
                        break;
                    }
                }
                if(i == patchTable->rowCount()) { // item was filtered out before, check if it is shown now
                    if(matchesSearchString(patchSearchField->text(), patch->comment, patchRegExCheck->isChecked())) {
                        insertPatch(patch, patchTable);
                    }
                }
                else if(patchSearchField->text().length() > 0) {
                    //was not filtered out before. check if it is filtered now
                    if(matchesSearchString(patchSearchField->text(), patch->comment, patchRegExCheck->isChecked()) == false) {
                        patchTable->removeRow(i);
                    }
                }
            }
        }
        else { // not active patch
            //check if patch is now filtered out by comment search
            if(patch->comment.isEmpty() == false) {
                patch->comment = item->text();
                if(item->text().length() == 0) {
                    if(patchSearchField->text().isEmpty() == false) {
                        patchTable->removeRow(item->row()); // filter out
                    }
                }
                else {
                    if(patchSearchField->text().isEmpty() == false) {
                        if(matchesSearchString(patchSearchField->text(),
                                               patch->comment, patchRegExCheck->isChecked()) == false) {
                            patchTable->removeRow(item->row());
                        }
                    }
                }
            }
            else { // comment was empty before and not filtered out, so it won't be filtered out after this, either
                patch->comment = item->text();
            }
        }
    }
}


void ToolsTreeviewTab::updateTreeColorCell(TreeTable *table, int row) {
    QTableWidgetItem *idItem = table->item(row, TREE_ID);
    treeListElement *tree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
    if(tree == NULL) {
        return;
    }
    QColor treeColor = QColor(tree->color.r*255, tree->color.g*255, tree->color.b*255, 153);
    QTableWidgetItem *colorItem = table->item(row, TREE_COLOR);
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

void ToolsTreeviewTab::setText(KTable *table, QTableWidgetItem *item, QString text) {
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
        Qt::ItemFlags flags;
        item = new QTableWidgetItem(QString::number(currentTree->treeID));
        flags = item->flags();
        flags |= Qt::ItemIsSelectable;
        flags &= ~Qt::ItemIsEditable;
        item->setFlags(flags);
        treeTable->setItem(treeIndex, TREE_ID, item);

        // tree color
        QColor treeColor = QColor(currentTree->color.r*255,
                                  currentTree->color.g*255,
                                  currentTree->color.b*255,
                                  0.6*255);
        item = new QTableWidgetItem();
        flags = item->flags();
        flags &= ~Qt::ItemIsEditable;
        item->setFlags(flags);
        item->setBackgroundColor(treeColor);
        treeTable->setItem(treeIndex, TREE_COLOR, item);

        // tree comment
        item = new QTableWidgetItem("");
        if(currentTree->comment) {
            setText(treeTable, item, QString(currentTree->comment));
        }
        treeTable->setItem(treeIndex, TREE_COMMENT, item);

        treeIndex++;
        currentTree = currentTree->next;
    }

    treeTable->setRowCount(treeIndex);
}

void ToolsTreeviewTab::updateNodesTable() {
    nodeTable->changeByCode = true;
    nodeTable->clearContents();

    if((uint)displayedNodes > state->skeletonState->totalNodeElements or displayedNodes == DISPLAY_ALL) {
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
        while(node and nodeIndex < displayedNodes or (node and displayedNodes == DISPLAY_ALL)) {
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
            flags &= ~Qt::ItemIsEditable;
            item->setFlags(flags);
            nodeTable->setItem(nodeIndex, NODE_ID, item);

            item = new QTableWidgetItem("");
            flags = item->flags();
            flags &= ~Qt::ItemIsSelectable;
            item->setFlags(flags);
            if(node->comment) {
                setText(nodeTable, item, node->comment->content);
            }
            nodeTable->setItem(nodeIndex, NODE_COMMENT, item);

            item = new QTableWidgetItem(QString::number(node->position.x + 1));
            flags = item->flags();
            flags &= ~Qt::ItemIsSelectable;
            item->setFlags(flags);
            nodeTable->setItem(nodeIndex, NODE_X, item);

            item = new QTableWidgetItem(QString::number(node->position.y + 1));
            flags = item->flags();
            flags &= ~Qt::ItemIsSelectable;
            item->setFlags(flags);
            nodeTable->setItem(nodeIndex, NODE_Y, item);

            item = new QTableWidgetItem(QString::number(node->position.z + 1));
            flags = item->flags();
            flags &= ~Qt::ItemIsSelectable;
            item->setFlags(flags);
            nodeTable->setItem(nodeIndex, NODE_Z, item);

            item = new QTableWidgetItem(QString::number(node->radius));
            flags = item->flags();
            flags &= ~Qt::ItemIsSelectable;
            item->setFlags(flags);
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

void ToolsTreeviewTab::updatePatchesTable() {
    patchTable->clearContents();
    patchTable->setRowCount(Patch::numPatches);

    Patch *currentPatch = Patch::activePatch;
    if(currentPatch == NULL) {
        return;
    }

    QTableWidgetItem *item;
    uint patchIndex = 0;
    do {
        // filter comment for search string match
        if(patchSearchField->text().length() > 0) {
            if(currentPatch->comment.length() == 0) {
                currentPatch = currentPatch->next;
                continue;
            }
            if(matchesSearchString(patchSearchField->text(), currentPatch->comment, treeRegExCheck->isChecked()) == false) {
                currentPatch = currentPatch->next;
                continue;
            }
        }
        // patch id
        QColor color(currentPatch->correspondingTree->color.r * 255,
                     currentPatch->correspondingTree->color.g * 255,
                     currentPatch->correspondingTree->color.b * 255,
                     0.6 * 255);

        Qt::ItemFlags flags;
        item = new QTableWidgetItem(QString::number(currentPatch->patchID));
        flags = item->flags();
        flags |= Qt::ItemIsSelectable;
        flags &= ~Qt::ItemIsEditable;
        item->setFlags(flags);
        item->setBackgroundColor(color);
        patchTable->setItem(patchIndex, PATCH_ID, item);

        // patch comment
        item = new QTableWidgetItem("");
        flags = item->flags();
        flags &= ~Qt::ItemIsSelectable;
        item->setFlags(flags);
        if(currentPatch->comment.length() > 0) {
            setText(patchTable, item, currentPatch->comment);
        }
        patchTable->setItem(patchIndex, PATCH_COMMENT, item);

        // patch visibility
        item = new QTableWidgetItem("show");
        if(currentPatch->visible == false) {
            item->setText("hide");
        }
        flags = item->flags();
        flags &= ~Qt::ItemIsSelectable & ~Qt::ItemIsEditable;
        item->setFlags(flags);
        patchTable->setItem(patchIndex, PATCH_VISIBLE, item);

        patchIndex++;
        currentPatch = currentPatch->next;
    } while(currentPatch != Patch::activePatch);

    patchTable->setRowCount(patchIndex);
}


void ToolsTreeviewTab::activeTreeChanged() {
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
        activeTreeChanged();
    }
    insertTree(tree, treeTable);
    emit updateToolsSignal();
}

void ToolsTreeviewTab::treesDeleted() {
    if(focusedTable == activeTreeTable) {
        if(activeTreeTable->rowCount() == 0) {
            return;
        }
        int activeID = activeTreeTable->item(0, TREE_ID)->text().toInt(); // for deletion in tree table

        if(state->skeletonState->activeTree == NULL) {
            activeTreeTable->clearContents();
        }
        else {
            activeTreeChanged(); // new tree will be active now
        }

        for(int i = 0; i < treeTable->rowCount(); ++i) {
            if(treeTable->item(i, TREE_ID)->text().toInt() == activeID) {
                treeTable->removeRow(i);
                break;
            }
        }
    }
    else { // tree table is focused
        for(int i = treeTable->rowCount() - 1; i >= 0; --i) {
            int treeID = treeTable->item(i, TREE_ID)->text().toInt();
            if(Skeletonizer::findTreeByTreeID(treeID) == NULL) {
                treeTable->removeRow(i);
            }
        }

        // might have been the displayed active tree
        if(activeTreeTable->item(0, TREE_ID) != NULL) {
            if(state->skeletonState->activeTree == NULL) {
                activeTreeTable->clearContents();
            }
            else if(state->skeletonState->activeTree->treeID != activeTreeTable->item(0, TREE_ID)->text().toInt()) {
                activeTreeChanged();
            }
        }
    }
    // displayed active node might have been in the deleted tree
    if(activeNodeTable->item(0, NODE_ID) != NULL) {
        if(state->skeletonState->activeNode == NULL) {
            activeNodeTable->clearContents();
        }
        else if(state->skeletonState->activeNode->nodeID != (uint)activeNodeTable->item(0, NODE_ID)->text().toInt()) {
            activeNodeChanged();
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

    // displayed active patch might have been in the deleted tree
    if(activePatchTable->rowCount() == 1) {
        if(Patch::activePatch == NULL) {
            activePatchChanged();
        }
    }
    // remove all patches of the tree
    for(int i = patchTable->rowCount() - 1; i >= 0; --i) {
        if(Skeletonizer::findPatchByPatchID(patchTable->item(i, PATCH_ID)->text().toInt()) == NULL) {
            patchTable->removeRow(i);
        }
    }

    emit updateToolsSignal(); // update the tree and node count
}

/**
 * @brief ToolsTreeviewTab::treesMerged
 * @param treeID2 the id of the deleted tree after the merge.
 */
void ToolsTreeviewTab::treesMerged(int treeID2) {
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
            activeTreeChanged();
        }
    }
    updateToolsSignal();
}

void ToolsTreeviewTab::treeComponentSplit() {
    // when a tree is split, a new tree has been created and made active
    activeTreeChanged();
    insertTree(state->skeletonState->activeTree, treeTable);
    updateToolsSignal();
}

// update node table
void ToolsTreeviewTab::activeNodeChanged() {
    activeNodeTable->clearContents();
    if(state->skeletonState->activeNode == NULL) {
        if(state->skeletonState->activeTree == NULL) {
            activeTreeChanged();
        }
        return;
    }
    insertNode(state->skeletonState->activeNode, activeNodeTable);
    activeNodeTable->setRowCount(1);
    activeTreeChanged(); // update active tree table in case of tree switch
    emit updateToolsSignal();
}

void ToolsTreeviewTab::nodeAdded() {
    activeNodeChanged();
    if(state->skeletonState->activeNode != NULL) {
        insertNode(state->skeletonState->activeNode, nodeTable);
    }
}

void ToolsTreeviewTab::nodesDeleted() {
    if(focusedTable == activeNodeTable) {
        if(activeNodeTable->rowCount() == 0) {
            return;
        }
        int activeID = activeNodeTable->item(0, NODE_ID)->text().toInt(); // to delete from nodeTable
        activeNodeChanged(); // a new node might be active now
        for(int i = 0; i < nodeTable->rowCount(); ++i) {
            if(nodeTable->item(i, NODE_ID)->text().toInt() == activeID) {
                nodeTable->removeRow(i);
                return;
            }
        }
    }
    // nodes have to be deleted in reverse order or the row numbers will shift
    for(int i = nodeTable->rowCount() - 1; i >= 0; --i) {
        int nodeID = nodeTable->item(i, NODE_ID)->text().toInt();
        if(Skeletonizer::findNodeByNodeID(nodeID) == NULL) {
            nodeTable->removeRow(i);
        }
    }

    activeNodeChanged();
    state->skeletonState->selectedNodes.clear();
    updateToolsSignal();
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
    activeNodeChanged();
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
            if((uint)nodeTable->item(i, NODE_ID)->text().toInt() == node->nodeID) {
                setText(nodeTable, nodeTable->item(i, NODE_X), QString::number(node->position.x + 1));
                setText(nodeTable, nodeTable->item(i, NODE_Y), QString::number(node->position.y + 1));
                setText(nodeTable, nodeTable->item(i, NODE_Z), QString::number(node->position.z + 1));
                break;
            }
        }
    }
}

void ToolsTreeviewTab::activePatchChanged() {
    activePatchTable->clearContents();
    if(Patch::activePatch == NULL) {
        if(state->skeletonState->activeTree == NULL) {
            activeTreeChanged();
        }
        return;
    }
    insertPatch(Patch::activePatch, activePatchTable);
    activePatchTable->setRowCount(1);
    activeTreeChanged(); // update active tree table in case of tree switch
    emit updateToolsSignal();
}

void ToolsTreeviewTab::patchAdded() {
    activePatchChanged();
    if(Patch::activePatch != NULL) {
        insertPatch(Patch::activePatch, patchTable);
    }
}

void ToolsTreeviewTab::patchesDeleted() {
    if(focusedTable == activePatchTable) {
        if(activePatchTable->rowCount() == 0) {
            return;
        }
        int activeID = activePatchTable->item(0, NODE_ID)->text().toInt(); // to delete from patchTable
        activePatchChanged();
        for(int i = 0; i < patchTable->rowCount(); ++i) {
            if(patchTable->item(i, PATCH_ID)->text().toInt() == activeID) {
                patchTable->removeRow(i);
                return;
            }
        }
    }
    // patches have to be deleted in reverse order or the row numbers will shift
    for(int i = patchTable->rowCount() - 1; i >= 0; --i) {
        int patchID = patchTable->item(i, PATCH_ID)->text().toInt();
        if(Skeletonizer::findPatchByPatchID(patchID) == NULL) {
            patchTable->removeRow(i);
        }
    }
    // update active patch table, in case active patch was deleted, too
    activePatchChanged();
}


void ToolsTreeviewTab::update() {
    updateTreesTable();
    updateNodesTable();
    activeNodeChanged();
    updatePatchesTable();
    activePatchChanged();
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
        Qt::ItemFlags flags;
        item = new QTableWidgetItem(QString::number(tree->treeID));
        flags = item->flags();
        flags &= ~Qt::ItemIsEditable;
        item->setFlags(flags);
        table->setItem(0, TREE_ID, item);
        item = new QTableWidgetItem();
        flags = item->flags();
        flags &= ~Qt::ItemIsEditable;
        flags &= ~Qt::ItemIsSelectable;
        item->setFlags(flags);
        item->setBackgroundColor(treeColor);
        table->setItem(0, TREE_COLOR, item);
        item = new QTableWidgetItem(QString(tree->comment));
        flags = item->flags();
        flags &= ~Qt::ItemIsSelectable;
        item->setFlags(flags);
        table->setItem(0, TREE_COMMENT, item);
    }
}

void ToolsTreeviewTab::insertNode(nodeListElement *node, KTable *table) {
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
    Qt::ItemFlags flags = item->flags();
    flags &= ~Qt::ItemIsEditable;
    item->setFlags(flags);
    table->setItem(0, NODE_ID, item);
    item = new QTableWidgetItem("");
    flags = item->flags();
    flags &= ~Qt::ItemIsSelectable;
    item->setFlags(flags);
    if(node->comment) {
        setText(table, item, node->comment->content);
    }
    table->setItem(0, NODE_COMMENT, item);
    item = new QTableWidgetItem(QString::number(node->position.x + 1));
    flags = item->flags();
    flags &= ~Qt::ItemIsSelectable;
    item->setFlags(flags);
    table->setItem(0, NODE_X, item);
    item = new QTableWidgetItem(QString::number(node->position.y + 1));
    flags = item->flags();
    flags &= ~Qt::ItemIsSelectable;
    item->setFlags(flags);
    table->setItem(0, NODE_Y, item);
    item = new QTableWidgetItem(QString::number(node->position.z + 1));
    flags = item->flags();
    flags &= ~Qt::ItemIsSelectable;
    item->setFlags(flags);
    table->setItem(0, NODE_Z, item);
    item = new QTableWidgetItem(QString::number(node->radius));
    flags = item->flags();
    flags &= ~Qt::ItemIsSelectable;
    item->setFlags(flags);
    table->setItem(0, NODE_RADIUS, item);
    if(table == nodeTable) {
        emit updateListedNodesSignal(nodeTable->rowCount());
    }
}

void ToolsTreeviewTab::insertPatch(Patch *patch, KTable *table) {
    if(patch == NULL) {
        return;
    }
    table->insertRow(0);

    QTableWidgetItem *item = new QTableWidgetItem(QString::number(patch->patchID));
    QColor treeColor = QColor(patch->correspondingTree->color.r*255,
                              patch->correspondingTree->color.g*255,
                              patch->correspondingTree->color.b*255,
                              0.6*255);
    Qt::ItemFlags flags = item->flags();
    flags &= ~Qt::ItemIsEditable;
    item->setFlags(flags);
    item->setBackgroundColor(treeColor);
    table->setItem(0, PATCH_ID, item);

    item = new QTableWidgetItem("");
    flags = item->flags();
    flags &= ~Qt::ItemIsSelectable;
    item->setFlags(flags);
    if(patch->comment.length() > 0) {
        setText(table, item, patch->comment);
    }
    table->setItem(0, PATCH_COMMENT, item);

    item = new QTableWidgetItem("show");
    if(patch->visible == false) {
        item->setText("hide");
    }
    flags = item->flags();
    flags &= ~Qt::ItemIsSelectable & ~Qt::ItemIsEditable;
    item->setFlags(flags);
    table->setItem(0, PATCH_VISIBLE, item);
}

int ToolsTreeviewTab::getActiveTreeRow() {
    int activeRow = -1;
    for(int i = 0; i < treeTable->rowCount(); ++i) {
        if(treeTable->item(i, TREE_ID)->text().toInt() == state->skeletonState->activeTree->treeID) {
            activeRow = i;
            break;
        }
    }
    return activeRow;
}

int ToolsTreeviewTab::getActiveNodeRow() {
    int activeRow = -1;
    for(int i = 0; i < nodeTable->rowCount(); ++i) {
        if((uint)nodeTable->item(i, NODE_ID)->text().toInt() == state->skeletonState->activeNode->nodeID) {
            activeRow = i;
            break;
        }
    }
    return activeRow;
}
