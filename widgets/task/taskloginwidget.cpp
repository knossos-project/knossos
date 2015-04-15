#include "taskloginwidget.h"

#include "network.h"
#include "stateInfo.h"
#include "widgets/GuiConstants.h"

#include <QDir>
#include <QIcon>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>

TaskLoginWidget::TaskLoginWidget(QWidget * parent) : QDialog(parent) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/resources/icons/task.png"));
    setWindowTitle("Task Login");
    setModal(true);

    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/tasks");
    taskDir.mkpath(".");

    //load settings
    QSettings settings;
    settings.beginGroup(HEIDELBRAIN_INTEGRATION);
    host = settings.value(HEIDELBRAIN_HOST, "http://heidelbrain.org").toString();
    Network::singleton().setCookies(settings.value(HEIDELBRAIN_COOKIES).toList());
    settings.endGroup();

    urlField.setText(host);
    passwordField.setEchoMode(QLineEdit::Password);

    line.setFrameShape(QFrame::HLine);
    line.setFrameShadow(QFrame::Sunken);

    box.addButton("Login", QDialogButtonBox::AcceptRole)->setAutoDefault(true);//default only did not work
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

void TaskLoginWidget::saveSettings() {
    if (!host.startsWith("http://")) {//qnam cannot handle url without protocol
        host = "http://" + host;
    }
    if (!host.endsWith("/")) {//cookies only apply with slash
        host += "/";
    }
    QSettings settings;
    settings.beginGroup(HEIDELBRAIN_INTEGRATION);
    settings.setValue(HEIDELBRAIN_HOST, host);
    settings.setValue(HEIDELBRAIN_COOKIES, Network::singleton().getCookiesForHost(host));
    settings.endGroup();
}

void TaskLoginWidget::resetSession(const QString & message) {
    setResponse(message);
}

void TaskLoginWidget::setResponse(const QString &message) {
    response.setText(message);
    show();
}
