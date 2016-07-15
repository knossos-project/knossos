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

#ifndef TASKMANAGEMENTWIDGET_H
#define TASKMANAGEMENTWIDGET_H

#include "taskloginwidget.h"
#include "widgets/DialogVisibilityNotify.h"

#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class TaskManagementWidget : public DialogVisibilityNotify {
    Q_OBJECT
    friend class TaskLoginWidget;
    const QString api{"/api/2"};
    void saveAndLoadFile(const QString & filename, const QByteArray content);
public:
    explicit TaskManagementWidget(QWidget *parent = nullptr);

    TaskLoginWidget taskLoginWidget;

protected:
    QLabel statusLabel;

    QVBoxLayout mainLayout;
    QHBoxLayout hLayout;
    QGroupBox taskInfoGroupBox{"Task Info"};
    QFormLayout formLayout;
    QVBoxLayout vLayout;
    QGroupBox taskManagementGroupBox{"Task Management"};
    QGridLayout gridLayout;

    QLabel taskLabel;
    QLabel categoryDescriptionLabel;
    QLabel taskCommentLabel;

    QPushButton startNewTaskButton{"Start new Task"};
    QPushButton loadLastSubmitButton{"Load last Submit"};
    QLineEdit submitCommentEdit;
    QPushButton submitButton{"Submit"};
    QPushButton submitFinalButton{"Final Submit"};

    QPushButton logoutButton{"Logout"};

public slots:
    void showOrLoginOrHide(bool enable);

    void updateAndRefreshWidget();

    void submitFinal();
    bool submit(const bool final = false);

    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void loginButtonClicked(const QString &host, const QString & username, const QString & password);
    void logoutButtonClicked();

private:
    bool handleError(const QPair<bool, QString> & res, const QString & successText = "");
};

#endif//TASKMANAGEMENTWIDGET_H
