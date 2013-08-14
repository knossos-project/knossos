#include "commentsnodecommentstab.h"
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QTableWidgetItem>
#include <QHeaderView>
#include "skeletonizer.h"


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

    filterNoCase = filterField->text();
    char *filter = const_cast<char *>(filterNoCase.toStdString().c_str());
    if(strlen(filter) > 0 )
        filtered = true;

    tableIndex = 0;
    nodeTable->clear();

    nodeTable->setRowCount(state->skeletonState->totalNodeElements);

    while(tree) {
        node = tree->firstNode;

        while(node) {
            if(!node->comment) {
                node = node->next;
                continue;
            }           

            if(filtered) {
                if(strstr(filter, node->comment->content) != NULL) {                    
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

void CommentsNodeCommentsTab::branchPointOnlyChecked(bool on) {
    updateCommentsTable();
}

void CommentsNodeCommentsTab::commentChanged(QTableWidgetItem *item) {
    int nodeID = item->text().toInt();


}

void CommentsNodeCommentsTab::itemSelected(int row, int col) {

}

void CommentsNodeCommentsTab::doubleClicked(QTableWidgetItem *item) {


}

