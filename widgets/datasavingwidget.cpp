#include "datasavingwidget.h"

DataSavingWidget::DataSavingWidget(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();

    autosaveButton = new QRadioButton("Auto-Saving");
    autosaveIntervalLabel = new QLabel("Autosave Interval");
    autosaveIntervalSpinBox = new QSpinBox();

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(autosaveIntervalLabel, autosaveIntervalSpinBox);

    autoincrementFileNameButton = new QRadioButton("Autoincrement File Name");

    mainLayout->addWidget(autosaveButton);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(autoincrementFileNameButton);

    setLayout(mainLayout);
}

void DataSavingWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
