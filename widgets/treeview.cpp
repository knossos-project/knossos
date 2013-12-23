#include <algorithm>

#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
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

#include "skeletonizer.h"
#include "widgetcontainer.h"
#include "treeview.h"

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
TreeTable::TreeTable(QWidget *parent) : QTableWidget(parent), droppedOnTreeID(0) {}

void TreeTable::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Delete) {
        deleteTreesAction();
    }
}

void TreeTable::dropEvent(QDropEvent *event) {
    QTableWidgetItem *droppedOnItem = itemAt(event->pos());
    droppedOnTreeID = item(droppedOnItem->row(), 0)->text().toInt();
    qDebug("dropped on tree %i", droppedOnTreeID);
    /* TDITEM: implement the movement from nodes of one tree to another tree
    if(droppedOnItem == NULL or state->skeletonState->selectedNodes.size() == 0) {
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
        for(iter = state->skeletonState->selectedNodes.begin();
            iter != state->skeletonState->selectedNodes.end(); ++iter) {
            //
        }
    }*/
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
        Skeletonizer::deleteSelectedTrees();
        emit updateTreeview();
    }
}
// node table
NodeTable::NodeTable(QWidget *parent) : QTableWidget(parent) {}

void NodeTable::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Delete) {
        deleteNodesAction();
    }
}

void NodeTable::deleteNodesAction() {
    qDebug("deleting");
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
        emit updateNodesTable();
    }
}

// treeview
Treeview::Treeview(QWidget *parent) :
    QDialog(parent), draggedNodeID(0), radiusBuffer(state->skeletonState->defaultNodeRadius), displayedNodes(1000)
{
    kState = state;

    setWindowTitle("Tree View");
    treeSearchField = new QLineEdit();
    treeSearchField->setPlaceholderText("search tree");
    nodeSearchField = new QLineEdit();
    nodeSearchField->setPlaceholderText("search node");
    treeRegExCheck = new QCheckBox("RegEx");
    treeRegExCheck->setToolTip("search by regular expression");
    nodeRegExCheck = new QCheckBox("RegEx");
    nodeRegExCheck->setToolTip("search by regular expression");
    nodesOfSelectedTreesRadio = new QRadioButton("nodes of selected trees");
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

    QTableWidgetItem *IDCol, *commentCol, *rCol, *gCol, *bCol, *aCol,
                     *radiusCol, *xCol, *yCol, *zCol;
    treeTable = new TreeTable(this);
    treeTable->setColumnCount(TREE_COLS);
    treeTable->setAlternatingRowColors(true);
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
    //treeTable->setDragDropMode(QAbstractItemView::DropOnly);

    nodeTable = new NodeTable(this);
    nodeTable->setColumnCount(NODE_COLS);
    nodeTable->setAlternatingRowColors(true);
    IDCol = new QTableWidgetItem("Node ID");
    commentCol = new QTableWidgetItem("Comment");
    xCol = new QTableWidgetItem("x");
    zCol = new QTableWidgetItem("y");
    yCol = new QTableWidgetItem("z");
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
    //nodeTable->setDragEnabled(true);
    //nodeTable->setDragDropMode(QAbstractItemView::DragOnly);
    createTreesContextMenu();
    createNodesContextMenu();
    createContextMenuDialogs();

    mainLayout = new QHBoxLayout();
    QVBoxLayout *vLayout = new QVBoxLayout();
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(treeSearchField);
    hLayout->addWidget(treeRegExCheck);
    vLayout->addLayout(hLayout);
    vLayout->addWidget(treeTable);
    mainLayout->addLayout(vLayout);

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
    vLayout->addWidget(nodeTable);
    mainLayout->addLayout(vLayout);
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

    connect(treeTable, SIGNAL(updateTreeview()), this, SLOT(update()));
    connect(nodeTable, SIGNAL(updateNodesTable()), this, SLOT(updateNodesTable()));
    connect(treeTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(treeContextMenuCalled(QPoint)));
    connect(treeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(treeItemChanged(QTableWidgetItem*)));
    connect(treeTable, SIGNAL(itemSelectionChanged()), this, SLOT(treeItemSelected()));

    connect(nodeTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(nodeContextMenuCalled(QPoint)));
    connect(nodeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(nodeItemChanged(QTableWidgetItem*)));
    connect(nodeTable, SIGNAL(itemSelectionChanged()), this, SLOT(nodeItemSelected()));
    connect(nodeTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(nodeItemDoubleClicked(QTableWidgetItem*)));
}

