#include "datasetloadwidget.h"

#include <stdexcept>

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "ftp.h"
#include "GuiConstants.h"
#include "knossos.h"
#include "knossos-global.h"
#include "mainwindow.h"
#include "viewer.h"

extern  stateInfo *state;

DatasetLoadWidget::DatasetLoadWidget(QWidget *parent) : QDialog(parent) {
    localGroup = new QGroupBox("Local Dataset");

    datasetfileDialog = new QPushButton("Select Dataset Path");
    datasetfileDialog->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->pathDropdown = new QComboBox();
    this->pathDropdown->setInsertPolicy(QComboBox::NoInsert);
    this->pathDropdown->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    this->pathDropdown->setEditable(true);
    supercubeEdgeSpin = new QSpinBox;
    int maxM = TEXTURE_EDGE_LEN / state->cubeEdgeLength;
    if (maxM % 2 == 0) {
        maxM -= 1;//set maxM to the next odd value
    }
    supercubeEdgeSpin->setRange(3, maxM);
    supercubeEdgeSpin->setSingleStep(2);
    supercubeEdgeSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    supercubeSizeLabel = new QLabel();
    cancelButton = new QPushButton("Cancel");
    processButton = new QPushButton("Use");

    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(pathDropdown);
    hLayout->addWidget(datasetfileDialog);
    auto hLayout2 = new QHBoxLayout;
    hLayout2->addWidget(supercubeEdgeSpin);
    supercubeEdgeSpin->setAlignment(Qt::AlignLeft);
    hLayout2->addWidget(supercubeSizeLabel);
    auto hLayout3 = new QHBoxLayout;
    hLayout3->addWidget(processButton);
    hLayout3->addWidget(cancelButton);

    auto localLayout = new QVBoxLayout();
    localLayout->addLayout(hLayout);
    localLayout->addLayout(hLayout2);
    localLayout->addWidget(&segmentationOverlayCheckbox);
    localLayout->addLayout(hLayout3);
    localGroup->setLayout(localLayout);

    auto mainLayout = new QVBoxLayout();
    mainLayout->addWidget(localGroup);
    setLayout(mainLayout);

    QObject::connect(datasetfileDialog, &QPushButton::clicked, this, &DatasetLoadWidget::datasetfileDialogClicked);
    QObject::connect(supercubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&segmentationOverlayCheckbox, &QCheckBox::stateChanged, this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(cancelButton, &QPushButton::clicked, this, &DatasetLoadWidget::cancelButtonClicked);
    QObject::connect(processButton, &QPushButton::clicked, this, &DatasetLoadWidget::processButtonClicked);

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

QStringList DatasetLoadWidget::getRecentPathItems() {
    QStringList recentPaths;
    int dirCount = this->pathDropdown->count();
    for (int i = 0; i < dirCount; i++) {
        recentPaths.append(this->pathDropdown->itemText(i));
    }
    return recentPaths;
}

void DatasetLoadWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);
    settings.setValue(DATASET_MRU, getRecentPathItems());
    settings.setValue(DATASET_M, state->M);
    settings.setValue(DATASET_OVERLAY, state->overlay);
    settings.endGroup();
}

void DatasetLoadWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);
    pathDropdown->clear();
    pathDropdown->insertItems(0, settings.value(DATASET_MRU).toStringList());
    if (QApplication::arguments().filter("supercube-edge").empty()) {//if not provided by cmdline
        state->M = settings.value(DATASET_M, 3).toInt();
    }
    if (QApplication::arguments().filter("overlay").empty()) {//if not provided by cmdline
        state->overlay = settings.value(DATASET_OVERLAY, true).toBool();
    }
    settings.endGroup();

    supercubeEdgeSpin->setValue(state->M);
    segmentationOverlayCheckbox.setCheckState(state->overlay ? Qt::Checked : Qt::Unchecked);
    adaptMemoryConsumption();

    //settings depending on M
    state->cubeSetElements = state->M * state->M * state->M;
    state->cubeSetBytes = state->cubeSetElements * state->cubeBytes;

    for(uint i = 0; i < state->viewerState->numberViewports; i++) {
        state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx /= static_cast<float>(state->magnification);
        state->viewerState->vpConfigs[i].texture.usedTexLengthDc = state->M;
    }

    if(state->M * state->cubeEdgeLength >= TEXTURE_EDGE_LEN) {
        qDebug() << "Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!";
        throw std::runtime_error("Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!");
    }

    // We're not doing stuff in parallel, yet. So we skip the locking
    // part.
    // This *10 thing is completely arbitrary. The larger the table size,
    // the lower the chance of getting collisions and the better the loading
    // order will be respected. *10 doesn't seem to have much of an effect
    // on performance but we should try to find the optimal value some day.
    // Btw: A more clever implementation would be to use an array exactly the
    // size of the supercube and index using the modulo operator.
    // sadly, that realization came rather late. ;)

    // creating the hashtables is cheap, keeping the datacubes is
    // memory expensive..
    for(int i = 0; i <= NUM_MAG_DATASETS; i = i * 2) {
        state->Dc2Pointer[int_log(i)] = Hashtable::ht_new(state->cubeSetElements * 10);
        state->Oc2Pointer[int_log(i)] = Hashtable::ht_new(state->cubeSetElements * 10);
        if(i == 0) i = 1;
    }
}

