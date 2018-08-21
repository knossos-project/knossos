#include "layerdialog.h"

#include "dataset.h"
#include "loader.h"
#include "mainwindow.h"
#include "network.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QHeaderView>

std::size_t LayerItemModel::ordered_i(std::size_t index) const {
    return state->viewerState->layerOrder[index];
}

LayerItemModel::LayerItemModel() {
    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, []() {
        // adjust datasetOrder size to fit all elements in dataset
        for(std::size_t i = state->viewerState->layerOrder.size(); i < Dataset::datasets.size(); ++i) {
            state->viewerState->layerOrder.emplace_back(i);
        }
    });

    QObject::connect(state->viewer, &Viewer::layerVisibilityChanged, [this]() {
        reset();
    });
}

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
        const auto& data = Dataset::datasets[ordered_i(index.row())];
        auto& layerSettings = state->viewerState->layerRenderSettings[ordered_i(index.row())];
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch(index.column()) {
            case 1: return QString::number(layerSettings.opacity * 100.0f) + (role == Qt::EditRole ? "" : "%");
            case 2: return layerSettings.layerName;
            case 3:
                return data.compressionString();
            case 4:
                switch(data.api) {
                case Dataset::API::Heidelbrain:
                    return "Heidelbrain";
                case Dataset::API::WebKnossos:
                    return "WebKnossos";
                case Dataset::API::GoogleBrainmaps:
                    return "GoogleBrainmaps";
                case Dataset::API::PyKnossos:
                    return "PyKnossos";
                case Dataset::API::OpenConnectome:
                    return "OpenConnectome";
                }
            case 5: return data.magnification;
            case 6: return data.cubeEdgeLength;
            case 7: return data.experimentname;
            }
        } else if(role == Qt::CheckStateRole) {
            if(index.column() == 0) {
                auto visible = state->viewerState->layerRenderSettings[ordered_i(index.row())].visible;
                return visible ? Qt::Checked : Qt::Unchecked;
            }
        }
    }
    return QVariant();
}

void reloadLayers() {
    Loader::Controller::singleton().restart(Dataset::datasets);
    state->viewer->updateDatasetMag();// clear vps and notify loader
    emit state->mainWindow->widgetContainer.datasetLoadWidget.datasetChanged();// HACK
}

bool LayerItemModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if(index.isValid()) {
        auto& layerSettings = state->viewerState->layerRenderSettings[ordered_i(index.row())];
        if (role == Qt::EditRole) {
            switch(index.column()) {
            case 1:
                layerSettings.opacity = std::min(value.toFloat() / 100.0f, 1.0f);
                break;
            case 2:
                layerSettings.layerName = value.toString();
                break;
            }
        } else if(role == Qt::CheckStateRole) {
            if(index.column() == 0) {
                auto & layer = Dataset::datasets[ordered_i(index.row())];
                layer.allocationEnabled = layer.loadingEnabled = layerSettings.visible = value.toBool();
                Segmentation::singleton().enabled = std::count_if(std::begin(Dataset::datasets), std::end(Dataset::datasets), [](const auto & dataset){
                    return dataset.loadingEnabled && dataset.isOverlay();
                });
                reloadLayers();
            }
        }
    }
    emit dataChanged(index, index, QVector<int>(role));
    return true;
}

void LayerItemModel::addItem() {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    // TODO: insert row
    endInsertRows();
}

void LayerItemModel::removeItem(const QModelIndex &index) {
    if(index.isValid()) {
        auto row = index.row();
        beginRemoveRows(QModelIndex(), row, row);
        // TODO: remove layer
        endRemoveRows();
    }
}

void LayerItemModel::moveItem(const QModelIndex &index, int offset) {
    if(index.isValid()) {
        auto row = index.row();
        if(row + offset >= 0 && row + offset < rowCount()) {
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), row + ((offset > 0) ? offset + 1 : offset)); // because moving is done into between rows
            std::swap(state->viewerState->layerOrder[row], state->viewerState->layerOrder[row + offset]);
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
    return nullptr;
}