void Treeview::createTreesContextMenu() {
    treeContextMenu = new QMenu();
    treeContextMenu->addAction("set as active tree", this, SLOT(setActiveTreeAction()));
    treeContextMenu->addAction("set comment for tree(s)", this, SLOT(setTreeCommentAction()));
    treeContextMenu->addAction("merge trees", this, SLOT(mergeTreesAction()));
    treeContextMenu->addAction("restore default color", this, SLOT(restoreColorAction()));
    treeContextMenu->addAction(QIcon(":/images/icons/user-trash.png"), "delete tree(s)", treeTable, SLOT(deleteTreesAction()));
}

void Treeview::createNodesContextMenu() {
    nodeContextMenu = new QMenu();
    nodeContextMenu->addAction("set comment for node(s)", this, SLOT(setNodeCommentAction()));
    nodeContextMenu->addAction("set radius for node(s)", this, SLOT(setNodeRadiusAction()));
    nodeContextMenu->addAction("Split component from tree", this, SLOT(splitComponentAction()));
    nodeContextMenu->addAction(QIcon(":/images/icons/user-trash.png"), "delete node(s)", nodeTable, SLOT(deleteNodesAction()));

}

void Treeview::createContextMenuDialogs() {
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

    // for node table
    nodeCommentEditDialog = new QDialog();
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

    connect(nodeCommentField, SIGNAL(textChanged(QString)), this, SLOT(updateNodeCommentBuffer(QString)));
    connect(nodeCommentApplyButton, SIGNAL(clicked()), this, SLOT(editNodeComments()));
    connect(nodeCommentCancelButton, SIGNAL(clicked()), nodeCommentEditDialog, SLOT(hide()));
    connect(radiusSpin, SIGNAL(valueChanged(double)), this, SLOT(updateNodeRadiusBuffer(double)));
    connect(nodeRadiusApplyButton, SIGNAL(clicked()), this, SLOT(editNodeRadii()));
    connect(nodeRadiusCancelButton, SIGNAL(clicked()), nodeRadiusEditDialog, SLOT(hide()));
}


// tree context menu
void Treeview::treeContextMenuCalled(QPoint pos) {
    treeContextMenu->move(treeTable->mapToGlobal(pos));
    treeContextMenu->show();
}

void Treeview::setActiveTreeAction() {
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
        update();
    }
}

bool Treeview::mergeTreesAction() {
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
            Skeletonizer::mergeTrees(CHANGE_MANUAL, state->skeletonState->selectedTrees.at(0)->treeID,
                                     state->skeletonState->selectedTrees.at(1)->treeID, true);
            update();
        }
    }
}

void Treeview::restoreColorAction() {
    if(state->skeletonState->selectedTrees.size() == 0) {
        return;
    }
    std::vector<treeListElement *>::iterator iter;
    for(iter = state->skeletonState->selectedTrees.begin();
        iter != state->skeletonState->selectedTrees.end(); ++iter) {
        Skeletonizer::restoreDefaultTreeColor(*iter);
    }
    updateTreesTable();
}

void Treeview::setTreeCommentAction() {
    if(state->skeletonState->selectedTrees.size() == 0) {
        return;
    }
    treeCommentField->setText(treeCommentBuffer);
    treeCommentEditDialog->show();
}

void Treeview::updateTreeCommentBuffer(QString comment) {
    treeCommentBuffer = comment;
}

