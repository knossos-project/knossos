#ifndef SEGMENTATIONTAB_H
#define SEGMENTATIONTAB_H

#include "segmentation/segmentation.h"

#include <QAbstractListModel>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>

class NonRemovableQComboBox : public QComboBox {
    bool event(QEvent * event) override;
};

class CategoryDelegate : public QStyledItemDelegate {
    mutable NonRemovableQComboBox box;
public:
    CategoryDelegate(class CategoryModel & categoryModel);
    QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
};

class SegmentationObjectModel : public QAbstractListModel {
Q_OBJECT
    friend class SegmentationTab;//selection
protected:
    const std::vector<QString> header{""/*color*/, "Object ID", "lock", "category", "comment", "#", "subobject IDs"};
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
    std::vector<QString> categoriesCache;
public:
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    void recreate();
};

class SegmentationTab : public QWidget {
Q_OBJECT
    QVBoxLayout layout;
    QHBoxLayout toolsLayout;
    QButtonGroup modeGroup;
    QLabel brushRadiusLabel{"Brush Radius"};
    QSpinBox brushRadiusEdit;
    QPushButton twodBtn{"2D"};
    QPushButton threedBtn{"3D"};
    QCheckBox showAllChck{"Show all objects"};
    QHBoxLayout filterLayout;
    CategoryModel categoryModel;
    QComboBox categoryFilter;
    QLineEdit commentFilter;
    QCheckBox regExCheckbox{"regex"};

    SegmentationObjectModel objectModel;
    QSortFilterProxyModel objectProxyModelCategory;
    QSortFilterProxyModel objectProxyModelComment;
    TouchedObjectModel touchedObjectModel;

    CategoryDelegate categoryDelegate;

    QTreeView touchedObjsTable;
    QSplitter splitter;
    QTreeView objectsTable;
    QHBoxLayout bottomHLayout;
    QLabel objectCountLabel;
    QLabel subobjectCountLabel;
    QLabel subobjectHoveredLabel;
    QPushButton objectCreateButton{"Create new object"};

    bool objectSelectionProtection = false;
    bool touchedObjectSelectionProtection = false;

public:
    explicit SegmentationTab(QWidget * const parent = nullptr);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void touchedObjSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void updateSelection();
    void updateTouchedObjSelection();
    void updateLabels();
public slots:
    void filter();
    void contextMenu(const QAbstractScrollArea & widget, const QPoint & pos);
};

#endif // SEGMENTATIONTAB_H
