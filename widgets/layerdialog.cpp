#include "layerdialog.h"

#include "dataset.h"
#include "mainwindow.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QHeaderView>

#include <cstddef>
#include <iterator>
#include <unordered_map>

std::size_t LayerItemModel::ordered_i(std::size_t index) const {
    return state->viewerState->layerOrder[index];
}

LayerItemModel::LayerItemModel() {
    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, []() {
        // adjust datasetOrder size to fit all elements in dataset
        state->viewerState->layerOrder.clear();
        for(std::size_t i = state->viewerState->layerOrder.size(); i < Dataset::datasets.size(); ++i) {
            state->viewerState->layerOrder.emplace_back(i);
        }
    });

    QObject::connect(state->viewer, &Viewer::layerVisibilityChanged, this, [this](const std::size_t idx) {
        const auto & layerOrder = state->viewerState->layerOrder;
        if (const auto it = std::find(std::cbegin(layerOrder), std::cend(layerOrder), idx); it != std::cend(layerOrder)) {
            const auto modelIndex = index(std::distance(std::begin(layerOrder), it));
            emit dataChanged(modelIndex, modelIndex.siblingAtColumn(columnCount()));// +seg layer mark
        }
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
        auto& layerSettings = Dataset::datasets[ordered_i(index.row())].renderSettings;
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            return std::array<QVariant,9>{
                QVariant{},
                QString::number(layerSettings.opacity * 100.0f) + (role == Qt::EditRole ? "" : "%"),
                layerSettings.color,
                data.description,
                data.experimentname,
                QString("mag%1").arg(data.toMag(data.magIndex)),
                data.cubeShape.x,
                data.compressionString(),
                data.apiString(),
            }[index.column()];
        } else if (role == Qt::CheckStateRole) {
            if (index.column() == 0) {
                auto visible = Dataset::datasets[ordered_i(index.row())].renderSettings.visible;
                return visible ? Qt::Checked : Qt::Unchecked;
            } else if (index.column() == 7) {
                return Segmentation::singleton().enabled && ordered_i(index.row()) == Segmentation::singleton().layerId ? Qt::PartiallyChecked : data.isOverlay() ? Qt::Unchecked : QVariant{};
            }
        } else if (role == Qt::ForegroundRole) {
            if (index.column() == 2) {
                const auto color = layerSettings.color;
                const auto luminance = 0.299 * color.redF() + 0.587 * color.greenF() + 0.114 * color.blueF();
                return QColor((luminance > 0.5) ? Qt::black : Qt::white);
            }
        } else if (role == Qt::BackgroundRole) {
            if (index.column() == 2) {
                return layerSettings.color;
            }
        }
    }
    return QVariant();
}

void reloadLayers() {
    Segmentation::singleton().enabled = std::count_if(std::begin(Dataset::datasets), std::end(Dataset::datasets), [](const auto & dataset){
        return dataset.loadingEnabled && dataset.isOverlay();
    });
    state->viewer->loader_notify();
    emit state->mainWindow->widgetContainer.datasetLoadWidget.datasetChanged();// HACK
}

