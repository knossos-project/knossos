#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDir>

#include <curl/curl.h>

#include "knossos-global.h"
#include "mainwindow.h"
#include "skeletonizer.h"
#include "taskmanagementwidget.h"

extern stateInfo *state;

TaskManagementWidget::TaskManagementWidget(TaskLoginWidget *loginWidget, QWidget *parent) :
    QWidget(parent)
{
    taskLoginWidget = loginWidget;
    statusLabel = new QLabel("");
    loggedAsLabel = new QLabel("Logged in As: ");
    logoutButton = new QPushButton("Logout");
    currentTaskLabel = new QLabel("Current: ");
    loadLastSubmitButton = new QPushButton("Load Last Submit");
    startNewTaskButton = new QPushButton("Start new Task");
    finalCheckbox = new QCheckBox("final");
    submitButton = new QPushButton("Submit");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(statusLabel);
    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(loggedAsLabel, logoutButton);
    formLayout->addRow(loadLastSubmitButton, startNewTaskButton);
    formLayout->addWidget(currentTaskLabel);
    formLayout->addRow(finalCheckbox);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(submitButton);

    setLayout(mainLayout);

    connect(logoutButton, SIGNAL(clicked()), this, SLOT(logoutButtonClicked()));
    connect(loadLastSubmitButton, SIGNAL(clicked()), this, SLOT(loadLastSubmitButtonClicked()));
    connect(startNewTaskButton, SIGNAL(clicked()), this, SLOT(startNewTaskButtonClicked()));
    connect(submitButton, SIGNAL(clicked()), this, SLOT(submitButtonClicked()));
}

void TaskManagementWidget::logoutButtonClicked() {
    CURLcode code;
    long httpCode;
    struct httpResponse response;
    char url[1024];

    memset(url, '\0', 1024);
    strcpy(url, state->taskState->host);
    strcat(url, "/knossos/session/");

    response.length = 0;
    response.content = (char *)calloc(1, 10240);

    code = taskState::httpDELETE(url, &response, &httpCode, state->taskState->cookieFile);

    if(code == CURLE_OK || code == -2) {
        resetSession("<font color='green'>Logged out successfully.</font>");
    }
    else {
        statusLabel->setText("<font color='red'>Could not connect to host. Please try again later.</font>");
    }
    free(response.content);
}

void TaskManagementWidget::loadLastSubmitButtonClicked() {
    CURLcode code;
    long httpCode = 0;
    FILE *lastNml;
    char url[1024];
    struct httpResponse header;

    char filename[1024];
    char tmpfilepath[1024];
    char filepath[1024];

    QDir taskDir("task-files");
    if(taskDir.exists() == false) {
    #ifdef LINUX
        taskDir.mkdir("task-files");
    #else
        taskDir.mkdir("task-files");
    #endif
    }
    memset(tmpfilepath, '\0', 1024);
#ifdef LINUX
    sprintf(tmpfilepath, "task-files/lastSubmit.tmp.nml");
#else
    sprintf(tmpfilepath, "task-files\\lastSubmit.tmp.nml");
#endif

    memset(url, '\0', 1024);
    sprintf(url, "%s%s", state->taskState->host, "/knossos/activeTask/lastSubmit/");

    header.length = 0;
    header.content = (char*)calloc(1, header.length + 1);
    memset(header.content, '\0', sizeof(header.content));

    lastNml = fopen(tmpfilepath, "w");
    if(lastNml == NULL) {
        statusLabel->setText("<font color='red'>Failed to get submit. No write permission in this folder.</font>");
        return;
    }
    code = taskState::httpFileGET(url, NULL, lastNml, &header, &httpCode, state->taskState->cookieFile);
    fclose(lastNml);

    if(code == -2) {
        resetSession("<font color='red'>Could not find session cookie. Please login again.</font>");
        return;
    }
    if(code != CURLE_OK) {
        statusLabel->setText("<font color='red'>Failed to connect to host. Please try again later.</font>");
    }

    if(httpCode == 400) {
        statusLabel->setText("<font color='red'>No submit found. Do you have an active task?</font>");
        return;
    }
    else if(httpCode == 403) {
        statusLabel->setText("<font color='red'>You are not authenticated. Permission denied.</font>");
        return;
    }

    // 200 - success. Retrieve the filename from response header and rename the previously created tmp.nml
    memset(filename, '\0', sizeof(filename));
    if(taskState::copyInfoFromHeader(filename, &header, "filename")) {
    #ifdef LINUX
        sprintf(filepath, "task-files/%s", filename);
        rename(tmpfilepath, filepath);
    #else
        sprintf(filepath, "task-files\\%s", filename);
        rename(state->taskState->taskFile, filepath);
    #endif
    }
    //MainWindow::fileDialogForSkeletonAndAsyncLoading(filepath);
    statusLabel->setText("<font color='green'>Loaded last submit successfully.</font>");
}

