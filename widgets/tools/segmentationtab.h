#ifndef SEGMENTATIONTAB_H
#define SEGMENTATIONTAB_H

#include <functional>

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QLabel>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include "segmentation.h"

class SegmentationObjectModel : public QAbstractItemModel {
Q_OBJECT
protected:
    const std::vector<QString> header{"", "Object ID", "lock", "category", "comment", "subobject IDs"};
    std::vector<std::reference_wrapper<Segmentation::Object>> objectCache;
public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &) const override;
    void recreate();
};

class TouchedObjectModel : public SegmentationObjectModel {
Q_OBJECT
public:
    void recreate();
};

class SegmentationTab : public QWidget {
Q_OBJECT
    QVBoxLayout layout;
    QSplitter splitter;
    SegmentationObjectModel objectModel;
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
    void updateSelection();
    void updateTouchedObjSelection();
    void updateLabels();
signals:
    void clearSegObjSelectionSignal();
public slots:
    void contextMenu(QPoint pos);
};

#endif // SEGMENTATIONTAB_H
