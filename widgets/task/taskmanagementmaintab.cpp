#include <curl/curl.h>

#include "knossos-global.h"
#include "skeletonizer.h"
#include "taskmanagementmaintab.h"
#include "taskmanagementwidget.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <QByteArray>
#include <QCheckBox>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

TaskManagementMainTab::TaskManagementMainTab(TaskLoginWidget *taskLoginWidget, QWidget *parent) :
    QWidget(parent)
{
    this->taskLoginWidget = taskLoginWidget;
    statusLabel = new QLabel("");
    statusLabel->setWordWrap(true);
    loggedAsLabel = new QLabel("Logged in As: ");
   // loggedAsLabel->setWordWrap(true);
    logoutButton = new QPushButton("Logout");
    currentTaskLabel = new QLabel("Current: ");
    currentTaskLabel->setWordWrap(true);
    loadLastSubmitButton = new QPushButton("Load Last Submit");
    startNewTaskButton = new QPushButton("Start new Task");

    submitButton = new QPushButton("Submit");

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(statusLabel);
    formLayout->addRow(loggedAsLabel, logoutButton);
    formLayout->addRow(loadLastSubmitButton, startNewTaskButton);
    formLayout->addRow(currentTaskLabel);
    formLayout->addRow(submitButton);
    setLayout(formLayout);

    // prepare the work submission dialog
    submitDialog = new QDialog();
    submitDialog->setWindowTitle("Submit Your Work");
    submitDialogCommentField = new QLineEdit();
    submitDialogCommentField->setPlaceholderText("submission comment (optional)");
    submitDialogFinalCheckbox = new QCheckBox("final");
    submitDialogFinalCheckbox->setToolTip("marks your work as finished.");
    submitDialogCancelButton = new QPushButton("Cancel");
    submitDialogOkButton = new QPushButton("Submit");

    formLayout = new QFormLayout();
    formLayout->addRow(submitDialogCommentField);
    formLayout->addRow(submitDialogFinalCheckbox);
    formLayout->addRow(submitDialogCancelButton, submitDialogOkButton);
    submitDialog->setLayout(formLayout);

    connect(submitDialogCancelButton, SIGNAL(clicked()), this, SLOT(submitDialogCanceled()));
    connect(submitDialogOkButton, SIGNAL(clicked()), this, SLOT(submitDialogOk()));

    connect(logoutButton, SIGNAL(clicked()), this, SLOT(logoutButtonClicked()));
    connect(loadLastSubmitButton, SIGNAL(clicked()), this, SLOT(loadLastSubmitButtonClicked()));
    connect(startNewTaskButton, SIGNAL(clicked()), this, SLOT(startNewTaskButtonClicked()));
    connect(submitButton, SIGNAL(clicked()), this, SLOT(submitButtonClicked()));
}


void TaskManagementMainTab::logoutButtonClicked() {
    auto url = state->taskState->host + "/knossos/session/";
    httpResponse response;
    response.length = 0;
    response.content = (char *)calloc(1, 10240);
    long httpCode;
    CURLcode code;

    setCursor(Qt::WaitCursor);
    bool result = taskState::httpDELETE(url.toUtf8().data(), &response, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 5);
    setCursor(Qt::ArrowCursor);

    if(result == false) {
        setResponse("<font color='red'>Request failed. Please check your connection.</font>");
    }
    if(code == CURLE_OK) {
        resetSession("<font color='green'>Logged out successfully.</font>");
    } else {
        setResponse("<font color='red'>Request failed. Please check your connection.</font>");
    }

    free(response.content);
}

