/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "datasavingwidget.h"

#include "viewer.h"
#include "mainwindow.h"

#include "GuiConstants.h"
#include "skeleton/skeletonizer.h"

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

DataSavingWidget::DataSavingWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Data Saving Options");
    QVBoxLayout *mainLayout = new QVBoxLayout();

    autosaveCheckbox = new QCheckBox("Auto-Saving (triggered by changes)");
    autosaveIntervalLabel = new QLabel("Saving interval [min]");
    autosaveIntervalSpinBox = new QSpinBox();
    autosaveIntervalSpinBox->setMinimum(1);
    autosaveIntervalSpinBox->setValue(5);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(autosaveIntervalLabel, autosaveIntervalSpinBox);

    autoincrementFileNameButton = new QCheckBox("Autoincrement File Name");

    mainLayout->addWidget(autosaveCheckbox);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(autoincrementFileNameButton);
    mainLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(mainLayout);

    QObject::connect(autoincrementFileNameButton, &QCheckBox::stateChanged, [](const bool on) { Session::singleton().autoFilenameIncrementBool = on; });
    QObject::connect(autosaveIntervalSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int value) {
        if (autosaveCheckbox->isChecked()) {
            Session::singleton().autoSaveTimer.start(value * 60 * 1000);
        }
    });
    QObject::connect(autosaveCheckbox, &QCheckBox::stateChanged, [this](const bool on) {
        if (on) {
            Session::singleton().autoSaveTimer.start(autosaveIntervalSpinBox->value() * 60 * 1000);
        }
        else {
            Session::singleton().autoSaveTimer.stop();
        }
        state->viewer->window->updateTitlebar();
    });

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}


void DataSavingWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(DATA_SAVING_WIDGET);
    width = (settings.value(WIDTH).isNull())? this->width() : settings.value(WIDTH).toInt();
    height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
    if(settings.value(POS_X).isNull() || settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().center().x();
        y = QApplication::desktop()->screen()->rect().topRight().y() + 50;
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }
    visible = (settings.value(VISIBLE).isNull())? false : settings.value(VISIBLE).toBool();

    autosaveIntervalSpinBox->setValue(settings.value(SAVING_INTERVAL, 5).toInt());
    autoincrementFileNameButton->setChecked(settings.value(AUTOINC_FILENAME, true).toBool());
    autosaveCheckbox->setChecked(settings.value(AUTO_SAVING, true).toBool()); // load this checkbox's state last to use loaded autosave settings in its slot

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

void DataSavingWidget::closeEvent(QCloseEvent */*event*/) {
    this->hide();
    emit uncheckSignal();
}
