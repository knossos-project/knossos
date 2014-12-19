#include "treetable.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

#include "knossos-global.h"
#include "skeletonizer.h"

TreeTable::TreeTable(QWidget *parent) : QTableWidget(parent), droppedOnTreeID(0), selectionProtection(true) {}

void TreeTable::setItem(int row, int column, QTableWidgetItem *item) {
    selectionProtection = true;
    QTableWidget::setItem(row, column, item);
    selectionProtection = false;
}

void TreeTable::setRow(const int row, const QString & treeId, const QColor & treeColor, const QString & cmt) {
    auto item = new QTableWidgetItem(treeId);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(row, TreeTable::TREE_ID, item);
    item = new QTableWidgetItem();
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setBackgroundColor(treeColor);
    setItem(row, TreeTable::TREE_COLOR, item);
    setItem(row, TreeTable::TREE_COMMENT, new QTableWidgetItem(cmt));
}

void TreeTable::dropEvent(QDropEvent * event) {
    QTableWidgetItem *droppedOnItem = itemAt(event->pos());
    if(droppedOnItem == NULL or ::state->skeletonState->selectedNodes.size() == 0) {
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

