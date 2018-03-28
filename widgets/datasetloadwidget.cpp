/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
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
    QObject::connect(&tableWidget, &QTableWidget::itemSelectionChanged, this, &DatasetLoadWidget::updateDatasetInfo);
    QObject::connect(&cubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int cubeedge){
        fovSpin.setCubeEdge(cubeedge);
        adaptMemoryConsumption();
    });
    QObject::connect(&fovSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&segmentationOverlayCheckbox, &QCheckBox::stateChanged, this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&processButton, &QPushButton::clicked, this, &DatasetLoadWidget::processButtonClicked);
    static auto resetSettings = [this]() {
        fovSpin.setValue(Dataset::current().cubeEdgeLength * (state->M - 1));
        segmentationOverlayCheckbox.setChecked(Dataset::current().overlay);
    };
    QObject::connect(this, &DatasetLoadWidget::rejected, []() { resetSettings(); });
    QObject::connect(&cancelButton, &QPushButton::clicked, [this]() { resetSettings(); hide(); });
    resize(600, 600);//random default size, will be overriden by settings if present

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
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
    addDatasetButton->setToolTip(tr("Select a dataset from file…"));
    QObject::connect(addDatasetButton, &QPushButton::clicked, [this, rowFromCell, addDatasetButton](){
        const auto selectedFile = state->viewer->suspend([this]{
            return QFileDialog::getOpenFileUrl(this, "Select a KNOSSOS dataset", QDir::homePath(), "*.conf").toString();
        });
        if (!selectedFile.isEmpty()) {
            QTableWidgetItem * const datasetPathItem = new QTableWidgetItem(selectedFile);
            const int row = rowFromCell(1, addDatasetButton);
            tableWidget.setItem(row, 0, datasetPathItem);
        }
    });

    QPushButton *removeDatasetButton = new QPushButton("×");
    removeDatasetButton->setToolTip(tr("Remove this dataset from the list"));
    QObject::connect(removeDatasetButton, &QPushButton::clicked, [this, rowFromCell, removeDatasetButton](){
        const int row = rowFromCell(2, removeDatasetButton);
        tableWidget.removeRow(row);
    });

    QTableWidgetItem *datasetPathItem = new QTableWidgetItem(dataset);
    tableWidget.setItem(row, 0, datasetPathItem);
    tableWidget.setCellWidget(row, 1, addDatasetButton);
    tableWidget.setCellWidget(row, 2, removeDatasetButton);
}

void DatasetLoadWidget::datasetCellChanged(int row, int col) {
    if (col == 0 && row == tableWidget.rowCount() - 1 && tableWidget.item(row, 0)->text() != "") {
        QSignalBlocker blocker{tableWidget};// changing an item would land here again
        const auto dataset = tableWidget.item(row, 0)->text();
        tableWidget.item(row, 0)->setText("");//clear edit row
        insertDatasetRow(dataset, tableWidget.rowCount() - 1);//insert before edit row
        tableWidget.selectRow(row);//select new item
    }
    updateDatasetInfo();
}

void DatasetLoadWidget::updateDatasetInfo() {
    bool bad = tableWidget.selectedItems().empty();
    QString dataset;
    bad = bad || (dataset = tableWidget.selectedItems().front()->text()).isEmpty();
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

    datasetSettingsLayout.takeRow(&cubeEdgeSpin);
    cubeEdgeSpin.setParent(nullptr);
    cubeEdgeLabel.setParent(nullptr);
    if (!Dataset::isHeidelbrain(url)) {
        datasetSettingsLayout.insertRow(0, &cubeEdgeSpin, &cubeEdgeLabel);
    }
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
        QMessageBox::information(this, "Unable to load", "No path selected");
    } else if (loadDataset(this, boost::none, dataset)) {
        hide(); //hide datasetloadwidget only if we could successfully load a dataset
    }
}

