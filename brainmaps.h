#include <QString>

std::pair<bool, QString> getBrainmapsToken();

#include "network.h"

auto googleRequest = [](auto token, auto path, QByteArray payload = QByteArray{}){
    static QNetworkAccessManager qnam;
    QNetworkRequest request(path);
    request.setRawHeader("Authorization", (QString("Bearer ") + token).toUtf8());
    if (payload != QByteArray{}) {
        request.setRawHeader("Content-Type", "application/octet-stream");
        return blockDownloadExtractData(*qnam.post(request, payload));
    } else {
        return blockDownloadExtractData(*qnam.get(request));
    }
};
