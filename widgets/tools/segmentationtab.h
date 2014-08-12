#ifndef SEGMENTATIONTAB_H
#define SEGMENTATIONTAB_H

#include "segmentation.h"

#include <QAbstractListModel>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>

class SegmentationObjectModel : public QAbstractListModel {
Q_OBJECT
    friend class SegmentationTab;//selection
protected:
    const std::vector<QString> header{"", "Object ID", "lock", "category", "comment", "#", "subobject IDs"};
    const std::size_t MAX_SHOWN_SUBOBJECTS = 10;
public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant objectGet(const Segmentation::Object & obj, const QModelIndex & index, int role) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool objectSet(Segmentation::Object & obj, const QModelIndex & index, const QVariant & value, int role);
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
    void recreate();
    void appendRowBegin();
    void popRowBegin();
    void appendRow();
    void popRow();
    void changeRow(int index);
};

class TouchedObjectModel : public SegmentationObjectModel {
    Q_OBJECT
public:
    std::vector<std::reference_wrapper<Segmentation::Object>> objectCache;
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    void recreate();
};

class CategoryModel : public QAbstractListModel {
    Q_OBJECT
    std::vector<QString> categories;
public:
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    void recreate();
};

class SegmentationTab : public QWidget {
Q_OBJECT
    QVBoxLayout layout;
    QHBoxLayout filterLayout;
    QSplitter splitter;
    QComboBox categoryFilter;
    QLineEdit commentFilter;
    QCheckBox regExCheckbox{"RegEx"};
    CategoryModel categoryModel;
    SegmentationObjectModel objectModel;
    QSortFilterProxyModel objectProxyModelCategory;
    QSortFilterProxyModel objectProxyModelComment;
    TouchedObjectModel touchedObjectModel;
    QCheckBox showAllChck{"Show all objects"};
    QTreeView touchedObjsTable;
    QTreeView objectsTable;
    QLabel objectCountLabel;
    QLabel subobjectCountLabel;
    QHBoxLayout bottomHLayout;
    bool objectSelectionProtection = false;
    bool touchedObjectSelectionProtection = false;
public:
    explicit SegmentationTab(QWidget & parent);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void touchedObjSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void commitSelection(const QItemSelection &selected, const QItemSelection &deselected);
    void updateSelection();
    void updateTouchedObjSelection();
    void updateLabels();
public slots:
    void filter();
    void updateCategories();
    void contextMenu(const QAbstractScrollArea & widget, const QPoint & pos);
};

#endif // SEGMENTATIONTAB_H
