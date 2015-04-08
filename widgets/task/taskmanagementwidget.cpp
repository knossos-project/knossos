#include "taskmanagementwidget.h"

#include "task.h"
#include "viewer.h"
#include "widgets/mainwindow.h"

#include <QByteArray>
#include <QDir>

#include <fstream>

TaskManagementWidget::TaskManagementWidget(QWidget *parent) : QDialog(parent), taskLoginWidget(this) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/resources/icons/task.png"));
    setWindowTitle("Task Management");

    statusLabel.setWordWrap(true);
    descriptionLabel.setWordWrap(true);
    commentLabel.setWordWrap(true);

    userNameLayout.addWidget(&userNameLabel);
    userNameLayout.addWidget(&logoutButton);
    formLayout.addRow("Logged in As: ", &userNameLayout);
    formLayout.addRow("Current Task: ", &taskLabel);
    formLayout.addRow("Description: ", &descriptionLabel);
    formLayout.addRow("Comment: ", &commentLabel);

    //first row is added in refresh
    gridLayout.addWidget(&submitCommentEdit, 1, 0, 1, 2);
    gridLayout.addWidget(&submitButton, 2, 0, 1, 1);
    gridLayout.addWidget(&submitFinalButton, 2, 1, 1, 1);

    vLayout.addLayout(&gridLayout);
    vLayout.addStretch();

    hLayout.addLayout(&formLayout);
    hLayout.addLayout(&vLayout);
    mainLayout.addWidget(&statusLabel);
    mainLayout.addLayout(&hLayout);
    setLayout(&mainLayout);

    submitCommentEdit.setPlaceholderText("submission comment (optional)");
    submitFinalButton.setToolTip("marks your work as finished.");

    QObject::connect(&startNewTaskButton, &QPushButton::clicked, this, &TaskManagementWidget::startNewTaskButtonClicked);
    QObject::connect(&loadLastSubmitButton, &QPushButton::clicked, this, &TaskManagementWidget::loadLastSubmitButtonClicked);
    QObject::connect(&submitButton, &QPushButton::clicked, this, &TaskManagementWidget::submit);
    QObject::connect(&submitFinalButton, &QPushButton::clicked, this, &TaskManagementWidget::submitFinal);

    QObject::connect(&logoutButton, &QPushButton::clicked, this, &TaskManagementWidget::logoutButtonClicked);

    QObject::connect(&taskLoginWidget, &TaskLoginWidget::accepted, [this]{
        loginButtonClicked(taskLoginWidget.urlField.text(), taskLoginWidget.usernameField.text(), taskLoginWidget.passwordField.text());
    });
}

template<typename Widget>
void handleError(Widget & instance, bool success, CURLcode code, long httpCode, const char * const response) {
    if (success == false) {
        instance.resetSession(QString("<font color='red'>Could not find session cookie. Please login again.</font><br />%0").arg(response));
    } else if (code != CURLE_OK) {
        instance.setResponse(QString("<font color='red'>Request failed. Please check your connection.<br />CURL code %1<br />%2</font><br />%3").arg(code).arg(curl_easy_strerror(code)).arg(response));
        taskState::removeCookie();
    } else if (httpCode == 400) {
        instance.setResponse(QString("<font color='red'>Not available.</font><br />%0").arg(response));
    } else if(httpCode == 403) {
        instance.setResponse(QString("<font color='red'>You are not authenticated. Permission denied.</font><br />%0").arg(response));
    } else if(httpCode != 200){
        instance.setResponse(QString("<font color='red'>Error received from server.</font><br />%0").arg(response));
    }
}

