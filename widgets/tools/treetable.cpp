#include "treetable.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

#include "knossos-global.h"
#include "skeletonizer.h"

extern stateInfo *state;

TreeTable::TreeTable(QWidget *parent) : QTableWidget(parent), droppedOnTreeID(0), selectionProtection(true) {}

void TreeTable::setItem(int row, int column, QTableWidgetItem *item) {
    if(item != NULL) {
        selectionProtection = true;
        QTableWidget::setItem(row, column, item);
        selectionProtection = false;
    }
}

void TreeTable::dropEvent(QDropEvent *event) {
    QTableWidgetItem *droppedOnItem = itemAt(event->pos());
    if(droppedOnItem == NULL or ::state->skeletonState->selectedNodes.size() == 0) {
        return;
    }
    droppedOnTreeID = item(droppedOnItem->row(), 0)->text().toInt();

    QMessageBox prompt;
    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
    prompt.setIcon(QMessageBox::Question);
    prompt.setWindowTitle("Cofirmation required");
    prompt.setText(QString("Do you really want to move selected nodes to tree %1?").
                            arg(droppedOnTreeID));
    QPushButton *confirmButton = prompt.addButton("Move", QMessageBox::ActionRole);
    prompt.addButton("Cancel", QMessageBox::ActionRole);
    prompt.exec();
    if(prompt.clickedButton() == confirmButton) {
        for (auto * const node : ::state->skeletonState->selectedNodes) {
            Skeletonizer::moveNodeToTree(node, droppedOnTreeID);
        }
    }
    emit updateNodesTable();
}

void TreeTable::focusInEvent(QFocusEvent *) {
    emit focused(this);
}

void TreeTable::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Delete) {
        emit deleteTreesSignal();
    }
}

