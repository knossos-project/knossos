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
    if (index.isValid()) {
        const auto& data = Dataset::datasets[ordered_i(index.row())];
        auto& layerSettings = state->viewerState->layerRenderSettings[ordered_i(index.row())];
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch(index.column()) {
            case 1: return QString::number(layerSettings.opacity * 100.0f) + (role == Qt::EditRole ? "" : "%");
            case 2: return data.compressionString();
            case 3: return data.apiString();
            case 4: return data.magnification;
            case 5: return data.cubeEdgeLength;
            case 6: return data.experimentname;
            case 7: return data.description;
            case 8: return layerSettings.color;
            }
        } else if (role == Qt::CheckStateRole) {
            if (index.column() == 0) {
                auto visible = state->viewerState->layerRenderSettings[ordered_i(index.row())].visible;
                return visible ? Qt::Checked : Qt::Unchecked;
            } else if (index.column() == 2) {
                return ordered_i(index.row()) == Segmentation::singleton().layerId ? Qt::PartiallyChecked : data.isOverlay() ? Qt::Unchecked : QVariant{};
            }
        } else if (role == Qt::ForegroundRole) {
            if (index.column() == 8) {
                const auto color = layerSettings.color;
                const auto luminance = 0.299 * color.redF() + 0.587 * color.greenF() + 0.114 * color.blueF();
                return QColor((luminance > 0.5) ? Qt::black : Qt::white);
            }
        } else if (role == Qt::BackgroundRole) {
            if (index.column() == 8) {
                return layerSettings.color;
            }
        }
    }
    return QVariant();
}

void reloadLayers(bool layerCountChanged = false) {
    if (layerCountChanged) {
        Loader::Controller::singleton().restart(Dataset::datasets);// state->cube2Pointer needs to be resized
    }
    state->viewer->loader_notify();
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
            case 8:
                layerSettings.color = value.value<QColor>();
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
            } else if (index.column() == 2 && value.toBool()) {
                const auto beginIt = std::cbegin(state->viewerState->layerOrder);
                const auto segi = std::distance(beginIt, std::find(beginIt, std::cend(state->viewerState->layerOrder), Segmentation::singleton().layerId));
                const auto prevSegLayerIndex = this->index(segi);
                Segmentation::singleton().clear();// clear selection and snappy cache
                Segmentation::singleton().layerId = ordered_i(index.row());
                emit dataChanged(prevSegLayerIndex, prevSegLayerIndex, QVector<int>(role));
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
        case 0: flags |= Qt::ItemIsUserCheckable; break;
        case 1: flags |= Qt::ItemIsEditable; break;
        case 2: flags |= Qt::ItemIsUserCheckable; break;
        case 7: flags |= Qt::ItemIsEditable; break;
        case 8: flags |= Qt::ItemIsEditable; break;
        }
        return flags;
    }
    return nullptr;
}