void TaskManagementWidget::refresh() {
    const auto url = state->taskState->host + "/knossos/session/";
    long httpCode = 0;
    httpResponse response;
    CURLcode code;

    setCursor(Qt::WaitCursor);
    bool success = taskState::httpGET(url.toUtf8().data(), &response, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 2);
    setCursor(Qt::ArrowCursor);

    qDebug() << "refresh: " << response.content;

    if (success && code == CURLE_OK && httpCode == 200) {
        QXmlStreamReader xml(response.content);
        QString username;
        if (xml.readNextStartElement() && xml.name() == "session") {
            QXmlStreamAttributes attributes = xml.attributes();
            username = attributes.value("username").toString();
            setActiveUser(username);
            setResponse("Hello " + username + "!");
            setTask(attributes.value("task").toString());
            state->taskState->taskFile = attributes.value("taskFile").toString();
            setDescription(QByteArray::fromBase64(attributes.value("description").toUtf8()));
            setComment(QByteArray::fromBase64(attributes.value("comment").toUtf8()));
        }
        if (!username.isEmpty()) {
            const bool hasTask = !state->taskState->taskName.isEmpty();
            gridLayout.removeWidget(&loadLastSubmitButton);
            gridLayout.removeWidget(&startNewTaskButton);
            if (hasTask) {
                gridLayout.addWidget(&loadLastSubmitButton, 0, 0, 1, 2);
            } else {
                gridLayout.addWidget(&startNewTaskButton, 0, 0, 1, 2);
            }
            //buttons would still be shown after removing them from the layout
            loadLastSubmitButton.setVisible(hasTask);
            startNewTaskButton.setVisible(!hasTask);

            submitCommentEdit.setEnabled(hasTask);
            submitButton.setEnabled(hasTask);
            submitFinalButton.setEnabled(hasTask);
            show();
        } else {
            emit visibilityChanged(false);
            taskLoginWidget.setResponse("Please login.");
        }
    } else {
        emit visibilityChanged(false);
        handleError(taskLoginWidget, success, code, httpCode, response.content);
    }
}

void TaskManagementWidget::loginButtonClicked(const QString & host, const QString & username, const QString & password) {
    // remove contents of cookie file to fill it with new cookie
    QFile cookie(state->taskState->cookieFile);
    cookie.open(QIODevice::WriteOnly);
    cookie.close();

    const auto url = host + "/knossos/session/";
    const auto postdata = QString("<login><username>%1</username><password>%2</password></login>").arg(username, password);
    long httpCode;
    httpResponse response;
    CURLcode code;

    setCursor(Qt::WaitCursor);
    bool success = taskState::httpPOST(url.toUtf8().data(), postdata.toUtf8().data(), &response, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 5);
    setCursor(Qt::ArrowCursor);

    if (success && code == CURLE_OK && httpCode == 200) {
        state->taskState->host = host;
        refresh();
    } else {
        hide();
        handleError(taskLoginWidget, success, code, httpCode, response.content);
    }
}

void TaskManagementWidget::logoutButtonClicked() {
    const auto url = state->taskState->host + "/knossos/session/";
    httpResponse response;
    long httpCode;
    CURLcode code;

    setCursor(Qt::WaitCursor);
    bool success = taskState::httpDELETE(url.toUtf8().data(), &response, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 5);
    setCursor(Qt::ArrowCursor);

    if (success && code == CURLE_OK && httpCode == 200) {
        resetSession("<font color='green'>Logged out successfully.</font>");
    } else {
        handleError(*this, success, code, httpCode, response.content);
    }
}

void TaskManagementWidget::saveAndLoadFile(httpResponse & header, httpResponse & response) {
    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/tasks");
    taskDir.mkpath("./task-files");//filenames of new tasks contain subfolder

    QString tmpFilename = taskDir.absolutePath() + "/temp";
    QFile tmpFile(tmpFilename);
    const auto sucess = tmpFile.open(QIODevice::WriteOnly) && (tmpFile.write(response.content, response.length) != -1);
    if (!sucess) {
        statusLabel.setText("<font color='red'>Cannot write »" + tmpFilename + "«.</font>");
        return;
    }
    tmpFile.close();

    //Retrieve the filename from response header and rename the previously created tmp.nml
    const auto filename = header.copyInfoFromHeader("filename");
    if (!filename.isEmpty()) {
        QString actualFilename = taskDir.absolutePath() + "/" + filename;
        QFile::remove(actualFilename);//rename does not overwrite
        qDebug() << "temp rename" << tmpFilename << actualFilename << QFile::rename(tmpFilename, actualFilename);
        if (loadAnnotationFiles(QStringList(actualFilename))) {//BUG signals shall not be used to return something
            state->taskState->taskFile = actualFilename;
            statusLabel.setText("<font color='green'>Loaded annotation successfully.</font>");
        } else {
            statusLabel.setText("<font color='red'>Failed to load »" + actualFilename + "«.</font>");
        }
    }
}