void TaskManagementMainTab::loadLastSubmitButtonClicked() {
    auto url = state->taskState->host + "/knossos/activeTask/lastSubmit/";
    httpResponse response;
    response.length = 0;
    response.content = (char *)calloc(1, 10240);
    httpResponse header;
    header.length = 0;
    header.content = (char*)calloc(1, header.length + 1);
    long httpCode = 0;
    CURLcode code;

    setCursor(Qt::WaitCursor);
    bool success = taskState::httpFileGET(url.toUtf8().data(), nullptr, &response, &header, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 10);
    setCursor(Qt::ArrowCursor);

    if(success == false) {
        resetSession(QString("<font color='red'>Could not find session cookie. Please login again.</font><br />%0").arg(response.content));
        return;
    }
    if(code != CURLE_OK) {
        setResponse(QString("<font color='red'>Request failed. Please check your connection.</font><br />%0").arg(response.content));
        return;
    }
    if(httpCode == 400) {
        setResponse(QString("<font color='red'>No submit found. Do you have an active task?</font><br />%0").arg(response.content));
        return;
    }
    else if(httpCode == 403) {
        setResponse(QString("<font color='red'>You are not authenticated. Permission denied.</font><br />%0").arg(response.content));
        return;
    }

    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/tasks");
    taskDir.mkpath(".");

    QString tmpfilepath = taskDir.absolutePath() + "/lastSubmit.tmp.nml";
    FILE * const lastNml = fopen(tmpfilepath.toUtf8().constData(), "w");
    if (lastNml == nullptr) {
        statusLabel->setText("<font color='red'>Failed to get submit. No write permission in this folder.</font>");
        return;
    }
    fwrite(response.content, 1, response.length, lastNml);
    fclose(lastNml);

    // 200 - success. Retrieve the filename from response header and rename the previously created tmp.nml
    char filename[1024] = {};
    if (taskState::copyInfoFromHeader(filename, &header, "filename")) {
        QFile tmpfile(tmpfilepath);
        tmpfile.rename(taskDir.absolutePath() + "/" + filename);
        if (loadSkeletonSignal(QStringList(tmpfile.fileName())) == false) {//BUG signals shall not be used to return something
            statusLabel->setText("<font color='red'>Failed to load skeleton.</font>");
        } else {
            statusLabel->setText("<font color='green'>Loaded last submit successfully.</font>");
        }
    }

    free(header.content);
    free(response.content);
}

void TaskManagementMainTab::startNewTaskButtonClicked() {
    CURLcode code;
    long httpCode;
    FILE *tasknml;
    struct httpResponse header;

    auto postdata = QString("csrfmiddlewaretoken=%0&data=<currentTask>%1</currentTask>").arg(taskState::CSRFToken(), state->taskState->taskFile);

    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/tasks");
    taskDir.mkpath(".");

    state->taskState->taskFile = taskDir.absolutePath() + "/task.tmp.nml";

    tasknml = fopen(state->taskState->taskFile.toUtf8().constData(), "w");
    if (tasknml == nullptr) {
        statusLabel->setText("<font color='red'>Failed to get new task. No write permission in this folder.</font>");
        return;
    }

    auto url = state->taskState->host + "/knossos/newTask/";

    header.length = 0;
    header.content = (char *)calloc(1, header.length + 1);

    setCursor(Qt::WaitCursor);
    httpResponse response;
    response.length = 0;
    response.content = (char *)calloc(1, 10240);
    bool success = taskState::httpFileGET(url.toUtf8().data(), postdata.toUtf8().data(), &response, &header, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 5);
    setCursor(Qt::ArrowCursor);
    if(success == false) {
        resetSession(QString("<font color='red'>Could not find session cookie. Please login again.</font><br />%0").arg(response.content));
        return;
    }
    if(code != CURLE_OK) {
        setResponse(QString("<font color='red'>Request failed. Please check your connection.</font><br />%0").arg(response.content));
        taskState::removeCookie();
        free(header.content);
        return;
    }

    if(httpCode == 400) {
        setResponse(QString("<font color='red'>Current task not finished or no new task available.</font><br />%0").arg(response.content));
        QFile(state->taskState->taskFile).remove();
        free(header.content);
        return;
    }
    else if(httpCode == 403) {
        setResponse(QString("<font color='red'>You are not authenticated. Permission denied.</font><br />%0").arg(response.content));
        QFile(state->taskState->taskFile).remove();
        free(header.content);
        return;
    }
    else if(httpCode != 200){
        setResponse(QString("<font color='red'>Error received from server.</font><br />%0").arg(response.content));
        QFile(state->taskState->taskFile).remove();
        free(header.content);
        return;
    }
    fwrite(response.content, 1, response.length, tasknml);
    fclose(tasknml);
    // 200 - success. Retrieve the filename from response header and rename the previously created tmp.nml
    char filename[1024] = {};
    if (taskState::copyInfoFromHeader(filename, &header, "filename")) {
        QFile tmpFile(state->taskState->taskFile);
        tmpFile.rename(filename);
        state->taskState->taskFile = tmpFile.fileName();
    }
    // get task name
    char taskname[1024] = {};
    taskState::copyInfoFromHeader(taskname, &header, "taskname");
    state->taskState->taskName = taskname;
    setTask(state->taskState->taskName);

    // get task category description and task comment
    QByteArray descriptionBuffer(8192, '\0');
    QByteArray commentBuffer(8192, '\0');
    taskState::copyInfoFromHeader(descriptionBuffer.data(), &header, "description");
    taskState::copyInfoFromHeader(commentBuffer.data(), &header, "comment");
    QString description = QByteArray::fromBase64(descriptionBuffer);
    QString comment = QByteArray::fromBase64(commentBuffer);

    QMessageBox prompt;
    prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
    prompt.setIcon(QMessageBox::Information);
    prompt.setWindowTitle(state->taskState->taskName);
    prompt.setText(QString("<p style='width:200px;'><b>Category %1:</b> %2<br><br><b>Task %3:</b> %4</p>")
                   .arg(taskState::getCategory())
                   .arg(description)
                   .arg(taskState::getTask())
                   .arg(comment));
    prompt.addButton("Ok", QMessageBox::ActionRole); // closes prompt by default
    prompt.resize(400, 300);
    prompt.exec();
    emit setDescriptionSignal(description);
    emit setCommentSignal(comment);
    emit loadSkeletonSignal(QStringList(state->taskState->taskFile));
    setResponse("<font color='green'>Loaded task successfully.</font>");
    free(header.content);
}

