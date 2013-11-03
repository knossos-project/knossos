#ifndef TASKLOGINWIDGET_H
#define TASKLOGINWIDGET_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class TaskManagementWidget;
class TaskLoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TaskLoginWidget(QWidget *parent = 0);
    void setResponse(QString message);
    void setTaskManagementWidget(TaskManagementWidget *management);
protected:
    QLabel *serverStatus;
    QLineEdit *urlField;
    QLineEdit *usernameField;
    QLineEdit *passwordField;
    QPushButton *loginButton;
    TaskManagementWidget *taskManagementWidget;
    void closeEvent(QCloseEvent *event);

signals:
    void taskResponseChanged();
    void loginResponseChanged();

public slots:
    void loginButtonClicked();
    void urlEditingFinished();
};

#endif // TASKLOGINWIDGET_H
