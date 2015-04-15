#ifndef NETWORK_H
#define NETWORK_H

#include "loader.h"

#include <QNetworkCookieJar>
#include <QNetworkAccessManager>
#include <QThread>

int downloadFile(const char *remote_path, char *local_filename);
std::string downloadRemoteConfFile(QString url);

struct FtpElement {
    bool isOverlay;
    C_Element *cube;
};

class FtpThread : public QThread
{
public:
    FtpThread(void *ctx);
    void run();
protected:
    void *ctx;
};

class Network : public QObject {
Q_OBJECT
    QNetworkAccessManager manager;
    QNetworkCookieJar cookieJar;

public:
    explicit Network(const QObject *parent = nullptr);
    void setCookies(const QVariantList & setting);
    QVariantList getCookiesForHost(const QString & host);

    static Network & singleton() {
        static Network network;
        return network;
    }

    QPair<bool, QString> login(const QUrl & url, const QString & username, const QString & password);
    QPair<bool, QString> logout(const QUrl & url);
    QPair<bool, QString> refresh(const QUrl & url);
    QPair<bool, QPair<QString, QByteArray> > getFile(const QUrl & url);
    QPair<bool, QPair<QString, QByteArray> > getPost(const QUrl & url);
    QPair<bool, QString> submitHeidelbrain(const QUrl & url, const QString & filePath, const QString & comment, const bool final);
    void submitSegmentationJob(const QString &filename);
};

#endif // NETWORK_H
