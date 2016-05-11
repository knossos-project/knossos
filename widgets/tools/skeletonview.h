#ifndef SKELETONVIEW_H
#define SKELETONVIEW_H

#include "widgets/Spoiler.h"
#include "widgets/UserOrientableSplitter.h"

#include <QAbstractListModel>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

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
    const std::vector<QString> header = {"ID", ""/*color*/, "Show", "#", "Comment"};
    const std::vector<Qt::ItemFlags> flagModifier = {Qt::ItemIsDropEnabled, 0, Qt::ItemIsUserCheckable, 0, Qt::ItemIsEditable};
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
    const std::vector<QString> header = {"ID", "x", "y", "z", "Radius", "Comment"};
    const std::vector<Qt::ItemFlags> flagModifier = {Qt::ItemIsDragEnabled, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable, Qt::ItemIsEditable};
public:
    std::vector<std::reference_wrapper<class nodeListElement>> cache;
    enum {
        ALL, PART_OF_SELECTED_TREE, SELECTED, BRANCH, NON_EMPTY_COMMENT
    } mode = PART_OF_SELECTED_TREE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    void recreate();
};

class NodeView : public QTreeView {
    QSortFilterProxyModel & proxy;
    NodeModel & source;
    virtual void mousePressEvent(QMouseEvent * event) override;
public:
    NodeView(QSortFilterProxyModel & proxy, NodeModel & source) : proxy{proxy}, source{source} {}
};

class SkeletonView : public QWidget {
    friend class AnnotationWidget;

    QVBoxLayout mainLayout;
    UserOrientableSplitter splitter;

    QWidget treeDummyWidget;
    QVBoxLayout treeLayout;
    QHBoxLayout treeOptionsLayout;
    QLineEdit treeCommentFilter;
    QCheckBox treeRegex{"regex"};
    TreeModel treeModel;
    QSortFilterProxyModel & treeSortAndCommentFilterProxy;
    int treeSortSectionIndex{-1};
    QTreeView treeView;
    QLabel treeCountLabel;

    QWidget nodeDummyWidget;
    QVBoxLayout nodeLayout;
    QHBoxLayout nodeOptionsLayout;
    QComboBox displayModeCombo;
    QLineEdit nodeCommentFilter;
    QCheckBox nodeRegex{"regex"};
    NodeModel nodeModel;
    QSortFilterProxyModel & nodeSortAndCommentFilterProxy;
    int nodeSortSectionIndex{-1};
    NodeView nodeView;
    QLabel nodeCountLabel;

    QMenu treeContextMenu{&treeView};
    QMenu nodeContextMenu{&nodeView};

    Spoiler commandsBox{"Commands"};
    QGridLayout commandsLayout;
    QLabel defaultRadiusLabel{tr("Default node radius:")};
    QDoubleSpinBox defaultRadiusSpin;

    QLabel lockingLabel{"<strong>Locking</strong>"};
    QCheckBox commentLockingCheck{tr("Lock to nodes with comment:")};
    QLineEdit commentLockEdit{"seed"};
    QLabel lockingRadiusLabel{tr("Locking radius:")};
    QSpinBox lockingRadiusSpin;
    QPushButton lockToActiveButton{tr("Lock to active node")};
    QPushButton disableCurrentLockingButton{tr("Disable current locking")};

public:
    explicit SkeletonView(QWidget * const parent = nullptr);
    QString getFilterComment() const;
};

#endif//SKELETONVIEW_H
