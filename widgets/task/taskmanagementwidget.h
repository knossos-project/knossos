#ifndef TASKMANAGEMENTWIDGET_H
#define TASKMANAGEMENTWIDGET_H

#include "taskloginwidget.h"

#include <QDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

struct httpResponse;
class TaskManagementWidget : public QDialog {
    Q_OBJECT
    friend class TaskLoginWidget;
    void saveAndLoadFile(httpResponse &, httpResponse &);
public:
    explicit TaskManagementWidget(QWidget *parent = nullptr);
    void setResponse(const QString & message);
    void setActiveUser(const QString & username);
    void setTask(const QString & task);
    void resetSession(const QString & message);

protected:
    QLabel statusLabel;

    QVBoxLayout mainLayout;
    QHBoxLayout hLayout;
    QHBoxLayout userNameLayout;
    QFormLayout formLayout;
    QGridLayout gridLayout;

    QLabel userNameLabel;
    QLabel taskLabel;
    QLabel descriptionLabel;
    QLabel commentLabel;

    QPushButton logoutButton{"Logout"};
    QPushButton startNewTaskButton{"Start new Task"};
    QPushButton loadLastSubmitButton{"Load last Submit"};
    QLineEdit submitCommentEdit;
    QPushButton submitButton{"Submit"};
    QPushButton submitFinalButton{"Final Submit"};

    TaskLoginWidget taskLoginWidget;

public slots:
    void refresh();
    void setDescription(const QString & description);
    void setComment(const QString & comment);

    void submitFinal();
    void submit(const bool final = false);

    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void loginButtonClicked(const QString &host, const QString & username, const QString & password);
    void logoutButtonClicked();

signals:
    void autosaveSignal();
    bool loadAnnotationFiles(QStringList fileNames);
    void visibilityChanged(bool);

private:
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
