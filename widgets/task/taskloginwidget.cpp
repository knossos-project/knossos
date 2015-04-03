#include "taskloginwidget.h"

#include "stateInfo.h"
#include "task.h"

#include <QDir>
#include <QIcon>
#include <QPushButton>
#include <QStandardPaths>

TaskLoginWidget::TaskLoginWidget(QWidget * parent) : QDialog(parent) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/resources/icons/task.png"));
    setWindowTitle("Task Login");
    setModal(true);

    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/tasks");
    taskDir.mkpath(".");
    state->taskState->cookieFile = taskDir.absolutePath() + "/cookie";
    state->taskState->taskFile = "";
    state->taskState->taskName = "";
    state->taskState->host = "heidelbrain.org";

    urlField.setText(state->taskState->host);
    passwordField.setEchoMode(QLineEdit::Password);

    line.setFrameShape(QFrame::HLine);
    line.setFrameShadow(QFrame::Sunken);

    box.addButton("Login", QDialogButtonBox::AcceptRole)->setDefault(true);
    box.addButton("Cancel", QDialogButtonBox::RejectRole);

    QObject::connect(&box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(&box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    formLayout.addRow(&response);
    formLayout.addRow(&hostLabel, &urlField);
    formLayout.addWidget(&line);
    formLayout.addRow(&usernameLabel, &usernameField);
    formLayout.addRow(&passwordLabel, &passwordField);
    formLayout.addRow(&box);

    setLayout(&formLayout);
}

void TaskLoginWidget::resetSession(const QString & message) {
    setResponse(message);
}

void TaskLoginWidget::setResponse(const QString &message) {
    response.setText(message);
    show();
}