void Treeview::editTreeComments() {
    if(state->skeletonState->selectedTrees.size() == 0) {
        return;
    }
    std::vector<treeListElement *>::iterator iter;
    for(iter = state->skeletonState->selectedTrees.begin();
        iter != state->skeletonState->selectedTrees.end(); ++iter) {
        Skeletonizer::addTreeComment(CHANGE_MANUAL, (*iter)->treeID, treeCommentBuffer.toLocal8Bit().data());
    }
    treeCommentEditDialog->hide();
    updateTreesTable();
}

//bool Treeview::deleteTreesAction() {
//    if(state->skeletonState->selectedTrees.size() == 0) {
//        return false;
//    }
//    QMessageBox prompt;
//    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
//    prompt.setIcon(QMessageBox::Question);
//    prompt.setWindowTitle("Cofirmation required");
//    prompt.setText("Do you really want to delete selected tree(s)?");
//    QPushButton *confirmButton = prompt.addButton("Delete", QMessageBox::ActionRole);
//    prompt.addButton("Cancel", QMessageBox::ActionRole);
//    prompt.exec();
//    if(prompt.clickedButton() == confirmButton) {
//        Skeletonizer::deleteSelectedTrees();
//        update();
//    }
//}

// node context menu
void Treeview::nodeContextMenuCalled(QPoint pos) {
    nodeContextMenu->move(nodeTable->mapToGlobal(pos));
    nodeContextMenu->show();
}

void Treeview::setNodeRadiusAction() {
    if(state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    radiusSpin->setValue(radiusBuffer);
    nodeRadiusEditDialog->show();
}

void Treeview::updateNodeRadiusBuffer(double value) {
    radiusBuffer = value;
}

void Treeview::editNodeRadii() {
    if(state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    std::vector<nodeListElement *>::iterator iter;
    for(iter = state->skeletonState->selectedNodes.begin();
        iter != state->skeletonState->selectedNodes.end(); ++iter) {
        Skeletonizer::editNode(CHANGE_MANUAL, 0, *iter, radiusBuffer,
                               (*iter)->position.x, (*iter)->position.y, (*iter)->position.z, (*iter)->createdInMag);
    }
    nodeRadiusEditDialog->hide();
    updateNodesTable();
}

bool Treeview::splitComponentAction() {
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
            Skeletonizer::splitConnectedComponent(CHANGE_MANUAL, state->skeletonState->selectedNodes.at(0)->nodeID, true);
            update();
        }
    }
}

void Treeview::setNodeCommentAction() {
    nodeCommentField->setText(nodeCommentBuffer);
    nodeCommentEditDialog->show();
}

void Treeview::updateNodeCommentBuffer(QString comment) {
    nodeCommentBuffer = comment;
}

void Treeview::editNodeComments() {
    if(state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    std::vector<nodeListElement *>::iterator iter;
    for(iter = state->skeletonState->selectedNodes.begin();
        iter != state->skeletonState->selectedNodes.end(); ++iter) {
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


void Treeview::treeSearchChanged() {
    if(treeRegExCheck->isChecked() and treeSearchField->text().length() > 0) {
        QRegularExpression regex(treeSearchField->text());
        if(regex.isValid() == false) {
            QToolTip::showText(treeSearchField->mapToGlobal(treeSearchField->pos()), "Invalid regular expression.");
           // return;
        }
    }
    updateTreesTable();
}
void Treeview::nodeSearchChanged() {
    if(nodeRegExCheck->isChecked() and nodeSearchField->text().length() > 0) {
        QRegularExpression regex(nodeSearchField->text());
        if(regex.isValid() == false) {
            QToolTip::showText(nodeSearchField->mapToGlobal(nodeSearchField->pos()), "Invalid regular expression.");
            return;
        }
    }
    updateNodesTable();
}

void Treeview::displayedNodesChanged(int index) {
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
    else if(displayedNodes < nodeTable->rowCount() and displayedNodes != 0) {
        nodeTable->setRowCount(displayedNodes);
    }
    else {
        updateNodesTable();
    }
}

void Treeview::treeItemChanged(QTableWidgetItem* item) {
    QTableWidgetItem *idItem = treeTable->item(item->row(), TREE_ID);
    treeListElement *selectedTree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
    if(selectedTree == NULL) {
        return;
    }
    float value;
    switch(item->column()) {
    case TREE_R:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            selectedTree->color.r = value;
            updateTreeColorCells(item->row());
        }
        else {
            item->setText(QString::number(selectedTree->color.r));
        }
        break;
    case TREE_G:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            selectedTree->color.g = value;
            updateTreeColorCells(item->row());
        }
        else {
            item->setText(QString::number(selectedTree->color.g));
        }
        break;
    case TREE_B:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            selectedTree->color.b = value;
            updateTreeColorCells(item->row());
        }
        else {
            item->setText(QString::number(selectedTree->color.b));
        }
        break;
    case TREE_A:
        value = item->text().toFloat();
        if(value >= 0 and value <= 1.) {
            selectedTree->color.a = value;
            updateTreeColorCells(item->row());
        }
        else {
            item->setText(QString::number(selectedTree->color.a));
        }
        break;
    case TREE_COMMENT:
        Skeletonizer::addTreeComment(CHANGE_MANUAL, selectedTree->treeID, item->text().toLocal8Bit().data());
        break;
    default:
        break;
    }
    state->skeletonState->selectedTrees.clear();
    emit updateToolsSignal();
}

