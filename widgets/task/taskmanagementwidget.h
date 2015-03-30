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
#include <QWidget>

class QPushButton;
class QCheckBox;
class QLineEdit;
struct httpResponse;
class TaskManagementWidget : public QDialog
{
    Q_OBJECT
    friend class TaskLoginWidget;
    void saveAndLoadFile(httpResponse &, httpResponse &);
public:
    explicit TaskManagementWidget(TaskLoginWidget *taskLoginWidget, QWidget *parent = 0);
    void setResponse(QString message);
    void setActiveUser(QString username);
    void setTask(QString task);
    void resetSession(QString message);

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

    TaskLoginWidget * const taskLoginWidget;

signals:
    void autosaveSignal();
    bool loadAnnotationFiles(QStringList fileNames);

public slots:
    void refresh();
    void setDescription(QString description);
    void setComment(QString comment);

    void submitFinal();
    void submit(const bool final = false);

    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void loginButtonClicked(const QString & username, const QString & password);
    void logoutButtonClicked();
};

#endif//TASKMANAGEMENTWIDGET_H
