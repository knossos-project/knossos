#include "datasetremotewidget.h"

#include "knossos-global.h"
#include "knossos.h"

#include "ftp.h"

#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QFormLayout>
#include <QProcess>
#include <QMessageBox>
#include <QApplication>

DatasetRemoteWidget::DatasetRemoteWidget(QWidget *parent) : QWidget(parent) {

    auto mainLayout = new QVBoxLayout();
    auto formLayout = new QFormLayout();

    urlField = new QLineEdit();
    urlField->setText(state->taskState->host);
    usernameField = new QLineEdit();
    passwordField = new QLineEdit();
    passwordField->setEchoMode(QLineEdit::Password);

    host = new QLabel("Host:");
    username = new QLabel("Username:");
    passwd = new QLabel("Password");

    formLayout->addRow(host, urlField);
    formLayout->addRow(username, usernameField);
    formLayout->addRow(passwd, passwordField);

    supercubeEdgeSpin = new QSpinBox;
    int maxM = TEXTURE_EDGE_LEN / state->cubeEdgeLength;
    if (maxM % 2 == 0) {
        maxM -= 1;//set maxM to the next odd value
    }
    supercubeEdgeSpin->setRange(3, maxM);
    supercubeEdgeSpin->setSingleStep(2);
    supercubeEdgeSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    supercubeSizeLabel = new QLabel();

    supercubeEdgeSpin->setAlignment(Qt::AlignLeft);
    formLayout->addRow(supercubeEdgeSpin, supercubeSizeLabel);

    formLayout->addWidget(&segmentationOverlayCheckbox);

    processButton = new QPushButton("Use");
    cancelButton = new QPushButton("Cancel");

    formLayout->addRow(processButton, cancelButton);

    mainLayout->addLayout(formLayout);
    setLayout(mainLayout);

    QObject::connect(cancelButton, &QPushButton::clicked, this, &DatasetRemoteWidget::cancelButtonClicked);
    QObject::connect(processButton, &QPushButton::clicked, this, &DatasetRemoteWidget::processButtonClicked);
    QObject::connect(supercubeEdgeSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DatasetRemoteWidget::adaptMemoryConsumption);
    QObject::connect(&segmentationOverlayCheckbox, &QCheckBox::stateChanged, this, &DatasetRemoteWidget::adaptMemoryConsumption);

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void DatasetRemoteWidget::cancelButtonClicked() {
    this->parentWidget()->parentWidget()->parentWidget()->hide();
}

void DatasetRemoteWidget::processButtonClicked() {
    changeDataset(true);
    this->parentWidget()->parentWidget()->parentWidget()->hide();
}

void DatasetRemoteWidget::changeDataset(bool isGUI) {

    if(urlField->displayText().isEmpty() || usernameField->displayText().isEmpty() || passwordField->displayText().isEmpty()) {
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

    std::string url = urlField->text().toStdString();

    //get rid of the beginning "http://"
    if(url.find("http://") != std::string::npos) {
        url.erase(url.find("http://"), 7);
    }

    //the url should point to a datsetfolder
    qDebug() << url.c_str();
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

    std::string path = url.substr(i); //path
    url.erase(i, std::string::npos); //hostname

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
    emit userMoveSignal(state->cubeEdgeLength, state->cubeEdgeLength, state->cubeEdgeLength,
                        USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);

    this->waitForLoader();

    strncpy(state->ftpHostName, url.c_str(), MAX_PATH);
    strncpy(state->ftpBasePath, path.c_str(), MAX_PATH);
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
                state->cubeEdgeLength,
                USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
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

    emit setlastusedremote();
}

void DatasetRemoteWidget::waitForLoader() {
    emit startLoaderSignal();
    state->protectLoadSignal->lock();
    while (state->loaderBusy) {
        state->conditionLoadFinished->wait(state->protectLoadSignal);
    }
    state->protectLoadSignal->unlock();
}

void DatasetRemoteWidget::adaptMemoryConsumption() {
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
