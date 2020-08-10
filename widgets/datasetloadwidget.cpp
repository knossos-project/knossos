/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "datasetloadwidget.h"

#include "brainmaps.h"
#include "dataset.h"
#include "GuiConstants.h"
#include "loader.h"
#include "mainwindow.h"
#include "network.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/tools/model_helper.h"

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <functional>
#include <stdexcept>

QVariant DatasetModel::data(const QModelIndex & index, int role) const {
    if (index.isValid() && (role == Qt::DisplayRole || role == Qt::EditRole)) {
        return datasets[index.row()];
    }
    return {};
}

bool DatasetModel::setData(const QModelIndex & index, const QVariant & value, int role) {
    if (index.isValid()) {
        datasets[index.row()] = value.toString();
        if (index.row() == rowCount() - 1 && !index.data().toString().isEmpty()) {
            beginInsertRows({}, datasets.size(), datasets.size());
            datasets.push_back("");
            endInsertRows();
        }
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}

Qt::ItemFlags DatasetModel::flags(const QModelIndex & index) const {
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled;
}

int DatasetModel::rowCount(const QModelIndex &) const {
    return datasets.size();
}

void DatasetModel::add(const QString & datasetPath) {
    if (datasets.empty()) {
        beginInsertRows({}, datasets.size(), datasets.size() + 1);
        datasets.push_back(datasetPath);
    } else {
        beginInsertRows({}, datasets.size(), datasets.size());
        datasets.back() = datasetPath;
    }
    datasets.push_back("");
    endInsertRows();
}

bool DatasetModel::removeRows(int row, int count, const QModelIndex & parent) {
    beginRemoveRows(parent, row, row + count - 1);
    datasets.erase(datasets.begin() + row, datasets.begin() + row + count);
    endRemoveRows();
    return true;
}

void DatasetModel::clear() {
    beginResetModel();
    datasets.clear();
    endResetModel();
}

bool SortFilterProxy::lessThan(const QModelIndex & source_left, const QModelIndex & source_right) const {
    if (source_left.row() == sourceModel()->rowCount() - 1) {
        return sortOrder() != Qt::AscendingOrder;
    } else if (source_right.row() == sourceModel()->rowCount() - 1) {
        return sortOrder() == Qt::AscendingOrder;
    } else {
        return source_left.data().toString() < source_right.data().toString();
    }
}

ButtonListView::ButtonListView(DatasetModel & datasetModel, SortFilterProxy & proxy, QWidget * parent) : QTreeView(parent), datasetModel(&datasetModel), proxy(&proxy) {
    setHeader(&listHeader);
    proxy.setSourceModel(&datasetModel);
    proxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
    setModel(&proxy);
    setSortingEnabled(true);
    sortByColumn(sortIndex = -1, Qt::SortOrder::AscendingOrder);
    fileDialogButton.setParent(this);
    deleteButton.setParent(this);
    fileDialogButton.hide();
    deleteButton.hide();

    QObject::connect(this, &ButtonListView::mouseLeft, [this]() {
        fileDialogButton.hide();
        deleteButton.hide();
    });
    QObject::connect(&listHeader, &ButtonHeaderView::mouseEntered, this, &ButtonListView::mouseLeft);
    QObject::connect(&listHeader, &ButtonHeaderView::sortIndicatorChanged, threeWaySorting(*this, sortIndex));

    QObject::connect(&fileDialogButton, &QPushButton::clicked, [this]() {
        auto indexToEdit = indexAt(fileDialogButton.pos() - QPoint(0, header()->height()));
        const auto selectedFile = ::state->viewer->suspend([this, &indexToEdit] {
            auto currentUrl = QUrl(indexToEdit.data().toString());
            auto workingDir = currentUrl.isEmpty() || !currentUrl.isLocalFile() ? QDir::homePath() : QString("file://") + currentUrl.toLocalFile();
            return QFileDialog::getOpenFileUrl(this, "Select a KNOSSOS dataset", workingDir, "*.conf").toString();
        });
        if (!selectedFile.isEmpty()) {
            model()->setData(indexToEdit, selectedFile);
        }
        selectionModel()->select(indexToEdit, QItemSelectionModel::ClearAndSelect);
    });
    QObject::connect(&deleteButton, &QPushButton::clicked, [this]() {
        auto indexToDel = indexAt(deleteButton.pos() - QPoint(0, header()->height()));
        model()->removeRows(indexToDel.row(), 1);
    });
}

void ButtonListView::addDatasetUrls(QDropEvent * e) {
    auto index = indexAt(e->pos());
    if (e->mimeData()->urls().size() == 1 && index.isValid()) {
        const auto droppedDataset = e->mimeData()->urls().first();
        model()->setData(index, droppedDataset);
    } else if(e->mimeData()->urls().size() > 0) {
        for (auto && url : e->mimeData()->urls()) {
            datasetModel->add(url.url());
        }
    }
    proxy->setFilterFixedString(filterString); // force update filter
    auto lastIndex = proxy->mapFromSource(datasetModel->index(datasetModel->rowCount() - 2, 0));
    selectionModel()->select(lastIndex, QItemSelectionModel::ClearAndSelect);
    scrollTo(lastIndex);
}

void ButtonListView::dragEnterEvent(QDragEnterEvent * e) {
    if (e->mimeData()->urls().size() > 0) {
        e->accept();
    }
}

void ButtonListView::dragMoveEvent(QDragMoveEvent * e) {
    e->accept();
}

void ButtonListView::dropEvent(QDropEvent * e) {
    addDatasetUrls(e);
}

void ButtonDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
    QStyledItemDelegate::paint(painter, option, index); // paint text
    if (option.state & QStyle::State_MouseOver) {
        auto * buttonView = qobject_cast<ButtonListView *>(parent());
        auto xOffset = buttonView->visualRect(index).right() - option.rect.height();
        for (auto * button : {&buttonView->fileDialogButton, &buttonView->deleteButton}) {
            button->setGeometry(QRect(xOffset, option.rect.top() + buttonView->header()->height(),
                                      option.rect.height(), option.rect.height())); // quadratic button
            auto row = buttonView->proxy->mapToSource(index).row();
            button->setVisible(row <  buttonView->datasetModel->rowCount() - 1 || button == &buttonView->fileDialogButton); // no delete button for empty last row
            xOffset -= option.rect.height();
        }
    }
}

