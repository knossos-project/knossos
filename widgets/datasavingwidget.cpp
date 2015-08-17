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

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(autosaveIntervalLabel, autosaveIntervalSpinBox);

    autoincrementFileNameButton = new QCheckBox("Autoincrement File Name");

    mainLayout->addWidget(autosaveCheckbox);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(autoincrementFileNameButton);
    mainLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(mainLayout);

    QObject::connect(autosaveCheckbox, &QCheckBox::clicked, [](const bool on) { Session::singleton().autoFilenameIncrementBool = on; });
    QObject::connect(autosaveIntervalSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](const int value) { Session::singleton().autoSaveInterval = value; });
    QObject::connect(autoincrementFileNameButton, &QPushButton::clicked, [this](const bool on) {
        Session::singleton().autoSaveBool = on;
        if(on) {
           Session::singleton().autoSaveInterval = autosaveIntervalSpinBox->value();
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

    auto & session = Session::singleton();
    session.autoSaveBool = settings.value(AUTO_SAVING, true).toBool();
    autosaveCheckbox->setChecked(session.autoSaveBool);
    session.autoSaveInterval = settings.value(SAVING_INTERVAL, 5).toInt();
    autosaveIntervalSpinBox->setValue(session.autoSaveInterval);
    session.autoFilenameIncrementBool = settings.value(AUTOINC_FILENAME, true).toBool();
    autoincrementFileNameButton->setChecked(session.autoFilenameIncrementBool);

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
