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
protected:
    QLabel *serverStatus;
    QLineEdit *urlField;
    QLineEdit *usernameField;
    QLineEdit *passwordField;
    QPushButton *loginButton;
    TaskManagementWidget *taskManagementWidget;
    void closeEvent(QCloseEvent *event);
signals:
    
public slots:
    void loginButtonClicked();
};

#endif // TASKLOGINWIDGET_H
