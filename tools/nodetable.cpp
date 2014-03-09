#include "nodetable.h"

#include <QKeyEvent>

NodeTable::NodeTable(QWidget *parent) : QTableWidget(parent), selectionProtection(true) {}

void NodeTable::setItem(int row, int column, QTableWidgetItem *item) {
    selectionProtection = true;
    QTableWidget::setItem(row, column, item);
    selectionProtection = false;
}

void NodeTable::focusInEvent(QFocusEvent *) {
    emit focused(this);
}

void NodeTable::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Delete) {
        emit deleteNodesSignal();
    }
}