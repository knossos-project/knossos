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

#include "GuiConstants.h"
#include "knossos-global.h"
#include "navigationwidget.h"
#include "session.h"

NavigationWidget::NavigationWidget(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Dataset Navigation");
    QVBoxLayout *mainLayout = new QVBoxLayout();

    QGroupBox *movementAreaGroup = new QGroupBox("Movement Area");
    QVBoxLayout *movementAreaLayout = new QVBoxLayout();
    QHBoxLayout *centerLayout = new QHBoxLayout();
    QHBoxLayout *rangeLayout = new QHBoxLayout();

    xCenterField.setRange(0, 1000000); yCenterField.setRange(0, 1000000); zCenterField.setRange(0, 1000000);
    xRangeField.setRange(0, 1000000); yRangeField.setRange(0, 1000000); zRangeField.setRange(0, 1000000);
    centerLayout->addWidget(new QLabel("Movement center:"));
    centerLayout->addWidget(new QLabel("x"));
    centerLayout->addWidget(&xCenterField);
    centerLayout->addWidget(new QLabel("y"));
    centerLayout->addWidget(&yCenterField);
    centerLayout->addWidget(new QLabel("z"));
    centerLayout->addWidget(&zCenterField);
    rangeLayout->addWidget(new QLabel("Movement range:"));
    rangeLayout->addWidget(new QLabel("x"));
    rangeLayout->addWidget(&xRangeField);
    rangeLayout->addWidget(new QLabel("y"));
    rangeLayout->addWidget(&yRangeField);
    rangeLayout->addWidget(new QLabel("z"));
    rangeLayout->addWidget(&zRangeField);
    movementAreaLayout->addLayout(centerLayout);
    movementAreaLayout->addLayout(rangeLayout);
    movementAreaGroup->setLayout(movementAreaLayout);
    mainLayout->addWidget(movementAreaGroup);

    QGroupBox *generalGroup = new QGroupBox("General");
    QFormLayout *formLayout = new QFormLayout();

    movementSpeedLabel = new QLabel("Movement Speed [Frames/s]:");
    jumpFramesLabel = new QLabel("Jump Frames (D, F)");
    walkFramesLabel = new QLabel("Walk Frames (E, R)");
    recenterTimeParallelLabel = new QLabel("Recenter Time parallel [ms]:");
    recenterTimeOrthoLabel = new QLabel("Recenter Time Orthogonal [ms]:");

    movementSpeedSpinBox = new QSpinBox();
    movementSpeedSpinBox->setRange(1, 1000);
    jumpFramesSpinBox = new QSpinBox;
    jumpFramesSpinBox->setMaximum(1000);
    walkFramesSpinBox = new QSpinBox;
    walkFramesSpinBox->setMaximum(1000);
    recenterTimeParallelSpinBox = new QSpinBox();
    recenterTimeParallelSpinBox->setMaximum(10000);
    recenterTimeOrthoSpinBox = new QSpinBox();
    recenterTimeOrthoSpinBox->setMaximum(10000);

    formLayout->addRow(movementSpeedLabel, movementSpeedSpinBox);
    formLayout->addRow(jumpFramesLabel, jumpFramesSpinBox);
    formLayout->addRow(walkFramesLabel, walkFramesSpinBox);
    formLayout->addRow(recenterTimeParallelLabel, recenterTimeParallelSpinBox);
    formLayout->addRow(recenterTimeOrthoLabel, recenterTimeOrthoSpinBox);

    generalGroup->setLayout(formLayout);
    mainLayout->addWidget(generalGroup);

    QGroupBox *advanceGroup = new QGroupBox("Advance Tracing Modes");
    mainLayout->addSpacing(10);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

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

    QObject::connect(&xCenterField, &QSpinBox::editingFinished, [this]() {
        xCenterField.setValue(std::min(xCenterField.value(), state->boundary.x + 1));
        Session::singleton().movementCenter.x = xCenterField.value() - 1;
        updateRangeField(xRangeField);
        emit movementAreaChanged();
    });
    QObject::connect(&yCenterField, &QSpinBox::editingFinished, [this]() {
        yCenterField.setValue(std::min(yCenterField.value(), state->boundary.y + 1));
        Session::singleton().movementCenter.y = yCenterField.value() - 1;
        updateRangeField(yRangeField);
        emit movementAreaChanged();
    });
    QObject::connect(&zCenterField, &QSpinBox::editingFinished, [this]() {
        zCenterField.setValue(std::min(zCenterField.value(), state->boundary.z + 1));
        Session::singleton().movementCenter.z = zCenterField.value() - 1;
        updateRangeField(zRangeField);
        emit movementAreaChanged();
    });
    QObject::connect(&xRangeField, &QSpinBox::editingFinished, [this]() { updateRangeField(xRangeField); });
    QObject::connect(&yRangeField, &QSpinBox::editingFinished, [this]() { updateRangeField(yRangeField); });
    QObject::connect(&zRangeField, &QSpinBox::editingFinished, [this]() { updateRangeField(zRangeField); });
    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, [this]() {
        auto & session = Session::singleton();
        xCenterField.setValue(session.movementCenter.x + 1);
        yCenterField.setValue(session.movementCenter.y + 1);
        zCenterField.setValue(session.movementCenter.z + 1);
        xRangeField.setValue(session.movementRange.x);
        yRangeField.setValue(session.movementRange.y);
        zRangeField.setValue(session.movementRange.z);
        emit sendLoadSignal(0);
    });

    connect(movementSpeedSpinBox, SIGNAL(valueChanged(int)), this, SLOT(movementSpeedChanged(int)));
    connect(jumpFramesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(jumpFramesChanged(int)));
    connect(this->walkFramesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(walkFramesChanged(int)));
    connect(recenterTimeParallelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(recenterTimeParallelChanged(int)));
    connect(recenterTimeOrthoSpinBox, SIGNAL(valueChanged(int)), this, SLOT(recenterTimeOrthoChanged(int)));

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

