#ifndef TASKMANAGEMENTWIDGET_H
#define TASKMANAGEMENTWIDGET_H

#include <QWidget>

class QPushButton;
class QLabel;
class QCheckBox;
class TaskManagementWidget : public QWidget
{
    Q_OBJECT    
public:
    explicit TaskManagementWidget(QWidget *parent = 0);
protected:
    QLabel *statusLabel;
    QLabel *loggedAsLabel;
    QLabel *currentTaskLabel;
    QCheckBox *finalCheckbox;
    QPushButton *logoutButton;
    QPushButton *loadLastSubmitButton;
    QPushButton *startNewTaskButton;
    QPushButton *submitButton;
    void closeEvent(QCloseEvent *event);

signals:
    
public slots:
    void submitButtonClicked();
    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void logoutButtonClicked();
    void finalCheckboxChecked(bool on);
};

#endif // TASKMANAGEMENTWIDGET_H