DatasetLoadWidget::DatasetLoadWidget(QWidget *parent) : DialogVisibilityNotify(DATASET_WIDGET, parent) {
    setModal(true);
    setWindowTitle("Load Dataset");
    setAcceptDrops(true);

    searchField.setPlaceholderText("Filter datasets…");
    tableWidget.setUniformRowHeights(true);
    tableWidget.setTextElideMode(Qt::ElideMiddle);
    tableWidget.setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget.setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget.setItemDelegate(&addButtonDelegate);
    tableWidget.setMouseTracking(true);
    tableWidget.setDragEnabled(true);
    tableWidget.setAcceptDrops(true);
    tableWidget.setDropIndicatorShown(true);
    tableWidget.setRootIsDecorated(false);
    tableWidget.setSortingEnabled(true);
    // add border line, requires restoring style for item:selected
    const auto & palette = tableWidget.palette();
    tableWidget.setStyleSheet(QString("QTreeView::item { border-bottom: 1px solid %1; } QTreeView::item:selected { background-color: %2; color: %3; }")
                              .arg(palette.midlight().color().name(), palette.highlight().color().name(), palette.highlightedText().color().name()));
    infoLabel.setOpenExternalLinks(true);
    infoLabel.setTextInteractionFlags(Qt::TextBrowserInteraction);
    infoLabel.setWordWrap(true);//allows shrinking below minimum width
    splitter.setOrientation(Qt::Vertical);
    splitter.addWidget(&tableWidget);
    splitter.addWidget(&infoLabel);
    splitter.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    cubeEdgeSpin.setRange(1, 256);
    cubeEdgeSpin.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    fovSpin.setSuffix(" px");
    fovSpin.setAlignment(Qt::AlignLeft);
    fovSpin.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    datasetSettingsLayout.addRow(&fovSpin, &superCubeSizeLabel);
    datasetSettingsLayout.addRow(&segmentationOverlayCheckbox);
    datasetSettingsLayout.addRow(&reloadRequiredLabel);
    datasetSettingsGroup.setLayout(&datasetSettingsLayout);

    buttonHLayout.addWidget(&processButton);
    buttonHLayout.addWidget(&cancelButton);
    mainLayout.addWidget(&searchField);
    mainLayout.addWidget(&splitter);
    mainLayout.addWidget(&datasetSettingsGroup);
    mainLayout.addLayout(&buttonHLayout);

    setLayout(&mainLayout);
    QObject::connect(&searchField, &QLineEdit::textChanged, [this](const QString & text) {
        tableWidget.filterString = text;
        sortAndFilterProxy.setFilterFixedString(text);
    });
    QObject::connect(&datasetModel, &DatasetModel::dataChanged, this, &DatasetLoadWidget::datasetCellChanged);
    QObject::connect(tableWidget.selectionModel(), &QItemSelectionModel::selectionChanged, [this]() {
        WidgetDisabler d{*this};// don’t allow widget interaction while Network has an event loop running
        bool bad = tableWidget.selectionModel()->selectedIndexes().isEmpty();
        QUrl dataset;
        bad = bad || (dataset = tableWidget.selectionModel()->selectedIndexes().front().data().toString()).isEmpty();
        decltype(Network::singleton().refresh(std::declval<QUrl>())) download;
        bad = bad || !(download = Network::singleton().refresh(dataset)).first;
        if (bad) {
            infoLabel.setText("");
        } else {
            updateDatasetInfo(dataset, download.second);
        }
    });
    QObject::connect(&cubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int cubeedge){
        fovSpin.setCubeEdge(cubeedge);
        adaptMemoryConsumption();
    });
    QObject::connect(&fovSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&segmentationOverlayCheckbox, &QCheckBox::stateChanged, this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&processButton, &QPushButton::clicked, this, &DatasetLoadWidget::processButtonClicked);
    static auto resetSettings = [this]() {
        fovSpin.setValue(Dataset::current().cubeEdgeLength * (state->M - 1));
        segmentationOverlayCheckbox.setChecked(Segmentation::singleton().enabled);
    };
    QObject::connect(this, &DatasetLoadWidget::rejected, []() { resetSettings(); });
    QObject::connect(&cancelButton, &QPushButton::clicked, [this]() { resetSettings(); hide(); });

    QObject::connect(&state->viewer->mainWindow, &MainWindow::datasetDropped, [this](const QUrl & url) {
        loadDataset(boost::none, url);
    });
    resize(600, 600);//random default size, will be overriden by settings if present
}

