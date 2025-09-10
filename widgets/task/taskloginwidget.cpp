#include "taskloginwidget.h"

#include "widgets/GuiConstants.h"

#include <QIcon>
#include <QSettings>
#include <QVariant>
#include <Qt>

class QWidget;

TaskLoginWidget::TaskLoginWidget(QWidget* parent) :
    QDialog(parent) {
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowIcon(QIcon(":/resources/icons/tasks-management.png"));
    setWindowTitle("Task Login");
    setModal(true);

    // load settings
    QSettings settings;
    settings.beginGroup(REDMINE_INTEGRATION);
    urlField.setText(
        settings.value(REDMINE_HOST, "https://dashboard.ariadne.ai/intern").toString());
    usernameField.setText(settings.value(REDMINE_USERNAME).toString());
    settings.endGroup();

    response.setTextInteractionFlags(Qt::TextBrowserInteraction);
    passwordField.setEchoMode(QLineEdit::Password);

    line.setFrameShape(QFrame::HLine);
    line.setFrameShadow(QFrame::Sunken);

    box.addButton("Login", QDialogButtonBox::AcceptRole);
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

    urlField.setMinimumWidth(250);
    if (usernameField.text().isEmpty())
        usernameField.setFocus();
    else
        passwordField.setFocus();
}

void TaskLoginWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(REDMINE_INTEGRATION);
    settings.setValue(REDMINE_HOST, urlField.text());
    settings.setValue(REDMINE_USERNAME, usernameField.text());
    settings.endGroup();
}
