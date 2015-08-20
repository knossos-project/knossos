#include "datasetloadwidget.h"

#include "datatset.h"
#include "GuiConstants.h"
#include "knossos.h"
#include "loader.h"
#include "mainwindow.h"
#include "network.h"
#include "renderer.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include <stdexcept>

DatasetLoadWidget::DatasetLoadWidget(QWidget *parent) : QDialog(parent) {
    setModal(true);
    setWindowTitle("Load Dataset");

    cubeEdgeSpin.setRange(1, 256);
    cubeEdgeSpin.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    superCubeEdgeSpin.setMinimum(3);
    superCubeEdgeSpin.setSingleStep(2);
    superCubeEdgeSpin.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    tableWidget.setColumnCount(3);

    tableWidget.verticalHeader()->setVisible(false);
    tableWidget.horizontalHeader()->setVisible(false);

    tableWidget.setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget.setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget.horizontalHeader()->resizeSection(1, 20);
    tableWidget.horizontalHeader()->resizeSection(2, 40);
    tableWidget.horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    infoLabel.setWordWrap(true);//allows shrinking below minimum width

    line.setFrameShape(QFrame::HLine);
    line.setFrameShadow(QFrame::Sunken);

    superCubeEdgeSpin.setAlignment(Qt::AlignLeft);
    superCubeEdgeHLayout.addWidget(&superCubeEdgeSpin);
    superCubeEdgeHLayout.addWidget(&superCubeSizeLabel);

    cubeEdgeHLayout.addWidget(&cubeEdgeSpin);
    cubeEdgeHLayout.addWidget(&cubeEdgeLabel);

    buttonHLayout.addWidget(&processButton);
    buttonHLayout.addWidget(&cancelButton);

    mainLayout.addWidget(&tableWidget);
    mainLayout.addWidget(&infoLabel);
    mainLayout.addWidget(&line);
    mainLayout.addLayout(&superCubeEdgeHLayout);
    mainLayout.addLayout(&cubeEdgeHLayout);
    mainLayout.addWidget(&segmentationOverlayCheckbox);
    mainLayout.addLayout(&buttonHLayout);

    setLayout(&mainLayout);

    QObject::connect(&tableWidget, &QTableWidget::cellChanged, this, &DatasetLoadWidget::datasetCellChanged);
    QObject::connect(&tableWidget, &QTableWidget::itemSelectionChanged, this, &DatasetLoadWidget::updateDatasetInfo);
    QObject::connect(&cubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&superCubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&segmentationOverlayCheckbox, &QCheckBox::stateChanged, this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&processButton, &QPushButton::clicked, this, &DatasetLoadWidget::processButtonClicked);
    QObject::connect(&cancelButton, &QPushButton::clicked, this, &DatasetLoadWidget::cancelButtonClicked);

    resize(512, 512);//random default size, will be overriden by settings if present

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

    QPushButton *addDs = new QPushButton("…");
    QObject::connect(addDs, &QPushButton::clicked, [this, rowFromCell, addDs](){
        state->viewerState->renderInterval = SLOW;
        QString selectFile = QFileDialog::getOpenFileName(this, "Select a KNOSSOS dataset", QDir::homePath(), "*.conf");
        state->viewerState->renderInterval = FAST;
        if (!selectFile.isEmpty()) {
            QTableWidgetItem * const t = new QTableWidgetItem(selectFile);
            const int row = rowFromCell(1, addDs);
            tableWidget.setItem(row, 0, t);
        }
    });

    QPushButton *delDs = new QPushButton("Del");
    QObject::connect(delDs, &QPushButton::clicked, [this, rowFromCell, delDs](){
        const int row = rowFromCell(2, delDs);
        tableWidget.removeRow(row);
    });

    QTableWidgetItem *t = new QTableWidgetItem(dataset);
    tableWidget.setItem(row, 0, t);
    tableWidget.setCellWidget(row, 1, addDs);
    tableWidget.setCellWidget(row, 2, delDs);
}