void DatasetLoadWidget::dragEnterEvent(QDragEnterEvent * e) {
    if (e->mimeData()->urls().size() > 0) {
        e->accept();
    }
}

void DatasetLoadWidget::dropEvent(QDropEvent * e) {
    tableWidget.addDatasetUrls(e);
}

void DatasetLoadWidget::datasetCellChanged(const QModelIndex & topLeft, const QModelIndex &, const QVector <int> &) {
    auto index = topLeft;
    if (!index.data().toString().isEmpty()) {
        WidgetDisabler d{*this};// don’t allow widget interaction while Network has an event loop running
        auto dataset = index.data().toString();
        decltype(Network::singleton().refresh(std::declval<QUrl>())) download = Network::singleton().refresh(dataset);
        if (download.first) {
            updateDatasetInfo(dataset, download.second);
        } else {
            infoLabel.setText("");
        }
    }
}

void DatasetLoadWidget::updateDatasetInfo(const QUrl & url, const QString & info) try {
    infos = Dataset::parse(url, info, segmentationOverlayCheckbox.isChecked());
    const auto datasetinfo = infos.front();
    //make sure supercubeedge is small again
    auto supercubeedge = (fovSpin.value() + cubeEdgeSpin.value()) / datasetinfo.cubeEdgeLength;
    supercubeedge = std::max(3, supercubeedge - !(supercubeedge % 2));
    fovSpin.setCubeEdge(datasetinfo.cubeEdgeLength);
    fovSpin.setValue((supercubeedge - 1) * datasetinfo.cubeEdgeLength);
    cubeEdgeSpin.setValue(datasetinfo.cubeEdgeLength);
    adaptMemoryConsumption();

    QString infotext = tr("<b>%1 Dataset</b><br />%2");
    if (!datasetinfo.url.isLocalFile()) {
        infotext = infotext.arg("Remote").arg("URL: <a href=\"%1\">%1</a><br />").arg(datasetinfo.url.toString());
    } else {
        infotext = infotext.arg("Local").arg("");
    }
    infotext += QString("Name: %1<br/>Boundary (x y z): %2 %3 %4<br />Compression: %5<br/>cubeEdgeLength: %6<br/>Magnification: %7<br/>Scale (x y z): %8 %9 %10<br/>Description: %11")
        .arg(datasetinfo.experimentname)
        .arg(datasetinfo.boundary.x).arg(datasetinfo.boundary.y).arg(datasetinfo.boundary.z)
        .arg(datasetinfo.compressionString())
        .arg(datasetinfo.cubeEdgeLength)
        .arg(datasetinfo.magnification)
        .arg(datasetinfo.scale.x)
        .arg(datasetinfo.scale.y)
        .arg(datasetinfo.scale.z)
        .arg(datasetinfo.description);

    infoLabel.setText(infotext);
    infoLabel.setMaximumHeight(infoLabel.sizeHint().height());
    if (datasetSettingsLayout.indexOf(&cubeEdgeSpin) != -1) {
        datasetSettingsLayout.takeRow(&cubeEdgeSpin);
    }
    cubeEdgeSpin.setParent(nullptr);
    cubeEdgeLabel.setParent(nullptr);
    if (!(datasetinfo.api == Dataset::API::Heidelbrain || datasetinfo.api == Dataset::API::PyKnossos)) {
        datasetSettingsLayout.insertRow(0, &cubeEdgeSpin, &cubeEdgeLabel);
    }
} catch (std::exception &) {
    infoLabel.setText("");
}

