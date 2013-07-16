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

#include "datasavingwidget.h"
#include "GUIConstants.h"
#include <QRadioButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <QSettings>
#include "knossos-global.h"

extern struct stateInfo *state;

DataSavingWidget::DataSavingWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Data Saving Options");
    QVBoxLayout *mainLayout = new QVBoxLayout();

    autosaveButton = new QCheckBox("Auto-Saving");
    autosaveIntervalLabel = new QLabel("Autosave Interval");
    autosaveIntervalSpinBox = new QSpinBox();


    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(autosaveIntervalLabel, autosaveIntervalSpinBox);

    autoincrementFileNameButton = new QCheckBox("Autoincrement File Name");


    mainLayout->addWidget(autosaveButton);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(autoincrementFileNameButton);

    setLayout(mainLayout);

    connect(autosaveButton, SIGNAL(clicked(bool)), this, SLOT(autosaveButtonPushed(bool)));
    connect(autoincrementFileNameButton, SIGNAL(clicked(bool)), this, SLOT(autonincrementFileNameButtonPushed(bool)));
}

void DataSavingWidget::loadSettings() {
   int width, height, x, y;
   bool visible;

   QSettings settings;
   settings.beginGroup(CONSOLE_WIDGET);
   width = settings.value(WIDTH).toInt();
   height = settings.value(HEIGHT).toInt();
   x = settings.value(POS_X).toInt();
   y = settings.value(POS_Y).toInt();
   visible = settings.value(VISIBLE).toBool();
   if(settings.value(AUTO_SAVING).toBool())
    this->autosaveButton->setChecked(settings.value(AUTO_SAVING).toBool());
   if(settings.value(SAVING_INTERVAL).toInt())
    this->autosaveIntervalSpinBox->setValue(settings.value(SAVING_INTERVAL).toInt());
   if(settings.value(AUTOINC_FILENAME).toBool())
    this->autoincrementFileNameButton->setChecked(settings.value(AUTOINC_FILENAME).toBool());
   settings.endGroup();

   //setGeometry(x, y, width, height);

   /*
   autosaveButton->setChecked(state->skeletonState->autoSaveBool);
   autosaveIntervalSpinBox->setValue(state->skeletonState->autoSaveInterval);
   autoincrementFileNameButton->setChecked(state->skeletonState->autoFilenameIncrementBool);
   */
}

void DataSavingWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(CONSOLE_WIDGET);
    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());
    settings.setValue(VISIBLE, this->isVisible());
    settings.setValue(AUTO_SAVING, this->autosaveButton->isChecked());
    settings.setValue(SAVING_INTERVAL, this->autosaveIntervalSpinBox->value());
    settings.setValue(AUTOINC_FILENAME, this->autoincrementFileNameButton->isChecked());
    settings.endGroup();
}

void DataSavingWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit uncheckSignal();
}

void DataSavingWidget::autosaveButtonPushed(bool on) {
    if(on) {
       state->skeletonState->autoSaveBool = true;

       state->skeletonState->autoSaveInterval = autosaveIntervalSpinBox->value();
    } else {
        state->skeletonState->autoSaveBool = false;
    }
}

void DataSavingWidget::autonincrementFileNameButtonPushed(bool on) {
    if(on) {
        state->skeletonState->autoFilenameIncrementBool = true;
    } else {
        state->skeletonState->autoFilenameIncrementBool = false;
    }

}