bool LayerItemModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if(index.isValid()) {
        auto& layerSettings = Dataset::datasets[ordered_i(index.row())].renderSettings;
        if (role == Qt::EditRole) {
            switch(index.column()) {
            case 1:
                layerSettings.opacity = std::min(value.toFloat() / 100.0f, 1.0f);
                break;
            case 2:
                layerSettings.color = value.value<QColor>();
                break;
            }
        } else if(role == Qt::CheckStateRole) {
            if(index.column() == 0) {
                state->viewer->setLayerVisibility(ordered_i(index.row()), value.toBool());
            } else if (index.column() == 7 && value.toBool()) {
                const auto beginIt = std::cbegin(state->viewerState->layerOrder);
                const auto segi = std::distance(beginIt, std::find(beginIt, std::cend(state->viewerState->layerOrder), Segmentation::singleton().layerId));
                const auto prevSegLayerModelIndex = index.siblingAtRow(segi);
                const auto prevSegLayerId = Segmentation::singleton().layerId;
                Segmentation::singleton().layerId = ordered_i(index.row());
                state->viewer->reslice_notify(prevSegLayerId);
                state->viewer->reslice_notify(Segmentation::singleton().layerId);
                emit dataChanged(prevSegLayerModelIndex, prevSegLayerModelIndex, QVector<int>(role));
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
    if (index.isValid()) {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        const std::unordered_map<int,Qt::ItemFlags> map{
            {0, Qt::ItemIsUserCheckable},
            {1, Qt::ItemIsEditable},
            {2, Qt::ItemIsEditable},
            {3, Qt::ItemIsEditable},
            {7, Qt::ItemIsUserCheckable},
        };
        if (auto it = map.find(index.column()); it != std::end(map)) {
            return flags | it->second;
        }
        return flags;
    }
    return {};
}

LayerDialogWidget::LayerDialogWidget(QWidget *parent) : DialogVisibilityNotify(PREFERENCES_WIDGET, parent), colorDialog(this) {
    setWindowTitle("Layers");

    int row = 0, col;
    opacitySlider.setMaximum(255);
    optionsLayout.addWidget(&opacitySliderLabel, row, col=0);
    optionsLayout.addWidget(&opacitySlider, row, ++col);
    optionsLayout.addWidget(&linearFilteringCheckBox, row, ++col);
    biasSlider.setMaximum(255);
    biasSpinBox.setMaximum(255);
    optionsLayout.addWidget(&biasSliderLabel, ++row, col=0);
    optionsLayout.addWidget(&biasSlider, row, ++col);
    optionsLayout.addWidget(&biasSpinBox, row, ++col);
    rangeDeltaSlider.setRange(-255, 255);
    rangeDeltaSlider.setTickInterval(255);
    rangeDeltaSpinBox.setRange(-255, 255);
    rangeDeltaSlider.setTickPosition(QSlider::TicksBothSides);
    optionsLayout.addWidget(&rangeDeltaSliderLabel, ++row, col=0);
    optionsLayout.addWidget(&rangeDeltaSlider, row, ++col);
    optionsLayout.addWidget(&rangeDeltaSpinBox, row, ++col);
    combineSlicesType.addItems({"min", "max"});
    optionsLayout.addWidget(&combineSlicesCheck, ++row, col=0);
    optionsLayout.addWidget(&combineSlicesType, row, ++col);
    optionsLayout.addWidget(&combineSlicesSpin, row, ++col);
    optionsLayout.addWidget(&combineSlicesXyOnlyCheck, row, ++col);
    optionsSpoiler.setContentLayout(optionsLayout, true);

    treeView.setModel(&itemModel);
    treeView.setRootIsDecorated(false);
    treeView.setUniformRowHeights(true); // for optimization
    for (auto i : {0,1,2,5,7,8}) {
        treeView.resizeColumnToContents(i);
    }

    dupLayerButton.setText("duplicate");
    addLayerButton.setText("add");
    addLayerButton.setEnabled(false);
    removeLayerButton.setText("remove");
    moveUpButton.setText("up");
    moveDownButton.setText("down");

    controlButtonLayout.addWidget(&invisibleButton);
    controlButtonLayout.addStretch();
    controlButtonLayout.addWidget(&moveUpButton);
    controlButtonLayout.addWidget(&moveDownButton);
    controlButtonLayout.addWidget(&dupLayerButton);
    controlButtonLayout.addWidget(&addLayerButton);
    controlButtonLayout.addWidget(&removeLayerButton);

    mainLayout.addWidget(&treeView);
    mainLayout.addLayout(&controlButtonLayout);
    mainLayout.addWidget(&optionsSpoiler);
    setLayout(&mainLayout);

    QObject::connect(&invisibleButton, &QToolButton::clicked, [](){
        for (std::size_t i{0}; i < Dataset::datasets.size(); ++i) {
            state->viewer->setLayerVisibility(i, false);
        }
    });
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
            state->viewer->resizeTexEdgeLength(Dataset::current().cubeShape.x, state->M, Dataset::datasets.size());// update layerRenderSettings and textures//FIXME
            if (Segmentation::singleton().layerId >= layeri) {
                ++Segmentation::singleton().layerId;
            }
            reloadLayers();
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
            state->viewer->resizeTexEdgeLength(Dataset::current().cubeShape.x, state->M, Dataset::datasets.size());// update layerRenderSettings and textures//FIXME
            if (Segmentation::singleton().layerId >= layeri) {
                --Segmentation::singleton().layerId;
            }
            reloadLayers();
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
            auto& layerSettings = Dataset::datasets[ordered_index].renderSettings;
            layerSettings.opacity = static_cast<float>(value) / opacitySlider.maximum();
            const auto& changeIndex = currentIndex.sibling(currentIndex.row(), 1); // todo: enum the 1
            emit itemModel.dataChanged(changeIndex, changeIndex, QVector<int>(Qt::EditRole));
        }
    });
    QObject::connect(&rangeDeltaSlider, &QAbstractSlider::valueChanged, this, &LayerDialogWidget::updateRangeDeltaHandle);
    QObject::connect(&rangeDeltaSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &LayerDialogWidget::updateRangeDeltaHandle);
    QObject::connect(&biasSlider, &QAbstractSlider::valueChanged, this, &LayerDialogWidget::updateBiasHandle);
    QObject::connect(&biasSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &LayerDialogWidget::updateBiasHandle);

    QObject::connect(&combineSlicesCheck, &QCheckBox::stateChanged, [this](int checkstate){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if (currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            Dataset::datasets[ordered_index].renderSettings.combineSlicesEnabled = checkstate == Qt::Checked;
            state->viewer->layerRenderSettingsChanged();
            state->viewer->reslice_notify(ordered_index);
        }
    });
    QObject::connect(&combineSlicesType, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if (currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            Dataset::datasets[ordered_index].renderSettings.combineSlicesType = static_cast<decltype(Dataset::LayerRenderSettings::combineSlicesType)>(index);
            state->viewer->layerRenderSettingsChanged();
            state->viewer->reslice_notify(ordered_index);
        }
    });
    QObject::connect(&combineSlicesSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int value){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if (currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            Dataset::datasets[ordered_index].renderSettings.combineSlices = value;
            state->viewer->layerRenderSettingsChanged();
            state->viewer->reslice_notify(ordered_index);
        }
    });
    QObject::connect(&combineSlicesXyOnlyCheck, &QCheckBox::stateChanged, [this](int checkstate){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if (currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            Dataset::datasets[ordered_index].renderSettings.combineSlicesXyOnly = checkstate == Qt::Checked;
            state->viewer->layerRenderSettingsChanged();
            state->viewer->reslice_notify(ordered_index);
        }
    });

    QObject::connect(&linearFilteringCheckBox, &QCheckBox::stateChanged, [this](int checkstate){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            auto& layerSettings = Dataset::datasets[ordered_index].renderSettings;
            layerSettings.textureFilter = (checkstate == Qt::Checked) ? QOpenGLTexture::Linear : QOpenGLTexture::Nearest;
            state->mainWindow->forEachOrthoVPDo([&](ViewportOrtho & orthoVP){
                orthoVP.applyTextureFilter();
            });
        }
    });

    createGlobalAction(state->mainWindow, Qt::CTRL + Qt::Key_M, [this](){// R for radius
        combineSlicesCheck.toggle();
        combineSlicesCheck.clicked(combineSlicesCheck.isChecked());
    });


    QObject::connect(&treeView, &QTreeView::doubleClicked, [this](const QModelIndex & index) {
        if (index.column() == 2) {
            colorDialog.setCurrentColor(treeView.model()->data(index, Qt::BackgroundRole).value<QColor>());
            if (state->viewer->suspend([this]{ return colorDialog.exec(); }) == QColorDialog::Accepted) {
                treeView.model()->setData(index, colorDialog.currentColor());
            }
        }
    });

    QObject::connect(state->viewer, &Viewer::layerRenderSettingsChanged, [this](){
        if (const auto & currentIndex = treeView.selectionModel()->currentIndex(); currentIndex.isValid()) {
            emit itemModel.dataChanged(currentIndex.siblingAtColumn(0), currentIndex.siblingAtColumn(itemModel.columnCount()-1));
        }
    });

    QObject::connect(treeView.selectionModel(), &QItemSelectionModel::selectionChanged, this, &LayerDialogWidget::updateLayerProperties);
    treeView.header()->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto action = new QAction("fit all columns to contents");
    treeView.header()->addAction(action);
    QObject::connect(action, &QAction::triggered, this, [this]{
        for (int i{}; i < itemModel.rowCount(); ++i) {
            treeView.resizeColumnToContents(i);
        }
    });
    action->trigger();
    QObject::connect(&itemModel, &QAbstractItemModel::dataChanged, this, &LayerDialogWidget::updateLayerProperties);

    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this]() {
        itemModel.reset();
        if (!treeView.selectionModel()->currentIndex().isValid() && itemModel.rowCount() > 0) {
            treeView.selectionModel()->setCurrentIndex(itemModel.index(0), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }
    });

    resize(800, 600);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void LayerDialogWidget::updateRangeDeltaHandle(const int value) {
    const auto& currentIndex = treeView.selectionModel()->currentIndex();
    if(currentIndex.isValid()) {
        std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
        auto& layerSettings = Dataset::datasets[ordered_index].renderSettings;
        layerSettings.rangeDelta = static_cast<float>(value) / rangeDeltaSlider.maximum();
        rangeDeltaSlider.setValue(value);
        rangeDeltaSpinBox.setValue(value);
        state->viewer->reslice_notify(ordered_index);
    }
}

