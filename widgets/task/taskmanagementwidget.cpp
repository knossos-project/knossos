/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "taskmanagementwidget.h"

#include "network.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <QByteArray>
#include <QDir>
#include <QStandardPaths>
#include <QXmlStreamReader>

#include <fstream>

TaskManagementWidget::TaskManagementWidget(QWidget *parent) : QDialog(parent), taskLoginWidget(parent) {//mainwindow as parent for login
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/resources/icons/task.png"));
    setWindowTitle("Task Management");

    for (auto && label : {&statusLabel, &categoryDescriptionLabel, &taskCommentLabel}) {
        label->setWordWrap(true);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    }
    categoryDescriptionLabel.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    taskCommentLabel.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);

    formLayout.setSizeConstraint(QLayout::SetMinimumSize);
    formLayout.addRow("Current Task: ", &taskLabel);
    formLayout.addRow("Description: ", &categoryDescriptionLabel);
    formLayout.addRow("Comment: ", &taskCommentLabel);

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

bool TaskManagementWidget::handleError(const QPair<bool, QString> & res, const QString & successText) {
    if (res.first) {
        statusLabel.setText("<font color='green'>" + successText + "</font>");
        return true;
    } else {
        statusLabel.setText("<font color='red'>" + res.second + "</font>");
        return false;
    }
}

void TaskManagementWidget::updateAndRefreshWidget() {
    const auto url = taskLoginWidget.host + api + "/session/";
    setCursor(Qt::BusyCursor);
    const auto res = Network::singleton().refresh(url);
    setCursor(Qt::ArrowCursor);

    QString statusText = "Please login.";
    bool hasUser{false};
    QXmlStreamReader xml(res.second);
    if (res.first && xml.readNextStartElement() && xml.name() == "session") {
        QString username, fullName, taskCategory, categoryDescription, taskName, taskComment;
        while (xml.readNextStartElement()) {
            if (xml.name() == "user") {
                hasUser = true;
                const auto attributes = xml.attributes();
                username = attributes.value("username").toString();
                fullName = attributes.value("first_name").toString() + ' ' + attributes.value("last_name").toString();
            } else if (xml.name() == "category") {
                const auto attributes = xml.attributes();
                taskCategory = attributes.value("name").toString();
                categoryDescription = QByteArray::fromBase64(attributes.value("description").toUtf8());
            } else if (xml.name() == "task") {
                const auto attributes = xml.attributes();
                taskName = attributes.value("name").toString();
                taskComment = QByteArray::fromBase64(attributes.value("comment").toUtf8());
            }
            xml.skipCurrentElement();// also discards unknown elements
        }
        xml.readNext();// </session>
        statusText = !xml.hasError() ? "" : tr("\nThere was an error parsing the response, please report this error: \n%1 (at line %2)").arg(xml.errorString()).arg(xml.lineNumber());

        Session::singleton().task = {taskCategory,taskName};
        if (hasUser) {
            statusText.prepend("Hello " + fullName + "!");// statusText may already contain error Information
            statusLabel.setText(statusText);
            logoutButton.setText("Logout");

            const bool hasTask = !taskName.isEmpty();
            //last submit/new task are mutually exclusive, show only one of both
            gridLayout.removeWidget(&loadLastSubmitButton);
            gridLayout.removeWidget(&startNewTaskButton);
            if (hasTask) {
                taskLabel.setText(tr("“%1” (%2)").arg(taskName).arg(taskCategory));
                gridLayout.addWidget(&loadLastSubmitButton, 0, 0, 1, 2);
            } else {
                taskLabel.setText(tr("None"));
                gridLayout.addWidget(&startNewTaskButton, 0, 0, 1, 2);
            }
            categoryDescriptionLabel.setText(categoryDescription);
            taskCommentLabel.setText(taskComment);
            //buttons would still be shown after removing them from the layout
            loadLastSubmitButton.setVisible(hasTask);
            startNewTaskButton.setVisible(!hasTask);

            submitCommentEdit.setEnabled(hasTask);
            submitButton.setEnabled(hasTask);
            submitFinalButton.setEnabled(hasTask);
            show();
        }
    }
    if (!hasUser) {
        emit visibilityChanged(false);
        taskLoginWidget.setResponse(statusText);
    }
    qDebug() << statusText;// so errors appear in logs
}

void TaskManagementWidget::loginButtonClicked(const QString & host, const QString & username, const QString & password) {
    const auto url = host + api + "/login/";
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
    const auto url = taskLoginWidget.host + api + "/logout/";
    setCursor(Qt::BusyCursor);
    const auto res = Network::singleton().refresh(url);
    setCursor(Qt::ArrowCursor);

    if (handleError(res)) {
        hide();
        taskLoginWidget.setResponse("<font color='green'>Logged out successfully.</font>");
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
    const auto url = taskLoginWidget.host + api + "/current_file/";
    setCursor(Qt::BusyCursor);
    const auto res = Network::singleton().getFile(url);

    if (handleError({res.first, res.second.second})) {
        saveAndLoadFile(res.second.first, res.second.second);
    }
    setCursor(Qt::ArrowCursor);
}

void TaskManagementWidget::startNewTaskButtonClicked() {
    const auto url = taskLoginWidget.host + api + "/new_task/";
    setCursor(Qt::BusyCursor);
    const auto res = Network::singleton().getPost(url);

    if (handleError({res.first, res.second.second})) {
        updateAndRefreshWidget();//task infos changed
        saveAndLoadFile(res.second.first, res.second.second);
    }
    setCursor(Qt::ArrowCursor);
}

void TaskManagementWidget::submitFinal() {
    if (submit(true)) {
        updateAndRefreshWidget();//task infos changed
        state->viewer->window->newAnnotationSlot();//clear the annotation to ease starting a new one
    }
}

bool TaskManagementWidget::submit(const bool final) {
    state->viewer->window->save();//save file to submit

    const auto url = taskLoginWidget.host + api + "/submit/";
    setCursor(Qt::BusyCursor);
    auto res = Network::singleton().submitHeidelbrain(url, Session::singleton().annotationFilename, submitCommentEdit.text(), final);
    setCursor(Qt::ArrowCursor);

    if (handleError(res, "Task successfully submitted!")) {
        submitCommentEdit.clear();//clean comment if submit was successful
        return true;
    }
    return false;
}
