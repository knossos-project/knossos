/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "taskloginwidget.h"

#include "network.h"
#include "session.h"
#include "widgets/GuiConstants.h"

#include <QDir>
#include <QIcon>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>

TaskLoginWidget::TaskLoginWidget(QWidget * parent) : QDialog(parent) {
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowIcon(QIcon(":/resources/icons/tasks-management.png"));
    setWindowTitle("Task Login");
    setModal(true);

    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/tasks");
    taskDir.mkpath(".");

    //load settings
    QSettings settings;
    settings.beginGroup(HEIDELBRAIN_INTEGRATION);
    host = settings.value(HEIDELBRAIN_HOST, "https://heidelbrain.org").toString();
    Network::singleton().setCookies(settings.value(HEIDELBRAIN_COOKIES).toList());
    settings.endGroup();

    urlField.setText(host.toString());
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
    QSettings settings;
    settings.beginGroup(HEIDELBRAIN_INTEGRATION);
    settings.setValue(HEIDELBRAIN_HOST, host.toString());
    settings.setValue(HEIDELBRAIN_COOKIES, Network::singleton().getCookiesForHost(host));
    settings.endGroup();
}

void TaskLoginWidget::setResponse(const QString &message) {
    Session::singleton().task = {};
    response.setText(message);
    show();
}
