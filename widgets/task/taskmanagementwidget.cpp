#include "taskmanagementwidget.h"

#include "task.h"
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

#include <fstream>

TaskManagementWidget::TaskManagementWidget(TaskLoginWidget *taskLoginWidget, QWidget *parent) : QDialog(parent) {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/resources/icons/task.png"));
    setWindowTitle("Task Management");

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

    categoryDescriptionLabel.setText("Category description: ");
    categoryDescriptionLabel.setWordWrap(true);
    taskCommentLabel.setText("Task comment: ");
    taskCommentLabel.setWordWrap(true);

    submitButton = new QPushButton("Submit");


    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(loggedAsLabel, logoutButton);
    formLayout->addRow(loadLastSubmitButton, startNewTaskButton);
    formLayout->addRow(currentTaskLabel);
    formLayout->addRow(submitButton);

    formLayout->addRow(&categoryDescriptionLabel);
    formLayout->addRow(&taskCommentLabel);

    auto mainLayout = new QVBoxLayout();
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(formLayout);
    setLayout(mainLayout);

    // prepare the work submission dialog
    submitDialog = new QDialog(this);
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

    connect(submitDialogOkButton, SIGNAL(clicked()), this, SLOT(submitDialogOk()));

    connect(logoutButton, SIGNAL(clicked()), this, SLOT(logoutButtonClicked()));
    connect(loadLastSubmitButton, SIGNAL(clicked()), this, SLOT(loadLastSubmitButtonClicked()));
    connect(startNewTaskButton, SIGNAL(clicked()), this, SLOT(startNewTaskButtonClicked()));
    QObject::connect(submitButton, &QPushButton::clicked, submitDialog, &QDialog::exec);
}

void handleError(TaskManagementWidget * instance, bool success, CURLcode code, long httpCode, const char * const response) {
    if (success == false) {
        instance->resetSession(QString("<font color='red'>Could not find session cookie. Please login again.</font><br />%0").arg(response));
    } else if (code != CURLE_OK) {
        instance->setResponse(QString("<font color='red'>Request failed. Please check your connection.<br />CURL code %1<br />%2</font><br />%3").arg(code).arg(curl_easy_strerror(code)).arg(response));
        taskState::removeCookie();
    } else if (httpCode == 400) {
        instance->setResponse(QString("<font color='red'>Not available.</font><br />%0").arg(response));
    } else if(httpCode == 403) {
        instance->setResponse(QString("<font color='red'>You are not authenticated. Permission denied.</font><br />%0").arg(response));
    } else if(httpCode != 200){
        instance->setResponse(QString("<font color='red'>Error received from server.</font><br />%0").arg(response));
    }
}

void TaskManagementWidget::logoutButtonClicked() {
    auto url = state->taskState->host + "/knossos/session/";
    httpResponse response;
    response.length = 0;
    response.content = (char *)calloc(1, 10240);
    long httpCode;
    CURLcode code;

    setCursor(Qt::WaitCursor);
    bool success = taskState::httpDELETE(url.toUtf8().data(), &response, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 5);
    setCursor(Qt::ArrowCursor);

    if (success && code == CURLE_OK && httpCode == 200) {
        resetSession("<font color='green'>Logged out successfully.</font>");
    } else {
        handleError(this, success, code, httpCode, response.content);
    }

    free(response.content);
}

void TaskManagementWidget::saveAndLoadFile(httpResponse & header, httpResponse & response) {
    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/tasks");
    taskDir.mkpath("./task-files");//filenames of new tasks contain subfolder

    QString tmpFilename = taskDir.absolutePath() + "/temp";
    QFile tmpFile(tmpFilename);
    const auto sucess = tmpFile.open(QIODevice::WriteOnly) && (tmpFile.write(response.content, response.length) != -1);
    if (!sucess) {
        statusLabel->setText("<font color='red'>Cannot write »" + tmpFilename + "«.</font>");
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
            statusLabel->setText("<font color='green'>Loaded annotation successfully.</font>");
        } else {
            statusLabel->setText("<font color='red'>Failed to load »" + actualFilename + "«.</font>");
        }
    }
}