void DatasetLoadWidget::adaptMemoryConsumption() {
    const auto fov = fovSpin.value();
    auto mebibytes = std::pow(fov + cubeEdgeSpin.value(), 3) / std::pow(1024, 2);
    mebibytes += segmentationOverlayCheckbox.isChecked() * OBJID_BYTES * mebibytes;
    mebibytes += infos.size() * std::pow(std::pow(2, std::ceil(std::log2(fov + cubeEdgeSpin.value()))), 2) * 4 * 2 * 3 / std::pow(2, 20);
    auto text = QString("FOV per dimension (%1 MiB memory)").arg(mebibytes);
    superCubeSizeLabel.setText(text);
}

void DatasetLoadWidget::processButtonClicked() {
    const auto selection = tableWidget.selectionModel()->selectedIndexes();
    if (selection.isEmpty()) {
        QMessageBox box{QApplication::activeWindow()};
        box.setIcon(QMessageBox::Information);
        box.setText(tr("Unable to load"));
        box.setInformativeText(tr("No path selected"));
        box.exec();
    } else if (loadDataset(boost::none, selection[0].data().toString())) {
        hide(); //hide datasetloadwidget only if we could successfully load a dataset
    }
}

/* dataset can be selected in three ways:
 * 1. by selecting the folder containing a k.conf (for multires datasets it's a "magX" folder)
 * 2. for multires datasets: by selecting the dataset folder (the folder containing the "magX" subfolders)
 * 3. by specifying a .conf directly.
 */
