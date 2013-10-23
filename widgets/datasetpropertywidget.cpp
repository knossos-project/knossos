#include "datasetpropertywidget.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QMessageBox>
#include <QLabel>
#include "knossos-global.h"
#include "knossos.h"
#include "ftp.h"

extern struct stateInfo *state;

DatasetPropertyWidget::DatasetPropertyWidget(QWidget *parent) :
    QWidget(parent)
{

    QVBoxLayout *mainLayout = new QVBoxLayout();

    QGridLayout *localLayout = new QGridLayout();

    localGroup = new QGroupBox("Local Dataset");

    //mainLayout->addWidget(localGroup);

    datasetfileDialog = new QPushButton("Select Dataset Path");
    this->path = new QLineEdit();
    supercubeSize = new QComboBox();
    supercubeSizeLabel = new QLabel("Supercube Size");

    cancelButton = new QPushButton("Cancel");
    processButton = new QPushButton("Use");

    for(int i = 3; i < 9; i+=2) {

        supercubeSize->addItem(QVariant::fromValue(i).toString());
    }

    localLayout->addWidget(path, 0, 0);
    localLayout->addWidget(datasetfileDialog, 0, 1);
    localLayout->addWidget(supercubeSizeLabel, 1, 0);
    localLayout->addWidget(supercubeSize, 1, 1);
    localLayout->addWidget(processButton, 2,0);
    localLayout->addWidget(cancelButton, 2, 1);

    localGroup->setLayout(localLayout);
    mainLayout->addWidget(localGroup);

    remoteGroup = new QGroupBox("Remote Dataset");
    QGridLayout *remoteLayout = new QGridLayout();

    QLabel *usernameLabel = new QLabel("Username:");
    QLabel *passwordLabel = new QLabel("Password");
    QLabel *urlLabel = new QLabel("Url");

    username = new QLineEdit();
    password = new QLineEdit();
    password->setEchoMode(QLineEdit::Password);
    url = new QLineEdit();
    remoteCancelButton = new QPushButton("Cancel");
    connectButton = new QPushButton("Connect");

    remoteLayout->addWidget(usernameLabel, 0, 0);
    remoteLayout->addWidget(username, 0, 1);
    remoteLayout->addWidget(passwordLabel, 1, 0);
    remoteLayout->addWidget(password, 1, 1);
    remoteLayout->addWidget(urlLabel, 2, 0);
    remoteLayout->addWidget(url, 2, 1);

    remoteLayout->addWidget(connectButton, 3, 0);
    remoteLayout->addWidget(remoteCancelButton, 3, 1);

    remoteGroup->setLayout(remoteLayout);
    mainLayout->addWidget(remoteGroup);

    setLayout(mainLayout);

    connect(this->datasetfileDialog, SIGNAL(clicked()), this, SLOT(datasetfileDialogClicked()));
    connect(this->cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClicked()));
    connect(this->processButton, SIGNAL(clicked()), this, SLOT(processButtonClicked()));
    connect(this->remoteCancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClicked()));
    connect(this->connectButton, SIGNAL(clicked()), this, SLOT(connectButtonClicked()));
}

void DatasetPropertyWidget::datasetfileDialogClicked() {
    QApplication::processEvents();
    this->dir = QFileDialog::getExistingDirectory(this, "Select a knossos.conf", QDir::homePath());
    qDebug() << dir;
    if(!dir.isNull()) {
        path->setText(dir);
       //Knossos::configFromCli();
    }
}

void DatasetPropertyWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}

void DatasetPropertyWidget::cancelButtonClicked() {
    this->dir.clear();
    this->hide();
}

void DatasetPropertyWidget::processButtonClicked() {
    if(!dir.isNull()) {
        QString conf = QString(dir).append("/knossos.conf");
        QFile confFile(conf);
        if(!confFile.exists()) {
            QMessageBox::information(this, "Attention", "There is no knossos.conf", QMessageBox::Ok);
            return;
        }

        strcpy(state->path, dir.toStdString().c_str());
        state->M = supercubeSize->currentText().toInt();

        if(Knossos::readConfigFile(conf.toStdString().c_str())) {
           emit clearSkeletonSignal();
           emit resetLoaderSignal();
           Knossos::findAndRegisterAvailableDatasets();
        } else {

        }

        qDebug() << state->path;
        qDebug() << state->M;
    }

    this->hide();
}

void DatasetPropertyWidget::connectButtonClicked() {
    if(!username->text().isEmpty() & !password->text().isEmpty() & !url->text().isEmpty()) {

        char curlPath[MAX_PATH];
        strcpy(curlPath, this->url->text().toStdString().c_str());
        strcpy(state->ftpUsername, username->text().toStdString().c_str());
        strcpy(state->ftpPassword, password->text().toStdString().c_str());
        downloadFile(curlPath, NULL);
    }

}
