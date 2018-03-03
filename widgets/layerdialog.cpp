#include "layerdialog.h"

#include <QHeaderView>
#include "dataset.h"
#include "mainwindow.h"
#include "stateInfo.h"
#include "network.h"
#include "viewer.h"

std::size_t LayerItemModel::ordered_i(std::size_t index) const {
    return datasetOrder[index];
}

LayerItemModel::LayerItemModel() {
    for(std::size_t i = 0; i < Dataset::datasets.size(); ++i) {
        datasetOrder.emplace_back(i);
    }

    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this]() {
        // adjust datasetOrder size to fit all elements in dataset
        for(std::size_t i = datasetOrder.size(); i < Dataset::datasets.size(); ++i) {
            datasetOrder.emplace_back(i);
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
                case Dataset::CubeType::RAW_PNG:
                    return "RAW_PNG";
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
                auto visible = state->viewerState->layerVisibility[ordered_i(index.row())];
                return visible ? Qt::Checked : Qt::Unchecked;
            }
        }
    }
    return QVariant();
}

bool LayerItemModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if(index.isValid()) {
        auto& data = Dataset::datasets[ordered_i(index.row())];
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
                state->viewerState->layerVisibility[ordered_i(index.row())] = value.toBool();
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
            std::swap(datasetOrder[row], datasetOrder[row + offset]);
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
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            auto& selectedData = Dataset::datasets[ordered_index];
            selectedData.opacity = static_cast<float>(value) / opacitySlider.maximum();
            const auto& changeIndex = currentIndex.sibling(currentIndex.row(), 1); // todo: enum the 1
            emit itemModel.dataChanged(changeIndex, changeIndex, QVector<int>(Qt::EditRole));
        }
    });

    QObject::connect(&rangeDeltaSlider, &QAbstractSlider::valueChanged, [this](int value){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {;
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            auto& data = Dataset::datasets[ordered_index];
            data.rangeDelta = static_cast<float>(value) / rangeDeltaSlider.maximum();
        }
    });

    QObject::connect(&biasSlider, &QAbstractSlider::valueChanged, [this](int value){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            auto& data = Dataset::datasets[ordered_index];
            data.bias = static_cast<float>(value) / biasSlider.maximum();
        }
    });

    QObject::connect(&linearFilteringCheckBox, &QCheckBox::stateChanged, [this](int state){
        const auto& currentIndex = treeView.selectionModel()->currentIndex();
        if(currentIndex.isValid()) {
            std::size_t ordered_index = itemModel.ordered_i(currentIndex.row());
            auto& data = Dataset::datasets[ordered_index];
            data.linearFiltering = (state == Qt::Checked) ? true : false;
        }
    });

    QObject::connect(treeView.selectionModel(), &QItemSelectionModel::selectionChanged, this, &LayerDialogWidget::updateLayerProperties);
    QObject::connect(&itemModel, &QAbstractItemModel::dataChanged, this, &LayerDialogWidget::updateLayerProperties);

    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this]() {
        itemModel.reset();
    });

    resize(800, 600);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void LayerDialogWidget::updateLayerProperties() {
    std::size_t ordered_index = itemModel.ordered_i(treeView.selectionModel()->currentIndex().row());
    auto& data = Dataset::datasets[ordered_index];
    opacitySlider.setValue(static_cast<int>(data.opacity * opacitySlider.maximum()));
    rangeDeltaSlider.setValue(static_cast<int>(data.rangeDelta * rangeDeltaSlider.maximum()));
    biasSlider.setValue(static_cast<int>(data.bias * biasSlider.maximum()));
    linearFilteringCheckBox.setChecked(data.linearFiltering);
}