void LayerDialogWidget::updateBiasHandle(const int value) {
    const auto& currentIndex = treeView.selectionModel()->currentIndex();
    if(currentIndex.isValid()) {
        std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
        auto& layerSettings = Dataset::datasets[ordered_index].renderSettings;
        layerSettings.bias = static_cast<float>(value) / biasSlider.maximum();
        biasSlider.setValue(value);
        biasSpinBox.setValue(value);
        state->viewer->reslice_notify(ordered_index);
    }
}

void LayerDialogWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(LAYER_DIALOG_WIDGET);
    restoreGeometry(settings.value(GEOMETRY).toByteArray());
    treeView.header()->restoreState(settings.value(HEADER).toByteArray());
    settings.endGroup();
}

void LayerDialogWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(LAYER_DIALOG_WIDGET);
    settings.setValue(GEOMETRY, saveGeometry());
    settings.setValue(HEADER, treeView.header()->saveState());
    settings.setValue(VISIBLE, isVisible());
    settings.endGroup();
}

void LayerDialogWidget::updateLayerProperties() {
    const auto & currentIndex = treeView.selectionModel()->currentIndex();
    optionsSpoiler.setEnabled(currentIndex.isValid());
    if (currentIndex.isValid()) {
        const auto ordered_index = itemModel.ordered_i(currentIndex.row());
        auto & layerSettings = Dataset::datasets[ordered_index].renderSettings;
        opacitySlider.setValue(static_cast<int>(layerSettings.opacity * opacitySlider.maximum()));
        biasSlider.setValue(static_cast<int>(layerSettings.bias * biasSlider.maximum()));
        rangeDeltaSlider.setValue(static_cast<int>(layerSettings.rangeDelta * rangeDeltaSlider.maximum()));
        linearFilteringCheckBox.setChecked(layerSettings.textureFilter == QOpenGLTexture::Linear);

        combineSlicesCheck.setChecked(layerSettings.combineSlicesEnabled);

        combineSlicesType.setEnabled(layerSettings.combineSlicesEnabled);
        combineSlicesSpin.setEnabled(layerSettings.combineSlicesEnabled);
        combineSlicesXyOnlyCheck.setEnabled(layerSettings.combineSlicesEnabled);

        combineSlicesType.setCurrentIndex(layerSettings.combineSlicesType);
        combineSlicesSpin.setRange(0, Dataset::datasets[ordered_index].cubeShape.x * 0.5 * (state->M - 1));//FIXME
        combineSlicesSpin.setValue(layerSettings.combineSlices);
        combineSlicesXyOnlyCheck.setChecked(layerSettings.combineSlicesXyOnly);
    }
}
