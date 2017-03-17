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

#include "navigationtab.h"

#include "session.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/GuiConstants.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpacerItem>

NavigationTab::NavigationTab(QWidget *parent) : QWidget(parent) {
    for (auto * editPtr : {&xMinField, &yMinField, &zMinField, &xMaxField, &yMaxField, &zMaxField}) {
//        edit->setSuffix(" px");// uses too much space for now
        editPtr->setRange(1, 1000000);
    }
    xMinField.setPrefix("x: "); xMaxField.setPrefix("x: ");
    yMinField.setPrefix("y: "); yMaxField.setPrefix("y: ");
    zMinField.setPrefix("z: "); zMaxField.setPrefix("z: ");
    outVisibilitySlider.setOrientation(Qt::Horizontal);
    auto row = 0;
    movementAreaLayout.addWidget(&minLabel, row++, 0, 1, 3);
    movementAreaLayout.addWidget(&xMinField, row, 0);
    movementAreaLayout.addWidget(&yMinField, row, 1);
    movementAreaLayout.addWidget(&zMinField, row++, 2);
    movementAreaLayout.addWidget(&maxLabel, row++, 0);
    movementAreaLayout.addWidget(&xMaxField, row, 0);
    movementAreaLayout.addWidget(&yMaxField, row, 1);
    movementAreaLayout.addWidget(&zMaxField, row++, 2);
    resetMovementAreaButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    movementAreaLayout.addWidget(&resetMovementAreaButton, row++, 0, 1, 3);
    separator.setFrameShape(QFrame::HLine);
    separator.setFrameShadow(QFrame::Sunken);
    separator.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    movementAreaLayout.addWidget(&separator, row++, 0, 1, 3);
    movementAreaLayout.addWidget(&outVisibilityLabel, row++, 0, 1, 3);
    movementAreaLayout.addWidget(&outVisibilitySlider, row, 0, 1, 2);
    movementAreaLayout.addWidget(&outVisibilitySpin, row++, 2);
    movementAreaGroup.setLayout(&movementAreaLayout);

    movementSpeedSpinBox.setRange(1, 10000);
    movementSpeedSpinBox.setSuffix(" Slices/s");
    jumpFramesSpinBox.setRange(1, 1000);
    walkFramesSpinBox.setRange(2, 1000);
    keyboardMovementLayout.addRow("Movement Speed", &movementSpeedSpinBox);
    keyboardMovementLayout.addRow("Jump Frames (D, F)", &jumpFramesSpinBox);
    keyboardMovementLayout.addRow("Walk Frames (E, R)", &walkFramesSpinBox);
    keyboardMovementGroup.setLayout(&keyboardMovementLayout);

    penModeCheckBox.setText("Pen mode");
    penModeCheckBox.setToolTip("Swap mouse buttons in the viewports for a more comfortable use of a pen device");
    penModeCheckBox.setCheckable(true);

    mouseBehaviourLayout.addRow(&penModeCheckBox);
    mouseBehaviourGroup.setLayout(&mouseBehaviourLayout);

    QObject::connect(&penModeCheckBox, &QCheckBox::clicked, [this]() {
        state->viewerState->penmode = penModeCheckBox.isChecked();
    });

    numberOfStepsSpinBox.setRange(1, 100);
    numberOfStepsSpinBox.setSuffix(" Steps");
    numberOfStepsSpinBox.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    recenteringButtonGroup.addButton(&recenteringButton, static_cast<int>(Recentering::OnNode));
    advancedFormLayout.addRow(&recenteringButton);
    recenteringButtonGroup.addButton(&noRecenteringButton, static_cast<int>(Recentering::Off));
    advancedFormLayout.addRow(&noRecenteringButton);
    recenteringButtonGroup.addButton(&additionalTracingDirectionMoveButton, static_cast<int>(Recentering::AheadOfNode));
    advancedFormLayout.addRow(&additionalTracingDirectionMoveButton, &numberOfStepsSpinBox);
    advancedGroup.setLayout(&advancedFormLayout);

    upperLayout.addWidget(&movementAreaGroup);

    rightupperLayout.addWidget(&keyboardMovementGroup);
    rightupperLayout.addWidget(&mouseBehaviourGroup);
    upperLayout.addLayout(&rightupperLayout);

    mainLayout.addLayout(&upperLayout);
    mainLayout.addWidget(&advancedGroup);
    setLayout(&mainLayout);

    QObject::connect(&xMinField, &QSpinBox::editingFinished, this, &NavigationTab::updateMovementArea);
    QObject::connect(&yMinField, &QSpinBox::editingFinished, this, &NavigationTab::updateMovementArea);
    QObject::connect(&zMinField, &QSpinBox::editingFinished, this, &NavigationTab::updateMovementArea);
    QObject::connect(&xMaxField, &QSpinBox::editingFinished, this, &NavigationTab::updateMovementArea);
    QObject::connect(&yMaxField, &QSpinBox::editingFinished, this, &NavigationTab::updateMovementArea);
    QObject::connect(&zMaxField, &QSpinBox::editingFinished, this, &NavigationTab::updateMovementArea);
    QObject::connect(&resetMovementAreaButton, &QPushButton::clicked, [this]() {
        Session::singleton().resetMovementArea();
    });
    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, [this]() {
        auto & session = Session::singleton();
        xMinField.setValue(session.movementAreaMin.x + 1);
        yMinField.setValue(session.movementAreaMin.y + 1);
        zMinField.setValue(session.movementAreaMin.z + 1);
        xMaxField.setValue(session.movementAreaMax.x + 1);
        yMaxField.setValue(session.movementAreaMax.y + 1);
        zMaxField.setValue(session.movementAreaMax.z + 1);
    });

    QObject::connect(&outVisibilitySlider, &QSlider::valueChanged, [this](const int value) {
        QSignalBlocker blocker{outVisibilitySpin};
        outVisibilitySpin.setValue(value);
        state->viewer->setMovementAreaFactor(value);
    });

    QObject::connect(&outVisibilitySpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int value) {
        QSignalBlocker blocker{outVisibilitySlider};
        outVisibilitySlider.setValue(value);
        state->viewer->setMovementAreaFactor(value);
    });

    QObject::connect(&movementSpeedSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](int value){ state->viewerState->movementSpeed = value; });
    QObject::connect(&jumpFramesSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](int value){ state->viewerState->dropFrames = value; });
    QObject::connect(&walkFramesSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](int value){ state->viewerState->walkFrames = value; });

    QObject::connect(&recenteringButtonGroup, static_cast<void(QButtonGroup::*)(int id)>(&QButtonGroup::buttonClicked), [this](auto id) {
        state->viewerState->autoTracingMode = static_cast<Recentering>(id);
        numberOfStepsSpinBox.setEnabled(id == static_cast<int>(Recentering::AheadOfNode));
    });
    QObject::connect(&numberOfStepsSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](int value){ state->viewerState->autoTracingSteps = value; });

    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void NavigationTab::updateMovementArea() {
    const Coordinate min{xMinField.value()-1, yMinField.value()-1, zMinField.value()-1};
    const Coordinate max{xMaxField.value()-1, yMaxField.value()-1, zMaxField.value()-1};
    Session::singleton().updateMovementArea(min, max);
}