void initRequest(httpResponse & header, httpResponse & response) {
    header.length = 0;
    header.content = (char*)calloc(1, header.length + 1);

    response.length = 0;
    response.content = (char *)calloc(1, 10240);
}

void TaskManagementWidget::loadLastSubmitButtonClicked() {
    httpResponse header, response;
    initRequest(header, response);

    long httpCode;
    CURLcode code;

    auto url = state->taskState->host + "/knossos/activeTask/lastSubmit/";

    setCursor(Qt::WaitCursor);
    bool success = taskState::httpFileGET(url.toUtf8().data(), nullptr, &response, &header, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 100);
    setCursor(Qt::ArrowCursor);

    if (success && code == CURLE_OK && httpCode == 200) {
        saveAndLoadFile(header, response);
    } else {
        handleError(this, success, code, httpCode, response.content);
    }

    free(header.content);
    free(response.content);
}

void TaskManagementWidget::startNewTaskButtonClicked() {
    httpResponse header, response;
    initRequest(header, response);

    long httpCode;
    CURLcode code;

    auto postdata = QString("csrfmiddlewaretoken=%0&data=<currentTask>%1</currentTask>").arg(taskState::CSRFToken(), state->taskState->taskFile);

    auto url = state->taskState->host + "/knossos/newTask/";

    setCursor(Qt::WaitCursor);
    bool success = taskState::httpFileGET(url.toUtf8().data(), postdata.toUtf8().data(), &response, &header, &httpCode, state->taskState->cookieFile.toUtf8().data(), &code, 5);
    setCursor(Qt::ArrowCursor);

    if (success && code == CURLE_OK && httpCode == 200) {
        saveAndLoadFile(header, response);

        // get task name
        const auto taskname = header.copyInfoFromHeader("taskname");
        state->taskState->taskName = taskname;
        setTask(state->taskState->taskName);

        // get task category description and task comment
        const auto descriptionBuffer = header.copyInfoFromHeader("description");
        const auto commentBuffer = header.copyInfoFromHeader("comment");
        QString description = QByteArray::fromBase64(descriptionBuffer.toUtf8());
        QString comment = QByteArray::fromBase64(commentBuffer.toUtf8());

        QMessageBox::information(this, state->taskState->taskName,
            QString("<p style='width:200px;'><b>Category %1:</b> %2<br><br><b>Task %3:</b> %4</p>")
               .arg(taskState::getCategory())
               .arg(description)
               .arg(taskState::getTask())
               .arg(comment));

        setDescription(description);
        setComment(comment);
    } else {
        handleError(this, success, code, httpCode, response.content);
    }

    free(header.content);
    free(response.content);
}

void TaskManagementWidget::setDescription(QString description) {
    categoryDescriptionLabel.setText(QString("<b>Category</b> <i>%1</i>: %2").arg(taskState::getCategory()).arg(description));
}

void TaskManagementWidget::setComment(QString comment) {
    taskCommentLabel.setText(QString("<b>Task</b> <i>%1</i>: %2").arg(taskState::getTask()).arg(comment));
}

void TaskManagementWidget::submitDialogOk() {
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
    submitDialogCommentField->clear();
}

void TaskManagementWidget::resetSession(QString message) {
    taskState::removeCookie();
    state->taskState->taskFile = "";
    taskLoginWidget->setResponse(message);
    setTask("");
    setActiveUser("");
    setDescription("");
    setComment("");

    hide();
    taskLoginWidget->show();
}

void TaskManagementWidget::setResponse(QString message) {
    statusLabel->setText(message);
}

void TaskManagementWidget::setActiveUser(QString username) {
    loggedAsLabel->setText("Logged in as: <font color='green'>" + username + "</font>");
}

void TaskManagementWidget::setTask(QString task) {
    state->taskState->taskName = task;
    currentTaskLabel->setText("Current task: <font color='green'>" + task + "</font>");
}
