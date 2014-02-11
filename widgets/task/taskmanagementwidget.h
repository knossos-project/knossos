#ifndef TASKMANAGEMENTWIDGET_H
#define TASKMANAGEMENTWIDGET_H

#include <QDialog>
#include <QTabWidget>

#include "taskmanagementmaintab.h"
#include "taskmanagementdetailstab.h"
#include "taskloginwidget.h"

class QPushButton;
class QLabel;
class QCheckBox;
class TaskManagementWidget : public QDialog
{
    Q_OBJECT
    friend class Viewer;
    friend class WidgetContainer;
    friend class MainWindow;
    friend class TaskLoginWidget;
public:
    explicit TaskManagementWidget(TaskLoginWidget *loginWidget, QWidget *parent = 0);
    void setResponse(QString message);
    void setActiveUser(QString username);
    void setTask(QString task);

protected:
    QTabWidget *tabs;
    TaskManagementMainTab *mainTab;
    TaskManagementDetailsTab *detailsTab;
    QLabel *statusLabel;
    QLabel *loggedAsLabel;
    QLabel *currentTaskLabel;
    QCheckBox *finalCheckbox;
    QPushButton *logoutButton;
    QPushButton *loadLastSubmitButton;
    QPushButton *startNewTaskButton;
    QPushButton *submitButton;

    void closeEvent(QCloseEvent *);

signals:

public slots:
};

#endif // TASKMANAGEMENTWIDGET_H
