#ifndef NETWORK_H
#define NETWORK_H

#include <QNetworkAccessManager>
#include <QThread>
#include "loader.h"

int downloadFile(const char *remote_path, char *local_filename);
std::string downloadRemoteConfFile(QString url);

struct FtpElement {
    bool isOverlay;
    C_Element *cube;
};

class Network {
    QNetworkAccessManager manager;
public:
    static Network & singleton() {
        static Network network;
        return network;
    }

    QString downloadFileProgressDialog(const QUrl & url, QWidget *parent);
    void submitSegmentationJob(const QString &filename);
};

#endif // NETWORK_H
