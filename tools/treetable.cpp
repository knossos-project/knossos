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

void TreeTable::focusInEvent(QFocusEvent *) {
    emit focused(this);
}

void TreeTable::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Delete) {
        emit deleteTreesSignal();
    }
}

void TreeTable::dropEvent(QDropEvent *event) {
    QTableWidgetItem *droppedOnItem = itemAt(event->pos());
    droppedOnTreeID = item(droppedOnItem->row(), 0)->text().toInt();
    if(droppedOnItem == NULL or ::state->skeletonState->selectedNodes.size() == 0) {
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
        for(iter = ::state->skeletonState->selectedNodes.begin();
            iter != ::state->skeletonState->selectedNodes.end(); ++iter) {
            Skeletonizer::moveNodeToTree((*iter), droppedOnTreeID);
        }
    }
    for(int i = 0; i < ::state->skeletonState->selectedNodes.size(); ++i) {
        ::state->skeletonState->selectedNodes[i]->selected = false;
    }
    ::state->skeletonState->selectedNodes.clear();
}
