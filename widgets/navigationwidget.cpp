#include "navigationwidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QSpacerItem>

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

}

void NavigationWidget::loadSettings() {

}

void NavigationWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