bool DatasetLoadWidget::loadDataset(const boost::optional<bool> loadOverlay, QUrl path, const bool silent) {
    WidgetDisabler d{*this};// don’t allow widget interaction while Network has an event loop running
    if (path.isEmpty() && datasetUrl.isEmpty()) {//no dataset available to load
        open();
        return false;
    } else if (path.isEmpty()) {//if empty reload previous
        path = datasetUrl;
    }
    const auto download = Network::singleton().refresh(path);
    if (!Dataset::isGoogleBrainmaps(path) && !download.first) {
        if (!silent) {
            QMessageBox warning{QApplication::activeWindow()};
            warning.setIcon(QMessageBox::Warning);
            warning.setText(tr("Unable to load Dataset."));
            warning.setInformativeText(tr("Failed to read config file from %1").arg(path.toString()));
            warning.exec();
            open();
        }
        qDebug() << "no config at" << path;
        return false;
    }

    const auto iter = std::find(std::begin(datasetModel.datasets), std::end(datasetModel.datasets), path.url());
    auto row = iter - std::begin(datasetModel.datasets);
    if (iter == std::end(datasetModel.datasets)) {
        row = datasetModel.rowCount() - 1;
        datasetModel.add(path.url());
    }
    tableWidget.selectionModel()->select(datasetModel.index(row, 0), QItemSelectionModel::ClearAndSelect);

    bool keepAnnotation = silent;
    if (!silent && !Annotation::singleton().isEmpty()) {
        QMessageBox question{QApplication::activeWindow()};
        question.setIcon(QMessageBox::Question);
        question.setText(tr("Keep the current annotation for the new dataset?"));
        question.setInformativeText(tr("It only makes sense to keep the annotation if the new dataset matches it."));
        const auto * const keepButton = question.addButton(tr("Keep"), QMessageBox::AcceptRole);
        const auto * const clearButton = question.addButton(tr("Start &new one"), QMessageBox::RejectRole);
        const auto * const cancelButton = question.addButton(QMessageBox::Cancel);
        question.exec();
        if (question.clickedButton() == cancelButton || (question.clickedButton() == clearButton && !state->viewer->window->newAnnotationSlot())) {// clear skeleton, mergelist and snappy cubes
            return false;
        }
        keepAnnotation = question.clickedButton() == keepButton;
    }
    auto data = download.second;

    QString token;
    if (Dataset::isGoogleBrainmaps(path)) {
        const auto pair = getBrainmapsToken();
        if (!pair.first) {
            qDebug() << "getBrainmapsToken failed";
            return false;
        }
        token = pair.second;

        auto googleRequest = [&token](auto path){
            QNetworkRequest request(path);
            request.setRawHeader("Authorization", (QString("Bearer ") + token).toUtf8());
            return request;
        };

        auto * reply = Network::singleton().manager.get(googleRequest(QUrl("https://brainmaps.googleapis.com/v1/volumes")));
        const auto datasets = blockDownloadExtractData(*reply);
        qDebug() << datasets.second;

        reply = Network::singleton().manager.get(googleRequest(path));
        const auto config = blockDownloadExtractData(*reply);

        if (config.first) {
            data = config.second;
        } else {
            qDebug() << "download failed";
            return false;
        }
    }

    auto layers = [this, &path, &data, &loadOverlay, &silent]() {
        try {
            return Dataset::parse(path, data, loadOverlay.get_value_or(segmentationOverlayCheckbox.isChecked()));
        } catch(std::exception & e) {
            if (!silent) {
                QMessageBox warning{QApplication::activeWindow()};
                warning.setIcon(QMessageBox::Warning);
                warning.setText(tr("Failed to load dataset"));
                warning.setInformativeText(tr("%1\n\n%2").arg(path.toString()).arg(e.what()));
                warning.exec();
            }
            qDebug() << "Failed to load dataset" << path << e.what();
            return Dataset::list_t{};
        }
    }();
    if (layers.empty()) {
        return false;
    }
    if (Dataset::isHeidelbrain(path)) {
        try {
            layers.front().checkMagnifications();
            for (auto & layer : layers) {// apply discovered mags to all layers
                layer.magnification = layers.front().magnification;
                layer.lowestAvailableMag = layers.front().lowestAvailableMag;
                layer.highestAvailableMag = layers.front().highestAvailableMag;
                for (int mag = 1; mag <= layer.highestAvailableMag; mag *= 2) {
                    layer.scales.emplace_back(layer.scale * mag);
                }
                layer.magIndex = static_cast<std::size_t>(std::log2(layer.magnification));
                layer.scale = layer.scales[layer.magIndex];
                layer.scaleFactor = layer.scale / layer.scales[0];
            }
        } catch (std::exception &) {
            if (!silent) {
                QMessageBox warning{QApplication::activeWindow()};
                warning.setIcon(QMessageBox::Warning);
                warning.setText(tr("Dataset will not be loaded."));
                warning.setInformativeText(tr("No magnifications could be detected. (knossos.conf in mag folder)"));
                warning.exec();
                open();
            }
            qDebug() << "no mags";
            return false;
        }
    } else if (Dataset::isGoogleBrainmaps(path)) {
        for (auto & layer : layers) {
            layer.token = token;
        }
    }

    qDebug() << "loading dataset" << datasetUrl;

    datasetUrl = {path};//remember config url
    Loader::Controller::singleton().suspendLoader();//we change variables the loader uses
    const bool changedBoundaryOrScale = layers.front().boundary != Dataset::current().boundary || layers.front().scale != Dataset::current().scale;

    // check if a fundamental geometry variable has changed. If so, the loader requires reinitialization
    const auto cubeEdgeLen = cubeEdgeSpin.text().toInt();
    for (auto && layer : layers) {
        if (!(layer.api == Dataset::API::Heidelbrain || layer.api == Dataset::API::PyKnossos)) {
            layer.cubeEdgeLength = cubeEdgeLen;
        }
    }
    state->M = (fovSpin.value() + cubeEdgeLen) / cubeEdgeLen;
    if (loadOverlay != boost::none) {
        segmentationOverlayCheckbox.setChecked(loadOverlay.get());
    }
    Segmentation::singleton().enabled = segmentationOverlayCheckbox.isChecked();
    for (std::size_t i = 0; i < layers.size(); ++i) {// determine segmentation layer
        if (layers[i].isOverlay()) {
            Segmentation::singleton().layerId = i;
            break;// only enable the first overlay layer by default
        }
    }
    Dataset::datasets = layers;

    state->viewer->resizeTexEdgeLength(cubeEdgeLen, state->M, Dataset::datasets.size());// resets textures

    updateDatasetInfo(path, download.second);
    applyGeometrySettings();

    if (changedBoundaryOrScale || !keepAnnotation) {
        emit datasetSwitchZoomDefaults();
        // reset skeleton viewport
        if (state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_RESET;
        }
    }

    emit updateDatasetCompression();

    if (changedBoundaryOrScale || !keepAnnotation) {
        Annotation::singleton().updateMovementArea({0, 0, 0}, Dataset::current().boundary);
        // ...beginning with loading the middle of dataset
        state->viewer->setPosition({Dataset::current().boundary / 2});
    }
    state->viewer->applyTextureFilterSetting(state->viewerState->textureFilter);// set filter for all layers
    state->viewer->datasetColorAdjustmentsChanged();// set range delta and bias for all layers
    state->viewer->updateDatasetMag();// clear vps and notify loader

    emit datasetChanged();

    return true;
}

void DatasetLoadWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    settings.setValue(DATASET_LAST_USED, datasetUrl);

    settings.setValue(DATASET_MRU, datasetModel.datasets.filter(QRegularExpression(".+")));

    settings.setValue(DATASET_CUBE_EDGE, Dataset::current().cubeEdgeLength);
    settings.setValue(DATASET_SUPERCUBE_EDGE, state->M);
    settings.setValue(DATASET_OVERLAY, Segmentation::singleton().enabled);

    settings.endGroup();
}

void DatasetLoadWidget::applyGeometrySettings() {
    //settings depending on supercube and cube size
    state->cubeSliceArea = std::pow(Dataset::current().cubeEdgeLength, 2);
    state->cubeBytes = std::pow(Dataset::current().cubeEdgeLength, 3);
    state->cubeSetElements = std::pow(state->M, 3);
    state->cubeSetBytes = state->cubeSetElements * state->cubeBytes;

    state->viewer->window->resetTextureProperties();
}

void DatasetLoadWidget::loadSettings() {
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

    restoreGeometry(settings.value(GEOMETRY).toByteArray());
    datasetUrl = transitionedDataset(settings.value(DATASET_LAST_USED, "").toString());
    datasetModel.clear(); // prevent dataset duplication on loading custom settings
    auto appendRowSelectIfLU = [this](const QString & dataset){
        datasetModel.add(dataset);
        if (dataset == datasetUrl.toString()) {
            tableWidget.selectionModel()->select(datasetModel.index(datasetModel.rowCount() - 2, 0), QItemSelectionModel::ClearAndSelect);
        }
    };

    {
        QSignalBlocker blocker{tableWidget};// we don’t want to process these in datasetCellChanged
        // add datasets from file
        for (const auto & dataset : settings.value(DATASET_MRU).toStringList()) {
            const auto obsoleteConfs = QSet<QString>{"file::/resources/datasets/e2006.k.conf", "file::/resources/datasets/ek0563.k.conf", "file::/resources/datasets/j0256.k.conf"};
            if (!obsoleteConfs.contains(dataset)) {
                appendRowSelectIfLU(transitionedDataset(dataset).toString());
            }
        }
        // add public datasets
        const auto datasetsDir = QDir(":/resources/datasets");
        for (const auto & dataset : datasetsDir.entryInfoList()) {
            const auto url = QUrl::fromLocalFile(dataset.absoluteFilePath()).toString();
            if (const auto iter = std::find(std::begin(datasetModel.datasets), std::end(datasetModel.datasets), url); iter == std::end(datasetModel.datasets)) {
                appendRowSelectIfLU(url);
            }
        }
    }// QSignalBlocker
    auto & cubeEdgeLen = Dataset::current().cubeEdgeLength;
    cubeEdgeLen = settings.value(DATASET_CUBE_EDGE, 128).toInt();
    state->M = settings.value(DATASET_SUPERCUBE_EDGE, 3).toInt();
    segmentationOverlayCheckbox.setChecked(settings.value(DATASET_OVERLAY, false).toBool());
    state->viewer->resizeTexEdgeLength(cubeEdgeLen, state->M, Dataset::datasets.size());

    cubeEdgeSpin.setValue(cubeEdgeLen);
    fovSpin.setCubeEdge(cubeEdgeLen);
    fovSpin.setValue(cubeEdgeLen * (state->M - 1));
    adaptMemoryConsumption();
    settings.endGroup();
    applyGeometrySettings();
}
