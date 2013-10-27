#ifndef TASKMANAGEMENTWIDGET_H
#define TASKMANAGEMENTWIDGET_H

#include <QWidget>

#include "taskloginwidget.h"

class QPushButton;
class QLabel;
class QCheckBox;
class TaskManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TaskManagementWidget(TaskLoginWidget *loginWidget, QWidget *parent = 0);
    void setResponse(QString message);
    void setActiveUser(QString username);
    void setTask(QString task);

protected:
    QLabel *statusLabel;
    QLabel *loggedAsLabel;
    QLabel *currentTaskLabel;
    QCheckBox *finalCheckbox;
    QPushButton *logoutButton;
    QPushButton *loadLastSubmitButton;
    QPushButton *startNewTaskButton;
    QPushButton *submitButton;
    void closeEvent(QCloseEvent *);
    TaskLoginWidget *taskLoginWidget;

    void resetSession(QString message);

signals:
    void saveSkeletonSignal();

public slots:
    void submitButtonClicked();
    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void logoutButtonClicked();
};

#endif // TASKMANAGEMENTWIDGET_H
