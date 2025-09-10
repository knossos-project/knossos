#include "taskmanagementwidget.h"

#include "network.h"
#include "widgets/DialogVisibilityNotify.h"
#include "widgets/GuiConstants.h"
#include "widgets/mainwindow.h"
#include "widgets/task/taskloginwidget.h"
#include "widgets/task/taskselectiontab.h"
#include "widgets/task/taskupdatetab.h"

#include <QDebug>
#include <QEventLoop>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLineEdit>
#include <QPair>
#include <QPlainTextEdit>
#include <QSizePolicy>
#include <QUrl>
#include <Qt>
#include <QtCore/qglobal.h>
#include <qt5keychain/keychain.h>

#include <initializer_list>

class QWidget;

TaskManagementWidget::TaskManagementWidget(QWidget* parent) :
    DialogVisibilityNotify(REDMINE_INTEGRATION, parent),
    taskLoginWidget(parent),
    taskSelectionTab(*this),
    taskUpdateTab(*this) {
    for (auto&& label : {&statusLabel, &fullName}) {
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    }
    baseUrl = taskLoginWidget.urlField.text();
    statusLabel.setTextInteractionFlags(Qt::TextBrowserInteraction);
    logoutButton.setAutoDefault(false);

    headerLayout.addWidget(&statusLabel, 0, 0);
    headerLayout.setColumnStretch(1, 1);
    headerLayout.addWidget(&fullName, 0, 2);
    headerLayout.addWidget(&logoutButton, 1, 2);

    tabWidget.addTab(&taskSelectionTab, "Task Selection");
    tabWidget.addTab(&taskUpdateTab, "Task Update");
    tabWidget.setTabEnabled(1, false);

    mainLayout.addLayout(&headerLayout);
    mainLayout.addWidget(&tabWidget);
    setLayout(&mainLayout);

    QObject::connect(&taskLoginWidget, &TaskLoginWidget::accepted, [this] {
        loginButtonClicked(taskLoginWidget.urlField.text(), taskLoginWidget.usernameField.text(),
                           taskLoginWidget.passwordField.text());
    });
    QObject::connect(&logoutButton, &QPushButton::clicked, this,
                     &TaskManagementWidget::logoutButtonClicked);
    QObject::connect(&taskSelectionTab, &TaskSelectionTab::readyOpenTask, [this](int issueID) {
        taskUpdateTab.fetchUpdateIssue(issueID);
        tabWidget.setTabEnabled(1, true);
        tabWidget.setCurrentIndex(1);
    });
};

bool TaskManagementWidget::removeApiKey() {
    QKeychain::DeletePasswordJob job("KNOSSOS", this);
    job.setKey("API_key");
    QEventLoop loop;
    connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    return !job.error();
}

bool TaskManagementWidget::storeApiKey(QByteArray jsonRaw) {
    QKeychain::WritePasswordJob job("KNOSSOS", this);
    job.setKey("API_key");
    const auto jmap = QJsonDocument::fromJson(jsonRaw).object();
    if (jmap.isEmpty()) {
        qWarning() << "unable to parse /my/account.json";
        return false;
    }
    job.setTextData(jmap["user"]["api_key"].toString());
    QEventLoop loop;
    connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    if (job.error()) {
        qWarning() << "error writing securely the API key with QtKeychain: " << job.errorString();
        return false;
    }
    return true;
}

bool TaskManagementWidget::updateStatus(bool ok, const QString& message) {
    if (ok) {
        statusLabel.setText("<font color='green'>" + message + "</font>");
        return true;
    } else {
        statusLabel.setText("<font color='red'>" + message + "</font>");
        return false;
    }
}

QString TaskManagementWidget::loadApiKey() {
    QKeychain::ReadPasswordJob job("KNOSSOS", this);
    job.setKey("API_key");
    QEventLoop loop;
    connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    if (job.error()) {
        qWarning() << "error reading the stored API key:" << job.errorString();
    }
    return job.textData();
}

void TaskManagementWidget::saveSettings() {
    DialogVisibilityNotify::saveSettings();
    taskLoginWidget.saveSettings();
}

void TaskManagementWidget::loginButtonClicked(QUrl host, const QString& username,
                                              const QString& password) {
    taskLoginWidget.passwordField.clear();
    if (host.scheme().isEmpty()) { // qnam cannot handle url without protocol
        host = "https://" + host.toString();
    }
    baseUrl = host.toString();
    WidgetDisabler d{
        *this}; // don’t allow widget interaction while Network has an event loop running
    LoadingCursor loadingcursor;
    const auto res = Network::singleton().login(baseUrl + "/my/account.json", username, password);

    if (res.first) {
        taskLoginWidget.urlField.setText(host.toString());
        if (storeApiKey(res.second)) {
            setVisible(true);
            return;
        }
        taskLoginWidget.response.setText("<font color='red'>Failed to store the API key.</font>");
    } else
        taskLoginWidget.response.setText("<font color='red'>Wrong username or password.</font>");
    taskLoginWidget.show();
}

void TaskManagementWidget::logoutButtonClicked() {
    if (!removeApiKey()) {
        qWarning() << "error removing the API key";
    }
    updated = false;
    setVisible(false);
    tabWidget.setTabEnabled(1, false);
    statusLabel.clear();
    taskUpdateTab.commentsEdit.clear();
    taskLoginWidget.response.setText("<font color='green'>Successfully logged out.</font>");
    taskLoginWidget.show();
}

void TaskManagementWidget::setVisible(bool visible) {
    if (visible) {
        if (updated || updateAndRefreshWidget()) {
            DialogVisibilityNotify::setVisible(true);
            return;
        } else {
            taskLoginWidget.show();
        };
    }
    DialogVisibilityNotify::setVisible(false);
}

bool TaskManagementWidget::updateAndRefreshWidget() {
    fullName.setText(taskLoginWidget.usernameField.text());
    taskSelectionTab.resetSearch();
    auto res = taskSelectionTab.updateAndRefreshWidget();
    updated |= res;
    return res;
}
