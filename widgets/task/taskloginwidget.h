#ifndef TASKLOGINWIDGET_H
#define TASKLOGINWIDGET_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>

class TaskLoginWidget : public QDialog {
    Q_OBJECT

    QFormLayout formLayout;
    QLabel response;
    QLabel hostLabel{"Host:"};
    QFrame line;
    QLabel usernameLabel{"Username:"};
    QLabel passwordLabel{"Password"};
    QDialogButtonBox box;
public:
    QLineEdit urlField;
    QLineEdit usernameField;
    QLineEdit passwordField;

    explicit TaskLoginWidget(QWidget * parent = nullptr);
    void setResponse(const QString & message);
    void resetSession(const QString & message);
};

#endif // TASKLOGINWIDGET_H
