/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
#include <QClipboard>
#include <QDesktopWidget>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpacerItem>

NavigationTab::NavigationTab(QWidget *parent) : QWidget(parent) {
    sizeLabel.setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    outVisibilitySlider.setOrientation(Qt::Horizontal);

    minAreaHeadLayout.setAlignment(Qt::AlignLeft);
    minAreaHeadLayout.addWidget(&minLabel);
    minAreaHeadLayout.addWidget(&minSpins.copyButton);
    minAreaHeadLayout.addWidget(&minSpins.pasteButton);
    movementAreaLayout.addLayout(&minAreaHeadLayout);
    minAreaSpinsLayout.addWidget(&minSpins.xSpin);
    minAreaSpinsLayout.addWidget(&minSpins.ySpin);
    minAreaSpinsLayout.addWidget(&minSpins.zSpin);
    movementAreaLayout.addLayout(&minAreaSpinsLayout);
    maxAreaHeadLayout.setAlignment(Qt::AlignLeft);
    maxAreaHeadLayout.addWidget(&maxLabel);
    maxAreaHeadLayout.addWidget(&maxSpins.copyButton);
    maxAreaHeadLayout.addWidget(&maxSpins.pasteButton);
    movementAreaLayout.addLayout(&maxAreaHeadLayout);
    maxAreaSpinsLayout.addWidget(&maxSpins.xSpin);
    maxAreaSpinsLayout.addWidget(&maxSpins.ySpin);
    maxAreaSpinsLayout.addWidget(&maxSpins.zSpin);
    movementAreaLayout.addLayout(&maxAreaSpinsLayout);
    movementAreaBottomLayout.addWidget(&resetMovementAreaButton);
    movementAreaBottomLayout.addWidget(&sizeLabel, Qt::AlignRight);
    movementAreaLayout.addLayout(&movementAreaBottomLayout);
    separator.setFrameShape(QFrame::HLine);
    separator.setFrameShadow(QFrame::Sunken);
    separator.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    movementAreaLayout.addWidget(&separator);
    movementAreaLayout.addWidget(&outVisibilityLabel);
    outVisibilitySlider.setRange(0, 100);
    outVisibilitySpin.setRange(0, 100);
    outVisibilityAdjustLayout.addWidget(&outVisibilitySlider);
    outVisibilityAdjustLayout.addWidget(&outVisibilitySpin);
    movementAreaLayout.addLayout(&outVisibilityAdjustLayout);
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
    QObject::connect(&minSpins, &CoordinateSpins::coordinatesChanged, this, &NavigationTab::updateMovementArea);
    QObject::connect(&maxSpins, &CoordinateSpins::coordinatesChanged, this, &NavigationTab::updateMovementArea);

    QObject::connect(&resetMovementAreaButton, &QPushButton::clicked, []() {
        Session::singleton().resetMovementArea();
    });
    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, [this]() {
        auto & session = Session::singleton();
        minSpins.set(session.movementAreaMin + 1);
        maxSpins.set(session.movementAreaMax + 1);
        const auto size = session.movementAreaMax - session.movementAreaMin;
        sizeLabel.setText(tr("Size: %1, %2, %3").arg(size.x).arg(size.y).arg(size.z));
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
}

void NavigationTab::updateMovementArea() {
    Session::singleton().updateMovementArea(minSpins.get() - 1, maxSpins.get() - 1);
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