LayerDialogWidget::LayerDialogWidget(QWidget *parent) : DialogVisibilityNotify(PREFERENCES_WIDGET, parent), colorDialog(this) {
    setWindowTitle("Layers");

    int row = 0, col;
    opacitySlider.setTracking(true);
    opacitySlider.setMaximum(100);
    optionsLayout.addWidget(&opacitySliderLabel, row, col=0);
    optionsLayout.addWidget(&opacitySlider, row, ++col);
    optionsLayout.addWidget(&linearFilteringCheckBox, row, ++col);

    rangeDeltaSlider.setTracking(true);
    rangeDeltaSlider.setMaximum(100);
    optionsLayout.addWidget(&rangeDeltaSliderLabel, ++row, col=0);
    optionsLayout.addWidget(&rangeDeltaSlider, row, ++col);

    biasSlider.setTracking(true);
    biasSlider.setMaximum(100);
    combineSlicesType.addItems({"min", "max"});
    optionsLayout.addWidget(&biasSliderLabel, ++row, col=0);
    optionsLayout.addWidget(&biasSlider, row, ++col);
    optionsLayout.addWidget(&combineSlicesCheck, ++row, col=0);
    optionsLayout.addWidget(&combineSlicesType, row, ++col);
    optionsLayout.addWidget(&combineSlicesSpin, row, ++col);
    optionsLayout.addWidget(&combineSlicesXyOnlyCheck, row, ++col);
    optionsSpoiler.setContentLayout(optionsLayout);

    treeView.setModel(&itemModel);
    treeView.resizeColumnToContents(0);
    treeView.resizeColumnToContents(1);
    treeView.setRootIsDecorated(false);
    treeView.setUniformRowHeights(true); // for optimization
    treeView.setDragDropMode(QAbstractItemView::InternalMove);

    dupLayerButton.setText("duplicate");
    addLayerButton.setText("add");
    addLayerButton.setEnabled(false);
    removeLayerButton.setText("remove");
    moveUpButton.setText("up");
    moveDownButton.setText("down");

    controlButtonLayout.addStretch();
    controlButtonLayout.addWidget(&moveUpButton);
    controlButtonLayout.addWidget(&moveDownButton);
    controlButtonLayout.addWidget(&dupLayerButton);
    controlButtonLayout.addWidget(&addLayerButton);
    controlButtonLayout.addWidget(&removeLayerButton);

    mainLayout.addWidget(&optionsSpoiler);
    mainLayout.addWidget(&treeView);
    mainLayout.addLayout(&controlButtonLayout);
    setLayout(&mainLayout);


    QObject::connect(&dupLayerButton, &QToolButton::clicked, [this](){
        for (const auto & mindex : treeView.selectionModel()->selectedRows()) {
            const auto layeri = itemModel.ordered_i(mindex.row());
            const auto datasetIt = std::next(std::begin(Dataset::datasets), layeri);
            Dataset::datasets.insert(datasetIt, Dataset{*datasetIt});
            state->viewerState->layerOrder.emplace(std::next(std::begin(state->viewerState->layerOrder), mindex.row()), layeri);
            for (std::size_t i = mindex.row() + 1; i < Dataset::datasets.size(); ++i) {
                if (state->viewerState->layerOrder[i] >= layeri) {
                    ++state->viewerState->layerOrder[i];
                }
            }
            state->viewer->resizeTexEdgeLength(Dataset::current().cubeEdgeLength, state->M, Dataset::datasets.size());// update layerRenderSettings and textures
            if (Segmentation::singleton().layerId >= layeri) {
                ++Segmentation::singleton().layerId;
            }
            reloadLayers(true);
        }
    });
    QObject::connect(&addLayerButton, &QToolButton::clicked, [this](){
        itemModel.addItem();
    });

    QObject::connect(&removeLayerButton, &QToolButton::clicked, [this](){
        for (const auto & mindex : treeView.selectionModel()->selectedRows()) {
            const auto layeri = itemModel.ordered_i(mindex.row());
            Dataset::datasets.erase(std::next(std::begin(Dataset::datasets), layeri));
            state->viewerState->layerOrder.erase(std::next(std::begin(state->viewerState->layerOrder), mindex.row()));
            for (std::size_t i{layeri}; i < Dataset::datasets.size(); ++i) {
                if (state->viewerState->layerOrder[i] > layeri) {
                    --state->viewerState->layerOrder[i];
                }
            }
            state->viewer->resizeTexEdgeLength(Dataset::current().cubeEdgeLength, state->M, Dataset::datasets.size());// update layerRenderSettings and textures
            if (Segmentation::singleton().layerId >= layeri) {
                --Segmentation::singleton().layerId;
            }
            reloadLayers(true);
        }
    });

    QObject::connect(&moveUpButton, &QToolButton::clicked, [this](){
        const auto isMoveSeg = itemModel.ordered_i(treeView.selectionModel()->currentIndex().row()) == Segmentation::singleton().layerId;
        itemModel.moveItem(treeView.selectionModel()->currentIndex(), -1);
        if (isMoveSeg) {
            Segmentation::singleton().layerId = itemModel.ordered_i(treeView.selectionModel()->currentIndex().row());
        }
    });

    QObject::connect(&moveDownButton, &QToolButton::clicked, [this](){
        const auto isMoveSeg = itemModel.ordered_i(treeView.selectionModel()->currentIndex().row()) == Segmentation::singleton().layerId;
        itemModel.moveItem(treeView.selectionModel()->currentIndex(), 1);
        if (isMoveSeg) {
            Segmentation::singleton().layerId = itemModel.ordered_i(treeView.selectionModel()->currentIndex().row());
        }
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

    QObject::connect(&combineSlicesCheck, &QCheckBox::stateChanged, [this](int checkstate){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if (currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            state->viewerState->layerRenderSettings[ordered_index].combineSlicesEnabled = checkstate == Qt::Checked;
            state->viewer->layerRenderSettingsChanged();
            state->viewer->reslice_notify(ordered_index);
        }
    });
    QObject::connect(&combineSlicesType, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if (currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            state->viewerState->layerRenderSettings[ordered_index].combineSlicesType = static_cast<decltype(LayerRenderSettings::combineSlicesType)>(index);
            state->viewer->layerRenderSettingsChanged();
            state->viewer->reslice_notify(ordered_index);
        }
    });
    QObject::connect(&combineSlicesSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int value){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if (currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            state->viewerState->layerRenderSettings[ordered_index].combineSlices = value;
            state->viewer->layerRenderSettingsChanged();
            state->viewer->reslice_notify(ordered_index);
        }
    });
    QObject::connect(&combineSlicesXyOnlyCheck, &QCheckBox::stateChanged, [this](int checkstate){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if (currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            state->viewerState->layerRenderSettings[ordered_index].combineSlicesXyOnly = checkstate == Qt::Checked;
            state->viewer->layerRenderSettingsChanged();
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

    QObject::connect(&treeView, &QTreeView::doubleClicked, [this](const QModelIndex & index) {
        if (index.column() == 8) {
            colorDialog.setCurrentColor(treeView.model()->data(index, Qt::BackgroundRole).value<QColor>());
            if (state->viewer->suspend([this]{ return colorDialog.exec(); }) == QColorDialog::Accepted) {
                treeView.model()->setData(index, colorDialog.currentColor());
            }
        }
    });

    QObject::connect(state->viewer, &Viewer::layerRenderSettingsChanged, this, &LayerDialogWidget::updateLayerProperties);

    QObject::connect(treeView.selectionModel(), &QItemSelectionModel::selectionChanged, this, &LayerDialogWidget::updateLayerProperties);
    QObject::connect(&itemModel, &QAbstractItemModel::dataChanged, this, &LayerDialogWidget::updateLayerProperties);

    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this]() {
        itemModel.reset();
        updateLayerProperties();
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
    optionsSpoiler.setEnabled(currentIndex.isValid());
    if (currentIndex.isValid()) {
        const auto ordered_index = itemModel.ordered_i(currentIndex.row());
        auto & layerSettings = state->viewerState->layerRenderSettings[ordered_index];
        opacitySlider.setValue(static_cast<int>(layerSettings.opacity * opacitySlider.maximum()));
        rangeDeltaSlider.setValue(static_cast<int>(layerSettings.rangeDelta * rangeDeltaSlider.maximum()));
        biasSlider.setValue(static_cast<int>(layerSettings.bias * biasSlider.maximum()));
        combineSlicesCheck.setChecked(layerSettings.combineSlicesEnabled);
        combineSlicesType.setEnabled(layerSettings.combineSlicesEnabled);
        combineSlicesSpin.setEnabled(layerSettings.combineSlicesEnabled);
        combineSlicesXyOnlyCheck.setEnabled(layerSettings.combineSlicesEnabled);
        combineSlicesSpin.setRange(0, Dataset::datasets[ordered_index].cubeEdgeLength * 0.5 * (state->M - 1));
        combineSlicesSpin.setValue(layerSettings.combineSlices);
        combineSlicesXyOnlyCheck.setChecked(layerSettings.combineSlicesXyOnly);
        linearFilteringCheckBox.setChecked(layerSettings.textureFilter == QOpenGLTexture::Linear);
    }
}
