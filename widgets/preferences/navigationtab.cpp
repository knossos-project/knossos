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

#include "navigationtab.h"

#include "session.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/GuiConstants.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QSettings>
#include <QSpacerItem>

NavigationTab::NavigationTab(QWidget *parent) : QWidget(parent) {
    for (auto * editPtr : {&xMinField, &yMinField, &zMinField, &xMaxField, &yMaxField, &zMaxField}) {
//        edit->setSuffix(" px");// uses too much space for now
        editPtr->setRange(1, 1000000);
    }
    xMinField.setPrefix("x: "); xMaxField.setPrefix("x: ");
    yMinField.setPrefix("y: "); yMaxField.setPrefix("y: ");
    zMinField.setPrefix("z: "); zMaxField.setPrefix("z: ");
    areaMinLayout.addWidget(new QLabel("Min"));
    areaMinLayout.addWidget(&xMinField);
    areaMinLayout.addWidget(&yMinField);
    areaMinLayout.addWidget(&zMinField);
    areaMaxLayout.addWidget(new QLabel("Max"));
    areaMaxLayout.addWidget(&xMaxField);
    areaMaxLayout.addWidget(&yMaxField);
    areaMaxLayout.addWidget(&zMaxField);
    movementAreaLayout.addLayout(&areaMinLayout);
    movementAreaLayout.addLayout(&areaMaxLayout);
    resetMovementAreaButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    movementAreaLayout.addWidget(&resetMovementAreaButton);
    movementAreaGroup.setLayout(&movementAreaLayout);

    movementSpeedSpinBox.setRange(1, 1000);
    movementSpeedSpinBox.setSuffix(" Slices/s");
    jumpFramesSpinBox.setMaximum(1000);
    walkFramesSpinBox.setMaximum(1000);
    keyboardMovementLayout.addRow("Movement Speed", &movementSpeedSpinBox);
    keyboardMovementLayout.addRow("Jump Frames (D, F)", &jumpFramesSpinBox);
    keyboardMovementLayout.addRow("Walk Frames (E, R)", &walkFramesSpinBox);
    keyboardMovementGroup.setLayout(&keyboardMovementLayout);

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
    upperLayout.addWidget(&keyboardMovementGroup);
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

    movementSpeedSpinBox.setValue(settings.value(MOVEMENT_SPEED, 100).toInt());
    jumpFramesSpinBox.setValue(settings.value(JUMP_FRAMES, 1).toInt());
    walkFramesSpinBox.setValue(settings.value(WALK_FRAMES, 10).toInt());
    const auto buttonId = settings.value(RECENTERING, static_cast<int>(Recentering::OnNode)).toInt();
    recenteringButtonGroup.button(buttonId)->setChecked(true);
    recenteringButtonGroup.buttonClicked(buttonId);
    numberOfStepsSpinBox.setValue(settings.value(NUMBER_OF_STEPS, 10).toInt());
}

void NavigationTab::saveSettings(QSettings & settings) {
    settings.setValue(MOVEMENT_SPEED, movementSpeedSpinBox.value());
    settings.setValue(JUMP_FRAMES, jumpFramesSpinBox.value());
    settings.setValue(WALK_FRAMES, walkFramesSpinBox.value());
    settings.setValue(RECENTERING, recenteringButtonGroup.checkedId());
    settings.setValue(NUMBER_OF_STEPS, numberOfStepsSpinBox.value());
}
