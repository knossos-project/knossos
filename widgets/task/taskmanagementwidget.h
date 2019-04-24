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
    QString baseUrl;
    void saveAndLoadFile(const QString & filename, const QByteArray & content);
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
    QFrame separator;
    QPushButton submitInvalidButton{"Flag invalid"};
    QPushButton rejectTaskButton{"Reject"};

    QPushButton logoutButton{"Logout"};

    bool submit(const bool final = false, const bool valid = true);
public slots:
    virtual void setVisible(bool enable) override;// showOrLoginOrHide

    void updateAndRefreshWidget();

    void rejectTask();
    void submitFinal();
    void submitInvalid();

    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void loginButtonClicked(QUrl host, const QString & username, const QString & password);
    void logoutButtonClicked();

private:
    bool handleError(const QPair<bool, QString> & res, const QString & successText = "");
};

#endif//TASKMANAGEMENTWIDGET_H
