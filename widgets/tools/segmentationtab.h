#ifndef SEGMENTATIONTAB_H
#define SEGMENTATIONTAB_H

#include <functional>

#include <QAbstractTableModel>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>
#include <QCheckBox>
#include <QSplitter>

#include "segmentation.h"

class SegmentationObjectModel : public QAbstractTableModel {
Q_OBJECT
protected:
    const std::vector<QString> header{"Object ID", "immutable", "category", "comment", "color", "subobject IDs"};
    std::vector<std::reference_wrapper<Segmentation::Object>> objectCache;
public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
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
    void contextMenu(QPoint pos);
};

#endif // SEGMENTATIONTAB_H
