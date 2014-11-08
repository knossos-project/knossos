#ifndef TASKMANAGEMENTWIDGET_H
#define TASKMANAGEMENTWIDGET_H

#include "taskloginwidget.h"

#include <QDialog>
#include <QLabel>
#include <QWidget>

#include <curl/curl.h>

class QPushButton;
class QCheckBox;
class QLineEdit;
struct httpResponse;
class TaskManagementWidget : public QDialog
{
    Q_OBJECT
    friend class TaskLoginWidget;
    void handleError(bool, CURLcode, long, const char * const response);
    void saveAndLoadFile(httpResponse &, httpResponse &);
public:
    explicit TaskManagementWidget(TaskLoginWidget *taskLoginWidget, QWidget *parent = 0);
    void setResponse(QString message);
    void setActiveUser(QString username);
    void setTask(QString task);

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

    void resetSession(QString message);

signals:
    void autosaveSignal();
    bool loadAnnotationFiles(QStringList fileNames);

public slots:
    void setDescription(QString description);
    void setComment(QString comment);

    void submitButtonClicked();
    void submitDialogCanceled();
    void submitDialogOk();

    void startNewTaskButtonClicked();
    void loadLastSubmitButtonClicked();
    void logoutButtonClicked();
};

#endif//TASKMANAGEMENTWIDGET_H
