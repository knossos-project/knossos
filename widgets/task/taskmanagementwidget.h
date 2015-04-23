#ifndef TASKMANAGEMENTWIDGET_H
#define TASKMANAGEMENTWIDGET_H

#include "taskloginwidget.h"

#include <QDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

struct httpResponse;
class TaskManagementWidget : public QDialog {
    Q_OBJECT
    friend class TaskLoginWidget;
    void saveAndLoadFile(const QString & filename, const QByteArray content);
public:
    explicit TaskManagementWidget(QWidget *parent = nullptr);
    void setResponse(const QString & message);
    void setActiveUser(const QString & username);
    void setTask(const QString & task);
    void resetSession(const QString & message);

    TaskLoginWidget taskLoginWidget;

protected:
    QLabel statusLabel;

    QVBoxLayout mainLayout;
    QHBoxLayout hLayout;
    QFormLayout formLayout;
    QVBoxLayout vLayout;
    QGridLayout gridLayout;

    QGroupBox taskInfoGroupBox{"Task Info"};
    QLabel taskLabel;
    QLabel descriptionLabel;
    QLabel commentLabel;

    QGroupBox taskManagementGroupBox{"Task Management"};
    QPushButton startNewTaskButton{"Start new Task"};
    QPushButton loadLastSubmitButton{"Load last Submit"};
    QLineEdit submitCommentEdit;
    QPushButton submitButton{"Submit"};
    QPushButton submitFinalButton{"Final Submit"};

    QPushButton logoutButton{"Logout"};

public slots:
    void updateAndRefreshWidget();
    void setDescription(const QString & description);
    void setComment(const QString & comment);

    void submitFinal();
    bool submit(const bool final = false);

    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void loginButtonClicked(const QString &host, const QString & username, const QString & password);
    void logoutButtonClicked();

signals:
    void autosaveSignal();
    bool loadAnnotationFiles(QStringList fileNames);
    void visibilityChanged(bool);

private:
    bool handleError(QPair<bool, QString> res);

    void showEvent(QShowEvent * showEvent) override {
        QDialog::showEvent(showEvent);
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent * hideEvent) override {
        QDialog::hideEvent(hideEvent);
        emit visibilityChanged(false);
    }
};

#endif//TASKMANAGEMENTWIDGET_H
