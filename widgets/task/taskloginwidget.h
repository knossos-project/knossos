#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QString>

class QWidget;

class TaskLoginWidget : public QDialog {
    Q_OBJECT

    QFormLayout formLayout;
    QLabel hostLabel{"Server:"};
    QFrame line;
    QLabel usernameLabel{"Username:"};
    QLabel passwordLabel{"Password"};
    QDialogButtonBox box;

  public:
    QLabel response;
    QLineEdit urlField;
    QLineEdit usernameField;
    QLineEdit passwordField;
    explicit TaskLoginWidget(QWidget* parent = nullptr);
    void saveSettings();
};