LayerLoadWidget::LayerLoadWidget(QWidget *parent) : QDialog(parent) {
    setModal(true);

    datasetSettingsLayout.addRow(&fovSpin, &superCubeSizeLabel);
    datasetSettingsLayout.addRow(&segmentationOverlayCheckbox);
    datasetSettingsLayout.addRow(&reloadRequiredLabel);
    datasetSettingsGroup.setLayout(&datasetSettingsLayout);

    int row = 0;
    listLayout.addWidget(&datasetLoadLabel, row, 0, Qt::AlignCenter);
    listLayout.addWidget(&sessionLayerLabel, row, 1, Qt::AlignCenter);

    listLayout.addWidget(&datasetLoadList, ++row, 0);
    listLayout.addWidget(&sessionLayerList, row, 1);

    listLayout.addWidget(&infoLabel, ++row, 0);
    listLayout.addWidget(&datasetSettingsGroup, row, 1);

    buttonLayout.addWidget(&loadButton, 0, Qt::AlignRight);
    buttonLayout.addWidget(&cancelButton, 0, Qt::AlignLeft);

    mainLayout.addLayout(&listLayout);
    mainLayout.addLayout(&buttonLayout);
    setLayout(&mainLayout);

    loadButton.setText("Load");
    cancelButton.setText("Cancel");

    cubeEdgeSpin.setRange(1, 256);
    cubeEdgeSpin.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    fovSpin.setSuffix(" px");
    fovSpin.setAlignment(Qt::AlignLeft);
    fovSpin.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    infoLabel.setOpenExternalLinks(true);
    infoLabel.setTextInteractionFlags(Qt::TextBrowserInteraction);
    infoLabel.setWordWrap(true);//allows shrinking below minimum width

    static QListWidgetItem testItem;
    static QWidget testWidget;
    static QLabel testWidgetText{"text"};
    static QPushButton testwidgetButton{"button"};
    static QHBoxLayout testwidgetLayout;
    testwidgetLayout.addWidget(&testWidgetText);
    testwidgetLayout.addWidget(&testwidgetButton);
    testwidgetLayout.addStretch();
    testwidgetLayout.setSizeConstraint(QLayout::SetFixedSize);
    testWidget.setLayout(&testwidgetLayout);
    testItem.setSizeHint(testWidget.sizeHint());
    datasetLoadList.setItemWidget(&testItem, &testWidget);

    QObject::connect(&datasetLoadList, &QListWidget::itemSelectionChanged, this, &LayerLoadWidget::updateDatasetInfo);
    QObject::connect(&fovSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &LayerLoadWidget::adaptMemoryConsumption);

    resize(900, 500);
}

void LayerLoadWidget::updateDatasetInfo() {
    bool bad = datasetLoadList.selectedItems().empty();
    QString dataset;
    bad = bad || (dataset = datasetLoadList.selectedItems().front()->text()).isEmpty();
    decltype(Network::singleton().refresh(std::declval<QUrl>())) download;
    const QUrl url{dataset + (!QUrl{dataset}.isLocalFile() && !Dataset::isWebKnossos(dataset) ? "/" : "")};// add slash to avoid redirects
    bad = bad || !(download = Network::singleton().refresh(url)).first;
    if (bad) {
        infoLabel.setText("");
        return;
    }

    const auto datasetinfo = Dataset::parse(url, download.second).front();

    //make sure supercubeedge is small again
    auto supercubeedge = (fovSpin.value() + cubeEdgeSpin.value()) / datasetinfo.cubeEdgeLength;
    supercubeedge = std::max(3, supercubeedge - !(supercubeedge % 2));
    fovSpin.setCubeEdge(datasetinfo.cubeEdgeLength);
    fovSpin.setValue((supercubeedge - 1) * datasetinfo.cubeEdgeLength);
    cubeEdgeSpin.setValue(datasetinfo.cubeEdgeLength);
    adaptMemoryConsumption();

    QString infotext = tr("<b>%1 Dataset</b><br />%2");
    if (datasetinfo.remote) {
        infotext = infotext.arg("Remote").arg("URL: <a href=\"%1\">%1</a><br />").arg(datasetinfo.url.toString());
    } else {
        infotext = infotext.arg("Local").arg("");
    }
    infotext += QString("Name: %1<br />Boundary (x y z): %2 %3 %4<br />Compression: %5<br />cubeEdgeLength: %6<br />Magnification: %7<br />Scale (x y z): %8 %9 %10")
        .arg(datasetinfo.experimentname)
        .arg(datasetinfo.boundary.x).arg(datasetinfo.boundary.y).arg(datasetinfo.boundary.z)
        .arg(datasetinfo.compressionString())
        .arg(datasetinfo.cubeEdgeLength)
        .arg(datasetinfo.magnification)
        .arg(datasetinfo.scale.x)
        .arg(datasetinfo.scale.y)
        .arg(datasetinfo.scale.z);

    infoLabel.setText(infotext);

    if (datasetSettingsLayout.indexOf(&cubeEdgeSpin) != -1) {
        datasetSettingsLayout.takeRow(&cubeEdgeSpin);
    }
    cubeEdgeSpin.setParent(nullptr);
//    cubeEdgeLabel.setParent(nullptr);
    if (!Dataset::isHeidelbrain(url)) {
//        datasetSettingsLayout.insertRow(0, &cubeEdgeSpin, &cubeEdgeLabel);
    }
}

