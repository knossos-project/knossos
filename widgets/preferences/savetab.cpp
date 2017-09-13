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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "savetab.h"

#include "file_io.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/GuiConstants.h"
#include "widgets/mainwindow.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

SaveTab::SaveTab(QWidget * parent) : QWidget(parent, Qt::WindowFlags() & ~Qt::WindowContextHelpButtonHint) {
    autosaveIntervalSpinBox.setMinimum(1);
    autosaveIntervalSpinBox.setSuffix(" min");
    const auto filePath = QFileInfo(annotationFileDefaultPath()).dir().absolutePath();
    autosaveLocationLabel.setOpenExternalLinks(true);
    autosaveLocationLabel.setText(QObject::tr("<a href=\"%1\">%2</a>").arg(QUrl::fromLocalFile(filePath).toString()).arg(filePath));
    autosaveLocationLabel.setTextInteractionFlags(Qt::TextBrowserInteraction);
    autosaveLocationLabel.setWordWrap(true);

    generalLayout.addWidget(&autoincrementFileNameButton);
    locationFormLayout.addRow("Default location: ", &autosaveLocationLabel);
    generalLayout.addLayout(&locationFormLayout);
    generalGroup.setLayout(&generalLayout);

    autosaveGroup.setCheckable(true);
    formLayout.addRow("Saving interval", &autosaveIntervalSpinBox);
    formLayout.setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    autosaveGroup.setLayout(&formLayout);

    plySaveButtonGroup.addButton(&plySaveAsBinRadio, true);
    plySaveButtonGroup.addButton(&plySaveAsTxtRadio, false);
    plyLayout.addWidget(&plySaveAsBinRadio);
    plyLayout.addWidget(&plySaveAsTxtRadio);
    plyLayout.setAlignment(Qt::AlignLeft);
    plyGroupBox.setLayout(&plyLayout);

    mainLayout.addWidget(&generalGroup);
    mainLayout.addWidget(&autosaveGroup);
    mainLayout.addWidget(&plyGroupBox);
    mainLayout.addStretch();
    setLayout(&mainLayout);

    QObject::connect(&autoincrementFileNameButton, &QCheckBox::stateChanged, [](const bool on) {
        Session::singleton().autoFilenameIncrementBool = on;
    });
    QObject::connect(&autosaveIntervalSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int value) {
        if (autosaveGroup.isChecked()) {
            Session::singleton().autoSaveTimer.start(value * 60 * 1000);
        }
    });
    QObject::connect(&autosaveGroup, &QGroupBox::toggled, [this](const bool on) {
        if (on) {
            Session::singleton().autoSaveTimer.start(autosaveIntervalSpinBox.value() * 60 * 1000);
        } else {
            Session::singleton().autoSaveTimer.stop();
        }
        state->viewer->window->updateTitlebar();
    });
    QObject::connect(&plySaveButtonGroup, static_cast<void(QButtonGroup::*)(int id)>(&QButtonGroup::buttonClicked), [this](auto id) {
        Session::singleton().savePlyAsBinary = static_cast<bool>(id);
    });
}

void SaveTab::loadSettings(const QSettings & settings) {
    autoincrementFileNameButton.setChecked(settings.value(AUTOINC_FILENAME, true).toBool());
    autoincrementFileNameButton.stateChanged(autoincrementFileNameButton.checkState());

    // autosaveGroup.toggled will handle the autosave timer and its time (therefore load the time first)
    autosaveIntervalSpinBox.setValue(settings.value(SAVING_INTERVAL, 5).toInt());
    autosaveGroup.setChecked(settings.value(AUTO_SAVING, true).toBool());

    const auto buttonId = static_cast<int>(settings.value(PLY_SAVE_AS_BIN, true).toBool());
    plySaveButtonGroup.button(buttonId)->setChecked(true);
    plySaveButtonGroup.buttonClicked(buttonId);
}

void SaveTab::saveSettings(QSettings & settings) {
    settings.setValue(AUTOINC_FILENAME, autoincrementFileNameButton.isChecked());
    settings.setValue(AUTO_SAVING, autosaveGroup.isChecked());
    settings.setValue(SAVING_INTERVAL, autosaveIntervalSpinBox.value());
    settings.setValue(PLY_SAVE_AS_BIN, plySaveAsBinRadio.isChecked());
}
