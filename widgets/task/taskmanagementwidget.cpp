#include "taskmanagementwidget.h"

#include "network.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <QByteArray>
#include <QDir>

#include <fstream>

TaskManagementWidget::TaskManagementWidget(QWidget *parent) : QDialog(parent), taskLoginWidget(this) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/resources/icons/task.png"));
    setWindowTitle("Task Management");

    statusLabel.setWordWrap(true);
    descriptionLabel.setWordWrap(true);
    commentLabel.setWordWrap(true);

    formLayout.addRow("Current Task: ", &taskLabel);
    formLayout.addRow("Description: ", &descriptionLabel);
    formLayout.addRow("Comment: ", &commentLabel);

    taskInfoGroupBox.setLayout(&formLayout);

    //first row is added in refresh
    gridLayout.addWidget(&submitCommentEdit, 1, 0, 1, 2);
    gridLayout.addWidget(&submitButton, 2, 0, 1, 1);
    gridLayout.addWidget(&submitFinalButton, 2, 1, 1, 1);

    taskManagementGroupBox.setLayout(&gridLayout);

    vLayout.addWidget(&taskManagementGroupBox);
    vLayout.addStretch();
    vLayout.addWidget(&logoutButton);

    hLayout.addWidget(&taskInfoGroupBox);
    hLayout.addLayout(&vLayout);
    mainLayout.addWidget(&statusLabel);
    mainLayout.addLayout(&hLayout);
    setLayout(&mainLayout);

    submitCommentEdit.setPlaceholderText("submission comment (optional)");
    submitFinalButton.setToolTip("marks your work as finished.");

    QObject::connect(&startNewTaskButton, &QPushButton::clicked, this, &TaskManagementWidget::startNewTaskButtonClicked);
    QObject::connect(&loadLastSubmitButton, &QPushButton::clicked, this, &TaskManagementWidget::loadLastSubmitButtonClicked);
    QObject::connect(&submitButton, &QPushButton::clicked, this, &TaskManagementWidget::submit);
    QObject::connect(&submitFinalButton, &QPushButton::clicked, this, &TaskManagementWidget::submitFinal);

    QObject::connect(&logoutButton, &QPushButton::clicked, this, &TaskManagementWidget::logoutButtonClicked);

    QObject::connect(&taskLoginWidget, &TaskLoginWidget::accepted, [this]{
        loginButtonClicked(taskLoginWidget.urlField.text(), taskLoginWidget.usernameField.text(), taskLoginWidget.passwordField.text());
    });
}

bool TaskManagementWidget::handleError(QPair<bool, QString> res) {
    if (res.first) {
        setResponse("<font color='green'>" + res.second + "</font>");
        return true;
    } else {
        setResponse("<font color='red'>" + res.second + "</font>");
        return false;
    }
}

void TaskManagementWidget::updateAndRefreshWidget() {
    const auto url = taskLoginWidget.host + "/knossos/session/";
    setCursor(Qt::BusyCursor);
    const auto res = Network::singleton().refresh(url);
    setCursor(Qt::ArrowCursor);

    if (res.first) {
        QXmlStreamReader xml(res.second);
        QString username;
        QString taskName;
        if (xml.readNextStartElement() && xml.name() == "session") {
            QXmlStreamAttributes attributes = xml.attributes();
            username = attributes.value("username").toString();
            taskName = attributes.value("task").toString();
            //attributes.value("taskFile").toString();//TODO is it needed
            setActiveUser(username);
            setResponse("Hello " + username + "!");
            setTask(taskName);
            setDescription(QByteArray::fromBase64(attributes.value("description").toUtf8()));
            setComment(QByteArray::fromBase64(attributes.value("comment").toUtf8()));
        }
        if (!username.isEmpty()) {
            const bool hasTask = !taskName.isEmpty();
            //last submit/new task are mutually exclusive, show only one of both
            gridLayout.removeWidget(&loadLastSubmitButton);
            gridLayout.removeWidget(&startNewTaskButton);
            if (hasTask) {
                gridLayout.addWidget(&loadLastSubmitButton, 0, 0, 1, 2);
            } else {
                gridLayout.addWidget(&startNewTaskButton, 0, 0, 1, 2);
            }
            //buttons would still be shown after removing them from the layout
            loadLastSubmitButton.setVisible(hasTask);
            startNewTaskButton.setVisible(!hasTask);

            submitCommentEdit.setEnabled(hasTask);
            submitButton.setEnabled(hasTask);
            submitFinalButton.setEnabled(hasTask);
            show();
        } else {
            emit visibilityChanged(false);
            taskLoginWidget.setResponse("Please login.");
        }
    } else {
        emit visibilityChanged(false);
        taskLoginWidget.setResponse("Please login.");
    }
}

