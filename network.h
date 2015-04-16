#ifndef NETWORK_H
#define NETWORK_H

#include "loader.h"

#include <QNetworkCookieJar>
#include <QNetworkAccessManager>
#include <QThread>

struct FtpElement {
    bool isOverlay;
    C_Element *cube;
};

class Network {
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

    QString downloadFileProgressDialog(const QUrl & url, QWidget *parent);
    QPair<bool, QString> login(const QUrl & url, const QString & username, const QString & password);
    QPair<bool, QString> logout(const QUrl & url);
    QPair<bool, QString> refresh(const QUrl & url);
    QPair<bool, QPair<QString, QByteArray> > getFile(const QUrl & url);
    QPair<bool, QPair<QString, QByteArray> > getPost(const QUrl & url);
    QPair<bool, QString> submitHeidelbrain(const QUrl & url, const QString & filePath, const QString & comment, const bool final);
    void submitSegmentationJob(const QString &filename);
};

#endif // NETWORK_H