void DatasetLoadWidget::datasetfileDialogClicked() {
    state->viewerState->renderInterval = SLOW;
    QApplication::processEvents();
    QString selectFile = QFileDialog::getOpenFileName(this, "Select a KNOSSOS dataset", QDir::homePath(), "*.conf");
    if(!selectFile.isNull()) {
        pathDropdown->setEditText(selectFile);
    }
    state->viewerState->renderInterval = FAST;
}

void DatasetLoadWidget::adaptMemoryConsumption() {
    const auto superCubeEdge = supercubeEdgeSpin->value();
    auto mibibytes = std::pow(state->cubeEdgeLength, 3) * std::pow(superCubeEdge, 3) / std::pow(1024, 2);
    mibibytes += segmentationOverlayCheckbox.isChecked() * OBJID_BYTES * mibibytes;
    auto text = QString("Data cache cube edge length (%1 MiB RAM").arg(mibibytes);
    if (state->M != supercubeEdgeSpin->value() || state->overlay != segmentationOverlayCheckbox.isChecked()) {
        text.append(", restart required");
    }
    text.append(")");
    supercubeSizeLabel->setText(text);
}

void DatasetLoadWidget::closeEvent(QCloseEvent *) {
    this->hide();
}

void DatasetLoadWidget::waitForLoader() {
    emit startLoaderSignal();
    state->protectLoadSignal->lock();
    while (state->loaderBusy) {
        state->conditionLoadFinished->wait(state->protectLoadSignal);
    }
    state->protectLoadSignal->unlock();
}

void DatasetLoadWidget::cancelButtonClicked() {
    this->hide();
}

void DatasetLoadWidget::processButtonClicked() {
    changeDataset(true);
}

/* dataset can be selected in three ways:
 * 1. by selecting the folder containing a k.conf (for multires datasets it's a "magX" folder)
 * 2. for multires datasets: by selecting the dataset folder (the folder containing the "magX" subfolders)
 * 3. by specifying a .conf directly.
 */
