#include "segmentationtab.h"

#include <chrono>
#include <QString>

#include <QMenu>
#include <QHeaderView>
#include <QSplitter>

#include "knossos-global.h"

extern stateInfo *state;

int TouchedObjectModel::rowCount(const QModelIndex &) const {
    return Segmentation::singleton().touchedObjects().size();
}

void TouchedObjectModel::recreate() {
    beginResetModel();
    objectCache.clear();
    for (auto & obj : Segmentation::singleton().touchedObjects()) {
        objectCache.emplace_back(obj);
    }

    emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
    endResetModel();
}

int SegmentationObjectModel::rowCount(const QModelIndex &) const {
    return Segmentation::singleton().objects.size();
}

int SegmentationObjectModel::columnCount(const QModelIndex &) const {
    return header.size();
}

QVariant SegmentationObjectModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return header[section];
    } else {
        return QVariant();//return invalid QVariant
    }
}

QVariant SegmentationObjectModel::data(const QModelIndex & index, int role) const {
    if (index.isValid()) {
        //http://coliru.stacked-crooked.com/a/98276b01d551fb41

        //auto it = std::begin(Segmentation::singleton().objects);
        //std::advance(it, index.row());
        //const auto & obj = it->second;

        //const auto & obj = Segmentation::singleton().objects[objectCache[index.row()]];
        const auto & obj = objectCache[index.row()].get();
        if (role == Qt::BackgroundRole && index.column() == 3) {
            const auto colorIndex = obj.id % 256;
            const auto red = Segmentation::singleton().overlayColorMap[0][colorIndex];
            const auto green = Segmentation::singleton().overlayColorMap[1][colorIndex];
            const auto blue = Segmentation::singleton().overlayColorMap[2][colorIndex];
            return QColor(red, green, blue);
        } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch (index.column()) {
            case 0: return static_cast<quint64>(obj.id);
            case 1: return obj.immutable;
            case 2: return obj.category;
            case 4: {
                QString output;
                for (const auto & elem : obj.subobjects) {
                    output += QString::number(elem.get().id) + ", ";
                }
                output.chop(2);
                return output;
            }
            }
        }
    }
    return QVariant();//return invalid QVariant
}

void SegmentationObjectModel::recreate() {
    beginResetModel();
    objectCache.clear();
    for (auto & pair : Segmentation::singleton().objects) {
        objectCache.emplace_back(pair.second);
    }

    emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
    endResetModel();
}

SegmentationTab::SegmentationTab(QWidget & parent) : QWidget(&parent) {
    showAllChck.setChecked(Segmentation::singleton().renderAllObjs);

    touchedObjsTable.setModel(&touchedObjectModel);
    touchedObjsTable.verticalHeader()->setVisible(false);
    touchedObjsTable.horizontalHeader()->setStretchLastSection(true);
    touchedObjsTable.setSelectionMode(QAbstractItemView::NoSelection);

    objectsTable.setModel(&objectModel);
    objectsTable.verticalHeader()->setVisible(false);
    objectsTable.horizontalHeader()->setStretchLastSection(true);
    objectsTable.setContextMenuPolicy(Qt::CustomContextMenu);
    objectsTable.setSelectionBehavior(QAbstractItemView::SelectRows);

    bottomHLayout.addWidget(&objectCountLabel);
    bottomHLayout.addWidget(&subobjectCountLabel);

    splitter.setOrientation(Qt::Vertical);
    splitter.addWidget(&touchedObjsTable);
    splitter.addWidget(&objectsTable);

    layout.addWidget(&showAllChck);
    layout.addWidget(&splitter);
    layout.addLayout(&bottomHLayout);
    setLayout(&layout);

    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, &objectModel, &SegmentationObjectModel::recreate);
    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, &touchedObjectModel, &TouchedObjectModel::recreate);
    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, this, &SegmentationTab::updateLabels);
    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, this, &SegmentationTab::updateSelection);
    QObject::connect(this, &SegmentationTab::clearSegObjSelectionSignal, &Segmentation::singleton(), &Segmentation::clearObjectSelection);
    QObject::connect(&objectsTable, &QTableView::customContextMenuRequested, this, &SegmentationTab::contextMenu);
    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, [&](){
        objectsTable.resizeColumnToContents(4);
        touchedObjsTable.resizeColumnToContents(4);
    });
    QObject::connect(objectsTable.selectionModel(), &QItemSelectionModel::selectionChanged, this, &SegmentationTab::selectionChanged);
    QObject::connect(&showAllChck, &QCheckBox::clicked, [&](bool value){
        Segmentation::singleton().renderAllObjs = value;
    });

    touchedObjectModel.recreate();
    objectModel.recreate();
    updateLabels();
}

void SegmentationTab::selectionChanged() {
    Segmentation::singleton().blockSignals(true);//prevent ping pong
    emit clearSegObjSelectionSignal();
    for (const auto & index : objectsTable.selectionModel()->selectedRows()) {
        Segmentation::singleton().selectObject(index.data().toInt());
    }
    updateSelection();
    Segmentation::singleton().blockSignals(false);
}

void SegmentationTab::updateSelection() {
    QItemSelection selectedItems;// = objectsTable.selectionModel()->selection();
    bool blockSelection = false;
    std::size_t startIndex;
    std::size_t objIndex = 0;
    for (auto & pair : Segmentation::singleton().objects) {
        if (!blockSelection && pair.second.selected) { //start block selection
            blockSelection = true;
            startIndex = objIndex;
        }
        if (blockSelection && !pair.second.selected) {//end block selection
            selectedItems.select(objectModel.index(startIndex, 0), objectModel.index(objIndex-1, objectModel.columnCount()-1));
            blockSelection = false;
        }
        ++objIndex;
    }
    //finish last blockselection – if any
    if (blockSelection) {
        selectedItems.select(objectModel.index(startIndex, 0), objectModel.index(objIndex-1, objectModel.columnCount()-1));
    }

    objectsTable.selectionModel()->blockSignals(true);
    objectsTable.clearSelection();
    objectsTable.selectionModel()->select(selectedItems, QItemSelectionModel::Select);
    objectsTable.selectionModel()->blockSignals(false);

    if (!selectedItems.indexes().isEmpty()) {// scroll to first selected entry
        objectsTable.scrollTo(selectedItems.indexes().front());
    }
}

void SegmentationTab::updateLabels() {
    objectCountLabel.setText(QString("Objects: %0").arg(Segmentation::singleton().objects.size()));
    subobjectCountLabel.setText(QString("Subobjects: %0").arg(Segmentation::singleton().subobjects.size()));
}

void SegmentationTab::contextMenu(QPoint pos) {
    QMenu contextMenu;
    QObject::connect(contextMenu.addAction("merge"), &QAction::triggered, &Segmentation::singleton(), &Segmentation::mergeSelectedObjects);
    QObject::connect(contextMenu.addAction("delete"), &QAction::triggered, &Segmentation::singleton(), &Segmentation::deleteSelectedObjects);
    contextMenu.exec(objectsTable.viewport()->mapToGlobal(pos));
}

