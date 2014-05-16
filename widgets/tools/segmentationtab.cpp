#include <chrono>
#include <QString>

#include <QMenu>
#include <QHeaderView>

#include "segmentationtab.h"
#include "knossos-global.h"

extern stateInfo *state;

int SegmentationObjectModel::rowCount(const QModelIndex &) const {
    return Segmentation::singleton().objects.size();
}

int SegmentationObjectModel::columnCount(const QModelIndex &) const {
    return 4;
}

QVariant SegmentationObjectModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Object ID";
        case 1: return "category";
        case 2: return "color";
        case 3: return "subobject IDs";
        default:
            break;
        }
    }
    return QVariant();//return invalid QVariant
}

QVariant SegmentationObjectModel::data(const QModelIndex & index, int role) const {
    if (index.isValid() && (role == Qt::DisplayRole || role == Qt::EditRole)) {
        //http://coliru.stacked-crooked.com/a/98276b01d551fb41

        //auto it = std::begin(Segmentation::singleton().objects);
        //std::advance(it, index.row());
        //const auto & obj = it->second;

        //const auto & obj = Segmentation::singleton().objects[objectCache[index.row()]];

        const auto & obj = objectCache[index.row()].get();

        switch (index.column()) {
        case 0: return static_cast<quint64>(obj.id);
        case 1: return obj.category;
        case 2: {
            const auto colorIndex = obj.id % 256;
            const auto red = Segmentation::singleton().overlayColorMap[0][colorIndex];
            const auto green = Segmentation::singleton().overlayColorMap[1][colorIndex];
            const auto blue = Segmentation::singleton().overlayColorMap[2][colorIndex];
            return QColor(red, green, blue);
        }
        case 3: {
            QString output;
            for (const auto & elem : obj.subobjects) {
                output += QString::number(elem.get().id) + ", ";
            }
            output.chop(2);
            return output;
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
    objectsTable.setModel(&objectModel);

    bottomHLayout.addWidget(&objectCountLabel);
    bottomHLayout.addWidget(&subobjectCountLabel);

    objectsTable.verticalHeader()->setVisible(false);
    objectsTable.setContextMenuPolicy(Qt::CustomContextMenu);
    objectsTable.setSelectionBehavior(QAbstractItemView::SelectRows);

    layout.addWidget(&objectsTable);
    layout.addLayout(&bottomHLayout);
    setLayout(&layout);

    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, &objectModel, &SegmentationObjectModel::recreate);
    QObject::connect(&Segmentation::singleton(), &Segmentation::dataChanged, this, &SegmentationTab::updateLabels);
    QObject::connect(&objectsTable, &QTableView::customContextMenuRequested, this, &SegmentationTab::contextMenu);
    objectModel.recreate();
    updateLabels();
}

void SegmentationTab::updateLabels() {
    objectCountLabel.setText(QString("Obects: %0").arg(Segmentation::singleton().objects.size()));
    subobjectCountLabel.setText(QString("Subobects: %0").arg(Segmentation::singleton().subobjects.size()));
}

void SegmentationTab::contextMenu(QPoint pos) {
    QMenu contextMenu;
    QObject::connect(contextMenu.addAction("merge"), &QAction::triggered, this, &SegmentationTab::mergeObjects);
    contextMenu.exec(objectsTable.viewport()->mapToGlobal(pos));
}

void SegmentationTab::mergeObjects() {
    QModelIndexList selected = objectsTable.selectionModel()->selectedRows();
    std::vector<uint64_t> selectedIDs;
    foreach(QModelIndex index, selected) {
        selectedIDs.push_back(index.data().toInt());
    }

    Segmentation::singleton().merge(selectedIDs);
}