void TaskManagementWidget::startNewTaskButtonClicked() {
    CURLcode code;
    long httpCode;
    FILE *tasknml;
    char url[1024];
    struct httpResponse header;

    char filename[1024];
    char filepath[1024];
    char postdata[1024];

    memset(postdata, '\0', 1024);
    sprintf(postdata, "<currentTask>%s</currentTask>", state->taskState->taskFile);

    QDir taskDir("task-files");
    if(taskDir.exists() == false) {
    #ifdef LINUX
        taskDir.mkdir("task-files");
    #else
        taskDir.mkdir("task-files");
    #endif
    }
    memset(state->taskState->taskFile, '\0', 1024);
#ifdef LINUX
    sprintf(state->taskState->taskFile, "task-files/task.tmp.nml");
#else
    sprintf(state->taskState->taskFile, "task-files\\task.tmp.nml");
#endif

    tasknml = fopen(state->taskState->taskFile, "w");
    if(tasknml == NULL) {
        statusLabel->setText("<font color='red'>Failed to get new task. No write permission in this folder.</font>");
        return;
    }
    memset(url, '\0', 1024);
    sprintf(url, "%s%s", state->taskState->host, "/knossos/newTask/");

    header.length = 0;
    header.content = (char *)calloc(1, header.length + 1);

    code = taskState::httpFileGET(url, postdata, tasknml, &header, &httpCode, state->taskState->cookieFile);
    fclose(tasknml);

    if(code == -2) {
        resetSession("<font color='red'>Could not find session cookie. Please login again.</font>");
        return;
    }
    if(code != CURLE_OK) {
        statusLabel->setText("<font color='red'>Failed to connect to host. Please try again later.</font>");
        taskState::removeCookie();
        free(header.content);
        return;
    }

    if(httpCode == 400) {
        statusLabel->setText("<font color='red'>Current task not finished or no new task available.</font>");
        remove(state->taskState->taskFile);
        free(header.content);
        return;
    }
    else if(httpCode == 403) {
        statusLabel->setText("<font color='red'>You are not authenticated. Permission denied.</font>");
        remove(state->taskState->taskFile);
        free(header.content);
        return;
    }

    // 200 - success. Retrieve the filename from response header and rename the previously created tmp.nml
    memset(filename, '\0', sizeof(filename));
    if(taskState::copyInfoFromHeader(filename, &header, "filename")) {
    #ifdef LINUX
        sprintf(filepath, "task-files/%s", filename);
        rename(state->taskState->taskFile, filepath);
        memset(state->taskState->taskFile, '\0', sizeof(state->taskState->taskFile));
        sprintf(state->taskState->taskFile, "task-files/%s", filename);
    #else
        sprintf(filepath, "task-files\\%s", filename);
        rename(state->taskState->taskFile, filepath);
        memset(state->taskState->taskFile, '\0', sizeof(state->taskState->taskFile));
        sprintf(state->taskState->taskFile, "task-files\\%s", filename);
    #endif
    }

    //MainWindow::fileDialogForSkeletonAndAsyncLoading(state->taskState->taskFile);

    // also update the task name for display
    char taskName[1024];
    memset(taskName, '\0', sizeof(taskName));
    taskState::copyInfoFromHeader(taskName, &header, "taskname");
    setTask(taskName);
    setResponse("<font color='green'>Loaded task successfully.</font>");
    free(header.content);
}

void TaskManagementWidget::submitButtonClicked() {
    // TDItem: write a function for multipart posts
    // for building the multipart formpost
    char url[1024];
    CURL *handle;
    CURLM *multihandle;
    int still_running;
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";
    CURLMcode code;
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
    qDebug("submitbutton clicked");
    cookie = fopen(state->taskState->cookieFile, "r");
    if(cookie == NULL) {
        resetSession("<font color='red'>Could not find session cookie. Please login again.</font>");
        return;
    }
    fclose(cookie);

    // save first, in case of errors during transmission
    Skeletonizer::setDefaultSkelFileName();
    emit saveSkeletonSignal(); //increment true

    // fill the multipart post form. TDItem: comments are not supported, yet.
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_file", CURLFORM_FILE, state->skeletonState->skeletonFile, CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "filename", CURLFORM_COPYCONTENTS, state->skeletonState->skeletonFile, CURLFORM_END);
    if(finalCheckbox->isChecked()) {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_isfinal", CURLFORM_COPYCONTENTS, "True", CURLFORM_END);
    }
    else {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_isfinal", CURLFORM_COPYCONTENTS, "False", CURLFORM_END);
    }

    handle = curl_easy_init();
    multihandle = curl_multi_init();

    headerlist = curl_slist_append(headerlist, buf);
    if(handle == NULL || multihandle == NULL) {
        setResponse("<font color='red'>Failed to initialize request. Please tell the developers!</font>");
        return;
    }

    memset(url, '\0', 1024);
    strcpy(url, state->taskState->host);
    strcat(url, "/knossos/activeTask/");

    response.length = 0;
    response.content =(char *) calloc(1, response.length + 1);
    memset(response.content, '\0', sizeof(response));

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(handle, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(handle, CURLOPT_COOKIEFILE, state->taskState->cookieFile);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, taskState::writeHttpResponse); // use this function to write the response into struct
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response); // write response into this struct
    curl_multi_add_handle(multihandle, handle);
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

    if(code == CURLM_OK) {
        setResponse(QString("<font color='green'>%1</font>").arg(response.content));
    }

    curl_multi_cleanup(multihandle);
    curl_easy_cleanup(handle);
    curl_formfree(formpost);
    curl_slist_free_all (headerlist);
    free(response.content);
}

void TaskManagementWidget::resetSession(QString message) {
    taskState::removeCookie();
    memset(state->taskState->taskFile, '\0', sizeof(state->taskState->taskFile));
    taskLoginWidget->setResponse(message);
    this->hide();
    taskLoginWidget->show();
}

void TaskManagementWidget::setResponse(QString message) {
    statusLabel->setText(message);
}

void TaskManagementWidget::setActiveUser(QString username) {
    loggedAsLabel->setText("Logged in as: <font color='green'>" + username + "</font>");
}

void TaskManagementWidget::setTask(QString task) {
    currentTaskLabel->setText("Current task: <font color='green'>" + task + "</font>");
}

void TaskManagementWidget::closeEvent(QCloseEvent *) {
    this->hide();
}
