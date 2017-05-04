#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include "widgets/DialogVisibilityNotify.h"

#include <QVBoxLayout>
#include <QTreeView>
#include <QAbstractItemModel>

class LayerItemModel : public QAbstractListModel {
Q_OBJECT
protected:
    const std::vector<QString> header{"h1", "h2", "h3"};
public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
};

class LayerDialogWidget : public DialogVisibilityNotify {
    Q_OBJECT
public:
    explicit LayerDialogWidget(QWidget * parent = 0);

    QVBoxLayout mainLayout;
    LayerItemModel itemModel;
    QTreeView treeView;
};

#endif // LAYERDIALOG_H