LayerDialogWidget::LayerDialogWidget(QWidget *parent) : DialogVisibilityNotify(PREFERENCES_WIDGET, parent) {
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
    treeView.setRootIsDecorated(false);
    treeView.setUniformRowHeights(true); // for optimization
    treeView.setDragDropMode(QAbstractItemView::InternalMove);

    addLayerButton.setText("add");
    addLayerButton.setEnabled(false);
    removeLayerButton.setText("remove");
    moveUpButton.setText("up");
    moveDownButton.setText("down");

    controlButtonLayout.addStretch();
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
    });

    QObject::connect(&removeLayerButton, &QToolButton::clicked, [this](){
        for (const auto & mindex : treeView.selectionModel()->selectedRows()) {
            Dataset::datasets.erase(std::next(std::begin(Dataset::datasets), itemModel.ordered_i(mindex.row())));
            reloadLayers();
        }
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
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            auto& layerSettings = state->viewerState->layerRenderSettings[ordered_index];
            layerSettings.opacity = static_cast<float>(value) / opacitySlider.maximum();
            const auto& changeIndex = currentIndex.sibling(currentIndex.row(), 1); // todo: enum the 1
            emit itemModel.dataChanged(changeIndex, changeIndex, QVector<int>(Qt::EditRole));
        }
    });

    QObject::connect(&rangeDeltaSlider, &QAbstractSlider::valueChanged, [this](int value){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {;
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            auto& layerSettings = state->viewerState->layerRenderSettings[ordered_index];
            layerSettings.rangeDelta = static_cast<float>(value) / rangeDeltaSlider.maximum();
            state->viewer->reslice_notify(ordered_index);
        }
    });

    QObject::connect(&biasSlider, &QAbstractSlider::valueChanged, [this](int value){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            auto& layerSettings = state->viewerState->layerRenderSettings[ordered_index];
            layerSettings.bias = static_cast<float>(value) / biasSlider.maximum();
            state->viewer->reslice_notify(ordered_index);
        }
    });

    QObject::connect(&linearFilteringCheckBox, &QCheckBox::stateChanged, [this](int checkstate){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            auto& layerSettings = state->viewerState->layerRenderSettings[ordered_index];
            layerSettings.textureFilter = (checkstate == Qt::Checked) ? QOpenGLTexture::Linear : QOpenGLTexture::Nearest;
            state->mainWindow->forEachOrthoVPDo([&](ViewportOrtho & orthoVP){
                orthoVP.applyTextureFilter();
            });
        }
    });

    QObject::connect(state->viewer, &Viewer::layerSettingsChanged, this, &LayerDialogWidget::updateLayerProperties);

    QObject::connect(treeView.selectionModel(), &QItemSelectionModel::selectionChanged, this, &LayerDialogWidget::updateLayerProperties);
    QObject::connect(&itemModel, &QAbstractItemModel::dataChanged, this, &LayerDialogWidget::updateLayerProperties);

    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this]() {
        itemModel.reset();
    });

    resize(800, 600);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void LayerDialogWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(LAYER_DIALOG_WIDGET);
    restoreGeometry(settings.value(GEOMETRY).toByteArray());
    settings.endGroup();
}

void LayerDialogWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(LAYER_DIALOG_WIDGET);
    settings.setValue(GEOMETRY, saveGeometry());
    settings.setValue(VISIBLE, isVisible());
    settings.endGroup();
}

void LayerDialogWidget::updateLayerProperties() {
    const auto & currentIndex = treeView.selectionModel()->currentIndex();
    if (currentIndex.isValid()) {
        const auto ordered_index = itemModel.ordered_i(currentIndex.row());
        auto & layerSettings = state->viewerState->layerRenderSettings[ordered_index];
        opacitySlider.setValue(static_cast<int>(layerSettings.opacity * opacitySlider.maximum()));
        rangeDeltaSlider.setValue(static_cast<int>(layerSettings.rangeDelta * rangeDeltaSlider.maximum()));
        biasSlider.setValue(static_cast<int>(layerSettings.bias * biasSlider.maximum()));
        linearFilteringCheckBox.setChecked(layerSettings.textureFilter == QOpenGLTexture::Linear);
    }
}
