#include "network.h"

#include "buildinfo.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVariant>
#include <QtCore/qglobal.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

Network::Network(const QObject*) {
    manager.setRedirectPolicy(
        QNetworkRequest::NoLessSafeRedirectPolicy); // default is manual redirect
}

std::pair<int, int> Network::checkOnlineMags(const QUrl& url) {
    int lowestAvailableMag = NUM_MAG_DATASETS;
    int highestAvailableMag = 0;
    const auto maxMagCount = static_cast<std::size_t>(std::log2(NUM_MAG_DATASETS)) + 1;

    std::vector<qint64> bytesReceivedAll(maxMagCount);
    std::vector<qint64> bytesTotalAll(maxMagCount);
    std::vector<std::unique_ptr<QNetworkReply>> replies(maxMagCount); // cleanup

    QEventLoop pause;
    int downloadCounter{0};
    for (int currMag = 1; currMag <= NUM_MAG_DATASETS; currMag *= 2) {
        QUrl magUrl = url;
        magUrl.setPath(QString("%1/mag%2/knossos.conf").arg(url.path()).arg(currMag));
        auto* replyPtr = manager.get(QNetworkRequest{magUrl});
        replies[static_cast<std::size_t>(std::log2(currMag))] =
            decltype(replies)::value_type{replyPtr};
        ++downloadCounter;
        QObject::connect(replyPtr, &QNetworkReply::finished,
                         [magUrl, &pause, &downloadCounter, currMag, replyPtr, &lowestAvailableMag,
                          &highestAvailableMag]() {
                             auto& reply = *replyPtr;
                             if (reply.error() == QNetworkReply::NoError) {
                                 lowestAvailableMag = std::min(lowestAvailableMag, currMag);
                                 highestAvailableMag = std::max(highestAvailableMag, currMag);
                             }
                             if (--downloadCounter ==
                                 0) { // exit event loop after last download finished
                                 qDebug() << reply.errorString() << reply.readAll();
                                 pause.exit();
                             }
                         });
        auto processProgress = [this, currMag, &bytesReceivedAll,
                                &bytesTotalAll](qint64 bytesReceived, qint64 bytesTotal) {
            bytesReceivedAll[static_cast<std::size_t>(std::log2(currMag))] = bytesReceived;
            bytesTotalAll[static_cast<std::size_t>(std::log2(currMag))] = bytesTotal;
            const auto received = std::accumulate(std::begin(bytesReceivedAll),
                                                  std::end(bytesReceivedAll), qint64{0});
            const auto total =
                std::accumulate(std::begin(bytesTotalAll), std::end(bytesTotalAll), qint64{0});
            emit Network::progressChanged(received, total);
        };
        emit Network::singleton().startedNetworkRequest(*replyPtr); // register abort
        QObject::connect(replyPtr, &QNetworkReply::downloadProgress, processProgress);
        QObject::connect(replyPtr, &QNetworkReply::uploadProgress, processProgress);
    }

    state->viewer->suspend([&pause]() { return pause.exec(); });
    emit Network::singleton().finishedNetworkRequest();

    if (lowestAvailableMag > highestAvailableMag) {
        throw std::runtime_error{"no mags detected"};
    }

    return {lowestAvailableMag, highestAvailableMag};
}

QPair<bool, QByteArray> blockDownloadExtractData(QNetworkReply& reply,
                                                 QNetworkAccessManager* manager) {
    QEventLoop pause;
    auto lastDataReply = &reply;
    if (manager != nullptr && !reply.request().url().path().endsWith("/auth")) {
        // if the request failed for an auth/access error, we want to get an auth token for that
        // path then retry
        QObject::connect(
            &reply, &QNetworkReply::finished, &pause, [&lastDataReply, &pause, manager] {
                if (lastDataReply->error() == QNetworkReply::ContentAccessDenied ||
                    lastDataReply->error() == QNetworkReply::AuthenticationRequiredError) {
                    auto authrequest = lastDataReply->request();
                    QUrl authurl = authrequest.url();
                    authurl.setQuery(QString{"path=%1"}.arg(authurl.path()));
                    authurl.setPath("/auth");
                    authrequest.setUrl(authurl);
                    qDebug() << "cdn auth" << authurl << "isValid" << authurl.isValid();
                    auto& tokenReply = *manager->get(authrequest);

                    QObject::connect(
                        &tokenReply, &QNetworkReply::finished, &pause,
                        [&lastDataReply, &tokenReply, &pause, manager] {
                            auto request = lastDataReply->request();
                            if (tokenReply.error() == QNetworkReply::NoError) {
                                QUrl url = request.url();
                                url.setQuery(
                                    QJsonDocument::fromJson(tokenReply.readAll())["token_string"]
                                        .toString());
                                request.setUrl(url);
                                lastDataReply->deleteLater(); // schedule deletion now to point to a
                                                              // new reply
                                lastDataReply = manager->get(request);
                                QObject::connect(lastDataReply, &QNetworkReply::finished, &pause,
                                                 &QEventLoop::quit);
                                QObject::connect(lastDataReply, &QNetworkReply::downloadProgress,
                                                 &Network::singleton(), &Network::progressChanged);
                                QObject::connect(lastDataReply, &QNetworkReply::uploadProgress,
                                                 &Network::singleton(), &Network::progressChanged);
                            } else {
                                qWarning() << "error while fetching tha auth token:"
                                           << tokenReply.errorString();
                                pause.quit();
                            }
                            tokenReply.deleteLater();
                        });
                } else {
                    pause.quit();
                }
            });
    } else {
        QObject::connect(lastDataReply, &QNetworkReply::finished, &pause, &QEventLoop::quit);
        QObject::connect(lastDataReply, &QNetworkReply::downloadProgress, &Network::singleton(),
                         &Network::progressChanged);
        QObject::connect(lastDataReply, &QNetworkReply::uploadProgress, &Network::singleton(),
                         &Network::progressChanged);
    }
    emit Network::singleton().startedNetworkRequest(*lastDataReply);
    if (!lastDataReply->isFinished()) { // workaround for replies that don’t notify their completion
        state->viewer->suspend([&pause]() { return pause.exec(); });
    }
    emit Network::singleton().finishedNetworkRequest();

    if (lastDataReply->error() != QNetworkReply::NoError) {
        qDebug() << lastDataReply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                 << lastDataReply->error() << lastDataReply->errorString();
    }
    lastDataReply->deleteLater();
    return {lastDataReply->error() == QNetworkReply::NoError, lastDataReply->readAll()};
}

QPair<bool, QByteArray> Network::refresh(const QUrl& url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QApplication::applicationName() + "/" + KREVISION);
    auto& reply = *manager.get(request);
    return blockDownloadExtractData(reply, &manager);
}

QPair<bool, QByteArray> Network::login(const QUrl& url, const QString& username,
                                       const QString& password) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QApplication::applicationName() + "/" + KREVISION);
    QString authorizationData = "Basic " + QString{username + ":" + password}.toUtf8().toBase64();
    request.setRawHeader("Authorization", authorizationData.toUtf8());
    auto& reply = *manager.get(request);
    return blockDownloadExtractData(reply);
}

QPair<bool, QByteArray> Network::send(const QUrl& url, const QString& method,
                                      const QString& contentTypeHeader, const QByteArray& data) {
    if (method != "post" && method != "put") {
        return QPair(false, QByteArray());
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentTypeHeader);
    QNetworkReply* reply;
    if (method == "post") {
        reply = manager.post(request, data);
    } else {
        reply = manager.put(request, data);
    }
    return blockDownloadExtractData(*reply);
}