void TaskManagementWidget::loginButtonClicked(const QString & host, const QString & username, const QString & password) {
    const auto url = host + "/knossos/session/";
    setCursor(Qt::BusyCursor);
    const auto res = Network::singleton().login(url, username, password);
    setCursor(Qt::ArrowCursor);

    if (res.first) {
        taskLoginWidget.host = host;//login was successful → save host
        updateAndRefreshWidget();//task infos changed
    } else {
        hide();
        taskLoginWidget.setResponse(res.second);
    }
}

void TaskManagementWidget::logoutButtonClicked() {
    const auto url = taskLoginWidget.host + "/knossos/session/";
    setCursor(Qt::BusyCursor);
    const auto res = Network::singleton().logout(url);
    setCursor(Qt::ArrowCursor);

    if (handleError(res)) {
        resetSession("<font color='green'>Logged out successfully.</font>");
    }
}

void TaskManagementWidget::saveAndLoadFile(const QString & filename, const QByteArray content) {
    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/tasks");
    taskDir.mkpath("./task-files");//filenames of new tasks contain subfolder

    //Retrieve the filename from response header and rename the previously created tmp.nml
    if (!filename.isEmpty()) {
        const QString actualFilename = taskDir.absolutePath() + "/" + filename;

        QFile tmpFile(actualFilename);
        const auto success = tmpFile.open(QIODevice::WriteOnly) && (tmpFile.write(content) != -1);
        tmpFile.close();//it’s reopened by openFileDispatch

        if (success && state->viewer->window->openFileDispatch({actualFilename})) {
            statusLabel.setText("<font color='green'>Loaded annotation successfully.</font>");
        } else {
            statusLabel.setText("<font color='red'>Failed to load »" + actualFilename + "«.</font>");
        }
    }
}

void TaskManagementWidget::loadLastSubmitButtonClicked() {
    const auto url = taskLoginWidget.host + "/knossos/activeTask/lastSubmit/";
    setCursor(Qt::BusyCursor);
    const auto res = Network::singleton().getFile(url);
    setCursor(Qt::ArrowCursor);

    if (handleError({res.first, res.second.first})) {
        saveAndLoadFile(res.second.first, res.second.second);
    }
}

void TaskManagementWidget::startNewTaskButtonClicked() {
    const auto url = taskLoginWidget.host + "/knossos/newTask/";
    setCursor(Qt::BusyCursor);
    const auto res = Network::singleton().getPost(url);
    setCursor(Qt::ArrowCursor);

    if (handleError({res.first, res.second.first})) {
        saveAndLoadFile(res.second.first, res.second.second);
        updateAndRefreshWidget();//task infos changed
    }
}

void TaskManagementWidget::submitFinal() {
    if (submit(true)) {
        updateAndRefreshWidget();//task infos changed
        state->viewer->window->newAnnotationSlot();//clear the annotation to ease starting a new one
    }
}

bool TaskManagementWidget::submit(const bool final) {
    state->viewer->window->autosaveSlot();//save file to submit

    const auto url = taskLoginWidget.host + "/knossos/activeTask/";
    setCursor(Qt::BusyCursor);
    auto res = Network::singleton().submitHeidelbrain(url, Session::singleton().annotationFilename, submitCommentEdit.text(), final);
    setCursor(Qt::ArrowCursor);

    if (handleError(res)) {
        submitCommentEdit.clear();//clean comment if submit was successful
        return true;
    } else {
        return false;
    }
}

void TaskManagementWidget::resetSession(const QString &message) {
    setResponse("");
    setActiveUser("");
    setTask("");
    setDescription("");
    setComment("");

    hide();
    taskLoginWidget.setResponse(message);
}

void TaskManagementWidget::setResponse(const QString &message) {
    statusLabel.setText(message);
}

void TaskManagementWidget::setActiveUser(const QString &username) {
    logoutButton.setText("Logout »" + username + "«");
}

void TaskManagementWidget::setTask(const QString & task) {
    taskLabel.setText("<font color='green'>" + task + "</font>");
}

void TaskManagementWidget::setDescription(const QString & description) {
    descriptionLabel.setText(description);
}

void TaskManagementWidget::setComment(const QString & comment) {
    commentLabel.setText(comment);
}
