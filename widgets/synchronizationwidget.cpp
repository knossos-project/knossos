#include "synchronizationwidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QLabel>
#include <QFormLayout>
#include <QSpinBox>
#include <QPushButton>
#include "knossos-global.h"
#include "mainwindow.h"

extern struct stateInfo *state;

SynchronizationWidget::SynchronizationWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Synchronization Settings");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    connectionLabel = new QLabel("No Connection");

    remotePortLabel = new QLabel("Remote Port");
    remotePortSpinBox = new QSpinBox();
    remotePortSpinBox->setMaximum(65535);
    remotePortSpinBox->setValue(7890);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(remotePortLabel, remotePortSpinBox);

    connectButton = new QPushButton("Connect");

    mainLayout->addWidget(connectionLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(connectButton);

    setLayout(mainLayout);

    connect(connectButton, SIGNAL(clicked()), this, SLOT(connectButtonPushed()));
}

void SynchronizationWidget::loadSettings() {

}

void SynchronizationWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    MainWindow *parent = (MainWindow *) this->parentWidget();
}

/**
  * @todo In GUI.c there are two methods UI_SyncConnect and UI_SyncDisconnect which requests state->clientState->connect but do not change the value ? Who does this job
  */
void SynchronizationWidget::connectButtonPushed() {
    if(!connected) {
        connected = true;
        connectButton->setText("Disconnect");
    } else {
        connected = false;
        connectButton->setText("Connect");
    }
        //sendClientSignal();

}

