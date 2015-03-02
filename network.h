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

public:
    explicit Network(const QObject *parent = nullptr);

    static Network & singleton() {
        static Network network;
        return network;
    }

    void submitSegmentationJob(const QString &filename);
};

#endif // NETWORK_H
