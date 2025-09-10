#pragma once

#include "widgets/DialogVisibilityNotify.h"
#include "widgets/task/taskloginwidget.h"
#include "widgets/task/taskselectiontab.h"
#include "widgets/task/taskupdatetab.h"

#include <QBoxLayout>
#include <QByteArray>
#include <QGridLayout>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>

class QUrl;
class QWidget;

class TaskManagementWidget : public DialogVisibilityNotify {
    Q_OBJECT

    bool updated{false}; // discrimate between first refresh for user and the following ones
    TaskLoginWidget taskLoginWidget;

    QVBoxLayout mainLayout;

    QGridLayout headerLayout;
    QLabel statusLabel;
    QLabel fullName;
    QPushButton logoutButton{"Logout"};

    QTabWidget tabWidget;
    TaskSelectionTab taskSelectionTab;
    TaskUpdateTab taskUpdateTab;

    bool removeApiKey();
    bool storeApiKey(QByteArray jsonRaw);

  public:
    QString baseUrl;
    explicit TaskManagementWidget(QWidget* parent = nullptr);
    bool updateStatus(bool ok, const QString& message);
    QString loadApiKey();
    void saveSettings() override;

  public slots:
    void loginButtonClicked(QUrl host, const QString& username, const QString& password);
    void logoutButtonClicked();
    void setVisible(bool visible) override;
    bool updateAndRefreshWidget();
};
