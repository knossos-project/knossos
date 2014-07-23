#ifndef SEGMENTATIONTAB_H
#define SEGMENTATIONTAB_H

#include <functional>

#include <QAbstractTableModel>
#include <QAbstractListModel>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>

#include "segmentation.h"

class SegmentationObjectModel : public QAbstractTableModel {
Q_OBJECT
protected:
    const std::vector<QString> header{"Object ID", "immutable", "category", "comment", "color", "subobject IDs"};
    std::vector<std::reference_wrapper<Segmentation::Object>> objectCache;
    bool filterOut(const Segmentation::Object &object, QString commentString, QString categoryString, bool useRegex, bool combineWithAnd);
    bool matchesSearchString(const QString searchString, const QString string, bool useRegEx);
public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
    void recreate(QString commentFilter, QString categoryFilter, bool useRegEx, bool combineFilterWithAnd);
};

class TouchedObjectModel : public SegmentationObjectModel {
Q_OBJECT
public:
    void recreate();
};

class CategoryModel : public QAbstractListModel {
    Q_OBJECT
    std::vector<QString> categories;
public:
    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    void recreate();

};

class SegmentationTab : public QWidget {
Q_OBJECT
    QVBoxLayout layout;
    QHBoxLayout filterLayout;
    QSplitter splitter;
    QComboBox categoryFilter;
    QPushButton filterCombineButton{"and"};
    QLineEdit commentFilter;
    QCheckBox regExCheckbox{"RegEx"};
    CategoryModel categoryModel;
    SegmentationObjectModel objectModel;
    TouchedObjectModel touchedObjectModel;
    QCheckBox showAllChck{"Show all objects"};
    QTableView touchedObjsTable;
    QTableView objectsTable;
    QLabel objectCountLabel;
    QLabel subobjectCountLabel;
    QHBoxLayout bottomHLayout;
    bool objectSelectionProtection = false;
    bool touchedObjectSelectionProtection = false;
public:
    explicit SegmentationTab(QWidget & parent);
    void selectionChanged();
    void touchedObjSelectionChanged();
    void updateSelection();
    void updateTouchedObjSelection();
    void updateLabels();
signals:
    void clearSegObjSelectionSignal();
public slots:
    void filter();
    void updateCategories();
    void contextMenu(QPoint pos);
};

#endif // SEGMENTATIONTAB_H
