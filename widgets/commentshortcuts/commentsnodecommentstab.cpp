#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QTableWidgetItem>
#include <QHeaderView>
#include "commentsnodecommentstab.h"
#include "knossos-global.h"

extern struct stateInfo *state;

CommentsNodeCommentsTab::CommentsNodeCommentsTab(QWidget *parent) :
    QWidget(parent)
{
    branchNodesOnlyCheckbox = new QCheckBox("Branch nodes only");
    filterLabel = new QLabel("Filter:");
    filterField = new QLineEdit();    

    QTableWidgetItem *left, *right;
    left = new QTableWidgetItem(QString("Node ID"));
    right = new QTableWidgetItem(QString("Comment"));

    nodeTable = new QTableWidget();
    nodeTable->setColumnCount(2);
    nodeTable->setAlternatingRowColors(true);

    nodeTable->setHorizontalHeaderItem(0, left);    
    nodeTable->setHorizontalHeaderItem(1, right);
    nodeTable->horizontalHeader()->setStretchLastSection(true);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(branchNodesOnlyCheckbox);
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(filterLabel);
    hLayout->addWidget(filterField);
    mainLayout->addLayout(hLayout);
    mainLayout->addWidget(nodeTable);

    this->setLayout(mainLayout);
    connect(branchNodesOnlyCheckbox, SIGNAL(clicked(bool)), this, SLOT(branchPointOnlyChecked(bool)));
    connect(filterField, SIGNAL(editingFinished()), this, SLOT(filterChanged()));
    connect(nodeTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(commentChanged(QTableWidgetItem*)));
    connect(nodeTable, SIGNAL(cellClicked(int,int)), this, SLOT(itemSelected(int,int)));
    connect(nodeTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(doubleClicked(QTableWidgetItem*)));
}

void CommentsNodeCommentsTab::updateCommentsTable() {
    struct treeListElement *tree;
    struct nodeListElement *node;
    char nodeDisplay[512];
    QString filterNoCase, *commentNoCase;
    bool filtered;

    if(filterField->text().isEmpty()) {
        filtered = false;
    }

    tree = state->skeletonState->firstTree;
    const char *filter = filterField->text().toStdString().c_str();
    if(strlen(filter) > 0 )
        filtered = true;

    tableIndex = 0;
    nodeTable->clear();

    QTableWidgetItem *left, *right;
    left = new QTableWidgetItem(QString("Node ID"));
    right = new QTableWidgetItem(QString("Comment"));

    nodeTable->verticalHeader()->setVisible(false);
    nodeTable->setHorizontalHeaderItem(0, left);
    nodeTable->setHorizontalHeaderItem(1, right);
    nodeTable->horizontalHeader()->setStretchLastSection(true);

    if(this->branchNodesOnlyCheckbox->isChecked())
        nodeTable->setRowCount(state->skeletonState->totalBranchpoints);
    else
        nodeTable->setRowCount(state->skeletonState->totalComments);

    while(tree) {
        node = tree->firstNode;

        while(node) {
            if(!node->comment) {
                node = node->next;
                continue;
            }           

            if(filtered) {               
                if(strstr(node->comment->content, filter) != NULL) {
                    if(branchNodesOnlyCheckbox->isChecked() and node->isBranchNode or !branchNodesOnlyCheckbox->isChecked()) {

                        QTableWidgetItem *nodeID = new QTableWidgetItem(QString("%1").arg(node->nodeID));
                        Qt::ItemFlags flags = nodeID->flags();
                        flags |= Qt::ItemIsSelectable;
                        flags &= ~Qt::ItemIsEditable;
                        nodeID->setFlags(flags);

                        QTableWidgetItem *comment = new QTableWidgetItem(QString(node->comment->content));
                        nodeTable->setItem(tableIndex, 0, nodeID);
                        nodeTable->setItem(tableIndex, 1, comment);
                        tableIndex += 1;
                    }
                }
            } else {
                if(branchNodesOnlyCheckbox->isChecked() && node->isBranchNode or !branchNodesOnlyCheckbox->isChecked()) {
                    QTableWidgetItem *nodeID = new QTableWidgetItem(QString("%1").arg(node->nodeID));
                    Qt::ItemFlags flags = nodeID->flags();
                    flags |= Qt::ItemIsSelectable;
                    flags &= ~Qt::ItemIsEditable;
                    nodeID->setFlags(flags);

                    QTableWidgetItem *comment = new QTableWidgetItem(QString(node->comment->content));
                    nodeTable->setItem(tableIndex, 0, nodeID);
                    nodeTable->setItem(tableIndex, 1, comment);
                    tableIndex += 1;
                }
            }
            node = node->next;
        }
        tree = tree->next;
    }
}

void CommentsNodeCommentsTab::filterChanged() {
    updateCommentsTable();
}

void CommentsNodeCommentsTab::branchPointOnlyChecked(bool on) {
    updateCommentsTable();
}


void CommentsNodeCommentsTab::commentChanged(QTableWidgetItem *item) {
    if(state->skeletonState->selectedCommentNode) {
        strcpy(state->skeletonState->selectedCommentNode->comment->content, item->text().toStdString().c_str());
        emit updateTools();
    }
}

void CommentsNodeCommentsTab::itemSelected(int row, int col) {
    qDebug() << "row:" << row;
    QTableWidgetItem *nodeID = nodeTable->item(row, 0);
    QTableWidgetItem *comment = nodeTable->item(row, 1);

    if(!nodeID or !comment)
        return;

    state->skeletonState->selectedCommentNode = findNodeByNodeIDSignal(nodeID->text().toInt());
}

/** If the row of a table contains node information and is double Clicked then knossos jump to this node   */
void CommentsNodeCommentsTab::doubleClicked(QTableWidgetItem *item) {
    if(!state->skeletonState->selectedCommentNode)
       return;

    emit setActiveNodeSignal(CHANGE_MANUAL, NULL, state->skeletonState->selectedCommentNode->nodeID);
    emit setJumpToActiveNodeSignal();
}
