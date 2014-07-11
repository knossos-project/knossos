#include "segmentationtab.h"

#include <chrono>
#include <QString>

#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QPushButton>
#include <QSplitter>

#include "knossos-global.h"

extern stateInfo *state;

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
    return objectCache.size();
}

int SegmentationObjectModel::columnCount(const QModelIndex &) const {
    return header.size();
}

QVariant SegmentationObjectModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
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
        if (index.column() == 0 && role == Qt::BackgroundRole) {
            const auto colorIndex = obj.id % 256;
            const auto red = Segmentation::singleton().overlayColorMap[0][colorIndex];
            const auto green = Segmentation::singleton().overlayColorMap[1][colorIndex];
            const auto blue = Segmentation::singleton().overlayColorMap[2][colorIndex];
            return QColor(red, green, blue);
        } else if (index.column() == 2 && role == Qt::CheckStateRole) {
            return (obj.immutable ? Qt::Checked : Qt::Unchecked);
        } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch (index.column()) {
            case 1: return static_cast<quint64>(obj.id);
            case 3: return obj.category;
            case 4: return obj.comment;
            case 5: return static_cast<quint64>(obj.subobjects.size());
            case 6: {
                QString output;
                const auto elemCount = std::min(MAX_SHOWN_SUBOBJECTS, obj.subobjects.size());
                for (std::size_t i = 0; i < elemCount; ++i) {
                    output += QString::number(obj.subobjects[i].get().id) + ", ";
                }
                output.chop(2);
                return output;
            }
            }
        }
    }
    return QVariant();//return invalid QVariant
}

bool SegmentationObjectModel::setData(const QModelIndex & index, const QVariant & value, int role) {
    if (index.isValid()) {
        auto & obj = objectCache[index.row()].get();
        if (index.column() == 2 && role == Qt::CheckStateRole) {
            if (!obj.immutable) {//don’t remove immutability
                QMessageBox prompt;
                prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
                prompt.setIcon(QMessageBox::Question);
                prompt.setWindowTitle(tr("Lock Object"));
                prompt.setText(tr("Lock the object with id %0?").arg(obj.id));
                const auto & lockButton = prompt.addButton(tr("Lock"), QMessageBox::YesRole);
                prompt.addButton(tr("Cancel"), QMessageBox::NoRole);
                prompt.exec();
                if (prompt.clickedButton() == lockButton) {
                    obj.immutable = value.toBool();
                }
            }
        } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch (index.column()) {
            case 3: obj.category = value.toString(); break;
            case 4: obj.comment  = value.toString(); break;
            default:
                return false;
            }
        }
    }
    return true;
}

Qt::ItemFlags SegmentationObjectModel::flags(const QModelIndex & index) const {
    Qt::ItemFlags flags = QAbstractItemModel::flags(index) | Qt::ItemNeverHasChildren;//not editable
    switch(index.column()) {
    case 2:
        return flags | Qt::ItemIsUserCheckable;
    case 3:
    case 4:
        return flags | Qt::ItemIsEditable;
    }
    return flags;
}

QModelIndex SegmentationObjectModel::index(int row, int column, const QModelIndex &) const {
    return createIndex(row, column);
}

QModelIndex SegmentationObjectModel::parent(const QModelIndex &) const {
    return QModelIndex();
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
    touchedObjsTable.setAllColumnsShowFocus(true);
    touchedObjsTable.setRootIsDecorated(false);
    touchedObjsTable.setSelectionMode(QAbstractItemView::ExtendedSelection);
    touchedObjsTable.setUniformRowHeights(true);//perf hint from doc

    objectsTable.setModel(&objectModel);
    objectsTable.setAllColumnsShowFocus(true);
    objectsTable.setContextMenuPolicy(Qt::CustomContextMenu);
    objectsTable.setRootIsDecorated(false);
    objectsTable.setSelectionMode(QAbstractItemView::ExtendedSelection);
    objectsTable.setUniformRowHeights(true);//perf hint from doc

    bottomHLayout.addWidget(&objectCountLabel);
    bottomHLayout.addWidget(&subobjectCountLabel);

    splitter.setOrientation(Qt::Vertical);
    splitter.addWidget(&touchedObjsTable);
    splitter.addWidget(&objectsTable);

    layout.addWidget(&showAllChck);
    layout.addWidget(&splitter);
    layout.addLayout(&bottomHLayout);
    setLayout(&layout);

    for (const auto & index : {0, 1, 2, 5}) {
        //comment (4) shall not waste space, also displaying all supervoxel ids (6) causes problems
        touchedObjsTable.header()->setSectionResizeMode(index, QHeaderView::ResizeToContents);
        objectsTable.header()->setSectionResizeMode(index, QHeaderView::ResizeToContents);
    }

    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, &objectModel, &SegmentationObjectModel::recreate);
    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, &touchedObjectModel, &TouchedObjectModel::recreate);
    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, this, &SegmentationTab::updateLabels);
    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, this, &SegmentationTab::updateSelection);
    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, this, &SegmentationTab::updateTouchedObjSelection);
    QObject::connect(this, &SegmentationTab::clearSegObjSelectionSignal, &Segmentation::singleton(), &Segmentation::clearObjectSelection);
    QObject::connect(&objectsTable, &QTreeView::customContextMenuRequested, this, &SegmentationTab::contextMenu);
    QObject::connect(objectsTable.selectionModel(), &QItemSelectionModel::selectionChanged, this, &SegmentationTab::selectionChanged);
    QObject::connect(touchedObjsTable.selectionModel(), &QItemSelectionModel::selectionChanged, this, &SegmentationTab::touchedObjSelectionChanged);
    QObject::connect(&showAllChck, &QCheckBox::clicked, [&](bool value){
        Segmentation::singleton().renderAllObjs = value;
    });

    touchedObjectModel.recreate();
    objectModel.recreate();

    updateLabels();
}