void Treeview::nodeItemChanged(QTableWidgetItem* item) {
    QTableWidgetItem *idItem = nodeTable->item(item->row(), NODE_ID);

    nodeListElement *selectedNode = Skeletonizer::findNodeByNodeID(idItem->text().toInt());
    if(selectedNode == NULL) {
        return;
    }
    switch(item->column()) {
    case NODE_COMMENT:
        if(selectedNode->comment) {
            if(item->text().length() == 0) {
                Skeletonizer::delComment(CHANGE_MANUAL, selectedNode->comment, 0, true);
                updateNodesTable(); // node might disappear in table, if only comment nodes are displayed
                break;
            }
            Skeletonizer::editComment(CHANGE_MANUAL, selectedNode->comment, 0,
                                      item->text().toLocal8Bit().data(), selectedNode, 0, true);
        }
        else {
            if(item->text().length() == 0) {
                break;
            }
            Skeletonizer::addComment(CHANGE_MANUAL, item->text().toLocal8Bit().data(), selectedNode, 0, true);
        }
        break;
    case NODE_X:
        if(item->text().toInt() < 1) { // out of bounds
            item->setText(QString::number(selectedNode->position.x + 1));
        } else {
            selectedNode->position.x = item->text().toInt() - 1;
        }
        break;
    case NODE_Y:
        if(item->text().toInt() < 1) { // out of bounds
            item->setText(QString::number(selectedNode->position.y + 1));
        } else {
            selectedNode->position.y = item->text().toInt() - 1;
        }
        break;
    case NODE_Z:
        if(item->text().toInt() < 1) { // out of bounds
            item->setText(QString::number(selectedNode->position.z + 1));
        } else {
            selectedNode->position.z = item->text().toInt() - 1;
        }
        break;
    case NODE_RADIUS:
        selectedNode->radius = item->text().toFloat();
        break;
    default:
        break;
    }
    state->skeletonState->selectedNodes.clear();
    emit updateToolsSignal();
}

