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
#include "loader.h"
#include "mainwindow.h"
#include "viewer.h"

DatasetLoadWidget::DatasetLoadWidget(QWidget *parent) : QDialog(parent) {

    local = new QRadioButton("Local");
    remote = new QRadioButton("Remote");
    local->setChecked(true);

    datasetfileDialog = new QPushButton("Select Dataset Path");
    datasetfileDialog->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pathDropdown = new QComboBox();
    pathDropdown->setInsertPolicy(QComboBox::NoInsert);
    pathDropdown->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    pathDropdown->setEditable(true);

    urlCombo = new QComboBox();
    urlCombo->setInsertPolicy(QComboBox::InsertAtBottom);
    urlCombo->setEditable(true);
    urlCombo->setEnabled(false);

    usernameField = new QLineEdit();
    usernameField->setEnabled(false);

    passwordField = new QLineEdit();
    passwordField->setEchoMode(QLineEdit::Password);
    passwordField->setEnabled(false);

    host = new QLabel("Host:");
    host->setEnabled(false);
    username = new QLabel("Username:");
    username->setEnabled(false);
    passwd = new QLabel("Password");
    passwd->setEnabled(false);

    supercubeEdgeSpin = new QSpinBox;
    supercubeEdgeSpin->setMinimum(3);
    supercubeEdgeSpin->setSingleStep(2);
    supercubeEdgeSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    supercubeSizeLabel = new QLabel();

    cancelButton = new QPushButton("Cancel");
    processButton = new QPushButton("Use");

    auto hLayout0 = new QHBoxLayout;
    hLayout0->addWidget(local);

    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(pathDropdown);
    hLayout->addWidget(datasetfileDialog);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    auto hLayoutR00 = new QHBoxLayout;
    hLayoutR00->addWidget(line);

    auto hLayoutR0 = new QHBoxLayout;
    hLayoutR0->addWidget(remote);

    auto hLayoutR1 = new QGridLayout;
    hLayoutR1->addWidget(host);
    hLayoutR1->addWidget(urlCombo);
    auto hLayoutR2 = new QGridLayout;
    hLayoutR2->addWidget(username);
    hLayoutR2->addWidget(usernameField);
    auto hLayoutR3 = new QGridLayout;
    hLayoutR3->addWidget(passwd);
    hLayoutR3->addWidget(passwordField);

    QFrame* line0 = new QFrame();
    line0->setFrameShape(QFrame::HLine);
    line0->setFrameShadow(QFrame::Sunken);
    auto hLayoutR4 = new QHBoxLayout;
    hLayoutR4->addWidget(line0);

    auto hLayout2 = new QHBoxLayout;
    hLayout2->addWidget(supercubeEdgeSpin);
    supercubeEdgeSpin->setAlignment(Qt::AlignLeft);
    hLayout2->addWidget(supercubeSizeLabel);
    auto hLayout3 = new QHBoxLayout;
    hLayout3->addWidget(processButton);
    hLayout3->addWidget(cancelButton);

    auto localLayout = new QVBoxLayout();

    localLayout->addLayout(hLayout0);
    localLayout->addLayout(hLayout);
    localLayout->addLayout(hLayoutR00);
    localLayout->addLayout(hLayoutR0);
    localLayout->addLayout(hLayoutR1);
    localLayout->addLayout(hLayoutR2);
    localLayout->addLayout(hLayoutR3);
    localLayout->addLayout(hLayoutR4);
    localLayout->addLayout(hLayout2);
    localLayout->addWidget(&segmentationOverlayCheckbox);
    localLayout->addLayout(hLayout3);

    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(localLayout);
    setLayout(mainLayout);

    QObject::connect(datasetfileDialog, &QPushButton::clicked, this, &DatasetLoadWidget::datasetfileDialogClicked);
    QObject::connect(supercubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(&segmentationOverlayCheckbox, &QCheckBox::stateChanged, this, &DatasetLoadWidget::adaptMemoryConsumption);
    QObject::connect(cancelButton, &QPushButton::clicked, this, &DatasetLoadWidget::cancelButtonClicked);
    QObject::connect(processButton, &QPushButton::clicked, this, &DatasetLoadWidget::processButtonClicked);

    QObject::connect(local, &QRadioButton::toggled, this, &DatasetLoadWidget::onRadiobutton);
    QObject::connect(remote, &QRadioButton::toggled, this, &DatasetLoadWidget::onRadiobutton);

    QObject::connect(urlCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onUrlChange()));
    QObject::connect(urlCombo, &QComboBox::currentTextChanged, this, &DatasetLoadWidget::onUrlAdd);

    QObject::connect(usernameField, &QLineEdit::editingFinished, this, &DatasetLoadWidget::updateUsername);
    QObject::connect(usernameField, &QLineEdit::editingFinished, this, &DatasetLoadWidget::updatePassword);

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void DatasetLoadWidget::updateUsername() {
    if(urlCombo->currentIndex() >= 0) {
        usernameList[urlCombo->currentIndex()] = usernameField->text();
    } else {
        usernameList.append(usernameField->text());
    }
}

void DatasetLoadWidget::updatePassword() {
    if(urlCombo->currentIndex() >= 0) {
        passwordList[urlCombo->currentIndex()] = passwordField->text();
    } else {
        passwordList.append(passwordField->text());
    }
}

void DatasetLoadWidget::onUrlAdd() {
    if(urlCombo->count() > usernameList.count()) { //did we add an item?
        usernameField->clear();
        passwordField->clear();
        usernameList.append("");
        passwordList.append("");
    }
}

void DatasetLoadWidget::onRadiobutton() {
    if(!local->isChecked()) {
        datasetfileDialog->setEnabled(false);
        pathDropdown->setEnabled(false);

        host->setEnabled(true);

        username->setEnabled(true);
        passwd->setEnabled(true);
        urlCombo->setEnabled(true);
        usernameField->setEnabled(true);
        passwordField->setEnabled(true);

    } else {
        datasetfileDialog->setEnabled(true);
        pathDropdown->setEnabled(true);

        host->setEnabled(false);
        username->setEnabled(false);
        passwd->setEnabled(false);
        urlCombo->setEnabled(false);
        usernameField->setEnabled(false);
        passwordField->setEnabled(false);
    }
}

QStringList DatasetLoadWidget::getRecentPathItems() {
    QStringList recentPaths;
    int dirCount = this->pathDropdown->count();
    for (int i = 0; i < dirCount; i++) {
        recentPaths.append(this->pathDropdown->itemText(i));
    }

    return recentPaths;
}

void DatasetLoadWidget::onUrlChange() {
    usernameField->setText(usernameList[urlCombo->currentIndex()]);
    passwordField->setText(passwordList[urlCombo->currentIndex()]);
}

QStringList DatasetLoadWidget::getRecentHosts() {
    QStringList hosts;
    int count = urlCombo->count();
    for(int i=0; i<count;++i) {
        hosts.append(urlCombo->itemText(i));
    }
    return hosts;
}

QStringList DatasetLoadWidget::getRecentUsernames() {
    QStringList usernames;
    int count = usernameList.count();
    for(int i=0; i<count;++i) {
        usernames.append(usernameList[i]);

    }
    return usernames;
}

QStringList DatasetLoadWidget::getRecentPasswords() {
    QStringList passwords;
    int count = passwordList.count();
    for(int i=0; i<count;++i) {
        passwords.append(passwordList[i]);
    }
    return passwords;
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
    auto text = QString("Data cache cube edge length (%1 MiB RAM)").arg(mibibytes);
    supercubeSizeLabel->setText(text);
}

void DatasetLoadWidget::cancelButtonClicked() {
    this->hide();
}

void DatasetLoadWidget::processButtonClicked() {
    changeDataset(true);

    this->hide();
}

/* dataset can be selected in three ways:
 * 1. by selecting the folder containing a k.conf (for multires datasets it's a "magX" folder)
 * 2. for multires datasets: by selecting the dataset folder (the folder containing the "magX" subfolders)
 * 3. by specifying a .conf directly.
 */
void DatasetLoadWidget::changeDataset(bool isGUI) {
    //stuff for local dataset
    QFile confFile;
    QString filePath; // for holding the whole path to a .conf file
    QFileInfo pathInfo;
    QString path;

    int pathRecentIndex;

    //stuff for remote dataset
    std::string url;
    std::string spath;

    if(local->isChecked()) { //only for local dataset
        path = pathDropdown->currentText();
        pathInfo.setFile(path);

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
    } else { //only for remote dataset
        if(urlCombo->currentText().isEmpty() || usernameField->displayText().isEmpty() || passwordField->displayText().isEmpty()) {
            if (isGUI) {
                QMessageBox info;
                info.setWindowFlags(Qt::WindowStaysOnTopHint);
                info.setIcon(QMessageBox::Information);
                info.setWindowTitle("Information");
                info.setText("Url, Username or Password missing!");
                info.addButton(QMessageBox::Ok);
                info.exec();
            }
            return;
        }

        url = urlCombo->currentText().toStdString();

        //get rid of the beginning "http://"
        if(url.find("http://") != std::string::npos) {
            url.erase(url.find("http://"), 7);
        }

        //the url should point to a datsetfolder
        auto i = url.find('/');
        if(i == std::string::npos) {
            QMessageBox info;
            info.setWindowFlags(Qt::WindowStaysOnTopHint);
            info.setIcon(QMessageBox::Information);
            info.setWindowTitle("Information");
            info.setText("Url not valid!");
            info.addButton(QMessageBox::Ok);
            info.exec();

            return;
        }

        spath = url.substr(i); //path
        url.erase(i, std::string::npos); //hostname
    }

    if(local->isChecked()) { //only for local dataset
        pathRecentIndex = this->getRecentPathItems().indexOf(path);
        if (-1 != pathRecentIndex) {
            this->pathDropdown->removeItem(pathRecentIndex);
        }
        this->pathDropdown->insertItem(0, path);
        this->pathDropdown->setCurrentIndex(0);
    }
    // Note:
    // We clear the skeleton *before* reading the new config. In case we fail later, the skeleton would be nevertheless be gone.
    // This is a gamble we take, in order to not have possible bugs where the skeleton depends on old configuration values.
    if (isGUI) {
        emit clearSkeletonSignalGUI();
    } else {
        emit clearSkeletonSignalNoGUI();
    }

    emit (breakLoaderSignal());

    Knossos::applyDefaultConfig();

    if(remote->isChecked()) { //only for remote dataset
        strncpy(state->ftpHostName, url.c_str(), MAX_PATH);
        strncpy(state->ftpBasePath, spath.c_str(), MAX_PATH);
        strncpy(state->ftpUsername, usernameField->text().toStdString().c_str(), MAX_PATH);
        strncpy(state->ftpPassword, passwordField->text().toStdString().c_str(), MAX_PATH);

        state->ftpFileTimeout = 30000;

        state->loadMode = LM_FTP;

        // We are setting compressionRatio here, because the compressionRatio is not yet online in the mag1/knossos.conf
        // If it is set, readConfigFile will override it with the proper value
        state->compressionRatio = 1000;

        snprintf(state->path, MAX_PATH, "%s", state->loadFtpCachePath);

        if(downloadMag1ConfFile() == EXIT_FAILURE) {
            QMessageBox info;
            info.setWindowFlags(Qt::WindowStaysOnTopHint);
            info.setIcon(QMessageBox::Information);
            info.setWindowTitle("Information");
            info.setText("Could not download remote knossos.conf!");
            info.addButton(QMessageBox::Ok);
            info.exec();

            return;
        }

        Knossos::readConfigFile(QString("%1mag1/knossos.conf").arg(state->path).toStdString().c_str());

        //because token "ftp_mode" is missing mag1/knossos.conf we have to set it again
        state->loadMode = LM_FTP;
    }

    if(local->isChecked()) {
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
    }

    knossos->commonInitStates();

    // check if a fundamental geometry variable has changed. If so, the loader requires reinitialization
    state->M = supercubeEdgeSpin->value();
    state->overlay = segmentationOverlayCheckbox.isChecked();

    if(state->M * state->cubeEdgeLength >= TEXTURE_EDGE_LEN) {
        qDebug() << "Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!";
        throw std::runtime_error("Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!");
    }

    applyGeometrySettings();

    emit datasetSwitchZoomDefaults();

    // reset skeleton viewport
    if(state->skeletonState->rotationcounter == 0) {
        state->skeletonState->definedSkeletonVpView = SKELVP_RESET;
    }

    emit (startLoaderSignal());

    emit updateDatasetCompression();

    // ...beginning with loading the middle of dataset
    SET_COORDINATE(state->viewerState->currentPosition,
                   state->boundary.x / 2 - state->cubeEdgeLength,
                   state->boundary.y / 2 - state->cubeEdgeLength,
                   state->boundary.z / 2 - state->cubeEdgeLength);
    Coordinate range = { state->boundary.x/2, state->boundary.y/2, state->boundary.z/2 };
    emit datasetChanged(range, range);
    emit userMoveSignal(
                state->cubeEdgeLength,
                state->cubeEdgeLength,
                state->cubeEdgeLength,
                USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
}

void DatasetLoadWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    settings.setValue(DATASET_MRU, getRecentPathItems());

    if(urlCombo->currentIndex() < 0 && !urlCombo->currentText().isEmpty() ) {
       if(!usernameField->text().isEmpty() && !passwordField->text().isEmpty()) { //we save the input
           settings.setValue(DATASET_URL, urlCombo->currentText());
           settings.setValue(DATASET_USER, usernameField->text());
           settings.setValue(DATASET_PWD, passwordField->text());
       }
    } else if(urlCombo->currentIndex() >= 0) { //everything is there
        settings.setValue(DATASET_URL, getRecentHosts());
        settings.setValue(DATASET_USER, getRecentUsernames());
        settings.setValue(DATASET_PWD, getRecentPasswords());
    }

    settings.setValue(DATASET_LASTUSED, local->isChecked() ? "local" : "remote");

    settings.setValue(DATASET_M, state->M);
    settings.setValue(DATASET_OVERLAY, state->overlay);

    settings.endGroup();
}

void DatasetLoadWidget::applyGeometrySettings() {
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
}

void DatasetLoadWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_WIDGET);

    pathDropdown->clear();
    pathDropdown->insertItems(0, settings.value(DATASET_MRU).toStringList());

    urlCombo->clear();
    urlCombo->insertItems(0, settings.value(DATASET_URL).toStringList());
    if(!settings.value(DATASET_USER).toStringList().isEmpty()) {
        usernameList = settings.value(DATASET_USER).toStringList();
        passwordList = settings.value(DATASET_PWD).toStringList();
        onUrlChange();
    }

    lastused = settings.value(DATASET_LASTUSED).toString().toStdString();

    local->setChecked(lastused == "local");
    remote->setChecked(lastused == "remote");
    onRadiobutton();

    if (QApplication::arguments().filter("supercube-edge").empty()) {//if not provided by cmdline
        state->M = settings.value(DATASET_M, 3).toInt();
    }
    if (QApplication::arguments().filter("overlay").empty()) {//if not provided by cmdline
        state->overlay = settings.value(DATASET_OVERLAY, true).toBool();
    }

    supercubeEdgeSpin->setValue(state->M);
    segmentationOverlayCheckbox.setCheckState(state->overlay ? Qt::Checked : Qt::Unchecked);
    adaptMemoryConsumption();

    settings.endGroup();


    applyGeometrySettings();
}
