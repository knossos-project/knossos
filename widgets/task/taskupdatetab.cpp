#include "taskupdatetab.h"

#include "annotation/annotation.h"
#include "network.h"
#include "stateInfo.h"
#include "widgets/mainwindow.h"
#include "widgets/task/taskmanagementwidget.h"

#include <QApplication>
#include <QByteArray>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSizePolicy>
#include <QUrl>

TaskUpdateTab::TaskUpdateTab(TaskManagementWidget& taskManagementWidget, QWidget* parent) :
    QWidget(parent),
    taskManagementWidget{taskManagementWidget} {
    separator.setFrameShape(QFrame::HLine);
    separator.setFrameShadow(QFrame::Sunken);
    openInBrowserButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    newStatusCombo.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    attachAnnotationCheckBox.setChecked(true);
    submitButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    submitButton.setStyleSheet("background-color: #90EE90; font-weight: bold;");

    headerLayout.addWidget(&currentTaskLabel);
    headerLayout.addWidget(&openInBrowserButton);

    formLayout.addRow(&statusLabel, &newStatusCombo);
    formLayout.addRow(&attachAnnotationLabel, &attachAnnotationCheckBox);
    formLayout.addRow(&commentsLabel, &commentsEdit);

    mainLayout.addLayout(&headerLayout);
    mainLayout.addWidget(&separator);
    mainLayout.addLayout(&formLayout);
    mainLayout.addWidget(&submitButton);
    setLayout(&mainLayout);

    QObject::connect(&submitButton, &QPushButton::clicked, this, &TaskUpdateTab::submit);
    QObject::connect(&openInBrowserButton, &QPushButton::clicked, this,
                     &TaskUpdateTab::openInBrowser);
    QObject::connect(this, &TaskUpdateTab::successfulSubmission, [this] {
        fetchUpdateIssue(issueId); // so we fetch the new correct statuses
    });
}

bool TaskUpdateTab::attachmentMismatch() {
    auto currentAnnotation = Annotation::singleton().annotationFilename;
    if (!currentAnnotation.contains(subject)) {
        QMessageBox box{QApplication::activeWindow()};
        box.setIcon(QMessageBox::Question);
        box.setText(QString("The current annotation file (%1) may not match the current task "
                            "(%2).\nSave and update anyway?")
                        .arg(currentAnnotation)
                        .arg(subject));
        box.addButton("Update", QMessageBox::AcceptRole);
        const auto* cancel = box.addButton("Cancel", QMessageBox::RejectRole);
        box.exec();
        return box.clickedButton() == cancel;
    }
    return false;
}

void TaskUpdateTab::fetchUpdateIssue(int issueId) {
    this->issueId = issueId;
    statusEnum.clear();
    WidgetDisabler d{
        *this}; // don’t allow widget interaction while Network has an event loop running
    LoadingCursor loadingCursor;
    const auto res = Network::singleton().refresh(
        taskManagementWidget.baseUrl + "/issues/" + QString::number(issueId) +
        ".json?include=allowed_statuses&key=" + taskManagementWidget.loadApiKey());

    if (!res.first) {
        taskManagementWidget.updateStatus(false, "Fetching allowed statuses failed.");
        return;
    }

    const auto jmap = QJsonDocument::fromJson(res.second).object()["issue"].toObject();
    auto currentId = jmap["status"].toObject()["id"].toInt();
    for (auto elt : jmap["allowed_statuses"].toArray()) {
        auto eltObj = elt.toObject();
        auto name = eltObj["name"].toString();
        auto id = eltObj["id"].toInt();
        statusEnum.append(QPair(id, name));
        newStatusCombo.addItem(name);
        if (id == currentId) { // select the current status
            newStatusCombo.setCurrentIndex(statusEnum.size() - 1);
        }
    }

    statusLabel.setText("New status (current: " + jmap["status"].toObject()["name"].toString() +
                        ")");
    subject = jmap["subject"].toString();
    auto currentTaskText = subject + " (" + jmap["project"].toObject()["name"].toString() + ")";
    auto taskDescription = jmap["description"].toString();
    if (!taskDescription.isEmpty()) {
        currentTaskText += ":\n\n" + taskDescription;
    }
    currentTaskLabel.setText(currentTaskText);
}

void TaskUpdateTab::submit() {
    if (attachAnnotationCheckBox.isChecked() && Annotation::singleton().isEmpty()) {
        taskManagementWidget.updateStatus(false, "Please open an annotation before submitting it.");
        return;
    }

    if (attachmentMismatch())
        return;

    WidgetDisabler d{
        *this}; // don’t allow widget interaction while Network has an event loop running
    LoadingCursor loadingcursor;

    QJsonArray attachments;
    if (attachAnnotationCheckBox.isChecked()) {
        state->mainWindow->saveSlot();
        QFile file(Annotation::singleton().annotationFilename);
        file.open(QIODevice::ReadOnly);
        QFileInfo fileInfo(file);

        const auto uploadRes =
            Network::singleton().send(taskManagementWidget.baseUrl +
                                          "/uploads.json?key=" + taskManagementWidget.loadApiKey() +
                                          "&filename=" + fileInfo.fileName(),
                                      "post", "application/octet-stream", file.readAll());
        if (!uploadRes.first) {
            taskManagementWidget.updateStatus(false,
                                              "Attachment upload failed: " + uploadRes.second);
            return;
        }

        const auto attachmentToken = QJsonDocument::fromJson(uploadRes.second).object();
        attachments.append(attachmentToken["upload"]);
    }

    QJsonObject issueObj;
    issueObj["project_id"] = issueId;
    issueObj["status_id"] = statusEnum[newStatusCombo.currentIndex()].first;
    issueObj["notes"] = commentsEdit.toPlainText();
    if (attachAnnotationCheckBox.isChecked()) {
        issueObj["uploads"] = attachments;
    }

    QJsonObject jsonObj;
    jsonObj["issue"] = issueObj;
    const QJsonDocument statusUpdateDocument(jsonObj);

    const auto statusRes = Network::singleton().send(
        taskManagementWidget.baseUrl + "/issues/" + QString::number(issueId) +
            ".json?key=" + taskManagementWidget.loadApiKey(),
        "put", "application/json", statusUpdateDocument.toJson());

    if (!statusRes.first) {
        taskManagementWidget.updateStatus(false, "Task submission failed: " + statusRes.second);
    } else {
        taskManagementWidget.updateStatus(true, "Task submission succeed");
        emit successfulSubmission();
    }
}

void TaskUpdateTab::openInBrowser() {
    QDesktopServices::openUrl(
        QUrl(taskManagementWidget.baseUrl + "/issues/" + QString::number(issueId)));
}
