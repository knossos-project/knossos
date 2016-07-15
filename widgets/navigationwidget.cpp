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

#include "navigationwidget.h"

#include "GuiConstants.h"
#include "session.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QSpacerItem>
#include <QSettings>
#include <QGroupBox>
#include <QApplication>
#include <QDesktopWidget>

NavigationWidget::NavigationWidget(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Dataset Navigation");
    QVBoxLayout *mainLayout = new QVBoxLayout();

    QGroupBox *movementAreaGroup = new QGroupBox("Movement Area");
    QVBoxLayout *movementAreaLayout = new QVBoxLayout();
    QHBoxLayout *areaMinLayout = new QHBoxLayout();
    QHBoxLayout *areaMaxLayout = new QHBoxLayout();

    xMinField.setRange(1, 1000000); yMinField.setRange(1, 1000000); zMinField.setRange(1, 1000000);
    xMaxField.setRange(1, 1000000); yMaxField.setRange(1, 1000000); zMaxField.setRange(1, 1000000);
    areaMinLayout->addWidget(new QLabel("Movement area min:"));
    areaMinLayout->addWidget(new QLabel("x"));
    areaMinLayout->addWidget(&xMinField);
    areaMinLayout->addWidget(new QLabel("y"));
    areaMinLayout->addWidget(&yMinField);
    areaMinLayout->addWidget(new QLabel("z"));
    areaMinLayout->addWidget(&zMinField);
    areaMaxLayout->addWidget(new QLabel("Movement area max:"));
    areaMaxLayout->addWidget(new QLabel("x"));
    areaMaxLayout->addWidget(&xMaxField);
    areaMaxLayout->addWidget(new QLabel("y"));
    areaMaxLayout->addWidget(&yMaxField);
    areaMaxLayout->addWidget(new QLabel("z"));
    areaMaxLayout->addWidget(&zMaxField);
    movementAreaLayout->addLayout(areaMinLayout);
    movementAreaLayout->addLayout(areaMaxLayout);
    resetMovementAreaButton = new QPushButton("Reset");
    movementAreaLayout->addWidget(resetMovementAreaButton);
    movementAreaGroup->setLayout(movementAreaLayout);
    mainLayout->addWidget(movementAreaGroup);

    QGroupBox *generalGroup = new QGroupBox("General");
    QFormLayout *formLayout = new QFormLayout();

    movementSpeedLabel = new QLabel("Movement Speed [Frames/s]:");
    jumpFramesLabel = new QLabel("Jump Frames (D, F)");
    walkFramesLabel = new QLabel("Walk Frames (E, R)");

    movementSpeedSpinBox = new QSpinBox();
    movementSpeedSpinBox->setRange(1, 1000);
    jumpFramesSpinBox = new QSpinBox;
    jumpFramesSpinBox->setMaximum(1000);
    walkFramesSpinBox = new QSpinBox;
    walkFramesSpinBox->setMaximum(1000);

    formLayout->addRow(movementSpeedLabel, movementSpeedSpinBox);
    formLayout->addRow(jumpFramesLabel, jumpFramesSpinBox);
    formLayout->addRow(walkFramesLabel, walkFramesSpinBox);

    generalGroup->setLayout(formLayout);
    mainLayout->addWidget(generalGroup);

    QGroupBox *advanceGroup = new QGroupBox("Advance Tracing Modes", this);
    mainLayout->addSpacing(10);

    normalModeButton = new QRadioButton("Normal Mode (Recentering)");
    additionalViewportDirectionMoveButton = new QRadioButton("Additional Viewport Direction Move");
    additionalTracingDirectionMoveButton = new QRadioButton("Additional Tracing Direction Move");
    additionalMirroredMoveButton = new QRadioButton("Additional Mirrored Move");

    QVBoxLayout *layout = new QVBoxLayout();

    layout->addWidget(normalModeButton);
    layout->addWidget(&noRecenteringButton);
    layout->addWidget(this->additionalViewportDirectionMoveButton);
    layout->addWidget(this->additionalTracingDirectionMoveButton);
    layout->addWidget(this->additionalMirroredMoveButton);

    advanceGroup->setLayout(layout);

    QFormLayout *restLayout = new QFormLayout();

    this->delayTimePerStepLabel = new QLabel("Delay Time Per Step [ms]");
    this->delayTimePerStepSpinBox = new QSpinBox();

    this->numberOfStepsLabel = new QLabel("Number Of Steps");
    this->numberOfStepsSpinBox = new QSpinBox();

    restLayout->addRow(delayTimePerStepLabel, delayTimePerStepSpinBox);
    restLayout->addRow(numberOfStepsLabel, numberOfStepsSpinBox);

    layout->addLayout(restLayout);
    mainLayout->addWidget(advanceGroup);
    setLayout(mainLayout);

    QObject::connect(&xMinField, &QSpinBox::editingFinished, this, &NavigationWidget::updateMovementArea);
    QObject::connect(&yMinField, &QSpinBox::editingFinished, this, &NavigationWidget::updateMovementArea);
    QObject::connect(&zMinField, &QSpinBox::editingFinished, this, &NavigationWidget::updateMovementArea);
    QObject::connect(&xMaxField, &QSpinBox::editingFinished, this, &NavigationWidget::updateMovementArea);
    QObject::connect(&yMaxField, &QSpinBox::editingFinished, this, &NavigationWidget::updateMovementArea);
    QObject::connect(&zMaxField, &QSpinBox::editingFinished, this, &NavigationWidget::updateMovementArea);
    QObject::connect(resetMovementAreaButton, &QPushButton::clicked, [this]() {
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

    connect(movementSpeedSpinBox, SIGNAL(valueChanged(int)), this, SLOT(movementSpeedChanged(int)));
    connect(jumpFramesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(jumpFramesChanged(int)));
    connect(this->walkFramesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(walkFramesChanged(int)));

    connect(normalModeButton, SIGNAL(clicked(bool)), this, SLOT(normalModeSelected(bool)));
    QObject::connect(&noRecenteringButton, &QRadioButton::clicked, [](const bool checked){
        if (checked) {
            state->viewerState->autoTracingMode = navigationMode::noRecentering;
        }
    });
    connect(additionalViewportDirectionMoveButton, SIGNAL(clicked(bool)), this, SLOT(additionalViewportDirectionMoveSelected(bool)));
    connect(additionalTracingDirectionMoveButton, SIGNAL(clicked(bool)), this, SLOT(additionalTracingDirectionMoveSelected(bool)));
    connect(additionalMirroredMoveButton, SIGNAL(clicked(bool)), this, SLOT(additionalMirroredMoveSelected(bool)));

    connect(this->delayTimePerStepSpinBox, SIGNAL(valueChanged(int)), this, SLOT(delayTimePerStepChanged(int)));
    connect(this->numberOfStepsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(numberOfStepsChanged(int)));

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void NavigationWidget::updateMovementArea() {
    const Coordinate min{xMinField.value()-1, yMinField.value()-1, zMinField.value()-1};
    const Coordinate max{xMaxField.value()-1, yMaxField.value()-1, zMaxField.value()-1};
    Session::singleton().updateMovementArea(min, max);
}

void NavigationWidget::movementSpeedChanged(int value) {
    state->viewerState->movementSpeed = value;
}

void NavigationWidget::jumpFramesChanged(int value) {
    state->viewerState->dropFrames = value;
}

void NavigationWidget::walkFramesChanged(int value) {
    state->viewerState->walkFrames = value;
}

void NavigationWidget::normalModeSelected(bool on) {
    if(on) {
        state->viewerState->autoTracingMode = navigationMode::recenter;
    }
}

void NavigationWidget::additionalViewportDirectionMoveSelected(bool on) {
    if(on) {
        state->viewerState->autoTracingMode = navigationMode::additionalVPMove;
    }
}

void NavigationWidget::additionalTracingDirectionMoveSelected(bool on) {
    if(on) {
        state->viewerState->autoTracingMode = navigationMode::additionalTracingDirectionMove;
    }
}

void NavigationWidget::additionalMirroredMoveSelected(bool on) {
    if(on) {
        state->viewerState->autoTracingMode = navigationMode::additionalMirroredMove;
    }
}

void NavigationWidget::delayTimePerStepChanged(int value) {
    state->viewerState->autoTracingDelay = value;
}

void NavigationWidget::numberOfStepsChanged(int value) {
    state->viewerState->autoTracingSteps = value;
}

void NavigationWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(NAVIGATION_WIDGET);
    restoreGeometry(settings.value(GEOMETRY).toByteArray());

    Session::singleton().resetMovementArea();

    movementSpeedSpinBox->setValue(settings.value(MOVEMENT_SPEED, 100).toInt());
    jumpFramesSpinBox->setValue(settings.value(JUMP_FRAMES, 1).toInt());
    walkFramesSpinBox->setValue(settings.value(WALK_FRAMES, 10).toInt());
    normalModeButton->setChecked(settings.value(NORMAL_MODE, true).toBool());
    additionalViewportDirectionMoveButton->setChecked(settings.value(ADDITIONAL_VIEWPORT_DIRECTION_MOVE, false).toBool());
    additionalTracingDirectionMoveButton->setChecked(settings.value(ADDITIONAL_TRACING_DIRECTION_MOVE, false).toBool());
    additionalMirroredMoveButton->setChecked(settings.value(ADDITIONAL_MIRRORED_MOVE, false).toBool());
    delayTimePerStepSpinBox->setValue(settings.value(DELAY_TIME_PER_STEP, 50).toInt());
    numberOfStepsSpinBox->setValue(settings.value(NUMBER_OF_STEPS, 10).toInt());
    settings.endGroup();
}

void NavigationWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(NAVIGATION_WIDGET);
    settings.setValue(GEOMETRY, saveGeometry());
    settings.setValue(VISIBLE, isVisible());

    settings.setValue(MOVEMENT_SPEED, movementSpeedSpinBox->value());
    settings.setValue(JUMP_FRAMES, jumpFramesSpinBox->value());
    settings.setValue(WALK_FRAMES, walkFramesSpinBox->value());
    settings.setValue(NORMAL_MODE, normalModeButton->isChecked());
    settings.setValue(ADDITIONAL_VIEWPORT_DIRECTION_MOVE, additionalViewportDirectionMoveButton->isChecked());
    settings.setValue(ADDITIONAL_TRACING_DIRECTION_MOVE, additionalTracingDirectionMoveButton->isChecked());
    settings.setValue(ADDITIONAL_MIRRORED_MOVE, additionalMirroredMoveButton->isChecked());
    settings.setValue(DELAY_TIME_PER_STEP, delayTimePerStepSpinBox->value());
    settings.setValue(NUMBER_OF_STEPS, numberOfStepsSpinBox->value());
    settings.endGroup();
}