void TaskManagementMainTab::submitButtonClicked() {
    submitDialog->show();
}

void TaskManagementMainTab::submitDialogCanceled() {
    submitDialog->hide();
}

void TaskManagementMainTab::submitDialogOk() {
    // TDItem: write a function for multipart posts
    // for building the multipart formpost
    CURL *handle;
    CURLM *multihandle;
    int still_running;
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";
    CURLMcode code;
    long httpCode;
    struct httpResponse response;
    //for checks during the transmission
    struct timeval timeout;
    int rc; // select() return code
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;
    long curl_timeo = -1;

    FILE *cookie;
    cookie = fopen(state->taskState->cookieFile.toUtf8().constData(), "r");
    if(cookie == NULL) {
        resetSession("<font color='red'>Could not find session cookie. Please login again.</font>");
        return;
    }
    fclose(cookie);

    // save first, in case of errors during transmission
    emit autosaveSignal(); //increment true

    //prompt for entering a comment for the submission


    handle = curl_easy_init();
    multihandle = curl_multi_init();
    if(handle == NULL || multihandle == NULL) {
        setResponse("<font color='red'>Connection error!</font>");
        return;
    }
    // fill the multipart post form. TDItem: comments are not supported, yet.
    std::string CSRFToken_stdstr = taskState::CSRFToken().toStdString();
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "csrfmiddlewaretoken",
                 CURLFORM_COPYCONTENTS, CSRFToken_stdstr.c_str(), CURLFORM_END);
    std::string skeletonFileAsQString_stdstr = state->viewer->window->annotationFilename.toStdString();
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_file",
                 CURLFORM_FILE, skeletonFileAsQString_stdstr.c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "filename",
                 CURLFORM_COPYCONTENTS, skeletonFileAsQString_stdstr.c_str(), CURLFORM_END);
    std::string submitDialogCommentField_stdstr = submitDialogCommentField->text().toStdString();
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_comment",
                 CURLFORM_COPYCONTENTS, submitDialogCommentField_stdstr.c_str(), CURLFORM_END);
    if(submitDialogFinalCheckbox->isChecked()) {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_isfinal",
                     CURLFORM_COPYCONTENTS, "True", CURLFORM_END);
    }
    else {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_isfinal",
                     CURLFORM_COPYCONTENTS, "False", CURLFORM_END);
    }

    headerlist = curl_slist_append(headerlist, buf);
    if(handle == NULL || multihandle == NULL) {
        setResponse("<font color='red'>Failed to initialize request. Please tell the developers!</font>");
        return;
    }

    auto url = state->taskState->host + "/knossos/activeTask/";

    response.length = 0;
    response.content =(char *) calloc(1, response.length + 1);

    curl_easy_setopt(handle, CURLOPT_URL, url.toUtf8().data());
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(handle, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(handle, CURLOPT_COOKIEFILE, state->taskState->cookieFile.toUtf8().data());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, taskState::writeHttpResponse); // use this function to write the response into struct
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response); // write response into this struct
    curl_multi_add_handle(multihandle, handle);
    setCursor(Qt::WaitCursor);
    code = curl_multi_perform(multihandle, &still_running);

    do {
        // TDItem: this timeout is chosen arbitrarily, it could be tweaked in the future.
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        curl_multi_timeout(multihandle, &curl_timeo);
        if(curl_timeo >= 0) {
            timeout.tv_sec = curl_timeo / 1000;
            if(timeout.tv_sec > 1) {
              timeout.tv_sec = 1;
            }
            else {
              timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }
        }

        // get file descriptors from the transfers
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);
        if(curl_multi_fdset(multihandle, &fdread, &fdwrite, &fdexcep, &maxfd) != CURLM_OK) {
            setResponse("<font color='red'>Failed to get file descriptors. Please try again.</font>");
            curl_multi_cleanup(multihandle);
            curl_easy_cleanup(handle);
            curl_formfree(formpost);
            curl_slist_free_all (headerlist);
            free(response.content);
            return;
        }
        // if maxfd is -1, it means libcurl is busy and we have to wait.
        // In that case calling select with maxfd+1 (== 0) is equal to sleep timeout seconds.
        rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

        switch(rc) {
        case -1:
            setResponse("<font color='red'>Failed to get file descriptors. Please try again.</font>");
            curl_multi_cleanup(multihandle);
            curl_easy_cleanup(handle);
            curl_formfree(formpost);
            curl_slist_free_all (headerlist);
            free(response.content);
            return;
        case 0:
        default:
            /* timeout or readable/writable sockets */
            code = curl_multi_perform(multihandle, &still_running);
            break;
        }
    } while(still_running);
    setCursor(Qt::ArrowCursor);
    if(code == CURLM_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &httpCode);
        if(httpCode >= 400) {
            setResponse(QString("<font color='red'>%1</font>").arg(response.content));
        }
        else {
            setResponse(QString("<font color='green'>%1</font>").arg(response.content));
        }
    }
    curl_multi_cleanup(multihandle);
    curl_easy_cleanup(handle);
    curl_formfree(formpost);
    curl_slist_free_all (headerlist);
    free(response.content);

    submitDialog->hide();
}

void TaskManagementMainTab::resetSession(QString message) {
    taskState::removeCookie();
    state->taskState->taskFile = "";
    taskLoginWidget->setResponse(message);
    setTask("Current: ");
    setActiveUser("Logged in as: ");
    emit setDescriptionSignal("Category description: ");
    emit setDescriptionSignal("Task comment: ");
    emit hideSignal();
    taskLoginWidget->show();
}

void TaskManagementMainTab::setResponse(QString message) {
    statusLabel->setText(message);
}

void TaskManagementMainTab::setActiveUser(QString username) {
    loggedAsLabel->setText("Logged in as: <font color='green'>" + username + "</font>");
}

void TaskManagementMainTab::setTask(QString task) {
    state->taskState->taskName = task;
    currentTaskLabel->setText("Current task: <font color='green'>" + task + "</font>");
}
