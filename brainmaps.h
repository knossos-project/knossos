#include <QString>

std::pair<bool, QString> getBrainmapsToken();

#include "network.h"

template<bool block = true>
auto googleRequest = [](auto token, auto path, QByteArray payload = QByteArray{}){
    static QNetworkAccessManager qnam;
    QNetworkRequest request(path);
    request.setRawHeader("Authorization", (QString("Bearer ") + token).toUtf8());
    QNetworkReply * reply;
    if (payload != QByteArray{}) {
        request.setRawHeader("Content-Type", "application/octet-stream");
        reply = qnam.post(request, payload);
    } else {
        reply = qnam.get(request);
    }
    auto ret = blockDownloadExtractData(*reply, block);
    if constexpr (block) {
        return ret;
    } else {
        return reply;
    }
};
