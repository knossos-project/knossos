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

    QObject::connect(&xMinField, &QSpinBox::editingFinished, [this]() {
        xMinField.setValue(std::min(xMinField.value(), state->boundary.x + 1));
        Session::singleton().movementAreaMin.x = xMinField.value() - 1;
        updateAreaMaxField(xMaxField);
        emit movementAreaChanged();
    });
    QObject::connect(&yMinField, &QSpinBox::editingFinished, [this]() {
        yMinField.setValue(std::min(yMinField.value(), state->boundary.y + 1));
        Session::singleton().movementAreaMin.y = yMinField.value() - 1;
        updateAreaMaxField(yMaxField);
        emit movementAreaChanged();
    });
    QObject::connect(&zMinField, &QSpinBox::editingFinished, [this]() {
        zMinField.setValue(std::min(zMinField.value(), state->boundary.z + 1));
        Session::singleton().movementAreaMin.z = zMinField.value() - 1;
        updateAreaMaxField(zMaxField);
        emit movementAreaChanged();
    });
    QObject::connect(&xMaxField, &QSpinBox::editingFinished, [this]() { updateAreaMaxField(xMaxField); });
    QObject::connect(&yMaxField, &QSpinBox::editingFinished, [this]() { updateAreaMaxField(yMaxField); });
    QObject::connect(&zMaxField, &QSpinBox::editingFinished, [this]() { updateAreaMaxField(zMaxField); });
    QObject::connect(&Session::singleton(), &Session::movementAreaChanged, [this]() {
        auto & session = Session::singleton();
        xMinField.setValue(session.movementAreaMin.x + 1);
        yMinField.setValue(session.movementAreaMin.y + 1);
        zMinField.setValue(session.movementAreaMin.z + 1);
        xMaxField.setValue(session.movementAreaMax.x + 1);
        yMaxField.setValue(session.movementAreaMax.y + 1);
        zMaxField.setValue(session.movementAreaMax.z + 1);
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

void NavigationWidget::updateAreaMaxField(QSpinBox &box) {
    auto & session = Session::singleton();
    int *boundary, *areaMin, *areaMax;
    if(&box == &xMaxField) {
        boundary = &state->boundary.x;
        areaMin = &session.movementAreaMin.x;
        areaMax = &session.movementAreaMax.x;
    }
    else if(&box == &yMaxField) {
        boundary = &state->boundary.y;
        areaMin = &session.movementAreaMin.y;
        areaMax = &session.movementAreaMax.y;
    }
    else if(&box == &zMaxField) {
        boundary = &state->boundary.z;
        areaMin = &session.movementAreaMin.z;
        areaMax = &session.movementAreaMax.z;
    }
    else {
        return;
    }
    box.setValue(std::min(box.value() + 1, *boundary + 1));
    *areaMax = box.value() - 1;
    emit sendLoadSignal(0);
    emit movementAreaChanged();
}

void NavigationWidget::resetMovementArea() {
    xMinField.setValue(1); yMinField.setValue(1); zMinField.setValue(1);
    xMaxField.setValue(state->boundary.x + 1); yMaxField.setValue(state->boundary.y + 1); zMaxField.setValue(state->boundary.z + 1);
    Session::singleton().movementAreaMin = { xMinField.value() - 1, yMinField.value() - 1, zMinField.value() - 1 };
    Session::singleton().movementAreaMax = { xMaxField.value() - 1, yMaxField.value() - 1, zMaxField.value() - 1};
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

    movementSpeedSpinBox->setValue(settings.value(MOVEMENT_SPEED, 100).toInt());

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
