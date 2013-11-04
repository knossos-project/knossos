#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDir>
#include <QMessageBox>

#include <curl/curl.h>

#include "knossos-global.h"
#include "widgets/mainwindow.h"
#include "skeletonizer.h"
#include "taskmanagementwidget.h"
#include "taskmanagementmaintab.h"

extern stateInfo *state;

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
    finalCheckbox = new QCheckBox("final");
    submitButton = new QPushButton("Submit");

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(statusLabel);
    formLayout->addRow(loggedAsLabel, logoutButton);
    formLayout->addRow(loadLastSubmitButton, startNewTaskButton);
    formLayout->addRow(currentTaskLabel);
    formLayout->addRow(finalCheckbox, submitButton);
    setLayout(formLayout);

    connect(logoutButton, SIGNAL(clicked()), this, SLOT(logoutButtonClicked()));
    connect(loadLastSubmitButton, SIGNAL(clicked()), this, SLOT(loadLastSubmitButtonClicked()));
    connect(startNewTaskButton, SIGNAL(clicked()), this, SLOT(startNewTaskButtonClicked()));
    connect(submitButton, SIGNAL(clicked()), this, SLOT(submitButtonClicked()));
}


void TaskManagementMainTab::logoutButtonClicked() {
    CURLcode code;
    long httpCode;
    struct httpResponse response;
    char url[1024];

    memset(url, '\0', 1024);
    strcpy(url, state->taskState->host);
    strcat(url, "/knossos/session/");

    response.length = 0;
    response.content = (char *)calloc(1, 10240);

    if(taskState::httpDELETE(url, &response, &httpCode, state->taskState->cookieFile, &code) == false) {
        setResponse("<font color='red'>Request failed. Please check your connection.</font>");
    }
    if(code == CURLE_OK) {
        resetSession("<font color='green'>Logged out successfully.</font>");
    }
    else {
        statusLabel->setText("<font color='red'>Request failed. Please check your connection.</font>");
    }
    free(response.content);
}

