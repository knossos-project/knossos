#include "taskloginwidget.h"
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include "taskManagementWidget.h"

TaskLoginWidget::TaskLoginWidget(QWidget *parent) :
    QWidget(parent)
{
    setWindowTitle("Task Login");

    taskManagementWidget = new TaskManagementWidget();
    taskManagementWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    taskManagementWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    taskManagementWidget->setWindowTitle("Task Management");

    QVBoxLayout *mainLayout = new QVBoxLayout;
    QFormLayout *formLayout = new QFormLayout();

    urlField = new QLineEdit();
    usernameField = new QLineEdit();
    passwordField = new QLineEdit();
    serverStatus = new QLabel("Please Login");
    passwordField->setEchoMode(QLineEdit::Password);
    loginButton = new QPushButton("Login");

    QLabel *hostLabel = new QLabel("Host:");
    QLabel *usernameLabel = new QLabel("Username:");
    QLabel *passwordLabel = new QLabel("Password");

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    formLayout->addWidget(serverStatus);
    formLayout->addRow(hostLabel, urlField);
    formLayout->addWidget(line);
    formLayout->addRow(usernameLabel, usernameField);
    formLayout->addRow(passwordLabel, passwordField);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(loginButton);

    setLayout(mainLayout);
    connect(loginButton, SIGNAL(clicked()), this, SLOT(loginButtonClicked()));
}

void TaskLoginWidget::loginButtonClicked() {
    taskManagementWidget->show();
    taskManagementWidget->adjustSize();
    if(taskManagementWidget->pos().x() <= 0 or taskManagementWidget->pos().y() <= 0) {
        taskManagementWidget->move(this->pos());
    }

}

void TaskLoginWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
