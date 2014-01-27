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
#include <QSpacerItem>
#include <QApplication>
#include <QDesktopWidget>

#include "knossos-global.h"

extern  stateInfo *state;

DataSavingWidget::DataSavingWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Data Saving Options");
    QVBoxLayout *mainLayout = new QVBoxLayout();

    autosaveCheckbox = new QCheckBox("Auto-Saving");
    autosaveIntervalLabel = new QLabel("Saving interval [min]");
    autosaveIntervalSpinBox = new QSpinBox();
    autosaveIntervalSpinBox->setMinimum(1);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(autosaveIntervalLabel, autosaveIntervalSpinBox);

    autoincrementFileNameButton = new QCheckBox("Autoincrement File Name");

    mainLayout->addWidget(autosaveCheckbox);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(autoincrementFileNameButton);
    mainLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(mainLayout);

    connect(autosaveCheckbox, SIGNAL(clicked(bool)), this, SLOT(autosaveCheckboxChecked(bool)));
    connect(autosaveIntervalSpinBox, SIGNAL(valueChanged(int)), this, SLOT(autosaveIntervalChanged(int)));
    connect(autoincrementFileNameButton, SIGNAL(clicked(bool)), this, SLOT(autonincrementFileNameButtonPushed(bool)));
}


void DataSavingWidget::loadSettings() {
   int width, height, x, y;
   bool visible;

   QSettings settings;
     settings.beginGroup(DATA_SAVING_WIDGET);
     width = (settings.value(WIDTH).isNull())? this->width() : settings.value(WIDTH).toInt();
     height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
     if(settings.value(POS_X).isNull() or settings.value(POS_Y).isNull()) {
         x = QApplication::desktop()->screen()->rect().center().x();
         y = QApplication::desktop()->screen()->rect().topRight().y() + 50;
     }
     else {
         x = settings.value(POS_X).toInt();
         y = settings.value(POS_Y).toInt();
     }
     visible = (settings.value(VISIBLE).isNull())? false : settings.value(VISIBLE).toBool();

     state->skeletonState->autoSaveBool =
             (settings.value(AUTO_SAVING).isNull())? true : settings.value(AUTO_SAVING).toBool();
     this->autosaveCheckbox->setChecked(state->skeletonState->autoSaveBool);

     state->skeletonState->autoSaveInterval =
             (settings.value(SAVING_INTERVAL).isNull())? 5 : settings.value(SAVING_INTERVAL).toInt();
     this->autosaveIntervalSpinBox->setValue(state->skeletonState->autoSaveInterval);

     state->skeletonState->autoFilenameIncrementBool =
             (settings.value(AUTOINC_FILENAME).isNull())? true : settings.value(AUTOINC_FILENAME).toBool();
     this->autoincrementFileNameButton->setChecked(state->skeletonState->autoFilenameIncrementBool);

   settings.endGroup();
   if(visible) {
       show();
   }
   else {
       hide();
   }
   setGeometry(x, y, width, height);
}

void DataSavingWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(DATA_SAVING_WIDGET);
    settings.setValue(WIDTH, this->geometry().width());
    settings.setValue(HEIGHT, this->geometry().height());
    settings.setValue(POS_X, this->geometry().x());
    settings.setValue(POS_Y, this->geometry().y());
    settings.setValue(VISIBLE, this->isVisible());
    settings.setValue(AUTO_SAVING, this->autosaveCheckbox->isChecked());
    settings.setValue(SAVING_INTERVAL, this->autosaveIntervalSpinBox->value());
    settings.setValue(AUTOINC_FILENAME, this->autoincrementFileNameButton->isChecked());
    settings.endGroup();
}

void DataSavingWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit uncheckSignal();
}

void DataSavingWidget::autosaveCheckboxChecked(bool on) {
    if(on) {
       state->skeletonState->autoSaveBool = true;
       state->skeletonState->autoSaveInterval = autosaveIntervalSpinBox->value();
    } else {
        state->skeletonState->autoSaveBool = false;
    }
}

void DataSavingWidget::autonincrementFileNameButtonPushed(bool on) {
    state->skeletonState->autoFilenameIncrementBool = on;
}

void DataSavingWidget::autosaveIntervalChanged(int value) {
    state->skeletonState->autoSaveInterval = value;
}

