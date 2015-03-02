#ifndef TASK_H
#define TASK_H

#include <QString>

#include <curl/curl.h>

struct httpResponse {
    char *content;
    size_t length;
    // for retrieving information from response headers. Useful for responses with file content
    // the information should be terminated with a ';' for sucessful parsing
    QString copyInfoFromHeader(const QString & info) const {
        QString value(content);
        value.remove(0, value.indexOf(info));//remove everything before the key
        value.remove(0, value.indexOf('=')+1);//remove key and equals sign
        return value.mid(0, value.indexOf(';'));//return everythin before the semicolon
    }
};

struct taskState {
    QString host;
    QString cookieFile;
    QString taskFile;
    QString taskName;

    static bool httpGET(char *url, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    static bool httpPOST(char *url, char *postdata, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    static bool httpDELETE(char *url, struct httpResponse *response, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    static bool httpFileGET(char *url, char *postdata, httpResponse *response, struct httpResponse *header, long *httpCode, char *cookiePath, CURLcode *code, long timeout);
    static size_t writeHttpResponse(void *ptr, size_t size, size_t nmemb, struct httpResponse *s);
    static size_t readFile(char *ptr, size_t size, size_t nmemb, void *stream);
    static void removeCookie();
    static QString CSRFToken();
    static QString getCategory();
    static QString getTask();
};

#endif//TASK_H
