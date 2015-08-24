#ifndef SKELETONVIEW_H
#define SKELETONVIEW_H

#include <QAbstractListModel>
#include <QLabel>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>

template<typename ConcreteModel>
class AbstractSkeletonModel : public QAbstractListModel {
public:
    bool selectionProtection{false};
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
};

class TreeModel : public AbstractSkeletonModel<TreeModel> {
    friend class AbstractSkeletonModel<TreeModel>;
    const std::vector<QString> header = {"Tree ID", "Comment", ""/*color*/, "Node Count", "Render"};
    const std::vector<Qt::ItemFlags> flagModifier = {0, Qt::ItemIsEditable, Qt::ItemIsEditable, 0, Qt::ItemIsUserCheckable};
public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    void recreate();
};

class NodeModel : public AbstractSkeletonModel<NodeModel> {
    friend class AbstractSkeletonModel<NodeModel>;
    const std::vector<QString> header = {"Node ID", "Comment", "x", "y", "z", "Radius"};
    const std::vector<Qt::ItemFlags> flagModifier = {0, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable};
public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    void recreate();
};

class SkeletonView : public QWidget {
    QVBoxLayout mainLayout;
    QSplitter splitter;
    TreeModel treeModel;
    NodeModel nodeModel;
    QTreeView treeView;
    QTreeView nodeView;

    QHBoxLayout bottomHLayout;
    QLabel treeCountLabel{"trees"};
    QLabel nodeCountLabel{"nodes"};
public:
    explicit SkeletonView(QWidget * const parent = nullptr);
};

#endif//SKELETONVIEW_H
