#ifndef TASKMANAGEMENTMAINTAB_H
#define TASKMANAGEMENTMAINTAB_H

#include <QWidget>

#include "taskloginwidget.h"

class QPushButton;
class QLabel;
class QCheckBox;
class TaskManagementMainTab : public QWidget
{
    Q_OBJECT
public:
    explicit TaskManagementMainTab(TaskLoginWidget *taskLoginWidget, QWidget *parent = 0);
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
    TaskLoginWidget *taskLoginWidget;

    void resetSession(QString message);

signals:
    void hideSignal();
    void updateDescriptionSignal(QString description, QString comment);
    void saveSkeletonSignal();
    bool loadSkeletonSignal(const QString &fileName);

public slots:
    void submitButtonClicked();
    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void logoutButtonClicked();
    
};

#endif // TASKMANAGEMENTMAINTAB_H
