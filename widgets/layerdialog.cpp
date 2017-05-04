#include "layerdialog.h"

#include <QHeaderView>

int LayerItemModel::rowCount(const QModelIndex &) const {
    return 3; // tmp
}

int LayerItemModel::columnCount(const QModelIndex &) const {
    return header.size();
}

QVariant LayerItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return header[section];
    } else {
        return QVariant();
    }
}

QVariant LayerItemModel::data(const QModelIndex &index, int role) const {
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return QString("[empty]");
    }
    return QVariant();
}


LayerDialogWidget::LayerDialogWidget(QWidget *parent) : DialogVisibilityNotify(PREFERENCES_WIDGET, parent) {
    setWindowTitle("Layers");

    treeView.setModel(&itemModel);
    treeView.setDragEnabled(true);
    treeView.setAcceptDrops(true);
    treeView.resizeColumnToContents(0);
    treeView.resizeColumnToContents(1);
    treeView.resizeColumnToContents(2);
    treeView.header()->setSectionResizeMode(QHeaderView::Fixed);
    treeView.header()->setSectionsClickable(false);
    treeView.header()->setSectionsMovable(false);
    treeView.setRootIsDecorated(false);
    treeView.setUniformRowHeights(true); // for optimization
    treeView.setDragDropMode(QAbstractItemView::InternalMove);

    mainLayout.addWidget(&treeView);
    setLayout(&mainLayout);

    resize(700, 600);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}
