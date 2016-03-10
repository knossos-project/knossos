#ifndef SKELETONVIEW_H
#define SKELETONVIEW_H

#include <QAbstractListModel>
#include <QComboBox>
#include <QLabel>
#include <QMenu>
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
    int rowCount(const QModelIndex &) const override;
};

class TreeModel : public AbstractSkeletonModel<TreeModel> {
    Q_OBJECT
    friend class AbstractSkeletonModel<TreeModel>;
    const std::vector<QString> header = {"Tree ID", "Comment", ""/*color*/, "Node Count", "Render"};
    const std::vector<Qt::ItemFlags> flagModifier = {Qt::ItemIsDropEnabled, Qt::ItemIsEditable, Qt::ItemIsEditable, 0, Qt::ItemIsUserCheckable};
public:
    std::vector<std::reference_wrapper<class treeListElement>> cache;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    virtual bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) override;
    void recreate();
signals:
    void moveNodes(const QModelIndex &);
};

class NodeModel : public AbstractSkeletonModel<NodeModel> {
    friend class AbstractSkeletonModel<NodeModel>;
    const std::vector<QString> header = {"Node ID", "Comment", "x", "y", "z", "Radius"};
    const std::vector<Qt::ItemFlags> flagModifier = {Qt::ItemIsDragEnabled, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable};
public:
    std::vector<std::reference_wrapper<class nodeListElement>> cache;
    enum {
        ALL, SELECTED_TREES, SELECTED_NODES
    } mode = SELECTED_TREES;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    void recreate();
};

class NodeView : public QTreeView {
    virtual void mousePressEvent(QMouseEvent * event) override;
};

class SkeletonView : public QWidget {
    QVBoxLayout mainLayout;
    QSplitter splitter;
    QWidget treeDummyWidget;
    QVBoxLayout treeLayout;
    TreeModel treeModel;
    QWidget nodeDummyWidget;
    QVBoxLayout nodeLayout;
    QComboBox displayModeCombo;
    NodeModel nodeModel;
    QTreeView treeView;
    NodeView nodeView;

    QHBoxLayout bottomHLayout;
    QLabel treeCountLabel{"trees"};
    QLabel nodeCountLabel{"nodes"};

    QMenu treeContextMenu{&treeView};
    QMenu nodeContextMenu{&nodeView};
public:
    explicit SkeletonView(QWidget * const parent = nullptr);
};

#endif//SKELETONVIEW_H
