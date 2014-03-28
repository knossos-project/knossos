#include "nodetable.h"

#include <QKeyEvent>
#include <QTableWidget>

NodeTable::NodeTable(QWidget *parent) : QTableWidget(parent), selectionProtection(true) {}

void NodeTable::resizeToFit() {
    // resize every column to size of content, to make them small,
    // omit comment column, because it might become large
    resizeColumnToContents(NODE_ID);
    resizeColumnToContents(NODE_X);
    resizeColumnToContents(NODE_Y);
    resizeColumnToContents(NODE_Z);
    resizeColumnToContents(NODE_RADIUS);
}

void NodeTable::setItem(int row, int column, QTableWidgetItem *item) {
    selectionProtection = true;
    QTableWidget::setItem(row, column, item);
    selectionProtection = false;
}

void NodeTable::setRow(const int row, const QString &nodeId, const QString &cmt, const QString &x, const QString &y, const QString &z, const QString &radius) {
    auto item = new QTableWidgetItem(nodeId);
    auto flags = item->flags();
    flags &= ~Qt::ItemIsEditable;
    item->setFlags(flags);
    setItem(row, NODE_ID, item);
    item = new QTableWidgetItem(cmt);
    setItem(row, NODE_COMMENT, item);
    item = new QTableWidgetItem(x);
    setItem(row, NODE_X, item);
    item = new QTableWidgetItem(y);
    setItem(row, NODE_Y, item);
    item = new QTableWidgetItem(z);
    setItem(row, NODE_Z, item);
    item = new QTableWidgetItem(radius);
    setItem(row, NODE_RADIUS, item);
}

void NodeTable::keyPressEvent(QKeyEvent * event) {
    QTableWidget::keyPressEvent(event);
    if(event->key() == Qt::Key_Delete) {
        emit deleteNodesSignal();
    }
}

void NodeTable::mouseReleaseEvent(QMouseEvent * event) {
    QTableWidget::mouseReleaseEvent(event);//invoke standard behaviour
    setDragEnabled(true);//re-enable dragging after multi-selection
}