void TaskManagementMainTab::loadLastSubmitButtonClicked() {
    CURLcode code;
    long httpCode = 0;
    FILE *lastNml;
    char url[1024];
    struct httpResponse header;

    char filename[1024];
    char tmpfilepath[1024];
    char filepath[1024];
    bool success;

    QDir taskDir("task-files");
    if(taskDir.exists() == false) {
        taskDir.mkdir(".");
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

    lastNml = fopen(tmpfilepath, "w");
    if(lastNml == NULL) {
        statusLabel->setText("<font color='red'>Failed to get submit. No write permission in this folder.</font>");
        return;
    }
    success = taskState::httpFileGET(url, NULL, lastNml, &header, &httpCode, state->taskState->cookieFile, &code);
    fclose(lastNml);
    if(success == false) {
        resetSession("<font color='red'>Could not find session cookie. Please login again.</font>");
        return;
    }
    if(code != CURLE_OK) {
        statusLabel->setText("<font color='red'>Request failed. Please check your connection.</font>");
        return;
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
        rename(tmpfilepath, filepath);
    #endif
    }
    state->skeletonState->skeletonFileAsQString = filepath;
    qDebug() << state->skeletonState->skeletonFileAsQString;
    if(loadSkeletonSignal(state->skeletonState->skeletonFileAsQString) == false) {
        statusLabel->setText("<font color='red'>Failed to load skeleton.</font>");
    }
    else {
        statusLabel->setText("<font color='green'>Loaded last submit successfully.</font>");
    }
}

void TaskManagementMainTab::startNewTaskButtonClicked() {
    CURLcode code;
    long httpCode;
    FILE *tasknml;
    char url[1024];
    struct httpResponse header;

    char filename[1024];
    char description[8192];
    char comment[8192];
    char filepath[1024];
    char postdata[1024];
    bool success;

    memset(postdata, '\0', 1024);
    sprintf(postdata, "csrfmiddlewaretoken=%s&data=<currentTask>%s</currentTask>", taskState::CSRFToken(), state->taskState->taskFile);

    QDir taskDir("task-files");
    if(taskDir.exists() == false) {
    #ifdef UNIX
        taskDir.mkdir(".");
    #else
        taskDir.mkdir(".");
    #endif
    }
    memset(state->taskState->taskFile, '\0', 1024);
#ifdef UNIX
    sprintf(state->taskState->taskFile, "task-files/task.tmp.nml");
#else
    sprintf(state->taskState->taskFile, "task-files/task.tmp.nml");
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

    success = taskState::httpFileGET(url, postdata, tasknml, &header, &httpCode, state->taskState->cookieFile, &code);
    fclose(tasknml);
    if(success == false) {
        resetSession("<font color='red'>Could not find session cookie. Please login again.</font>");
        return;
    }
    if(code != CURLE_OK) {
        statusLabel->setText("<font color='red'>Request failed. Please check your connection.</font>");
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
        qDebug(header.content);
        remove(state->taskState->taskFile);
        free(header.content);
        return;
    }
    // 200 - success. Retrieve the filename from response header and rename the previously created tmp.nml
    memset(filename, '\0', sizeof(filename));
    if(taskState::copyInfoFromHeader(filename, &header, "filename")) {
    #ifdef Q_OS_LINUX
        sprintf(filepath, "task-files/%s", filename);
        QFile tmpFile(state->taskState->taskFile);
        tmpFile.rename(filepath);
        memset(state->taskState->taskFile, '\0', sizeof(state->taskState->taskFile));
        sprintf(state->taskState->taskFile, "task-files/%s", filename);
    #else
        sprintf(filepath, "task-files\\%s", filename);
        QFile tmpFile(state->taskState->taskFile);
        tmpFile.rename(filepath);
        memset(state->taskState->taskFile, '\0', sizeof(state->taskState->taskFile));
        sprintf(state->taskState->taskFile, "task-files\\%s", filename);
    #endif
    }
    // get task name
    memset(state->taskState->taskName, '\0', sizeof(state->taskState->taskName));
    taskState::copyInfoFromHeader(state->taskState->taskName, &header, "taskname");
    setTask(QString(state->taskState->taskName));

    // get task category description and task comment
    memset(description, '\0', sizeof(description));
    memset(comment, '\0', sizeof(comment));
    taskState::copyInfoFromHeader(description, &header, "description");
    taskState::copyInfoFromHeader(comment, &header, "comment");

    QMessageBox prompt;
    prompt.setWindowTitle(state->taskState->taskName);
    prompt.setText(QString("<p style='width:200px;'><b>Category %1:</b> %2<br><br><b>Task %3:</b> %4</p>")
                   .arg(taskState::getCategory())
                   .arg(description)
                   .arg(taskState::getTask())
                   .arg(comment));
    prompt.addButton("Ok", QMessageBox::ActionRole); // closes prompt by default
    prompt.resize(400, 300);
    prompt.exec();
    emit updateDescriptionSignal(description, comment);
    emit loadSkeletonSignal(state->taskState->taskFile);
    setResponse("<font color='green'>Loaded task successfully.</font>");
    free(header.content);
}

void TaskManagementMainTab::submitButtonClicked() {
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
    cookie = fopen(state->taskState->cookieFile, "r");
    if(cookie == NULL) {
        resetSession("<font color='red'>Could not find session cookie. Please login again.</font>");
        return;
    }
    fclose(cookie);

    // save first, in case of errors during transmission
    Skeletonizer::setDefaultSkelFileName();
    emit saveSkeletonSignal(); //increment true

    handle = curl_easy_init();
    multihandle = curl_multi_init();

    // fill the multipart post form. TDItem: comments are not supported, yet.
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "csrfmiddlewaretoken", CURLFORM_COPYCONTENTS, taskState::CSRFToken(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_file", CURLFORM_FILE, state->skeletonState->skeletonFileAsQString.toStdString().c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "filename", CURLFORM_COPYCONTENTS, state->skeletonState->skeletonFileAsQString.toStdString().c_str(), CURLFORM_END);
    if(finalCheckbox->isChecked()) {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_isfinal", CURLFORM_COPYCONTENTS, "True", CURLFORM_END);
    }
    else {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_isfinal", CURLFORM_COPYCONTENTS, "False", CURLFORM_END);
    }

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
}

void TaskManagementMainTab::resetSession(QString message) {
    taskState::removeCookie();
    memset(state->taskState->taskFile, '\0', sizeof(state->taskState->taskFile));
    taskLoginWidget->setResponse(message);
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
    memset(state->taskState->taskName, '\0', sizeof(state->taskState->taskName));
    strcpy(state->taskState->taskName, task.toStdString().c_str());
    currentTaskLabel->setText("Current task: <font color='green'>" + task + "</font>");
}
