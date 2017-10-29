#include "layerdialog.h"

#include <QHeaderView>
#include "dataset.h"
#include "mainwindow.h"
#include "stateInfo.h"

int LayerItemModel::rowCount(const QModelIndex &) const {
    return Dataset::datasets.size();
}

int LayerItemModel::columnCount(const QModelIndex &) const {
    return header.size();
}

QVariant LayerItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return header[section];
    } else {
        return QVariant();
    }
}

QVariant LayerItemModel::data(const QModelIndex &index, int role) const {
    if(index.isValid()) {
        auto& data = Dataset::datasets[index.row()];
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch(index.column()) {
            case 1: return QString::number(data.opacity * 100.0f) + (role == Qt::EditRole ? "" : "%");
            case 2: return data.layerName;
            case 3:
                switch(data.type) {
                case Dataset::CubeType::RAW_UNCOMPRESSED:
                    return "RAW_UNCOMPRESSED";
                case Dataset::CubeType::RAW_JPG:
                    return "RAW_JPG";
                case Dataset::CubeType::RAW_J2K:
                    return "RAW_J2K";
                case Dataset::CubeType::RAW_JP2_6:
                    return "RAW_JP2_6";
                case Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_16:
                    return "SEGMENTATION_UNCOMPRESSED_16";
                case Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_64:
                    return "SEGMENTATION_UNCOMPRESSED_64";
                case Dataset::CubeType::SEGMENTATION_SZ_ZIP:
                    return "SEGMENTATION_SZ_ZIP";
                case Dataset::CubeType::SNAPPY:
                    return "SNAPPY";
                }
            case 4:
                switch(data.api) {
                case Dataset::API::Heidelbrain:
                    return "Heidelbrain";
                case Dataset::API::WebKnossos:
                    return "WebKnossos";
                case Dataset::API::GoogleBrainmaps:
                    return "GoogleBrainmaps";
                case Dataset::API::OpenConnectome:
                    return "OpenConnectome";
                }
            case 5: return data.magnification;
            case 6: return data.cubeEdgeLength;
            case 7: return data.experimentname;
            }
        } else if(role == Qt::CheckStateRole) {
            if(index.column() == 0) {
                return data.visible ? Qt::Checked : Qt::Unchecked;
            }
        }
    }
    return QVariant();
}

bool LayerItemModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if(index.isValid()) {
        auto& data = Dataset::datasets[index.row()];
        if (role == Qt::EditRole) {
            switch(index.column()) {
            case 1:
                data.opacity = std::min(value.toFloat() / 100.0f, 1.0f);
                break;
            case 2:
                data.layerName = value.toString();
                break;
            }
        } else if(role == Qt::CheckStateRole) {
            if(index.column() == 0) {
                data.visible = value.toBool();
            }
        }
    }
    emit dataChanged(index, index, QVector<int>(role));
    return true;
}

void LayerItemModel::addItem() {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    // insert row
    endInsertRows();
}

void LayerItemModel::removeItem(const QModelIndex &index) {
    if(index.isValid()) {
        auto row = index.row();
        beginRemoveRows(QModelIndex(), row, row);
        auto& datasets = Dataset::datasets;
        datasets.erase(datasets.begin() + row);
        endRemoveRows();
    }
}

void LayerItemModel::moveItem(const QModelIndex &index, int offset) {
    if(index.isValid()) {
        auto row = index.row();
        auto& datasets = Dataset::datasets;
        if(row + offset >= 0 && row + offset < datasets.size()) {
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), row + ((offset > 0) ? offset + 1 : offset)); // because moving is done into between rows
            datasets.swap(row, row + offset);
            endMoveRows();
        }
    }
}

void LayerItemModel::reset() {
    beginResetModel();
    endResetModel();
}

Qt::ItemFlags LayerItemModel::flags(const QModelIndex &index) const {
    if(index.isValid()) {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        switch(index.column()) {
        case 0:
            flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
            break;
        case 1:
        case 2:
            flags |= Qt::ItemIsEditable;
            break;
        }
        return flags;
    }
    return 0;
}


