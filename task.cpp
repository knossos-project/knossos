#include <stdio.h>

#include <QFile>

#include "knossos-global.h"

extern struct stateInfo *state;

// for looking up CURLcode: http://curl.haxx.se/libcurl/c/libcurl-errors.html

bool taskState::httpGET(char *url, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code) {
    FILE *cookie;
    CURL *handle;

    if(cookiePath) {
        cookie = fopen(cookiePath, "r");
        if(cookie == NULL) {
            qDebug("no cookie");
            return false;
        }
        fclose(cookie);
    }

    handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url); // send to this url
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L); // follow in case of redirect
    if(cookiePath) {
        curl_easy_setopt(handle, CURLOPT_COOKIEFILE, cookiePath); // send this cookie to authenticate oneself
    }
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeHttpResponse); // use this function to write the response into struct
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, response); // write response into this struct
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5LL);
    *code = curl_easy_perform(handle); // send the request

    if(*code == CURLE_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, httpCode);
        curl_easy_cleanup(handle);
        return true;
    }
    else {
        qDebug("curle not ok. %i", *code);
        curl_easy_cleanup(handle);
        return false;
    }
}

bool taskState::httpPOST(char *url, char *postdata, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code) {
    CURL *handle;
    FILE *cookie;

    if(cookiePath) {
        cookie = fopen(cookiePath, "r");
        if(cookie == NULL) {
            return false;
        }
    }

    handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url); // send to this url
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postdata); // send this post data
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L); // follow in case of redirect
    if(cookiePath) {
        curl_easy_setopt(handle, CURLOPT_COOKIEJAR, cookiePath);
    }
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeHttpResponse); // use this function to write the response into struct
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, response); // write response into this struct
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5LL);
    *code = curl_easy_perform(handle); // send the request

    if(*code == CURLE_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, httpCode);
        curl_easy_cleanup(handle);
        return true;
    }
    curl_easy_cleanup(handle);
    return true;
}

bool taskState::httpDELETE(char *url, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code) {
    CURL *handle;
    FILE *cookie;

    if(cookiePath) {
        cookie = fopen(cookiePath, "r");
        if(cookie == NULL) {
            return true; // no cookie means logged out anyway
        }
    }

    handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    if(cookiePath) {
        curl_easy_setopt(handle, CURLOPT_COOKIEFILE, cookiePath);
    }
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeHttpResponse);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5LL);
    *code = curl_easy_perform(handle);

    if(*code == CURLE_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, httpCode);
        curl_easy_cleanup(handle);
        return true;
    }
    curl_easy_cleanup(handle);
    return false;
}

bool taskState::httpFileGET(char *url, char *postdata, FILE *file, struct httpResponse *header, long *httpCode, char *cookiePath, CURLcode *code) {
    CURL *handle;
    FILE *cookie;

    if(cookiePath) {
        cookie = fopen(cookiePath, "r");
        if(cookie == NULL) {
            return false;
        }
        fclose(cookie);
    }

    handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    if(cookiePath) {
        curl_easy_setopt(handle, CURLOPT_COOKIEFILE, cookiePath);
    }
    if(postdata) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postdata);
    }
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToFile);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, writeHttpResponse);
    curl_easy_setopt(handle, CURLOPT_WRITEHEADER, header);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5LL);
    *code = curl_easy_perform(handle);

    if(*code == CURLE_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, httpCode);
    }
    curl_easy_cleanup(handle);
    return true;
}

// for writing http response to memory
size_t taskState::writeHttpResponse(void *ptr, size_t size, size_t nmemb, struct httpResponse *s) {
    size_t new_len = s->length + size*nmemb;
    s->content = (char*)realloc(s->content, new_len+1);
    memcpy(s->content + s->length, ptr, size*nmemb);
    s->content[new_len] = '\0';
    s->length = new_len;

    return size*nmemb;
}

// for writing http response to file
size_t taskState::writeToFile(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

// for sending file contents over http request
size_t taskState::readFile(char *ptr, size_t size, size_t nmemb, void *stream) {
    return fread(ptr, size, nmemb, (FILE*)stream);
}

// for retrieving information from response headers. Useful for responses with file content
// the information should be terminated with an ';' for sucessful parsing
int taskState::copyInfoFromHeader(char *dest, struct httpResponse *header, char *info) {
    int i, numChars = 0;
    char *pos = strstr(header->content, info);
    if(pos == NULL) {
        return false;
    }

    for(i = 0; i <= strlen(pos + strlen(info)+1); i++) { // +1 for "="
        if((pos + strlen(info)+1)[i] == ';') {
            numChars = i;
            break;
        }
    }

    strncpy(dest, pos + strlen(info)+1, numChars);
    return true;
}

void taskState::removeCookie() {
    if(remove(state->taskState->cookieFile) != 0) {
        perror("Failed to delete file.");
    }
}

const char *taskState::CSRFToken() {
    QFile cookie(state->taskState->cookieFile);
    if(cookie.exists() == false) {
        return NULL;
    }
    if(cookie.open(QIODevice::ReadOnly|QIODevice::Text) == false) {
        qDebug("failed to open cookie file");
        return NULL;
    }
    QString content;
    QTextStream in(&cookie);
    int index;
    while(in.atEnd() == false) {
        content = in.readLine();
        if((index = content.indexOf("csrftoken")) == -1) {
            continue;
        }
        return content.mid(index + strlen("csrftoken ")).toStdString().c_str();
    }
    qDebug("no csrf token found!");
    return NULL;
}

QString taskState::getCategory() {
    QString taskName = state->taskState->taskName;
    int index = taskName.indexOf("/");
    if(index != -1) {
        return taskName.mid(0, index-1);
    }
    return NULL;
}

QString taskState::getTask() {
    QString taskName = state->taskState->taskName;
    int index = taskName.indexOf("/");
    if(index != -1) {
        return taskName.mid(index+2);
    }
    return NULL;
}
