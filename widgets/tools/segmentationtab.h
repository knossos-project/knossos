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
    static const std::vector<QString> header;
    std::vector<std::reference_wrapper<Segmentation::Object>> objectCache;
public:
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    void recreate();
};

class TouchedObjectModel : public QAbstractTableModel {
Q_OBJECT
    static const std::vector<QString> header;
    std::vector<std::reference_wrapper<Segmentation::Object>> objectCache;
public:
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
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
    bool selectionProtection;
public:
    explicit SegmentationTab(QWidget & parent);
    void selectionChanged();
    void updateSelection();
    void updateLabels();
signals:
    void clearSegObjSelectionSignal();
public slots:
    void contextMenu(QPoint pos);
};

#endif // SEGMENTATIONTAB_H