void TaskManagementWidget::loadLastSubmitButtonClicked() {
    const auto url = state->taskState->host + "/knossos/activeTask/lastSubmit/";
    httpResponse header, response;
    long httpCode;
    CURLcode code;

    setCursor(Qt::WaitCursor);
    bool success = taskState::httpFileGET(url.toUtf8().data(), nullptr, &response, &header, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 100);
    setCursor(Qt::ArrowCursor);

    if (success && code == CURLE_OK && httpCode == 200) {
        saveAndLoadFile(header, response);
    } else {
        handleError(*this, success, code, httpCode, response.content);
    }
}

void TaskManagementWidget::startNewTaskButtonClicked() {
    const auto url = state->taskState->host + "/knossos/newTask/";
    const auto postdata = QString("csrfmiddlewaretoken=%0&data=<currentTask>%1</currentTask>").arg(taskState::CSRFToken(), state->taskState->taskFile);
    httpResponse header, response;
    long httpCode;
    CURLcode code;

    setCursor(Qt::WaitCursor);
    bool success = taskState::httpFileGET(url.toUtf8().data(), postdata.toUtf8().data(), &response, &header, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 5);
    setCursor(Qt::ArrowCursor);

    if (success && code == CURLE_OK && httpCode == 200) {
        saveAndLoadFile(header, response);
        refresh();
    } else {
        handleError(*this, success, code, httpCode, response.content);
    }
}

void TaskManagementWidget::submitFinal() {
    if (submit(true)) {
        refresh();
        state->viewer->window->newAnnotationSlot();
    }
}

bool TaskManagementWidget::submit(const bool final) {
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
    httpResponse response;
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
        return false;
    }
    fclose(cookie);

    // save first, in case of errors during transmission
    emit autosaveSignal(); //increment true

    handle = curl_easy_init();
    multihandle = curl_multi_init();
    if(handle == NULL || multihandle == NULL) {
        setResponse("<font color='red'>Connection error!</font>");
        return false;
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
    std::string submitDialogCommentField_stdstr = submitCommentEdit.text().toStdString();
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_comment",
                 CURLFORM_COPYCONTENTS, submitDialogCommentField_stdstr.c_str(), CURLFORM_END);
    if (final) {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_isfinal",
                     CURLFORM_COPYCONTENTS, "True", CURLFORM_END);
    } else {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit_work_isfinal",
                     CURLFORM_COPYCONTENTS, "False", CURLFORM_END);
    }

    headerlist = curl_slist_append(headerlist, buf);
    if(handle == NULL || multihandle == NULL) {
        setResponse("<font color='red'>Failed to initialize request. Please tell the developers!</font>");
        return false;
    }

    auto url = state->taskState->host + "/knossos/activeTask/";

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
            return false;
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
            return false;
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

    submitCommentEdit.clear();
    return true;
}

void TaskManagementWidget::resetSession(const QString &message) {
    taskState::removeCookie();
    state->taskState->taskFile = "";
    setResponse("");
    setActiveUser("");
    setTask("");
    setDescription("");
    setComment("");

    hide();
    taskLoginWidget.setResponse(message);
}

void TaskManagementWidget::setResponse(const QString &message) {
    statusLabel.setText(message);
}

void TaskManagementWidget::setActiveUser(const QString &username) {
    userNameLabel.setText("<font color='green'>" + username + "</font>");
}

void TaskManagementWidget::setTask(const QString & task) {
    state->taskState->taskName = task;
    taskLabel.setText("<font color='green'>" + task + "</font>");
}

void TaskManagementWidget::setDescription(const QString & description) {
    descriptionLabel.setText(description);
}

void TaskManagementWidget::setComment(const QString & comment) {
    commentLabel.setText(comment);
}
