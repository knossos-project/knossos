#include "taskloginwidget.h"

#include "stateInfo.h"
#include "task.h"
#include "taskmanagementwidget.h"

#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QString>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>

#include <curl/curl.h>

TaskLoginWidget::TaskLoginWidget(QWidget *parent) : QDialog(parent), taskManagementWidget(NULL) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/resources/icons/task.png"));
    setWindowTitle("Task Login");

    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/tasks");
    taskDir.mkpath(".");
    state->taskState->cookieFile = taskDir.absolutePath() + "/cookie";
    state->taskState->taskFile = "";
    state->taskState->taskName = "";
    state->taskState->host = "heidelbrain.org";

    urlField = new QLineEdit();
    urlField->setText(state->taskState->host);
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

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(hostLabel, urlField);
    formLayout->addWidget(line);
    formLayout->addRow(usernameLabel, usernameField);
    formLayout->addRow(passwordLabel, passwordField);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(serverStatus);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(loginButton);
    setLayout(mainLayout);

    connect(urlField, SIGNAL(editingFinished()), this, SLOT(urlEditingFinished()));
    connect(passwordField, SIGNAL(returnPressed()), this, SLOT(loginButtonClicked()));
    connect(usernameField, SIGNAL(returnPressed()), this, SLOT(loginButtonClicked()));
    connect(urlField, SIGNAL(returnPressed()), this, SLOT(loginButtonClicked()));
    connect(loginButton, SIGNAL(clicked()), this, SLOT(loginButtonClicked()));
}

void TaskLoginWidget::urlEditingFinished() {
    state->taskState->host = urlField->text();
}

void TaskLoginWidget::resetSession(const QString & message) {
    getReady(message);
}

void TaskLoginWidget::getReady(const QString & message) {
    setResponse(message);
    show();
}

void TaskLoginWidget::loginButtonClicked() {
    taskManagementWidget->loginButtonClicked(usernameField->text(), passwordField->text());
}

void TaskLoginWidget::setResponse(QString message) {
    serverStatus->setText(message);
}

void TaskLoginWidget::setTaskManagementWidget(TaskManagementWidget *management) {
    delete taskManagementWidget;
    taskManagementWidget = management;
}

void TaskLoginWidget::closeEvent(QCloseEvent *) {
    this->hide();
}
