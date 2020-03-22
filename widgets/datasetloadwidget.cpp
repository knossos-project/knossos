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

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <stdexcept>

DatasetLoadWidget::DatasetLoadWidget(QWidget *parent) : DialogVisibilityNotify(DATASET_WIDGET, parent) {
    setModal(true);
    setWindowTitle("Load Dataset");

    tableWidget.setColumnCount(3);
    tableWidget.verticalHeader()->setVisible(false);
    tableWidget.horizontalHeader()->setVisible(false);
    tableWidget.setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget.setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget.horizontalHeader()->resizeSection(1, 25);
    tableWidget.horizontalHeader()->resizeSection(2, 20);
    tableWidget.horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
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
    mainLayout.addWidget(&splitter);
    mainLayout.addWidget(&datasetSettingsGroup);
    mainLayout.addLayout(&buttonHLayout);

    setLayout(&mainLayout);

    QObject::connect(&tableWidget, &QTableWidget::cellChanged, this, &DatasetLoadWidget::datasetCellChanged);
    QObject::connect(&tableWidget, &QTableWidget::itemSelectionChanged, [this]() {
        WidgetDisabler d{*this};// don’t allow widget interaction while Network has an event loop running
        bool bad = tableWidget.selectedItems().empty();
        QUrl dataset;
        bad = bad || (dataset = tableWidget.selectedItems().front()->text()).isEmpty();
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

void DatasetLoadWidget::insertDatasetRow(const QString & dataset, const int row) {
    tableWidget.insertRow(row);

    auto rowFromCell = [this](int column, QPushButton * const button){
        for(int row = 0; row < tableWidget.rowCount(); ++row) {
            if (button == tableWidget.cellWidget(row, column)) {
                return row;
            }
        }
        return -1;
    };

    QPushButton *addDatasetButton = new QPushButton("…");
    addDatasetButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    addDatasetButton->setToolTip(tr("Select a dataset from file…"));
    QObject::connect(addDatasetButton, &QPushButton::clicked, [this, rowFromCell, addDatasetButton](){
        const auto selectedFile = state->viewer->suspend([this]{
            return QFileDialog::getOpenFileUrl(this, "Select a KNOSSOS dataset", QDir::homePath(), "*.conf").toString();
        });
        if (!selectedFile.isEmpty()) {
            tableWidget.item(rowFromCell(1, addDatasetButton), 0)->setText(selectedFile);
        }
    });

    QPushButton *removeDatasetButton = new QPushButton("×");
    removeDatasetButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    removeDatasetButton->setToolTip(tr("Remove this dataset from the list"));
    QObject::connect(removeDatasetButton, &QPushButton::clicked, [this, rowFromCell, removeDatasetButton](){
        const int row = rowFromCell(2, removeDatasetButton);
        tableWidget.removeRow(row);
        if (row == tableWidget.rowCount() - 1) {
            tableWidget.selectRow(row - 1);// select last item
        }
    });

    tableWidget.setItem(row, 0, new QTableWidgetItem(dataset));
    tableWidget.setCellWidget(row, 1, addDatasetButton);
    tableWidget.setCellWidget(row, 2, removeDatasetButton);
}

void DatasetLoadWidget::datasetCellChanged(int row, int col) {
    if (col == 0 && row == tableWidget.rowCount() - 1 && tableWidget.item(row, 0)->text() != "") {
        QSignalBlocker blocker{tableWidget};// changing an item would land here again
        auto dataset = tableWidget.item(row, 0)->text();
        tableWidget.item(row, 0)->setText("");//clear edit row
        insertDatasetRow(dataset, tableWidget.rowCount() - 1);//insert before edit row
        processButton.setFocus();// move focus away from the … button – needed to select a different row
        tableWidget.selectRow(row);//select new item

        WidgetDisabler d{*this};// don’t allow widget interaction while Network has an event loop running
        decltype(Network::singleton().refresh(std::declval<QUrl>())) download = Network::singleton().refresh(dataset);
        if (download.first) {
            updateDatasetInfo(dataset, download.second);
        } else {
            infoLabel.setText("");
        }
    }
}

void DatasetLoadWidget::updateDatasetInfo(const QUrl & url, const QString & info) try {
    const auto datasetinfo = Dataset::parse(url, info).front();
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

QStringList DatasetLoadWidget::getRecentPathItems() {
    QStringList recentPaths;

    for(int row = 0; row < tableWidget.rowCount() - 1; ++row) {
        if(tableWidget.item(row, 0)->text() != "") {
            recentPaths.append(tableWidget.item(row, 0)->text());
        }
    }

    return recentPaths;
}

void DatasetLoadWidget::adaptMemoryConsumption() {
    const auto fov = fovSpin.value();
    auto mebibytes = std::pow(fov + cubeEdgeSpin.value(), 3) / std::pow(1024, 2);
    mebibytes += segmentationOverlayCheckbox.isChecked() * OBJID_BYTES * mebibytes;
    auto text = QString("FOV per dimension (%1 MiB RAM)").arg(mebibytes);
    superCubeSizeLabel.setText(text);
}

void DatasetLoadWidget::processButtonClicked() {
    const auto dataset = tableWidget.item(tableWidget.currentRow(), 0)->text();
    if (dataset.isEmpty()) {
        QMessageBox box{QApplication::activeWindow()};
        box.setIcon(QMessageBox::Information);
        box.setText(tr("Unable to load"));
        box.setInformativeText(tr("No path selected"));
        box.exec();
    } else if (loadDataset(boost::none, dataset)) {
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
    const auto existingEntries = tableWidget.findItems(path.url(), Qt::MatchFlag::MatchExactly);
    if (existingEntries.size() == 0) {
        insertDatasetRow(path.url(), tableWidget.rowCount() - 1);
        tableWidget.selectRow(tableWidget.rowCount() - 2);
    } else {
        tableWidget.selectRow(existingEntries.front()->row());
    }

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

    auto layers = [&path, &data, &silent]() {
        try {
            return Dataset::parse(path, data);
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
    bool overlayPresent{false};
    for (std::size_t i = 0; i < layers.size(); ++i) {// determine segmentation layer
        if (layers[i].isOverlay()) {
            overlayPresent = true;
            layers[i].allocationEnabled = layers[i].loadingEnabled = Segmentation::singleton().enabled;
            Segmentation::singleton().layerId = i;
            break;// only enable the first overlay layer by default
        }
    }
    if (!overlayPresent) {// add empty overlay channel
        const auto i = layers.size();
        layers.emplace_back(Dataset{layers.front()});// explicitly copy, because the reference will get invalidated
        layers[i].allocationEnabled = layers[i].loadingEnabled = Segmentation::singleton().enabled;
        Segmentation::singleton().layerId = i;
        layers.back().type = Dataset::CubeType::SNAPPY;
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

    settings.setValue(DATASET_MRU, getRecentPathItems());

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

    restoreGeometry(settings.value(DATASET_GEOMETRY, "").toByteArray());
    datasetUrl = transitionedDataset(settings.value(DATASET_LAST_USED, "").toString());
    tableWidget.setRowCount(0); // prevent dataset duplication on loading custom settings
    auto appendRowSelectIfLU = [this](const QString & dataset){
        insertDatasetRow(dataset, tableWidget.rowCount());
        if (dataset == datasetUrl.toString()) {
            tableWidget.selectRow(tableWidget.rowCount() - 1);
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
            if (tableWidget.findItems(url, Qt::MatchExactly).empty()) {
                appendRowSelectIfLU(url);
            }
        }
        // add Empty row at the end
        appendRowSelectIfLU("");
        tableWidget.cellWidget(tableWidget.rowCount() - 1, 2)->setEnabled(false);//don’t delete empty row
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
