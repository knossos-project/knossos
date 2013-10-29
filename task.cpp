#include <stdio.h>

#include <QFile>

#include "knossos-global.h"

extern struct stateInfo *state;

CURLcode taskState::httpGET(char *url, struct httpResponse *response, long *httpCode, char *cookiePath) {
    FILE *cookie;
    CURL *handle;
    CURLcode code;

    if(cookiePath) {
        cookie = fopen(cookiePath, "r");
        if(cookie == NULL) {
            return (CURLcode)-2;
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
    code = curl_easy_perform(handle); // send the request

    if(code == CURLE_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, httpCode);
    }
    curl_easy_cleanup(handle);
    return code;
}

CURLcode taskState::httpPOST(char *url, char *postdata, struct httpResponse *response, long *httpCode, char *cookiePath) {
    CURL *handle;
    CURLcode code;
    FILE *cookie;

    if(cookiePath) {
        cookie = fopen(cookiePath, "r");
        if(cookie == NULL) {
            return (CURLcode)-2;
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
    code = curl_easy_perform(handle); // send the request

    if(code == CURLE_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, httpCode);
    }
    curl_easy_cleanup(handle);
    return code;
}

CURLcode taskState::httpDELETE(char *url, struct httpResponse *response, long *httpCode, char *cookiePath) {
    CURL *handle;
    CURLcode code;
    FILE *cookie;

    if(cookiePath) {
        cookie = fopen(cookiePath, "r");
        if(cookie == NULL) {
            return (CURLcode)-2;
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
    code = curl_easy_perform(handle);

    if(code == CURLE_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, httpCode);
    }
    curl_easy_cleanup(handle);
    return code;
}

CURLcode taskState::httpFileGET(char *url, char *postdata, FILE *file, struct httpResponse *header, long *httpCode, char *cookiePath) {
    CURL *handle;
    CURLcode code;
    FILE *cookie;

    if(cookiePath) {
        cookie = fopen(cookiePath, "r");
        if(cookie == NULL) {
            return (CURLcode)-2;
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
    code = curl_easy_perform(handle);

    if(code == CURLE_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, httpCode);
    }
    curl_easy_cleanup(handle);
    return code;
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
    FILE *cookie;
    if(QFile::remove(state->taskState->cookieFile) != 0) {
        cookie = fopen(state->taskState->cookieFile, "w");
        if(cookie) {
            fclose(cookie);
        }
    }
}
