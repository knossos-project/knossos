/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */
#include "synchronizationwidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QLabel>
#include <QFormLayout>
#include <QSpinBox>
#include <QPushButton>
#include "knossos-global.h"
#include "knossos.h"

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
    remotePortSpinBox->setEnabled(false);

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

void SynchronizationWidget::saveSettings() {

}

void SynchronizationWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit uncheckSignal();
}


void SynchronizationWidget::connectButtonPushed() {
    if(!connected) {
        connected = true;

        Knossos::sendClientSignal();

        connectButton->setText("Disconnect");
    } else {
        connected = false;
        connectButton->setText("Connect");
    }


}

void SynchronizationWidget::updateConnectionInfo() {
    connectionLabel->setText("Connected");
    connectButton->setText("Disconnect");
}

void SynchronizationWidget::updateDisconnectionInfo() {
    connectionLabel->setText("No Connection");
    connectButton->setText("Connect");
}