void DatasetLoadWidget::changeDataset(bool isGUI) {
    QString path = pathDropdown->currentText();
    if(path.isNull() || path.isEmpty()) {
        if (isGUI) {
            QMessageBox info;
            info.setWindowFlags(Qt::WindowStaysOnTopHint);
            info.setIcon(QMessageBox::Information);
            info.setWindowTitle("Information");
            info.setText("No path selected!");
            info.addButton(QMessageBox::Ok);
            info.exec();
        }
        return;
    }

    QFile confFile;
    QString filePath; // for holding the whole path to a .conf file
    QFileInfo pathInfo(path);
    if(pathInfo.isFile()) { // .conf file selected (case 3)
        filePath = path;
        confFile.setFileName(filePath);
    }
    else { // folder selected
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
                if (isGUI) {
                    QMessageBox info;
                    info.setWindowFlags(Qt::WindowStaysOnTopHint);
                    info.setIcon(QMessageBox::Information);
                    info.setWindowTitle("Information");
                    info.setText("Could not find a dataset file (*.conf)");
                    info.addButton(QMessageBox::Ok);
                    info.exec();
                }
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
    // check if user changed the supercube size. If so, KNOSSOS needs to restart
    const auto superCubeChange = state->M != supercubeEdgeSpin->value();
    const auto segmentationOverlayChange = state->overlay != segmentationOverlayCheckbox.isChecked();
    if (isGUI && (superCubeChange || segmentationOverlayChange)) {
        auto text = QString("You chose to:\n");
        if (superCubeChange) {
            text += "• change the data cache cube edge length\n";
        }
        if (segmentationOverlayChange) {
            text += "• change the display of the segmentation data\n";
        }
        text += QString("\nKnossos needs to restart to apply this.\
                        You will loose your annotation if you didn’t save or already cleared it.");
        if (QMessageBox::question(this, "Knossos restart – loss of unsaved annotation",
                                  text, QMessageBox::Ok | QMessageBox::Abort) == QMessageBox::Ok) {
            auto args = QApplication::arguments(); //append to previous args, newer values will overwrite previous ones
            //change via cmdline so it is not saved on a crash/kill
            if (superCubeChange) {
                args.append(QString("--supercube-edge=%0").arg(supercubeEdgeSpin->value()));
            }
            if (segmentationOverlayChange) {
                args.append(QString("--overlay=%0").arg(segmentationOverlayCheckbox.isChecked()));
            }
            qDebug() << args;
            QApplication::closeAllWindows();//stop loader, and queue application closing
            loader.reset();//free cubes before loading the new knossos instance
            const auto program = args[0];
            args.pop_front();//remove program path from args
            QProcess::startDetached(program, args);
            return;
        }
    }

    int pathRecentIndex = this->getRecentPathItems().indexOf(path);
    if (-1 != pathRecentIndex) {
        this->pathDropdown->removeItem(pathRecentIndex);
    }
    this->pathDropdown->insertItem(0, path);
    this->pathDropdown->setCurrentIndex(0);

    // Note:
    // We clear the skeleton *before* reading the new config. In case we fail later, the skeleton would be nevertheless be gone.
    // This is a gamble we take, in order to not have possible bugs where the skeleton depends on old configuration values.
    if (isGUI) {
        emit clearSkeletonSignalGUI();
    } else {
        emit clearSkeletonSignalNoGUI();
    }

    // BUG BUG BUG
    // The following code, combined with the way loader::run in currently implemented
    // (revision 966) contains a minor timing    issue that may result in a crash, namely
    // since loader::loadCubes begins executing in LM_LOCAL mode and ends in LM_FTP,
    // if at this point in the code we're in LM_LOCAL, and are about an FTP dataset
    // BUG BUG BUG

    state->loaderDummy = true;

    // Stupid userMove hack-around. In order to move somewhere, you have to currently be at another supercube.
    state->viewerState->currentPosition.x =
            state->viewerState->currentPosition.y =
            state->viewerState->currentPosition.z = 0;
    emit userMoveSignal(state->cubeEdgeLength, state->cubeEdgeLength, state->cubeEdgeLength);

    this->waitForLoader();

    if(false == Knossos::readConfigFile(filePath.toStdString().c_str())) {
        QMessageBox info;
        info.setWindowFlags(Qt::WindowStaysOnTopHint);
        info.setIcon(QMessageBox::Information);
        info.setWindowTitle("Information");
        info.setText(QString("Failed to read config from %1").arg(filePath));
        info.addButton(QMessageBox::Ok);
        info.exec();
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

    this->waitForLoader();

    emit changeDatasetMagSignal(DATA_SET);
    emit updateDatasetCompression();

    // Back to usual...
    state->loaderDummy = false;

    // ...beginning with loading the middle of dataset, as upon startup
    SET_COORDINATE(state->viewerState->currentPosition,
                   state->boundary.x / 2 - state->cubeEdgeLength,
                   state->boundary.y / 2 - state->cubeEdgeLength,
                   state->boundary.z / 2 - state->cubeEdgeLength);
    emit userMoveSignal(
                state->cubeEdgeLength,
                state->cubeEdgeLength,
                state->cubeEdgeLength);
    // reset skeleton viewport
    if(state->skeletonState->rotationcounter == 0) {
        state->skeletonState->definedSkeletonVpView = SKELVP_RESET;
    }

    //Viewer::changeDatasetMag cannot be used when ther’re no other mags available
    //sets viewport settings according to current mag
    for(size_t i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != VIEWPORT_SKELETON) {
            state->viewerState->vpConfigs[i].texture.zoomLevel = VPZOOMMIN;
            state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx = 1. / TEXTURE_EDGE_LEN / state->magnification;
        }
    }

    emit datasetSwitchZoomDefaults();

    this->hide();
}
