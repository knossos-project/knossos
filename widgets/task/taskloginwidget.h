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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QUrl>

class TaskLoginWidget : public QDialog {
    Q_OBJECT

    QFormLayout formLayout;
    QLabel response;
    QLabel hostLabel{"Host:"};
    QFrame line;
    QLabel usernameLabel{"Username:"};
    QLabel passwordLabel{"Password"};
    QDialogButtonBox box;
public:
    QUrl host;
    QLineEdit urlField;
    QLineEdit usernameField;
    QLineEdit passwordField;

    explicit TaskLoginWidget(QWidget * parent = nullptr);
    void saveSettings();
    void setResponse(const QString & message);
};
