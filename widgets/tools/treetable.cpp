#include "treetable.h"

#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

TreeTable::TreeTable(QWidget *parent) : QTableWidget(parent), droppedOnTreeID(0), selectionProtection(true) {}

void TreeTable::setItem(int row, int column, QTableWidgetItem *item) {
    selectionProtection = true;
    QTableWidget::setItem(row, column, item);
    selectionProtection = false;
}

void TreeTable::setRow(const int row, const QString & treeId, const QColor & treeColor, const QString & cmt) {
    this->setRowHeight(row, 34);

    auto item = new QTableWidgetItem(treeId);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(row, TreeTable::TREE_ID, item);
    item = new QTableWidgetItem();
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setBackgroundColor(treeColor);
    setItem(row, TreeTable::TREE_COLOR, item);
    setItem(row, TreeTable::TREE_COMMENT, new QTableWidgetItem(cmt));
    QWidget *pWidget = new QWidget();
    QCheckBox *pCheckBox = new QCheckBox();
    pCheckBox->setChecked(Skeletonizer::singleton().findTreeByTreeID(treeId.toInt())->render);
    QHBoxLayout *pLayout = new QHBoxLayout(pWidget);
    pLayout->addWidget(pCheckBox);
    pLayout->setAlignment(Qt::AlignCenter);
    pWidget->setLayout(pLayout);

    QTableWidget::setCellWidget(row, TreeTable::TREE_COMMENT + 1, pWidget);

    auto rowFromCell = [this](int column, QCheckBox * const checkbox) {
        for(int row = 0; row < this->rowCount(); ++row) {
            if(checkbox == this->cellWidget(row, column)->layout()->itemAt(0)->widget()) {
                return row;
            }

        }

        return -1;
    };

    QObject::connect(pCheckBox, &QCheckBox::clicked, [this, rowFromCell, pCheckBox](){
        const int row = rowFromCell(TreeTable::TREE_RENDER, pCheckBox);
        if(row == -1) {
            qDebug() << "Checkbox couldn't be located!";
            return;
        }

        const int treeID = this->item(row, 0)->text().toInt();
        auto a = Skeletonizer::singleton().findTreeByTreeID(treeID);
        a->render = pCheckBox->isChecked();

        emit cellClicked(row, TreeTable::TREE_RENDER);
    });
}

void TreeTable::dropEvent(QDropEvent * event) {
    QTableWidgetItem *droppedOnItem = itemAt(event->pos());
    if (droppedOnItem == nullptr || ::state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    droppedOnTreeID = item(droppedOnItem->row(), 0)->text().toInt();

    QMessageBox prompt;
    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
    prompt.setIcon(QMessageBox::Question);
    prompt.setWindowTitle("Cofirmation required");
    prompt.setText(QString("Do you really want to move selected nodes to tree %1?").arg(droppedOnTreeID));
    QPushButton *confirmButton = prompt.addButton("Move", QMessageBox::ActionRole);
    prompt.addButton("Cancel", QMessageBox::ActionRole);
    prompt.exec();
    if(prompt.clickedButton() == confirmButton) {
        Skeletonizer::singleton().moveSelectedNodesToTree(droppedOnTreeID);
    }
    event->accept();
    //this prevents the items in the table from being draggable after the drop
    //setState(DraggingState) is set in the default dragEnterEvent
    setState(NoState);
}

void TreeTable::keyPressEvent(QKeyEvent * event) {
    QTableWidget::keyPressEvent(event);
    if(event->key() == Qt::Key_Delete) {
        emit deleteTreesSignal();
    }
}

