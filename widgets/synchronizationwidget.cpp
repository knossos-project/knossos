#include "synchronizationwidget.h"

SynchronizationWidget::SynchronizationWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Synchronization Settings");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    connectionLabel = new QLabel("No Connection");

    remotePortLabel = new QLabel("Remote Port");
    remotePortSpinBox = new QSpinBox();

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(remotePortLabel, remotePortSpinBox);

    connectButton = new QPushButton("Connect");

    mainLayout->addWidget(connectionLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(connectButton);

    setLayout(mainLayout);
}

void SynchronizationWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}

