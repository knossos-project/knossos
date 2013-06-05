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

#include "navigationwidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QSpacerItem>
#include <QSettings>
#include "GUIConstants.h"
#include "knossos-global.h"

extern struct stateInfo *state;
extern struct stateInfo *tempConfig;

NavigationWidget::NavigationWidget(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Navigation Settings");
    QVBoxLayout *mainLayout = new QVBoxLayout();

    // General section
    generalLabel = new QLabel("General");

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(generalLabel);
    mainLayout->addWidget(line);

    QFormLayout *formLayout = new QFormLayout();

    movementSpeedLabel = new QLabel("Movement Speed:");
    jumpFramesLabel = new QLabel("Jump Frames");
    recenterTimeParallelLabel = new QLabel("Recenter Time parallel [ms]:");
    recenterTimeOrthoLabel = new QLabel("Recenter Time Orthogonal [ms]:");

    movementSpeedSpinBox = new QSpinBox();
    jumpFramesSpinBox = new QSpinBox;
    recenterTimeParallelSpinBox = new QSpinBox();
    recenterTimeOrthoSpinBox = new QSpinBox();

    formLayout->addRow(movementSpeedLabel, movementSpeedSpinBox);
    formLayout->addRow(jumpFramesLabel, jumpFramesSpinBox);
    formLayout->addRow(recenterTimeParallelLabel, recenterTimeParallelSpinBox);
    formLayout->addRow(recenterTimeOrthoLabel, recenterTimeOrthoSpinBox);

    this->advanceTracingModesLabel = new QLabel("Advance Tracing Modes");

    mainLayout->addLayout(formLayout);

    mainLayout->addSpacing(10);

    // Advance Tracing Modes section
    mainLayout->addWidget(advanceTracingModesLabel);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line2);

    normalModeButton = new QRadioButton("Normal Mode");
    additionalViewportDirectionMoveButton = new QRadioButton("Additional Viewport Direction Move");
    additionalTracingDirectionMoveButton = new QRadioButton("Additional Tracing Direction Move");
    additionalMirroredMoveButton = new QRadioButton("Additional Mirrored Move");

    mainLayout->addWidget(normalModeButton);
    mainLayout->addWidget(this->additionalViewportDirectionMoveButton);
    mainLayout->addWidget(this->additionalTracingDirectionMoveButton);
    mainLayout->addWidget(this->additionalMirroredMoveButton);

    QFormLayout *restLayout = new QFormLayout();

    this->delayTimePerStepLabel = new QLabel("Delay Time Per Step [ms]");
    this->delayTimePerStepSpinBox = new QSpinBox();

    this->numberOfStepsLabel = new QLabel("Number Of Steps");
    this->numberOfStepsSpinBox = new QSpinBox();

    restLayout->addRow(delayTimePerStepLabel, delayTimePerStepSpinBox);
    restLayout->addRow(numberOfStepsLabel, numberOfStepsSpinBox);

    mainLayout->addLayout(restLayout);
    this->setLayout(mainLayout);

    connect(movementSpeedSpinBox, SIGNAL(valueChanged(int)), this, SLOT(movementSpeedChanged(int)));
    connect(jumpFramesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(jumpFramesChanged(int)));
    connect(recenterTimeParallelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(recenterTimeParallelChanged(int)));
    connect(recenterTimeOrthoSpinBox, SIGNAL(valueChanged(int)), this, SLOT(recenterTimeOrthoChanged(int)));

    connect(normalModeButton, SIGNAL(clicked(bool)), this, SLOT(normalModeButtonClicked(bool)));
    connect(additionalViewportDirectionMoveButton, SIGNAL(clicked(bool)), this, SLOT(additionalViewportDirectionMoveButtonClicked(bool)));
    connect(additionalTracingDirectionMoveButton, SIGNAL(clicked(bool)), this, SLOT(additionalTracingDirectionMoveButtonClicked(bool)));
    connect(additionalMirroredMoveButton, SIGNAL(clicked(bool)), this, SLOT(additionalMirroredMoveButtonClicked(bool)));

    connect(this->delayTimePerStepSpinBox, SIGNAL(valueChanged(int)), this, SLOT(delayTimePerStepChanged(int)));
    connect(this->numberOfStepsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(numberOfStepsChanged(int)));

    loadSettings();
}

