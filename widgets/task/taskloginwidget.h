#ifndef TASKLOGINWIDGET_H
#define TASKLOGINWIDGET_H

#include <QDialog>
#include <curl/curl.h>

class QLabel;
class QLineEdit;
class QPushButton;
class TaskManagementWidget;

// to taskwidget
struct httpResponse {
    char *content;
    size_t length;
};

// to taskwidget
namespace taskState {
    extern QString host;
    extern QString cookieFile;
    extern QString taskFile;
    extern QString taskName;

    extern bool httpGET(char *url, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    extern bool httpPOST(char *url, char *postdata, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    extern bool httpDELETE(char *url, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    extern bool httpFileGET(char *url, char *postdata, httpResponse *response, struct httpResponse *header, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    extern size_t writeHttpResponse(void *ptr, size_t size, size_t nmemb, struct httpResponse *s);
    extern size_t writeToFile(void *ptr, size_t size, size_t nmemb, FILE *stream);
    extern size_t readFile(char *ptr, size_t size, size_t nmemb, void *stream);
    extern int copyInfoFromHeader(char *dest, struct httpResponse *header, char* info);
    extern void removeCookie();
    extern QString CSRFToken();
    extern QString getCategory();
    extern QString getTask();
}

class TaskLoginWidget : public QDialog
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