void SegmentationTab::touchedObjSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {
    if (touchedObjectSelectionProtection) {
        return;
    }

    Segmentation::singleton().blockSignals(true);//prevent ping pong
    for (const auto & index : deselected.indexes()) {
        Segmentation::singleton().unselectObject(index.data().toInt());
    }
    for (const auto & index : selected.indexes()) {
        Segmentation::singleton().selectObject(index.data().toInt());
    }
    updateSelection();
    Segmentation::singleton().blockSignals(false);
}

void SegmentationTab::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {
    if (objectSelectionProtection) {
        return;
    }

    Segmentation::singleton().blockSignals(true);//prevent ping pong
    for (const auto & index : deselected.indexes()) {
        Segmentation::singleton().unselectObject(index.data().toInt());
    }
    for (const auto & index : selected.indexes()) {
        Segmentation::singleton().selectObject(index.data().toInt());
    }
    updateTouchedObjSelection();
    Segmentation::singleton().blockSignals(false);
}

void SegmentationTab::updateTouchedObjSelection() {
    QItemSelection selectedItems;
    bool blockSelection = false;
    std::size_t startIndex;
    std::size_t objIndex = 0;

    for (auto & elem : Segmentation::singleton().touchedObjects()) {
        if (!blockSelection && elem.get().selected) { //start block selection
            blockSelection = true;
            startIndex = objIndex;
        }
        if (blockSelection && !elem.get().selected) { //end block selection
            selectedItems.select(touchedObjectModel.index(startIndex, 0),
                                 touchedObjectModel.index(objIndex-1, touchedObjectModel.columnCount()-1));
            blockSelection = false;
        }
        ++objIndex;
    }

    //finish last blockselection – if any
    if (blockSelection) {
        selectedItems.select(touchedObjectModel.index(startIndex, 0),
                             touchedObjectModel.index(objIndex-1, touchedObjectModel.columnCount()-1));
    }

    touchedObjectSelectionProtection = true;//using block signals prevents update of the tableview
    touchedObjsTable.clearSelection();
    touchedObjsTable.selectionModel()->select(selectedItems, QItemSelectionModel::Select);
    touchedObjectSelectionProtection= false;
}

void SegmentationTab::updateSelection() {
    QItemSelection selectedItems;// = objectsTable.selectionModel()->selection();
    bool blockSelection = false;
    std::size_t startIndex;
    std::size_t objIndex = 0;

    for (auto & elem : Segmentation::singleton().objects) {
        if (!blockSelection && elem.second.selected) { //start block selection
            blockSelection = true;
            startIndex = objIndex;
        }
        if (blockSelection && !elem.second.selected) {//end block selection
            selectedItems.select(objectModel.index(startIndex, 0),
                                 objectModel.index(objIndex-1, objectModel.columnCount()-1));
            blockSelection = false;
        }
        ++objIndex;
    }

    //finish last blockselection – if any
    if (blockSelection) {
        selectedItems.select(objectModel.index(startIndex, 0),
                             objectModel.index(objIndex-1, objectModel.columnCount()-1));
    }

    objectSelectionProtection = true;//using block signals prevents update of the tableview
    objectsTable.clearSelection();
    objectsTable.selectionModel()->select(selectedItems, QItemSelectionModel::Select);
    objectSelectionProtection = false;

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

