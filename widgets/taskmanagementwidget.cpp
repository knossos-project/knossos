#include "taskmanagementwidget.h"
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QFormLayout>

TaskManagementWidget::TaskManagementWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    statusLabel = new QLabel("Status");
    loggedAsLabel = new QLabel("Logged in As:");
    logoutButton = new QPushButton("Logout");
    currentTaskLabel = new QLabel("Current");
    loadLastSubmitButton = new QPushButton("Load Last Submit");
    startNewTaskButton = new QPushButton("Start new Task");
    finalCheckbox = new QCheckBox("final");
    submitButton = new QPushButton("Submit");

    mainLayout->addWidget(statusLabel);
    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(loggedAsLabel, logoutButton);
    formLayout->addRow(loadLastSubmitButton, startNewTaskButton);
    formLayout->addWidget(currentTaskLabel);
    formLayout->addRow(finalCheckbox);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(submitButton);

    setLayout(mainLayout);
}

void TaskManagementWidget::logoutButtonClicked() {

}

void TaskManagementWidget::loadLastSubmitButtonClicked() {

}

void TaskManagementWidget::startNewTaskButtonClicked() {

}

void TaskManagementWidget::finalCheckboxChecked(bool on) {

}

void TaskManagementWidget::submitButtonClicked() {

}

void TaskManagementWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
