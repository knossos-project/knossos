#include "datasetpropertywidget.h"

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

DatasetPropertyWidget::DatasetPropertyWidget(QWidget *parent) : QDialog(parent) {
    localGroup = new QGroupBox("Local Dataset");

    datasetfileDialog = new QPushButton("Select Dataset Path");
    datasetfileDialog->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->path = new QComboBox();
    this->path->setInsertPolicy(QComboBox::NoInsert);
    this->path->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    this->path->setEditable(true);
    supercubeEdgeSpin = new QSpinBox;
    int maxM = TEXTURE_EDGE_LEN / state->cubeEdgeLength;
    if (maxM % 2 == 0) {
        maxM -= 1;//set maxM to the next odd value
    }
    supercubeEdgeSpin->setRange(3, maxM);
    supercubeEdgeSpin->setSingleStep(2);
    supercubeEdgeSpin->setValue(state->M);
    supercubeEdgeSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    supercubeSizeLabel = new QLabel();
    supercubeEdgeSpinValueChanged(state->M);//init label
    cancelButton = new QPushButton("Cancel");
    processButton = new QPushButton("Use");

    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(path);
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
    localLayout->addLayout(hLayout3);
    localGroup->setLayout(localLayout);

    auto mainLayout = new QVBoxLayout();
    mainLayout->addWidget(localGroup);
    setLayout(mainLayout);

    QObject::connect(datasetfileDialog, &QPushButton::clicked, this, &DatasetPropertyWidget::datasetfileDialogClicked);
    QObject::connect(supercubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetPropertyWidget::supercubeEdgeSpinValueChanged);
    QObject::connect(cancelButton, &QPushButton::clicked, this, &DatasetPropertyWidget::cancelButtonClicked);
    QObject::connect(processButton, &QPushButton::clicked, this, &DatasetPropertyWidget::processButtonClicked);

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

QStringList DatasetPropertyWidget::getRecentDirsItems() {
    QStringList recentDirs;
    int dirCount = this->path->count();
    for (int i = 0; i < dirCount; i++) {
        recentDirs.append(this->path->itemText(i));
    }
    return recentDirs;
}

void DatasetPropertyWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);
    settings.setValue(DATASET_MRU, getRecentDirsItems());
    if(state->M == 0) {
        settings.setValue(DATASET_M, 3);
    }
    else {
        settings.setValue(DATASET_M, state->M);
    }
    settings.endGroup();
}

void DatasetPropertyWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);
    path->clear();
    path->insertItems(0, settings.value(DATASET_MRU).toStringList());
    if (state->M == 0) {//M is invalid
        state->M = settings.value(DATASET_M, 3).toInt();
    }
    settings.endGroup();

    supercubeEdgeSpin->setValue(state->M);
    supercubeEdgeSpinValueChanged(state->M);//refill label

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

void DatasetPropertyWidget::datasetfileDialogClicked() {
    state->viewerState->renderInterval = SLOW;
    QApplication::processEvents();
    QString selectDir = QFileDialog::getExistingDirectory(this, "Select a knossos.conf", QDir::homePath());
    if(!selectDir.isNull()) {
        path->setEditText(selectDir);
    }
    state->viewerState->renderInterval = FAST;
}

void DatasetPropertyWidget::supercubeEdgeSpinValueChanged(const int value) {
    const auto mibibytes = std::pow(state->cubeEdgeLength, 3) * std::pow(value, 3) / std::pow(1024, 2);
    auto text = QString("Data cache cube edge length (%1 MiB RAM").arg(mibibytes);
    if (state->M != supercubeEdgeSpin->value()) {
        text.append(", restart required");
    }
    text.append(")");
    supercubeSizeLabel->setText(text);
}

void DatasetPropertyWidget::closeEvent(QCloseEvent *) {
    this->hide();
}

void DatasetPropertyWidget::waitForLoader() {
    emit startLoaderSignal();
    state->protectLoadSignal->lock();
    while (state->loaderBusy) {
        state->conditionLoadFinished->wait(state->protectLoadSignal);
    }
    state->protectLoadSignal->unlock();
}

void DatasetPropertyWidget::cancelButtonClicked() {
    this->hide();
}

void DatasetPropertyWidget::processButtonClicked() {
    changeDataSet(true);
}

void DatasetPropertyWidget::changeDataSet(bool isGUI) {
    QString dir = this->path->currentText();
    if(dir.isNull() || dir.isEmpty()) {
        if (isGUI) {
            QMessageBox info;
            info.setWindowFlags(Qt::WindowStaysOnTopHint);
            info.setIcon(QMessageBox::Information);
            info.setWindowTitle("Information");
            info.setText("No directory specified!");
            info.addButton(QMessageBox::Ok);
            info.exec();
        }
        return;
    }

    QString conf = QString(dir).append("/knossos.conf");
    QFile confFile(conf);
    if(!confFile.exists()) {
        if (isGUI) {
            QMessageBox info;
            info.setWindowFlags(Qt::WindowStaysOnTopHint);
            info.setIcon(QMessageBox::Information);
            info.setWindowTitle("Information");
            info.setText("There is no knossos.conf");
            info.addButton(QMessageBox::Ok);
            info.exec();
        }
        return;
    }

    int dirRecentIndex = this->getRecentDirsItems().indexOf(dir);
    if (-1 != dirRecentIndex) {
        this->path->removeItem(dirRecentIndex);
    }
    this->path->insertItem(0, dir);
    this->path->setCurrentIndex(0);

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
    // (revision 966) contains a minor timing issue that may result in a crash, namely
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

    strcpy(state->path, dir.toStdString().c_str());

    if(false == Knossos::readConfigFile(conf.toStdString().c_str())) {
        QMessageBox info;
        info.setWindowFlags(Qt::WindowStaysOnTopHint);
        info.setIcon(QMessageBox::Information);
        info.setWindowTitle("Information");
        info.setText(QString("Failed to read config from %s").arg(conf));
        info.addButton(QMessageBox::Ok);
        info.exec();
        return;
    }

    knossos->commonInitStates();

    if (isGUI && state->M != supercubeEdgeSpin->value()) {
        auto text = QString("You chose to change the data cache cube edge length. \n")
                +QString("\nKnossos needs to restart to apply this.\n\n")
                +QString("You will loose your skeleton if you didn’t save or already cleared it.");
        if (QMessageBox::question(this, "Knossos restart", text, QMessageBox::Ok | QMessageBox::Abort) == QMessageBox::Ok) {
            //ideally one would use qApp->quit(), but the cleanup steps are not connected to this
            static_cast<MainWindow*>(parent())->close();//call Knossos cleanup func
            auto args = qApp->arguments();
            args.append(QString("--supercube-edge=%0").arg(supercubeEdgeSpin->value()));//change M via cmdline so it is not saved on a crash/kill
            qDebug() << args;
            QProcess::startDetached(qApp->arguments()[0], args);
        }
    }

    this->waitForLoader();

    emit changeDatasetMagSignal(DATA_SET);

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