void DatasetLoadWidget::datasetCellChanged(int row, int col) {
    if (col == 0 && row == tableWidget.rowCount() - 1 && tableWidget.item(row, 0)->text() != "") {
        const auto dataset = tableWidget.item(row, 0)->text();
        const auto blockState = tableWidget.signalsBlocked();
        tableWidget.blockSignals(true);//changing an item would land here again

        tableWidget.item(row, 0)->setText("");//clear edit row
        insertDatasetRow(dataset, tableWidget.rowCount() - 1);//insert before edit row
        tableWidget.selectRow(row);//select new item

        tableWidget.blockSignals(blockState);
    }
    updateDatasetInfo();
}

void DatasetLoadWidget::updateDatasetInfo() {
    bool bad = tableWidget.selectedItems().empty();
    QString dataset;
    bad = bad || (dataset = tableWidget.selectedItems().front()->text()).isEmpty();
    QUrl url(dataset);
    if (url.isRelative()) {//assume file if no protocol is present
        url.setScheme("file");
    }
    decltype(Network::singleton().refresh(std::declval<QUrl>())) download;
    bad = bad || !(download = Network::singleton().refresh(url)).first;
    if (bad) {
        infoLabel.setText("");
        return;
    }

    const auto datasetinfo = Dataset::fromLegacyConf(download.second);

    //make sure supercubeedge is small again
    auto supercubeedge = superCubeEdgeSpin.value() * cubeEdgeSpin.value() / datasetinfo.cubeEdgeLength;
    supercubeedge = std::max(3, supercubeedge - !(supercubeedge % 2));
    superCubeEdgeSpin.setValue(supercubeedge);
    cubeEdgeSpin.setValue(datasetinfo.cubeEdgeLength);
    adaptMemoryConsumption();

    QString infotext = tr("<b>%1 Dataset</b><br />%2");
    if (datasetinfo.remote) {
        infotext = infotext.arg("Remote").arg("URL: %1<br />").arg(datasetinfo.url.toString());
    } else {
        infotext = infotext.arg("Local").arg("");
    }
    infotext += QString("Boundary (x y z): %1 %2 %3<br />Compression: %4<br />cubeEdgeLength: %5<br />Magnification: %6<br />Scale (x y z): %7 %8 %9")
        .arg(datasetinfo.boundary.x).arg(datasetinfo.boundary.y).arg(datasetinfo.boundary.z)
        .arg(datasetinfo.compressionRatio)
        .arg(datasetinfo.cubeEdgeLength)
        .arg(datasetinfo.magnification)
        .arg(datasetinfo.scale.x)
        .arg(datasetinfo.scale.y)
        .arg(datasetinfo.scale.z);

    infoLabel.setText(infotext);
}

bool DatasetLoadWidget::parseGoogleJson(const QString & json_raw) {
    QJsonDocument json_conf = QJsonDocument::fromJson(json_raw.toUtf8());

    auto jmap = json_conf.object();
    auto boundary_json = jmap["geometry"].toArray()[0].toObject()["volumeSize"].toObject();

    auto bx = boundary_json["x"].toString().toInt();
    auto by = boundary_json["y"].toString().toInt();
    auto bz = boundary_json["z"].toString().toInt();

    auto scale_json = jmap["geometry"].toArray()[0].toObject()["pixelSize"].toObject();

    auto sx = static_cast<float>(scale_json["x"].toDouble());
    auto sy = static_cast<float>(scale_json["y"].toDouble());
    auto sz = static_cast<float>(scale_json["z"].toDouble());

    if((bx == 0) || (bx == 0) || (bx == 0) || (bx == 0) || (bx == 0) || (bx == 0)) {
        return false;
    }

    state->boundary = {bx, by, bz};
    state->scale = {sx, sy, sz};

    state->path[0] = '\0'; //dont't check for other mags
    state->highestAvailableMag = std::pow(2,(jmap["geometry"].toArray().size()-1)); //highest google mag

    state->compressionRatio = 1000;

    state->overlay = false; //google does not have this

    return true;
}

