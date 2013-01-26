#include "datasavingwidget.h"

extern struct stateInfo *state;

DataSavingWidget::DataSavingWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Data Saving Options");
    QVBoxLayout *mainLayout = new QVBoxLayout();

    autosaveButton = new QRadioButton("Auto-Saving");
    autosaveButton->setChecked(state->skeletonState->autoSaveBool);
    autosaveIntervalLabel = new QLabel("Autosave Interval");
    autosaveIntervalSpinBox = new QSpinBox();
    autosaveIntervalSpinBox->setValue(state->skeletonState->autoSaveInterval);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(autosaveIntervalLabel, autosaveIntervalSpinBox);

    autoincrementFileNameButton = new QRadioButton("Autoincrement File Name");
    autoincrementFileNameButton->setChecked(state->skeletonState->autoFilenameIncrementBool);

    mainLayout->addWidget(autosaveButton);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(autoincrementFileNameButton);

    setLayout(mainLayout);

    connect(autosaveButton, SIGNAL(clicked(bool)), this, SLOT(autosaveButtonPushed(bool)));
    connect(autoincrementFileNameButton, SIGNAL(clicked(bool)), this, SLOT(autonincrementFileNameButtonPushed(bool)));
}

void DataSavingWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}

void DataSavingWidget::autosaveButtonPushed(bool on) {
    if(on) {
       state->skeletonState->autoSaveBool = true;
       state->skeletonState->autoSaveInterval = autosaveIntervalSpinBox->value();
    } else {
        state->skeletonState->autoSaveBool = false;
    }
}

void DataSavingWidget::autonincrementFileNameButtonPushed(bool on) {
    if(on) {
        state->skeletonState->autoFilenameIncrementBool = true;
    } else {
        state->skeletonState->autoFilenameIncrementBool = false;
    }

}