void Treeview::treeItemSelected() {
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

void Treeview::nodeItemSelected() {
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
void Treeview::nodeItemDoubleClicked(QTableWidgetItem* item) {
    if(state->skeletonState->selectedNodes.size() != 1) {
        return;
    }
    emit setActiveNodeSignal(CHANGE_MANUAL, NULL, state->skeletonState->selectedNodes.at(0)->nodeID);
    emit JumpToActiveNodeSignal();
    updateNodesTable();
    state->skeletonState->selectedNodes.clear();
}

void Treeview::updateTreeColorCells(int row) {
    QTableWidgetItem *idItem = treeTable->item(row, TREE_ID);
    treeListElement *tree = Skeletonizer::findTreeByTreeID(idItem->text().toInt());
    if(tree == NULL) {
        return;
    }
    QColor treeColor = QColor(tree->color.r*255, tree->color.g*255, tree->color.b*255, 153);
    QTableWidgetItem *colorItem = treeTable->item(row, TREE_R);
    if(colorItem) {
        colorItem->setBackgroundColor(treeColor);
    }
    colorItem = treeTable->item(row, TREE_G);
    if(colorItem) {
        colorItem->setBackgroundColor(treeColor);
    }
    colorItem = treeTable->item(row, TREE_B);
    if(colorItem) {
        colorItem->setBackgroundColor(treeColor);
    }
    colorItem = treeTable->item(row, TREE_A);
    if(colorItem) {
        colorItem->setBackgroundColor(treeColor);
    }
}

bool Treeview::matchesSearchString(QString searchString, QString string, bool useRegEx) {
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

void Treeview::updateTreesTable() {
    treeTable->clearContents();
    treeTable->setRowCount(state->skeletonState->treeElements);
    treeTable->clearSpans();
    treeTable->verticalHeader()->setVisible(false);

    treeListElement *currentTree = state->skeletonState->firstTree;
    if(currentTree == NULL) { 
        return;
    }

    QTableWidgetItem *item;
    QTableWidgetItem *actIDItem = NULL,
                     *actRItem = NULL, *actGItem = NULL, *actBItem = NULL, *actAItem = NULL, *actCommentItem = NULL;
    uint treeIndex = 1; // save row 0 for active tree
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
        if(currentTree == state->skeletonState->activeTree) {
            item->setBackground(QColor(255, 0, 0, 153));
            actIDItem = item;
        }
        else {
            treeTable->setItem(treeIndex, TREE_ID, item);
        }
        // tree color
        QColor treeColor = QColor(currentTree->color.r*255,
                                  currentTree->color.g*255,
                                  currentTree->color.b*255,
                                  0.6*255);
        item = new QTableWidgetItem(QString::number(currentTree->color.r, 'g', 2)); // precision of 2
        item->setBackgroundColor(treeColor);
        if(currentTree == state->skeletonState->activeTree) {
            actRItem = item;
        } else {
            treeTable->setItem(treeIndex, TREE_R, item);
        }
        item = new QTableWidgetItem(QString::number(currentTree->color.g, 'g', 2));
        item->setBackgroundColor(treeColor);
        if(currentTree == state->skeletonState->activeTree) {
                actGItem = item;
        } else {
            treeTable->setItem(treeIndex, TREE_G, item);
        }
        item = new QTableWidgetItem(QString::number(currentTree->color.b, 'g', 2));
        item->setBackgroundColor(treeColor);
        if(currentTree == state->skeletonState->activeTree) {
            actBItem = item;
        } else {
            treeTable->setItem(treeIndex, TREE_B, item);
        }
        item = new QTableWidgetItem(QString::number(currentTree->color.a, 'g', 2));
        item->setBackgroundColor(treeColor);
        if(currentTree == state->skeletonState->activeTree) {
            actAItem = item;
        } else {
            treeTable->setItem(treeIndex, TREE_A, item);
        }
        // tree comment
        if(currentTree->comment) {
            item = new QTableWidgetItem(QString(currentTree->comment));
        }
        if(currentTree == state->skeletonState->activeTree) {
            actCommentItem = item;
        } else {
            treeTable->setItem(treeIndex, TREE_COMMENT, item);
        }
        if(currentTree != state->skeletonState->activeTree) {
            treeIndex++;
        }
        currentTree = currentTree->next;
    }
    if(actIDItem != NULL) {
        treeTable->setItem(0, TREE_ID, actIDItem);
        treeTable->setItem(0, TREE_R, actRItem);
        treeTable->setItem(0, TREE_G, actGItem);
        treeTable->setItem(0, TREE_B, actBItem);
        treeTable->setItem(0, TREE_A, actAItem);
        treeTable->setItem(0, TREE_COMMENT, actCommentItem);
    }
    else {
        actIDItem = new QTableWidgetItem("No active tree");
        treeTable->setItem(0, 0, actIDItem);
        treeTable->setSpan(0, 0, 1, TREE_COLS);
    }

    // resize every column to size of content, to make them small,
    // omit comment column, because it might become large
    treeTable->resizeColumnToContents(TREE_R);
    treeTable->resizeColumnToContents(TREE_G);
    treeTable->resizeColumnToContents(TREE_B);
    treeTable->resizeColumnToContents(TREE_A);
}

void Treeview::updateNodesTable() {
    nodeTable->clearContents();
    nodeTable->clearSpans();

    if(displayedNodes > state->skeletonState->totalNodeElements or displayedNodes == DISPLAY_ALL) {
        nodeTable->setRowCount(state->skeletonState->totalNodeElements);
    }
    else {
        nodeTable->setRowCount(displayedNodes);
    }
    nodeTable->verticalHeader()->setVisible(false);

    treeListElement *currentTree = state->skeletonState->firstTree;
    if(currentTree == NULL) {
        return;
    }
    QTableWidgetItem *item;
    QTableWidgetItem *actIDItem = NULL, *actCommentItem = NULL,
                     *actXItem = NULL, *actYItem = NULL, *actZItem = NULL, *actRadiusItem = NULL;
    uint nodeIndex = 1; // save row 1 for active node
    nodeListElement *node;
    while(currentTree) {
        node = currentTree->firstNode;
        while(node and nodeIndex <= displayedNodes or node and displayedNodes == DISPLAY_ALL) {
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
            if(node == state->skeletonState->activeNode) { // mark as active node
                item->setBackgroundColor(QColor(255, 0, 0, 153));
                actIDItem = item;
            }
            else {
                nodeTable->setItem(nodeIndex, NODE_ID, item);
            }
            if(node->comment) {
                item = new QTableWidgetItem(node->comment->content);
                if(node == state->skeletonState->activeNode) {
                    actCommentItem = item;
                }
                else {
                    nodeTable->setItem(nodeIndex, NODE_COMMENT, item);
                }
            }
            item = new QTableWidgetItem(QString::number(node->position.x + 1));
            if(node == state->skeletonState->activeNode) {
                actXItem = item;
            }
            else {
                nodeTable->setItem(nodeIndex, NODE_X, item);
            }
            item = new QTableWidgetItem(QString::number(node->position.y + 1));
            if(node == state->skeletonState->activeNode) {
                actYItem = item;
            }
            else {
                nodeTable->setItem(nodeIndex, NODE_Y, item);
            }
            item = new QTableWidgetItem(QString::number(node->position.z + 1));
            if(node == state->skeletonState->activeNode) {
                actZItem = item;
            }
            else {
                nodeTable->setItem(nodeIndex, NODE_Z, item);
            }
            item = new QTableWidgetItem(QString::number(node->radius));
            if(node == state->skeletonState->activeNode) {
                actRadiusItem = item;
            }
            else {
                nodeTable->setItem(nodeIndex, NODE_RADIUS, item);
            }
            if(node != state->skeletonState->activeNode) {
                nodeIndex++;
            }
            node = node->next;
        }
        currentTree = currentTree->next;
    }
    // put active node row on top of the table
    if(actIDItem != NULL) {
        nodeTable->setItem(0, NODE_ID, actIDItem);
        nodeTable->setItem(0, NODE_COMMENT, actCommentItem);
        nodeTable->setItem(0, NODE_X, actXItem);
        nodeTable->setItem(0, NODE_Y, actYItem);
        nodeTable->setItem(0, NODE_Z, actZItem);
        nodeTable->setItem(0, NODE_RADIUS, actRadiusItem);
    }
    else {
        actIDItem = new QTableWidgetItem("no active node in current selection");
        nodeTable->setItem(0, NODE_ID, actIDItem);
        nodeTable->setSpan(0, NODE_ID, 1, NODE_COLS);
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
}

void Treeview::update() {
    updateTreesTable();
    updateNodesTable();
}