bool DatasetLoadWidget::parseWebKnossosJson(const QString & json_raw) {
    QJsonDocument json_conf = QJsonDocument::fromJson(json_raw.toUtf8());

    auto jmap = json_conf.object();

    auto boundary_json = jmap["dataSource"].toObject()["dataLayers"].toArray()[1].toObject()["sections"].toArray()[0].toObject()["bboxBig"].toObject(); //use bboxBig from color because its bigger :X

    auto bx = boundary_json["width"].toInt();
    auto by = boundary_json["height"].toInt();
    auto bz = boundary_json["depth"].toInt();

    auto scale_json = jmap["dataSource"].toObject()["scale"].toArray();

    auto sx = static_cast<float>(scale_json[0].toDouble());
    auto sy = static_cast<float>(scale_json[1].toDouble());
    auto sz = static_cast<float>(scale_json[2].toDouble());

    if((bx == 0) || (bx == 0) || (bx == 0) || (bx == 0) || (bx == 0) || (bx == 0)) {
        return false;
    }

    state->boundary = {bx, by, bz};

    state->scale = {sx, sy, sz};

    state->path[0] = '\0'; //dont't check for other mags

    auto mag = jmap["dataSource"].toObject()["dataLayers"].toArray()[0].toObject()["sections"].toArray()[0].toObject()["resolutions"].toArray();

    state->highestAvailableMag = mag[mag.size()-1].toInt();

    state->compressionRatio = 0;

    state->overlay = false; //webknossos does not have this

    return true;
}

QString DatasetLoadWidget::extractWebKnossosToken(QString & json_raw) {
    QJsonDocument json_conf = QJsonDocument::fromJson(json_raw.toUtf8());
    auto jmap = json_conf.object();

    return jmap["token"].toString();
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
    const auto cubeEdge = cubeEdgeSpin.value();
    const auto superCubeEdge = superCubeEdgeSpin.value();
    auto mibibytes = std::pow(cubeEdge, 3) * std::pow(superCubeEdge, 3) / std::pow(1024, 2);
    mibibytes += segmentationOverlayCheckbox.isChecked() * OBJID_BYTES * mibibytes;
    const auto fov = cubeEdge * (superCubeEdge - 1);
    auto text = QString("Data cache cube edge length (%1 MiB RAM)\nFOV %2 pixel per dimension").arg(mibibytes).arg(fov);
    superCubeSizeLabel.setText(text);
    const auto maxsupercubeedge = TEXTURE_EDGE_LEN / cubeEdge;
    //make sure it’s an odd number
    superCubeEdgeSpin.setMaximum(maxsupercubeedge - (maxsupercubeedge % 2 == 0 ? 1 : 0));
}

void DatasetLoadWidget::cancelButtonClicked() {
    this->hide();
}

void DatasetLoadWidget::processButtonClicked() {
    const auto dataset = tableWidget.item(tableWidget.currentRow(), 0)->text();
    if (dataset.isEmpty()) {
        QMessageBox::information(this, "Unable to load", "No path selected");
    } else if (loadDataset(dataset)) {
        hide(); //hide datasetloadwidget only if we could successfully load a dataset
    }
}