void NavigationWidget::updateRangeField(QSpinBox &box) {
    auto & session = Session::singleton();
    int *boundary, *movementCenter, *movementRange;
    if(&box == &xRangeField) {
        boundary = &state->boundary.x;
        movementCenter = &session.movementCenter.x;
        movementRange = &session.movementRange.x;
    }
    else if(&box == &yRangeField) {
        boundary = &state->boundary.y;
        movementCenter = &session.movementCenter.y;
        movementRange = &session.movementRange.y;
    }
    else if(&box == &zRangeField) {
        boundary = &state->boundary.z;
        movementCenter = &session.movementCenter.z;
        movementRange = &session.movementRange.z;
    }
    else {
        return;
    }
    box.setValue(std::min(box.value(), *boundary));
    *movementRange = box.value();
    emit sendLoadSignal(0);
    emit movementAreaChanged();
}

void NavigationWidget::resetMovementArea() {
    xCenterField.setValue(state->boundary.x/2 + 1); yCenterField.setValue(state->boundary.y/2 + 1); zCenterField.setValue(state->boundary.z/2 + 1);
    xRangeField.setValue(state->boundary.x/2); yRangeField.setValue(state->boundary.y/2); zRangeField.setValue(state->boundary.z/2);
    Session::singleton().movementCenter = { xCenterField.value() - 1, yCenterField.value() - 1, zCenterField.value() - 1 };
    Session::singleton().movementRange = { xRangeField.value(), yRangeField.value(), zRangeField.value() };
}

void NavigationWidget::movementSpeedChanged(int value) {
    state->viewerState->stepsPerSec = value;
}

void NavigationWidget::jumpFramesChanged(int value) {
    state->viewerState->dropFrames = value;
}

void NavigationWidget::walkFramesChanged(int value) {
    state->viewerState->walkFrames = value;
}

void NavigationWidget::recenterTimeOrthoChanged(int value) {
    state->viewerState->recenteringTimeOrth = value;
}