/* dataset can be selected in three ways:
 * 1. by selecting the folder containing a k.conf (for multires datasets it's a "magX" folder)
 * 2. for multires datasets: by selecting the dataset folder (the folder containing the "magX" subfolders)
 * 3. by specifying a .conf directly.
 */
bool DatasetLoadWidget::loadDataset(QWidget * parent, const boost::optional<bool> loadOverlay, QUrl path, const bool silent) {
    if (path.isEmpty() && datasetUrl.isEmpty()) {//no dataset available to load
        open();
        return false;
    } else if (path.isEmpty()) {//if empty reload previous
        path = datasetUrl;
    }
    path.setPath(path.path() + (!path.isLocalFile() && !path.toString().endsWith("/") && !Dataset::isWebKnossos(path) ? "/" : ""));// add slash to avoid redirects
    const auto download = Network::singleton().refresh(path);
    if (!Dataset::isGoogleBrainmaps(path) && !download.first) {
        if (!silent) {
            QMessageBox warning(parent);
            warning.setWindowFlags(Qt::WindowFlags{warning.windowFlags() | Qt::WindowStaysOnTopHint});
            warning.setIcon(QMessageBox::Warning);
            warning.setText("Unable to load Daataset.");
            warning.setInformativeText(QString("Failed to read config file from %1").arg(path.toString()));
            warning.exec();
            open();
        }
        qDebug() << "no config at" << path;
        return false;
    }

    bool keepAnnotation = silent;
    if (!silent && (!Session::singleton().annotationFilename.isEmpty() || Session::singleton().unsavedChanges)) {
        QMessageBox question(parent);
        question.setWindowFlags(Qt::WindowFlags{question.windowFlags() | Qt::WindowStaysOnTopHint});
        question.setIcon(QMessageBox::Question);
        question.setText(tr("Keep the current annotation for the new dataset?"));
        question.setInformativeText(tr("It only makes sense to keep the annotation if the new dataset matches it."));
        const auto * const keepButton = question.addButton(tr("Yes, &keep"), QMessageBox::YesRole);
        const auto * const clearButton = question.addButton(tr("No, start &new one"), QMessageBox::NoRole);
        const auto * const cancelButton = question.addButton(QMessageBox::Cancel);
        question.exec();
        if (question.clickedButton() == cancelButton || (question.clickedButton() == clearButton && !state->viewer->window->newAnnotationSlot())) {// clear skeleton, mergelist and snappy cubes
            return false;
        }
        keepAnnotation = question.clickedButton() == keepButton;
    }

    auto layers = Dataset::parse(path, download.second);
    if (Dataset::isHeidelbrain(path)) {
        try {
            layers.front().checkMagnifications();
        } catch (std::exception &) {
            if (!silent) {
                QMessageBox warning(parent);
                warning.setWindowFlags(Qt::WindowFlags{warning.windowFlags() | Qt::WindowStaysOnTopHint});
                warning.setIcon(QMessageBox::Warning);
                warning.setText("Dataset will not be loaded.");
                warning.setInformativeText("No magnifications could be detected. (knossos.conf in mag folder)");
                warning.exec();
                open();
            }
            qDebug() << "no mags";
            return false;
        }
        layers.push_back(layers.front().createCorrespondingOverlayLayer());
    } else if (Dataset::isGoogleBrainmaps(path)) {
        const auto pair = getBrainmapsToken();
        if (!pair.first) {
            return false;
        }
        const auto googleRequest = [&pair](const auto & path){
            QNetworkRequest request(path);
            request.setRawHeader("Authorization", (QString("Bearer ") + pair.second).toUtf8());
            return request;
        };

        auto & reply = *Network::singleton().manager.get(googleRequest(path));
        const auto config = blockDownloadExtractData(reply);

        qDebug() << config.first;

        Dataset info;
        if (config.first) {
            info = Dataset::parseGoogleJson(path, config.second).first();
            info.token = pair.second;
        } else {
            qDebug() << "googleRequest failed";
            return false;
        }
        layers = {info};
    }

    datasetUrl = {path};//remember config url
    Loader::Controller::singleton().suspendLoader();//we change variables the loader uses
    const bool changedBoundaryOrScale = layers.front().boundary != Dataset::current().boundary || layers.front().scale != Dataset::current().scale;

    Dataset::datasets = layers;

    // check if a fundamental geometry variable has changed. If so, the loader requires reinitialization
    auto & cubeEdgeLen = Dataset::current().cubeEdgeLength;
    cubeEdgeLen = cubeEdgeSpin.text().toInt();
    state->M = (fovSpin.value() + cubeEdgeLen) / cubeEdgeLen;
    if (loadOverlay != boost::none) {
        segmentationOverlayCheckbox.setChecked(loadOverlay.get());
    }
    Dataset::current().overlay = segmentationOverlayCheckbox.isChecked();
    if (Dataset::current().overlay && Dataset::datasets.size() < 2) {// add empty overlay channel
        Dataset::datasets.push_back(Dataset::datasets.front());
        Dataset::datasets.back().type = Dataset::CubeType::SNAPPY;
    }

    state->viewer->resizeTexEdgeLength(cubeEdgeLen, state->M);

    applyGeometrySettings();

    if (changedBoundaryOrScale || !keepAnnotation) {
        emit datasetSwitchZoomDefaults();
        // reset skeleton viewport
        if (state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_RESET;
        }
    }

    Loader::Controller::singleton().restart(Dataset::datasets);

    emit updateDatasetCompression();

    if (changedBoundaryOrScale || !keepAnnotation) {
        Session::singleton().updateMovementArea({0, 0, 0}, Dataset::current().boundary);
        // ...beginning with loading the middle of dataset
        state->viewerState->currentPosition = {Dataset::current().boundary / 2};
        state->viewer->updateDatasetMag();
        state->viewer->userMove({0, 0, 0}, USERMOVE_NEUTRAL);
    }
    emit datasetChanged(segmentationOverlayCheckbox.isChecked());

    return true;
}

void DatasetLoadWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    settings.setValue(DATASET_LAST_USED, datasetUrl);

    settings.setValue(DATASET_MRU, getRecentPathItems());

    settings.setValue(DATASET_CUBE_EDGE, Dataset::current().cubeEdgeLength);
    settings.setValue(DATASET_SUPERCUBE_EDGE, state->M);
    settings.setValue(DATASET_OVERLAY, Dataset::current().overlay);

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
        if (url.isRelative() && url.toString() != "google" && url.toString() != "webknossos") {
            url = QUrl::fromLocalFile(dataset);
        }
        return url;
    };

    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    restoreGeometry(settings.value(DATASET_GEOMETRY, "").toByteArray());
    datasetUrl = transitionedDataset(settings.value(DATASET_LAST_USED, "").toString());

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
            appendRowSelectIfLU(transitionedDataset(dataset).toString());
        }
        // add public datasets
        auto datasetsDir = QDir(":/resources/datasets");
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
    updateDatasetInfo();
    auto & cubeEdgeLen = Dataset::current().cubeEdgeLength;
    cubeEdgeLen = settings.value(DATASET_CUBE_EDGE, 128).toInt();
    if (QApplication::arguments().filter("supercube-edge").empty()) {//if not provided by cmdline
        state->M = settings.value(DATASET_SUPERCUBE_EDGE, 3).toInt();
    }
    if (QApplication::arguments().filter("overlay").empty()) {//if not provided by cmdline
        Dataset::current().overlay = settings.value(DATASET_OVERLAY, false).toBool();
    }
    state->viewer->resizeTexEdgeLength(cubeEdgeLen, state->M);

    cubeEdgeSpin.setValue(cubeEdgeLen);
    fovSpin.setCubeEdge(cubeEdgeLen);
    fovSpin.setValue(cubeEdgeLen * (state->M - 1));
    segmentationOverlayCheckbox.setChecked(Dataset::current().overlay);
    adaptMemoryConsumption();
    settings.endGroup();
    applyGeometrySettings();
}