void DatasetLoadWidget::gatherHeidelbrainDatasetInformation(QString & path) {
    //check if we have a remote conf
    if(path.startsWith("http", Qt::CaseInsensitive)) {
        if(!Network::singleton().refresh(path).first) return;
    }

    QFileInfo pathInfo;
    pathInfo.setFile(path);

    QString filePath; // for holding the whole path to a .conf file
    QFile confFile;
    if(pathInfo.isFile()) { // .conf file selected (case 3)
        filePath = path;
        confFile.setFileName(filePath);
    }  else { // folder selected
        if(path.endsWith('/') == false && path.endsWith('\\') == false) {
            // qFileInfo only recognizes paths with trailing slash as directories.
            path.append('/');
            pathInfo.setFile(path);
        }
        QDir directory(path);
        QStringList dirContent = directory.entryList(QStringList("*.conf"));
        if(dirContent.empty()) { // apparently the base dataset folder (case 2) was selected
            // find the magnification subfolders and look for a .conf file starting at lowest mag
            bool foundConf = false;
            dirContent = directory.entryList(QStringList("*mag*"), QDir::Dirs, QDir::Name);
            for(const auto magPath : dirContent) {
                QDir magDir(QString("%1/%2").arg(directory.absolutePath()).arg(magPath));
                QStringList subDirContent = magDir.entryList(QStringList("*.conf"), QDir::Files);
                if(subDirContent.empty() == false) {
                    filePath = QString("%1/%2/%3").arg(directory.absolutePath()).arg(magPath).arg(subDirContent.front());
                    confFile.setFileName(filePath);
                    QFile::copy(filePath, QString("%1/%2.k.conf").arg(directory.absolutePath()).arg(directory.dirName()));
                    foundConf = true;
                    break;
                }
            }
            if(foundConf == false) {
                QMessageBox::information(this, "Unable to load", "Could not find a dataset file (*.conf)");
                return;
            }
        }
        else {
            filePath = QString("%1/%2").arg(directory.absolutePath()).arg(dirContent.front());
            if(QRegExp(".*mag[0-9]+").exactMatch(directory.absolutePath())) {
                // apparently the magnification folder was selected (case 1)
                directory.cdUp();
                QFile::copy(filePath, QString("%1/%2.k.conf").arg(directory.absolutePath()).arg(directory.dirName()));
            }
        }
    }

    if (!Knossos::readConfigFile(filePath.toStdString().c_str())) {
        QMessageBox::information(this, "Unable to load", QString("Failed to read config from %1").arg(filePath));
        return;
    }

    // we want state->path to hold the path to the dataset folder
    // instead of a path to a subfolder of a specific magnification
    QDir datasetDir(pathInfo.absolutePath());
    if(QRegExp(".*mag[0-9]+").exactMatch(datasetDir.absolutePath())) {
        datasetDir.cdUp();
    }
    strcpy(state->path, datasetDir.absolutePath().toStdString().c_str());

    Knossos::findAndRegisterAvailableDatasets();
}

/* dataset can be selected in three ways:
 * 1. by selecting the folder containing a k.conf (for multires datasets it's a "magX" folder)
 * 2. for multires datasets: by selecting the dataset folder (the folder containing the "magX" subfolders)
 * 3. by specifying a .conf directly.
 */
bool DatasetLoadWidget::loadDataset(QString path,  const bool keepAnnotation) {
    if (path.isEmpty() && datasetPath.isEmpty()) {//no dataset available to load
        show();
        return false;
    } else if (path.isEmpty()) {
        path = datasetPath;
    }

    if (!keepAnnotation) {
        state->viewer->window->newAnnotationSlot();//clear skeleton, mergelist and snappy cubes
    }
    if (Session::singleton().unsavedChanges) {//if annotation wasn’t cleared, abort loading of dataset
        return false;
    }

    Loader::Controller::singleton().waitForWorkerThread();//we change variables the loader uses

    // actually load the dataset
    datasetPath = path;

    // check if a fundamental geometry variable has changed. If so, the loader requires reinitialization
    state->cubeEdgeLength = cubeEdgeSpin.text().toInt();
    state->M = superCubeEdgeSpin.value();
    state->overlay = segmentationOverlayCheckbox.isChecked();

    Loader::API api;
    QString url;
    Loader::CubeType raw_compression;
    {
        api = Loader::API::Heidelbrain;
        gatherHeidelbrainDatasetInformation(path);
        if (state->loadMode == LM_FTP) {
            url = QString("http://%1:%2@%3/%4").arg(state->ftpUsername).arg(state->ftpPassword).arg(state->ftpHostName).arg(state->ftpBasePath);
        } else {
            url = QString("file:///%1").arg(state->path);
        }
        raw_compression = state->compressionRatio == 0 ? Loader::CubeType::RAW_UNCOMPRESSED : state->compressionRatio == 1000 ? Loader::CubeType::RAW_JPG
                : state->compressionRatio == 6 ? Loader::CubeType::RAW_JP2_6 : Loader::CubeType::RAW_J2K;
    }
    //skeleton vp boundary
    if ((state->boundary.x >= state->boundary.y) && (state->boundary.x >= state->boundary.z)) {
        state->skeletonState->volBoundary = state->boundary.x * 2;
    }
    if ((state->boundary.y >= state->boundary.x) && (state->boundary.y >= state->boundary.z)) {
        state->skeletonState->volBoundary = state->boundary.y * 2;
    }
    if ((state->boundary.z >= state->boundary.x) && (state->boundary.z >= state->boundary.y)) {
        state->skeletonState->volBoundary = state->boundary.z * 2;
    }

    applyGeometrySettings();

    emit datasetSwitchZoomDefaults();

    // reset skeleton viewport
    if (state->skeletonState->rotationcounter == 0) {
        state->skeletonState->definedSkeletonVpView = SKELVP_RESET;
    }

    Loader::Controller::singleton().restart(url, api, raw_compression, Loader::CubeType::SEGMENTATION_SZ_ZIP, state->name);

    emit updateDatasetCompression();

    Session::singleton().updateMovementArea({0, 0, 0}, state->boundary);
    // ...beginning with loading the middle of dataset
    state->viewerState->currentPosition = {state->boundary / 2};
    state->viewer->changeDatasetMag(DATA_SET);
    state->viewer->userMove(0, 0, 0, USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
    emit datasetChanged(segmentationOverlayCheckbox.isChecked());

    return true;
}

void DatasetLoadWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    settings.setValue(DATASET_GEOMETRY, saveGeometry());
    settings.setValue(DATASET_LAST_USED, datasetPath);

    settings.setValue(DATASET_MRU, getRecentPathItems());

    settings.setValue(DATASET_SUPERCUBE_EDGE, state->M);
    settings.setValue(DATASET_OVERLAY, state->overlay);

    settings.endGroup();
}

