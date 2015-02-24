#ifndef TASKMANAGEMENTWIDGET_H
#define TASKMANAGEMENTWIDGET_H

#include "taskloginwidget.h"

#include <QDialog>
#include <QLabel>
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
    QLabel *statusLabel;
    QLabel *loggedAsLabel;
    QLabel *currentTaskLabel;
    QPushButton *logoutButton;
    QPushButton *loadLastSubmitButton;
    QPushButton *startNewTaskButton;
    QPushButton *submitButton;
    TaskLoginWidget *taskLoginWidget;

    QDialog *submitDialog;
    QLineEdit *submitDialogCommentField;
    QCheckBox *submitDialogFinalCheckbox;
    QPushButton *submitDialogCancelButton;
    QPushButton *submitDialogOkButton;

    QLabel categoryDescriptionLabel;
    QLabel taskCommentLabel;

signals:
    void autosaveSignal();
    bool loadAnnotationFiles(QStringList fileNames);

public slots:
    void setDescription(QString description);
    void setComment(QString comment);

    void submitDialogOk();

    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void logoutButtonClicked();
};

#endif//TASKMANAGEMENTWIDGET_H
