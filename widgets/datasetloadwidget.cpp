#include "datasetloadwidget.h"

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

    supercubeEdgeSpin = new QSpinBox;
    supercubeEdgeSpin->setMinimum(3);
    supercubeEdgeSpin->setSingleStep(2);
    supercubeEdgeSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    supercubeSizeLabel = new QLabel();

    cancelButton = new QPushButton("Cancel");
    processButton = new QPushButton("Use");

    QFrame* line0 = new QFrame();
    line0->setFrameShape(QFrame::HLine);
    line0->setFrameShadow(QFrame::Sunken);
    auto hLayoutLine0 = new QHBoxLayout;
    hLayoutLine0->addWidget(line0);

    auto hDatasetInfoSplitter = new QSplitter();
    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(3);

    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setVisible(false);

    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->horizontalHeader()->resizeSection(1, 20);
    tableWidget->horizontalHeader()->resizeSection(2, 40);
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);


    infolabel = new QLabel();

    scrollarea = new QScrollArea();
    scrollarea->setWidgetResizable(true);
    scrollarea->setWidget(infolabel);

    hDatasetInfoSplitter->addWidget(tableWidget);
    hDatasetInfoSplitter->addWidget(scrollarea);
    const auto splitterWidth = hDatasetInfoSplitter->size().width();
    hDatasetInfoSplitter->setSizes({splitterWidth/2, splitterWidth/2});

    QFrame* line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);

    auto hLayoutLine1 = new QHBoxLayout;
    hLayoutLine1->addWidget(line1);

    auto hLayout2 = new QHBoxLayout;
    hLayout2->addWidget(supercubeEdgeSpin);
    supercubeEdgeSpin->setAlignment(Qt::AlignLeft);
    hLayout2->addWidget(supercubeSizeLabel);

    auto hLayoutCubeSize = new QHBoxLayout;
    hLayoutCubeSize->addWidget(&cubeEdgeSpin);
    hLayoutCubeSize->addWidget(&cubeEdgeLabel);

    auto hLayout3 = new QHBoxLayout;
    hLayout3->addWidget(processButton);
    hLayout3->addWidget(cancelButton);

    auto localLayout = new QVBoxLayout();

    localLayout->addLayout(hLayoutLine0);
    localLayout->addWidget(hDatasetInfoSplitter, 1);
    localLayout->addLayout(hLayoutLine1);
    localLayout->addLayout(hLayoutCubeSize);
    localLayout->addLayout(hLayout2);
    localLayout->addWidget(&segmentationOverlayCheckbox);
    localLayout->addLayout(hLayout3);

    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(localLayout);
    setLayout(mainLayout);

    QObject::connect(tableWidget, &QTableWidget::cellChanged, this, &DatasetLoadWidget::datasetCellChanged);
    QObject::connect(tableWidget, &QTableWidget::itemSelectionChanged, this, &DatasetLoadWidget::updateDatasetInfo);
    QObject::connect(&cubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(supercubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&segmentationOverlayCheckbox, &QCheckBox::stateChanged, this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(processButton, &QPushButton::clicked, this, &DatasetLoadWidget::processButtonClicked);
    QObject::connect(cancelButton, &QPushButton::clicked, this, &DatasetLoadWidget::cancelButtonClicked);

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void DatasetLoadWidget::insertDatasetRow(const QString & dataset, const int row) {
    tableWidget->insertRow(row);

    auto rowFromCell = [this](int column, QPushButton * const button){
        for(int row = 0; row < tableWidget->rowCount(); ++row) {
            if (button == tableWidget->cellWidget(row, column)) {
                return row;
            }
        }
        return -1;
    };

    QPushButton *addDs = new QPushButton("…");
    QObject::connect(addDs, &QPushButton::clicked, [this, rowFromCell, addDs](){
        QString selectFile = QFileDialog::getOpenFileName(this, "Select a KNOSSOS dataset", QDir::homePath(), "*.conf");
        if (!selectFile.isEmpty()) {
            QTableWidgetItem * const t = new QTableWidgetItem(selectFile);
            const int row = rowFromCell(1, addDs);
            tableWidget->setItem(row, 0, t);
        }
    });

    QPushButton *delDs = new QPushButton("Del");
    QObject::connect(delDs, &QPushButton::clicked, [this, rowFromCell, delDs](){
        const int row = rowFromCell(2, delDs);
        tableWidget->removeRow(row);
    });

    QTableWidgetItem *t = new QTableWidgetItem(dataset);
    tableWidget->setItem(row, 0, t);
    tableWidget->setCellWidget(row, 1, addDs);
    tableWidget->setCellWidget(row, 2, delDs);
}

void DatasetLoadWidget::datasetCellChanged(int row, int col) {
    if (col == 0 && row == tableWidget->rowCount() - 1 && tableWidget->item(row, 0)->text() != "") {
        const auto dataset = tableWidget->item(row, 0)->text();
        tableWidget->blockSignals(true);

        tableWidget->item(row, 0)->setText("");//clear edit row
        insertDatasetRow(dataset, tableWidget->rowCount() - 1);//insert before edit row
        tableWidget->selectRow(row);//select new item

        tableWidget->blockSignals(false);
    }
}

void DatasetLoadWidget::updateDatasetInfo() {
    if (tableWidget->selectedItems().empty()) return;

    const auto dataset = tableWidget->selectedItems().front()->text();

    if (dataset.contains("google") || dataset.contains("oxalis")) {
        infolabel->setText("wip loader");
        return;
    } else {//make sure supercubeedge is small again
        supercubeEdgeSpin->setValue(supercubeEdgeSpin->value() * cubeEdgeSpin.value() / 128);
        cubeEdgeSpin.setValue(128);
        adaptMemoryConsumption();
    }

    QString infotext;
    if (dataset != "") {
        datasetinfo = getConfigFileInfo(dataset);

        if (datasetinfo.remote) {
            infotext = QString("<b>Remote Dataset</b><br>URL: %0%1<br>Boundary (x y z): %2 %3 %4<br>Compression: %5<br>cubeEdgeLength: %6<br>Magnification: %7<br>Scale (x y z): %8 %9 %10")
                    .arg(datasetinfo.ftphostname.c_str())
                    .arg(datasetinfo.ftpbasepath.c_str())
                    .arg(datasetinfo.boundary.x).arg(datasetinfo.boundary.y).arg(datasetinfo.boundary.z)
                    .arg(datasetinfo.compressionRatio)
                    .arg(datasetinfo.cubeEdgeLength)
                    .arg(datasetinfo.magnification)
                    .arg(datasetinfo.scale.x)
                    .arg(datasetinfo.scale.y)
                    .arg(datasetinfo.scale.z);
        } else {
            infotext = QString("<b>Local Dataset</b><br>Boundary (x y z): %0 %1 %2<br>Compression: %3<br>cubeEdgeLength: %4<br>Magnification: %5<br>Scale (x y z): %6 %7 %8")
                    .arg(datasetinfo.boundary.x).arg(datasetinfo.boundary.y).arg(datasetinfo.boundary.z)
                    .arg(datasetinfo.compressionRatio)
                    .arg(datasetinfo.cubeEdgeLength)
                    .arg(datasetinfo.magnification)
                    .arg(datasetinfo.scale.x)
                    .arg(datasetinfo.scale.y)
                    .arg(datasetinfo.scale.z);
        }
    }

    infolabel->setText(infotext);
}

bool DatasetLoadWidget::parseGoogleJson(const QString & json_raw) {
    QJsonDocument json_conf = QJsonDocument::fromJson(json_raw.toUtf8());

    auto jmap = json_conf.object();
    auto boundary_json = jmap["geometrys"].toArray()[0].toObject()["volumeSize"].toObject();

    auto bx = boundary_json["x"].toString().toInt();
    auto by = boundary_json["y"].toString().toInt();
    auto bz = boundary_json["z"].toString().toInt();

    auto scale_json = jmap["geometrys"].toArray()[0].toObject()["pixelSize"].toObject();

    auto sx = static_cast<float>(scale_json["x"].toDouble());
    auto sy = static_cast<float>(scale_json["y"].toDouble());
    auto sz = static_cast<float>(scale_json["z"].toDouble());

    if((bx == 0) || (bx == 0) || (bx == 0) || (bx == 0) || (bx == 0) || (bx == 0)) {
        return false;
    }

    state->boundary = {bx, by, bz};
    state->scale = {sx, sy, sz};

    state->path[0] = '\0'; //dont't check for other mags
    knossos->commonInitStates();
    state->highestAvailableMag = std::pow(2,(jmap["geometrys"].toArray().size()-1)); //highest google mag
    qDebug() << "POWER: " << std::pow(2,(jmap["geometrys"].toArray().size()-1));

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
    knossos->commonInitStates();
    state->highestAvailableMag = jmap["dataSource"].toObject()["dataLayers"].toArray()[0].toObject()["sections"].toArray()[0].toObject()["resolutions"].toObject()[0].toInt();

    state->compressionRatio = 0;

    state->overlay = false; //webknossos does not have this

    return true;
}

QStringList DatasetLoadWidget::getRecentPathItems() {
    QStringList recentPaths;

    for(int row = 0; row < tableWidget->rowCount() - 1; ++row) {
        if(tableWidget->item(row, 0)->text() != "") {
            recentPaths.append(tableWidget->item(row, 0)->text());
        }
    }

    return recentPaths;
}

void DatasetLoadWidget::adaptMemoryConsumption() {
    const auto cubeEdge = cubeEdgeSpin.value();
    const auto superCubeEdge = supercubeEdgeSpin->value();
    auto mibibytes = std::pow(cubeEdge, 3) * std::pow(superCubeEdge, 3) / std::pow(1024, 2);
    mibibytes += segmentationOverlayCheckbox.isChecked() * OBJID_BYTES * mibibytes;
    const auto fov = cubeEdge * (superCubeEdge - 1);
    auto text = QString("Data cache cube edge length (%1 MiB RAM) – FOV %2 pixel per dimension").arg(mibibytes).arg(fov);
    supercubeSizeLabel->setText(text);
    const auto maxsupercubeedge = TEXTURE_EDGE_LEN / cubeEdge;
    //make sure it’s an odd number
    supercubeEdgeSpin->setMaximum(maxsupercubeedge - (maxsupercubeedge % 2 == 0 ? 1 : 0));
}

void DatasetLoadWidget::cancelButtonClicked() {
    this->hide();
}

void DatasetLoadWidget::processButtonClicked() {
    const auto dataset = tableWidget->item(tableWidget->currentRow(), 0)->text();
    if (dataset.isEmpty()) {
        QMessageBox::information(this, "Unable to load", "No path selected");
    } else if (loadDataset(dataset)) {
        hide(); //hide datasetloadwidget only if we could successfully load a dataset
    }
}

void DatasetLoadWidget::gatherHeidelbrainDatasetInformation(QString & path) {
    //check if we have a remote conf
    if(path.startsWith("http", Qt::CaseInsensitive)) {
        std::string tmp = downloadRemoteConfFile(path);
        path = QString::fromStdString(tmp);
        if(path.isEmpty()) return;
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

    knossos->commonInitStates();
}

/* dataset can be selected in three ways:
 * 1. by selecting the folder containing a k.conf (for multires datasets it's a "magX" folder)
 * 2. for multires datasets: by selecting the dataset folder (the folder containing the "magX" subfolders)
 * 3. by specifying a .conf directly.
 */
bool DatasetLoadWidget::loadDataset(QString path) {
    if (path.isEmpty() && datasetPath.isEmpty()) {//no dataset available to load
        show();
        return false;
    } else if (path.isEmpty()) {
        path = datasetPath;
    }

    state->viewer->window->newAnnotationSlot();//clear skeleton, mergelist and snappy cubes
    if (state->skeletonState->unsavedChanges) {//if annotation wasn’t cleared, abort loading of dataset
        return false;
    }

    Loader::Controller::singleton().waitForWorkerThread();//we change variables the loader uses

    // actually load the dataset
    datasetPath = path;

    // check if a fundamental geometry variable has changed. If so, the loader requires reinitialization
    state->cubeEdgeLength = cubeEdgeSpin.text().toInt();
    state->M = supercubeEdgeSpin->value();
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
    if ((state->boundary.y >= state->boundary.x) && (state->boundary.y >= state->boundary.y)) {
        state->skeletonState->volBoundary = state->boundary.y * 2;
    }
    if ((state->boundary.z >= state->boundary.x) && (state->boundary.z >= state->boundary.y)) {
        state->skeletonState->volBoundary = state->boundary.x * 2;
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

DatasetLoadWidget::Datasetinfo DatasetLoadWidget::getConfigFileInfo(const QString & path) {
    Datasetinfo info;
    QString qpath{path};

    if(qpath.startsWith("http", Qt::CaseInsensitive)) {
        std::string tmp = downloadRemoteConfFile(qpath);
        qpath = QString::fromStdString(tmp);

        if(qpath == "") return info;
    }

    QFile file(qpath);

    if(!file.open(QIODevice::ReadOnly)) {
        qDebug("Error reading config file at path:%s", qpath.toStdString().c_str());
        return info;
    }

    QTextStream stream(&file);
    while(!stream.atEnd()) {
        QString line = stream.readLine();
        if(line.isEmpty())
            continue;

        QStringList tokenList = line.split(
            QRegExp("[ ;]"),
            QString::SkipEmptyParts
        );

        QString token = tokenList.at(0);

        if(token == "experiment") {
            token = tokenList.at(2);
            QStringList experimentTokenList = token.split(
                        QRegExp("[\"]"),
                        QString::SkipEmptyParts);
            info.experimentname = experimentTokenList.at(0).toStdString();

        } else if(token == "scale") {
            token = tokenList.at(1);
            if(token == "x") {
                info.scale.x = tokenList.at(2).toFloat();
            } else if(token == "y") {
                info.scale.y = tokenList.at(2).toFloat();
            } else if(token == "z") {
                info.scale.z = tokenList.at(2).toFloat();
            }
        } else if(token == "boundary") {
            token = tokenList.at(1);
            if(token == "x") {
                info.boundary.x = tokenList.at(2).toFloat();
            } else if(token == "y") {
                info.boundary.y = tokenList.at(2).toFloat();
            } else if(token == "z") {
                info.boundary.z = tokenList.at(2).toFloat();
            }
        } else if(token == "magnification") {
            info.magnification = tokenList.at(1).toInt();
        } else if(token == "cube_edge_length") {
            info.cubeEdgeLength = tokenList.at(1).toInt();
        } else if(token == "ftp_mode") {
            info.remote = true;

            info.ftphostname = tokenList.at(1).toStdString();
            info.ftpbasepath = tokenList.at(2).toStdString();

        } else if (token == "compression_ratio") {
            info.compressionRatio = tokenList.at(1).toInt();
        }
    }

    return info;
}

void DatasetLoadWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    settings.setValue(DATASET_LAST_USED, datasetPath);

    settings.setValue(DATASET_MRU, getRecentPathItems());

    settings.setValue(DATASET_SUPERCUBE_EDGE, state->M);
    settings.setValue(DATASET_OVERLAY, state->overlay);

    settings.endGroup();
}

void DatasetLoadWidget::applyGeometrySettings() {
    //settings depending on supercube and cube size
    state->cubeSliceArea = state->cubeEdgeLength * state->cubeEdgeLength;
    state->cubeBytes = state->cubeEdgeLength * state->cubeEdgeLength * state->cubeEdgeLength;
    state->cubeSetElements = state->M * state->M * state->M;
    state->cubeSetBytes = state->cubeSetElements * state->cubeBytes;

    for(uint i = 0; i <  Viewport::numberViewports; i++) {
        state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx /= static_cast<float>(state->magnification);
        state->viewerState->vpConfigs[i].texture.usedTexLengthDc = state->M;
    }

    if(state->M * state->cubeEdgeLength >= TEXTURE_EDGE_LEN) {
        qDebug() << "Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!";
        throw std::runtime_error("Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!");
    }
}

void DatasetLoadWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    datasetPath = settings.value(DATASET_LAST_USED, "").toString();

    auto appendRowSelectIfLU = [this](const QString & dataset){
        insertDatasetRow(dataset, tableWidget->rowCount());
        if (dataset == datasetPath) {
            tableWidget->selectRow(tableWidget->rowCount() - 1);
        }
    };

    tableWidget->blockSignals(true);

    //add datasets from file
    for(const auto & dataset : settings.value(DATASET_MRU).toStringList()) {
        appendRowSelectIfLU(dataset);
    }
    //add public datasets
    auto datasetsDir = QDir(":/resources/datasets");
    for (const auto & dataset : datasetsDir.entryInfoList()) {
        if (tableWidget->findItems(dataset.absoluteFilePath(), Qt::MatchExactly).empty()) {
            appendRowSelectIfLU(dataset.absoluteFilePath());
        }
    }
    //add Empty row at the end
    appendRowSelectIfLU("");
    tableWidget->cellWidget(tableWidget->rowCount() - 1, 2)->setEnabled(false);//don’t delete empty row

    tableWidget->blockSignals(false);
    updateDatasetInfo();


    if (QApplication::arguments().filter("supercube-edge").empty()) {//if not provided by cmdline
        state->M = settings.value(DATASET_SUPERCUBE_EDGE, 3).toInt();
    }
    if (QApplication::arguments().filter("overlay").empty()) {//if not provided by cmdline
        state->overlay = settings.value(DATASET_OVERLAY, false).toBool();
    }

    cubeEdgeSpin.setValue(state->cubeEdgeLength);
    supercubeEdgeSpin->setValue(state->M);
    segmentationOverlayCheckbox.setCheckState(state->overlay ? Qt::Checked : Qt::Unchecked);
    adaptMemoryConsumption();

    settings.endGroup();

    applyGeometrySettings();
}