void DatasetLoadWidget::applyGeometrySettings() {
    if(state->M * state->cubeEdgeLength >= TEXTURE_EDGE_LEN) {
        const auto msg = "Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!";
        qDebug() << msg;
        throw std::runtime_error(msg);
    }
    //settings depending on supercube and cube size
    state->cubeSliceArea = std::pow(state->cubeEdgeLength, 2);
    state->cubeBytes = std::pow(state->cubeEdgeLength, 3);
    state->cubeSetElements = std::pow(state->M, 3);
    state->cubeSetBytes = state->cubeSetElements * state->cubeBytes;

    Viewport::resetTextureProperties();
}

void DatasetLoadWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    restoreGeometry(settings.value(DATASET_GEOMETRY, "").toByteArray());
    datasetPath = settings.value(DATASET_LAST_USED, "").toString();

    auto appendRowSelectIfLU = [this](const QString & dataset){
        insertDatasetRow(dataset, tableWidget.rowCount());
        if (dataset == datasetPath) {
            tableWidget.selectRow(tableWidget.rowCount() - 1);
        }
    };

    tableWidget.blockSignals(true);

    //add datasets from file
    for(const auto & dataset : settings.value(DATASET_MRU).toStringList()) {
        appendRowSelectIfLU(dataset);
    }
    //add public datasets
    auto datasetsDir = QDir(":/resources/datasets");
    for (const auto & dataset : datasetsDir.entryInfoList()) {
        if (tableWidget.findItems(dataset.absoluteFilePath(), Qt::MatchExactly).empty()) {
            appendRowSelectIfLU(dataset.absoluteFilePath());
        }
    }
    //add Empty row at the end
    appendRowSelectIfLU("");
    tableWidget.cellWidget(tableWidget.rowCount() - 1, 2)->setEnabled(false);//don’t delete empty row

    tableWidget.blockSignals(false);
    updateDatasetInfo();


    if (QApplication::arguments().filter("supercube-edge").empty()) {//if not provided by cmdline
        state->M = settings.value(DATASET_SUPERCUBE_EDGE, 3).toInt();
    }
    if (QApplication::arguments().filter("overlay").empty()) {//if not provided by cmdline
        state->overlay = settings.value(DATASET_OVERLAY, false).toBool();
    }

    cubeEdgeSpin.setValue(state->cubeEdgeLength);
    superCubeEdgeSpin.setValue(state->M);
    segmentationOverlayCheckbox.setCheckState(state->overlay ? Qt::Checked : Qt::Unchecked);
    adaptMemoryConsumption();

    settings.endGroup();

    applyGeometrySettings();
}
