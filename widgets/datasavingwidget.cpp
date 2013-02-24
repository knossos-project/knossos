#include "datasavingwidget.h"
#include <QRadioButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include "knossos-global.h"
#include "mainwindow.h"

extern struct stateInfo *state;

DataSavingWidget::DataSavingWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Data Saving Options");
    QVBoxLayout *mainLayout = new QVBoxLayout();

    autosaveButton = new QCheckBox("Auto-Saving");
    autosaveIntervalLabel = new QLabel("Autosave Interval");
    autosaveIntervalSpinBox = new QSpinBox();


    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(autosaveIntervalLabel, autosaveIntervalSpinBox);

    autoincrementFileNameButton = new QCheckBox("Autoincrement File Name");


    mainLayout->addWidget(autosaveButton);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(autoincrementFileNameButton);

    setLayout(mainLayout);

    connect(autosaveButton, SIGNAL(clicked(bool)), this, SLOT(autosaveButtonPushed(bool)));
    connect(autoincrementFileNameButton, SIGNAL(clicked(bool)), this, SLOT(autonincrementFileNameButtonPushed(bool)));
}

void DataSavingWidget::loadSettings() {
   autosaveButton->setChecked(state->skeletonState->autoSaveBool);
   autosaveIntervalSpinBox->setValue(state->skeletonState->autoSaveInterval);
   autoincrementFileNameButton->setChecked(state->skeletonState->autoFilenameIncrementBool);
}

void DataSavingWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    MainWindow *parent = (MainWindow *) this->parentWidget();
    parent->uncheckDataSavingAction();
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