void LayerLoadWidget::adaptMemoryConsumption() {
    const auto fov = fovSpin.value();
    auto mebibytes = std::pow(fov + cubeEdgeSpin.value(), 3) / std::pow(1024, 2);
    mebibytes += segmentationOverlayCheckbox.isChecked() * OBJID_BYTES * mebibytes;
    auto text = QString("FOV per dimension (%1 MiB RAM)").arg(mebibytes);
    superCubeSizeLabel.setText(text);
}

void LayerLoadWidget::loadSettings() {
    auto transitionedDataset = [](const QString & dataset){//update old files from settings
        QUrl url = dataset;
        if (QRegularExpression("^[A-Z]:").match(dataset).hasMatch()) {//set file scheme for windows drive letters
            url = QUrl::fromLocalFile(dataset);
        }
        if (url.isRelative()) {
            url = QUrl::fromLocalFile(dataset);
        }
        return url;
    };

    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    restoreGeometry(settings.value(DATASET_GEOMETRY, "").toByteArray());
    auto datasetUrl = transitionedDataset(settings.value(DATASET_LAST_USED, "").toString());

    // add datasets from file
    for (const auto & dataset : settings.value(DATASET_MRU).toStringList()) {
        datasetLoadList.addItem(dataset);
    }
    // add public datasets
    auto datasetsDir = QDir(":/resources/datasets");
    for (const auto & dataset : datasetsDir.entryInfoList()) {
        const auto url = QUrl::fromLocalFile(dataset.absoluteFilePath()).toString();
        if (datasetLoadList.findItems(url, Qt::MatchExactly).empty()) {
            datasetLoadList.addItem(url);
        }
    }
    // add Empty row at the end
    datasetLoadList.addItem("");

//    updateDatasetInfo();
    auto & cubeEdgeLen = Dataset::current().cubeEdgeLength;
//    cubeEdgeLen = settings.value(DATASET_CUBE_EDGE, 128).toInt();
//    if (QApplication::arguments().filter("supercube-edge").empty()) {//if not provided by cmdline
//        state->M = settings.value(DATASET_SUPERCUBE_EDGE, 3).toInt();
//    }
//    if (QApplication::arguments().filter("overlay").empty()) {//if not provided by cmdline
//        Dataset::current().overlay = settings.value(DATASET_OVERLAY, false).toBool();
//    }
//    state->viewer->resizeTexEdgeLength(cubeEdgeLen, state->M);

    cubeEdgeSpin.setValue(cubeEdgeLen);
    fovSpin.setCubeEdge(cubeEdgeLen);
    fovSpin.setValue(cubeEdgeLen * (state->M - 1));
    segmentationOverlayCheckbox.setChecked(Dataset::current().isOverlay());
    adaptMemoryConsumption();
    settings.endGroup();
    datasetLoadList.setCurrentRow(0);

//    updateDatasetInfo();
//    applyGeometrySettings();
}