void NavigationTab::loadSettings(const QSettings & settings) {
    Session::singleton().resetMovementArea();

    const auto visibility = settings.value(OUTSIDE_AREA_VISIBILITY, 80).toInt();
    outVisibilitySlider.setValue(visibility);
    outVisibilitySlider.valueChanged(visibility);
    movementSpeedSpinBox.setValue(settings.value(MOVEMENT_SPEED, 100).toInt());
    jumpFramesSpinBox.setValue(settings.value(JUMP_FRAMES, 1).toInt());
    walkFramesSpinBox.setValue(settings.value(WALK_FRAMES, 10).toInt());
    const auto buttonId = settings.value(RECENTERING, static_cast<int>(Recentering::OnNode)).toInt();
    recenteringButtonGroup.button(buttonId)->setChecked(true);
    recenteringButtonGroup.buttonClicked(buttonId);
    numberOfStepsSpinBox.setValue(settings.value(NUMBER_OF_STEPS, 10).toInt());
}

void NavigationTab::saveSettings(QSettings & settings) {
    settings.setValue(OUTSIDE_AREA_VISIBILITY, outVisibilitySlider.value());
    settings.setValue(MOVEMENT_SPEED, movementSpeedSpinBox.value());
    settings.setValue(JUMP_FRAMES, jumpFramesSpinBox.value());
    settings.setValue(WALK_FRAMES, walkFramesSpinBox.value());
    settings.setValue(RECENTERING, recenteringButtonGroup.checkedId());
    settings.setValue(NUMBER_OF_STEPS, numberOfStepsSpinBox.value());
}