void NavigationWidget::movementSpeedChanged(int value) {
    tempConfig->viewerState->stepsPerSec = value;
}

void NavigationWidget::jumpFramesChanged(int value) {
    tempConfig->viewerState->recenteringTimeOrth = value;
}

void NavigationWidget::recenterTimeOrthoChanged(int value) {
    tempConfig->viewerState->recenteringTimeOrth = value;
}

void NavigationWidget::recenterTimeParallelChanged(int value) {
    tempConfig->viewerState->recenteringTime = value;
}

void NavigationWidget::normalModeButtonClicked(bool on) {
    if(on) {
        state->viewerState->autoTracingMode = AUTOTRACING_MODE_NORMAL;
    }
}

void NavigationWidget::additionalViewportDirectionMoveButtonClicked(bool on) {
    if(on) {
        state->viewerState->autoTracingMode = AUTOTRACING_MODE_ADDITIONAL_VIEWPORT_DIRECTION_MOVE;
    }
}

void NavigationWidget::additionalTracingDirectionMoveButtonClicked(bool on) {
    if(on) {
        state->viewerState->autoTracingMode = AUTOTRACING_MODE_ADDITIONAL_TRACING_DIRECTION_MOVE;
    }
}

void NavigationWidget::additionalMirroredMoveButtonClicked(bool on) {
    if(on) {
        state->viewerState->autoTracingMode = AUTOTRACING_MODE_ADDITIONAL_MIRRORED_MOVE;
    }
}

void NavigationWidget::delayTimePerStepChanged(int value) {
    state->viewerState->autoTracingDelay = value;
}

void NavigationWidget::numberOfStepsChanged(int value) {
    state->viewerState->autoTracingSteps = value;
}

void NavigationWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit uncheckSignal();

}

void NavigationWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(NAVIGATION_WIDGET);
    width = settings.value(WIDTH).toInt();
    height = settings.value(HEIGHT).toInt();
    x = settings.value(POS_X).toInt();
    y = settings.value(POS_Y).toInt();
    visible = settings.value(VISIBLE).toBool();

    setGeometry(x, y, width, height);

    this->movementSpeedSpinBox->setValue(settings.value(MOVEMENT_SPEED).toInt());
    this->jumpFramesSpinBox->setValue(settings.value(JUMP_FRAMES).toInt());
    this->recenterTimeParallelSpinBox->setValue(settings.value(RECENTERING_TIME_PARALLEL).toInt());
    this->recenterTimeOrthoSpinBox->setValue(settings.value(RECENTERING_TIME_ORTHO).toInt());
    this->normalModeButton->setChecked(settings.value(NORMAL_MODE).toBool());
    this->additionalViewportDirectionMoveButton->setChecked(settings.value(ADDITIONAL_VIEWPORT_DIRECTION_MOVE).toBool());
    this->additionalTracingDirectionMoveButton->setChecked(settings.value(ADDITIONAL_TRACING_DIRECTION_MOVE).toBool());
    this->additionalMirroredMoveButton->setChecked(settings.value(ADDITIONAL_MIRRORED_MOVE).toBool());


    settings.endGroup();

}

void NavigationWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(NAVIGATION_WIDGET);
    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());
    settings.setValue(VISIBLE, this->isVisible());

    settings.setValue(MOVEMENT_SPEED, this->movementSpeedSpinBox->value());
    settings.setValue(JUMP_FRAMES, this->jumpFramesSpinBox->value());
    settings.setValue(RECENTERING_TIME_PARALLEL, this->recenterTimeParallelSpinBox->value());
    settings.setValue(RECENTERING_TIME_ORTHO, this->recenterTimeOrthoSpinBox->value());
    settings.setValue(NORMAL_MODE, this->normalModeButton->isChecked());
    settings.setValue(ADDITIONAL_VIEWPORT_DIRECTION_MOVE, this->additionalViewportDirectionMoveButton->isChecked());
    settings.setValue(ADDITIONAL_TRACING_DIRECTION_MOVE, this->additionalTracingDirectionMoveButton->isChecked());
    settings.setValue(ADDITIONAL_MIRRORED_MOVE, this->additionalMirroredMoveButton->isChecked());
    settings.setValue(DELAY_TIME_PER_STEP, this->delayTimePerStepSpinBox->value());
    settings.setValue(NUMBER_OF_STEPS, this->numberOfStepsSpinBox->value());
    settings.endGroup();


}