void NavigationWidget::recenterTimeParallelChanged(int value) {
    state->viewerState->recenteringTime = value;
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

void NavigationWidget::closeEvent(QCloseEvent */*event*/) {
    this->hide();
    emit uncheckSignal();
}

void NavigationWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(NAVIGATION_WIDGET);
    width = (settings.value(WIDTH).isNull())? this->width() : settings.value(WIDTH).toInt();
    height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
    if(settings.value(POS_X).isNull() or settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().topRight().x() - this->width() - 20;
        y = QApplication::desktop()->screen()->rect().topRight().y() + 50;
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }
    visible = (settings.value(VISIBLE).isNull())? false : settings.value(VISIBLE).toBool();


    if(visible) {
        show();
    }
    else {
        hide();
    }
    setGeometry(x, y, width, height);

    resetMovementArea();

    if(!settings.value(MOVEMENT_SPEED).isNull()) {
        this->movementSpeedSpinBox->setValue(settings.value(MOVEMENT_SPEED).toInt());
    } else {
        this->movementSpeedSpinBox->setValue(40);
    }

    if(!settings.value(JUMP_FRAMES).isNull()) {
        this->jumpFramesSpinBox->setValue(settings.value(JUMP_FRAMES).toInt());
    } else {
        this->jumpFramesSpinBox->setValue(1);
    }

    if(!settings.value(WALK_FRAMES).isNull()) {
        this->walkFramesSpinBox->setValue(settings.value(WALK_FRAMES).toInt());
    } else {
        this->walkFramesSpinBox->setValue(10);
    }

    if(!settings.value(RECENTERING_TIME_PARALLEL).isNull()) {
        this->recenterTimeParallelSpinBox->setValue(settings.value(RECENTERING_TIME_PARALLEL).toInt());
    } else {

    }

    if(!settings.value(RECENTERING_TIME_ORTHO).isNull()) {
        this->recenterTimeOrthoSpinBox->setValue(settings.value(RECENTERING_TIME_ORTHO).toInt());
    } else {
        this->recenterTimeOrthoSpinBox->setValue(500);
    }

    if(!settings.value(NORMAL_MODE).isNull()) {
        this->normalModeButton->setChecked(settings.value(NORMAL_MODE).toBool());
    } else {
        this->normalModeButton->setVisible(true);
    }

    if(!settings.value(ADDITIONAL_VIEWPORT_DIRECTION_MOVE).isNull()) {
        this->additionalViewportDirectionMoveButton->setChecked(settings.value(ADDITIONAL_VIEWPORT_DIRECTION_MOVE).toBool());
    } else {

    }

    if(!settings.value(ADDITIONAL_TRACING_DIRECTION_MOVE).isNull()) {
        this->additionalTracingDirectionMoveButton->setChecked(settings.value(ADDITIONAL_TRACING_DIRECTION_MOVE).toBool());
    } else {

    }

    if(!settings.value(ADDITIONAL_MIRRORED_MOVE).isNull()) {
         this->additionalMirroredMoveButton->setChecked(settings.value(ADDITIONAL_MIRRORED_MOVE).toBool());
    } else {

    }

    if(!normalModeButton->isChecked() and !additionalMirroredMoveButton->isChecked() and !additionalTracingDirectionMoveButton->isChecked() and !additionalTracingDirectionMoveButton->isChecked())
        normalModeButton->setChecked(true);

    if(!settings.value(DELAY_TIME_PER_STEP).isNull()) {
        this->delayTimePerStepSpinBox->setValue(settings.value(DELAY_TIME_PER_STEP).toInt());
    } else {
        this->delayTimePerStepSpinBox->setValue(50);
    }

    if(!settings.value(NUMBER_OF_STEPS).isNull()) {
        this->numberOfStepsSpinBox->setValue(settings.value(NUMBER_OF_STEPS).toInt());
    } else {
        this->numberOfStepsSpinBox->setValue(10);
    }

    settings.endGroup();
}

void NavigationWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(NAVIGATION_WIDGET);
    settings.setValue(WIDTH, this->geometry().width());
    settings.setValue(HEIGHT, this->geometry().height());
    settings.setValue(POS_X, this->geometry().x());
    settings.setValue(POS_Y, this->geometry().y());
    settings.setValue(VISIBLE, this->isVisible());

    settings.setValue(MOVEMENT_SPEED, this->movementSpeedSpinBox->value());
    settings.setValue(JUMP_FRAMES, this->jumpFramesSpinBox->value());
    settings.setValue(WALK_FRAMES, this->walkFramesSpinBox->value());
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