LayerDialogWidget::LayerDialogWidget(QWidget *parent) : DialogVisibilityNotify(PREFERENCES_WIDGET, parent), layerLoadWidget(this) {
    setWindowTitle("Layers");

    int row = 0;
    opacitySlider.setTracking(true);
    opacitySlider.setMaximum(100);
    optionsLayout.addWidget(&opacitySliderLabel, row, 0);
    optionsLayout.addWidget(&opacitySlider, row, 1);
    optionsLayout.addWidget(&linearFilteringCheckBox, row, 2);

    rangeDeltaSlider.setTracking(true);
    rangeDeltaSlider.setMaximum(100);
    optionsLayout.addWidget(&rangeDeltaSliderLabel, ++row, 0);
    optionsLayout.addWidget(&rangeDeltaSlider, row, 1);

    biasSlider.setTracking(true);
    biasSlider.setMaximum(100);
    optionsLayout.addWidget(&biasSliderLabel, ++row, 0);
    optionsLayout.addWidget(&biasSlider, row, 1);
    optionsSpoiler.setContentLayout(optionsLayout);

    treeView.setModel(&itemModel);
    treeView.resizeColumnToContents(0);
    treeView.resizeColumnToContents(1);
//    treeView.header()->setSectionResizeMode(QHeaderView::Fixed); // can cause some unwanted column sizes that are unresizable
//    treeView.header()->setSectionsClickable(false);
//    treeView.header()->setSectionsMovable(false);
    treeView.setRootIsDecorated(false);
    treeView.setUniformRowHeights(true); // for optimization
    treeView.setDragDropMode(QAbstractItemView::InternalMove);

//    addLayerButton.setIcon(QIcon(":/resources/icons/dialog-ok.png"));
//    removeLayerButton.setIcon(QIcon(":/resources/icons/application-exit.png"));

    addLayerButton.setText("➕");
    removeLayerButton.setText("➖");
    moveUpButton.setText("🡅");
    moveDownButton.setText("🡇");

    controlButtonLayout.addWidget(&infoTextLabel);
    controlButtonLayout.addWidget(&moveUpButton);
    controlButtonLayout.addWidget(&moveDownButton);
    controlButtonLayout.addWidget(&addLayerButton);
    controlButtonLayout.addWidget(&removeLayerButton);

    mainLayout.addWidget(&optionsSpoiler);
    mainLayout.addWidget(&treeView);
    mainLayout.addLayout(&controlButtonLayout);
    setLayout(&mainLayout);

    QObject::connect(&addLayerButton, &QToolButton::clicked, [this](){
        itemModel.addItem();
        layerLoadWidget.show();
    });

    QObject::connect(&removeLayerButton, &QToolButton::clicked, [this](){
        itemModel.removeItem(treeView.selectionModel()->currentIndex());
    });

    QObject::connect(&moveUpButton, &QToolButton::clicked, [this](){
        itemModel.moveItem(treeView.selectionModel()->currentIndex(), -1);
    });

    QObject::connect(&moveDownButton, &QToolButton::clicked, [this](){
        itemModel.moveItem(treeView.selectionModel()->currentIndex(), 1);
    });

    QObject::connect(&opacitySlider, &QAbstractSlider::valueChanged, [this](int value){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {
            auto& selectedData = Dataset::datasets[currentIndex.row()];
            selectedData.opacity = static_cast<float>(value) / opacitySlider.maximum();
            const auto& changeIndex = currentIndex.sibling(currentIndex.row(), 1); // todo: enum the 1
            emit itemModel.dataChanged(changeIndex, changeIndex, QVector<int>(Qt::EditRole));
        }
    });

    QObject::connect(&rangeDeltaSlider, &QAbstractSlider::valueChanged, [this](int value){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {;
            auto& data = Dataset::datasets[currentIndex.row()];
            data.rangeDelta = static_cast<float>(value) / rangeDeltaSlider.maximum();
        }
    });

    QObject::connect(&biasSlider, &QAbstractSlider::valueChanged, [this](int value){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {
            auto& data = Dataset::datasets[currentIndex.row()];
            data.bias = static_cast<float>(value) / biasSlider.maximum();
        }
    });

    QObject::connect(&linearFilteringCheckBox, &QCheckBox::stateChanged, [this](int state){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {
            auto& data = Dataset::datasets[currentIndex.row()];
            data.linearFiltering = (state == Qt::Checked) ? true : false;
        }
    });

    QObject::connect(treeView.selectionModel(), &QItemSelectionModel::selectionChanged, this, &LayerDialogWidget::updateLayerProperties);
    QObject::connect(&itemModel, &QAbstractItemModel::dataChanged, this, &LayerDialogWidget::updateLayerProperties);

    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this](bool /*showOverlays*/) {
        itemModel.reset();
    });

    resize(800, 600);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void LayerDialogWidget::updateLayerProperties() {
    auto& data = Dataset::datasets[treeView.selectionModel()->currentIndex().row()];
    opacitySlider.setValue(static_cast<int>(data.opacity * opacitySlider.maximum()));
    rangeDeltaSlider.setValue(static_cast<int>(data.rangeDelta * rangeDeltaSlider.maximum()));
    biasSlider.setValue(static_cast<int>(data.bias * biasSlider.maximum()));
    linearFilteringCheckBox.setChecked(data.linearFiltering);
}

LayerLoadWidget::LayerLoadWidget(QWidget *parent) : QDialog(parent) {
    setModal(true);

    int row = 0;
    listLayout.addWidget(&datasetLoadLabel, row, 0, Qt::AlignCenter);
    listLayout.addWidget(&sessionLayerLabel, row, 1, Qt::AlignCenter);

    listLayout.addWidget(&datasetLoadList, ++row, 0);
    listLayout.addWidget(&sessionLayerList, row, 1);

    listLayout.addWidget(&datasetLoadDescription, ++row, 0);
    listLayout.addWidget(&sessionLayerDescription, row, 1);

    buttonLayout.addWidget(&loadButton, 0, Qt::AlignRight);
    buttonLayout.addWidget(&cancelButton, 0, Qt::AlignLeft);

    mainLayout.addLayout(&listLayout);
    mainLayout.addLayout(&buttonLayout);
    setLayout(&mainLayout);

    loadButton.setText("Load");
    cancelButton.setText("Cancel");

    resize(900, 500);
}
