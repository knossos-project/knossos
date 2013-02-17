#include "navigationwidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QSpacerItem>
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
}

/**
  * @todo the numerical order of autoTracingMode
  */
void NavigationWidget::loadSettings() {
    movementSpeedSpinBox->setValue(state->viewerState->stepsPerSec);
    jumpFramesSpinBox->setValue(state->viewerState->dropFrames);
    recenterTimeOrthoSpinBox->setValue(state->viewerState->recenteringTimeOrth);
    recenterTimeParallelSpinBox->setValue(state->viewerState->recenteringTime);

    if(state->viewerState->autoTracingMode == 0) {

    }

    delayTimePerStepSpinBox->setValue(state->viewerState->autoTracingDelay);
    numberOfStepsSpinBox->setValue(state->viewerState->autoTracingSteps);
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

/**
  * @todo find out the numeric order of tracing modes
  */
void NavigationWidget::normalModeButtonClicked(bool on) {

}

void NavigationWidget::additionalViewportDirectionMoveButtonClicked(bool on) {

}

void NavigationWidget::additionalTracingDirectionMoveButtonClicked(bool on) {

}

void NavigationWidget::additionalMirroredMoveButtonClicked(bool on) {

}

void NavigationWidget::delayTimePerStepChanged(int value) {
    state->viewerState->autoTracingDelay = value;
}

void NavigationWidget::numberOfStepsChanged(int value) {
    state->viewerState->autoTracingSteps = value;
}

void NavigationWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
