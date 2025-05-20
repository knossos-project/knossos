/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "network.h"

#include "buildinfo.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/GuiConstants.h"
#include "widgets/mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QMessageBox>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QSemaphore>
#include <QSettings>
#include <QTimer>
#include <QUrlQuery>
#include <QVariantList>
#include <QXmlStreamWriter>

Network::Network(const QObject *) {
    manager.setCookieJar(&cookieJar);
    manager.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);// default is manual redirect
}

QVariantList Network::getCookiesForHost(const QUrl & host) {
    QVariantList cookies;
    for (const auto & cookie : cookieJar.cookiesForUrl(host)) {
        cookies.append(cookie.toRawForm());
    }
    return cookies;
}

void Network::setCookies(const QVariantList & setting) {
    for (const auto & cookie : setting) {
        cookieJar.insertCookie(QNetworkCookie::parseCookies(cookie.toByteArray()).front());
    }
}

std::pair<int, int> Network::checkOnlineMags(const QUrl & url) {
    std::size_t lowestAvailableMagIndex = NUM_MAG_DATASETS;
    std::size_t highestAvailableMagIndex = 0;
    const auto maxMagCount = static_cast<std::size_t>(NUM_MAG_DATASETS) + 1;

    std::vector<qint64> bytesReceivedAll(maxMagCount);
    std::vector<qint64> bytesTotalAll(maxMagCount);
    std::vector<std::unique_ptr<QNetworkReply>> replies(maxMagCount);// cleanup

    QEventLoop pause;
    int downloadCounter{0};
    for (std::size_t currMagIndex = 0; currMagIndex <= NUM_MAG_DATASETS; currMagIndex += 1) {
        QUrl magUrl = url;
        magUrl.setPath(QString("%1/mag%2/knossos.conf").arg(url.path()).arg(std::pow(2, currMagIndex)));
        auto * replyPtr = manager.get(QNetworkRequest{magUrl});
        replies[currMagIndex] = decltype(replies)::value_type{replyPtr};
        ++downloadCounter;
        QObject::connect(replyPtr, &QNetworkReply::finished, [magUrl, &pause, &downloadCounter, currMagIndex, replyPtr, &lowestAvailableMagIndex, &highestAvailableMagIndex]() {
            auto & reply = *replyPtr;
            if (reply.error() == QNetworkReply::NoError) {
                lowestAvailableMagIndex = std::min(lowestAvailableMagIndex, currMagIndex);
                highestAvailableMagIndex = std::max(highestAvailableMagIndex, currMagIndex);
            }
            if (--downloadCounter == 0) {// exit event loop after last download finished
                qDebug() << reply.errorString() << reply.readAll();
                pause.exit();
            }
        });
        auto processProgress = [this, currMagIndex, &bytesReceivedAll, &bytesTotalAll](qint64 bytesReceived, qint64 bytesTotal){
            bytesReceivedAll[static_cast<std::size_t>(currMagIndex)] = bytesReceived;
            bytesTotalAll[static_cast<std::size_t>(currMagIndex)] = bytesTotal;
            const auto received = std::accumulate(std::begin(bytesReceivedAll), std::end(bytesReceivedAll), qint64{0});
            const auto total = std::accumulate(std::begin(bytesTotalAll), std::end(bytesTotalAll), qint64{0});
            emit Network::progressChanged(received, total);
        };
        emit Network::singleton().startedNetworkRequest(*replyPtr);// register abort
        QObject::connect(replyPtr, &QNetworkReply::downloadProgress, processProgress);
        QObject::connect(replyPtr, &QNetworkReply::uploadProgress, processProgress);
    }

    state->viewer->suspend([&pause](){
        return pause.exec();
    });
    emit Network::singleton().finishedNetworkRequest();

    if (lowestAvailableMagIndex > highestAvailableMagIndex) {
        throw std::runtime_error{"no mags detected"};
    }

    return {lowestAvailableMagIndex, highestAvailableMagIndex};
}

QPair<bool, QByteArray> blockDownloadExtractData(QNetworkReply & reply, QNetworkAccessManager * manager) {
    QEventLoop pause;
    auto replyRef = std::ref(reply);
    if (manager != nullptr && !reply.request().url().path().endsWith("/auth")) {
        QObject::connect(&reply, &QNetworkReply::finished, &pause, [&replyRef, &reply, &pause, manager](){
            if (reply.error() == QNetworkReply::ContentAccessDenied || reply.error() == QNetworkReply::AuthenticationRequiredError) {
                auto authrequest = reply.request();
                QUrl authurl = authrequest.url();
                authurl.setQuery(QString{"path=%1"}.arg(authurl.path()));
                authurl.setPath("/auth");
                authrequest.setUrl(authurl);
                qDebug() << "cdn auth" << authurl << "isValid" << authurl.isValid();
                auto & tokenReply = *manager->get(authrequest);

                QObject::disconnect(&reply , &QNetworkReply::finished, &pause, &QEventLoop::quit);
                QObject::connect(&tokenReply, &QNetworkReply::finished, &pause, [&replyRef, &tokenReply, &pause, request=reply.request(), manager]()mutable{
                    qDebug() << tokenReply.errorString();
                    if (tokenReply.error() == QNetworkReply::NoError) {
                        QUrl url = request.url();
                        url.setQuery(QJsonDocument::fromJson(tokenReply.readAll())["token_string"].toString());
                        request.setUrl(url);
                        auto & retryReply = *manager->get(request);
                        QObject::   connect(&retryReply, &QNetworkReply::finished, &pause, &QEventLoop::quit);
                        replyRef = retryReply;
                    } else {
                        pause.quit();
                    }
                });
            }
        });
    }
    QObject::connect(&reply, &QNetworkReply::finished, &pause, &QEventLoop::quit);
    emit Network::singleton().startedNetworkRequest(reply);
    QObject::connect(&reply, &QNetworkReply::downloadProgress, &Network::singleton(), &Network::progressChanged);
    QObject::connect(&reply, &QNetworkReply::uploadProgress, &Network::singleton(), &Network::progressChanged);

    if (!reply.isFinished()) {// workaround for replies that don’t notify their completion
        state->viewer->suspend([&pause](){
            return pause.exec();
        });
    }
    {
        auto & reply = replyRef.get();

        if (reply.error() != QNetworkReply::NoError) {
            qDebug() << reply.attribute(QNetworkRequest::HttpStatusCodeAttribute) << reply.error() << reply.errorString();
        }
        emit Network::singleton().finishedNetworkRequest();
        reply.deleteLater();
        return {reply.error() == QNetworkReply::NoError, reply.readAll()};
    }
}

// for retrieving information from response headers. Useful for responses with file content
// the information should be terminated with a ';' for sucessful parsing
QString copyInfoFromHeader(const QString & header, const QString & info) {
    const auto begin = header.indexOf('=', header.indexOf(info)) + 1;//index after key and equals sign
    const auto length = header.indexOf(';', begin) - begin;//count to next semicolon
    return header.mid(begin, length);
}

QPair<bool, QPair<QString, QByteArray>> blockDownloadExtractFilenameAndData(QNetworkReply & reply) {
    auto res = blockDownloadExtractData(reply);
    //QNetworkRequest::ContentDispositionHeader is broken – https://bugreports.qt.io/browse/QTBUG-45610
    auto filename = copyInfoFromHeader(reply.rawHeader("Content-Disposition"), "filename");
    return {res.first, {filename, res.second}};
}

QPair<bool, QString> Network::refresh(const QUrl & url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QApplication::applicationName() + "/" + KREVISION);
    auto & reply = *manager.get(request);
    return blockDownloadExtractData(reply, &manager);
}

QPair<bool, QString> Network::login(const QUrl & url, const QString & username, const QString & password) {
    QString postdata;
    QXmlStreamWriter writer{&postdata};
    writer.writeStartElement("login");
    writer.writeTextElement("username", username);
    writer.writeTextElement("password", password);
    writer.writeEndElement();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QApplication::applicationName() + "/" + KREVISION);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml; charset=utf-8");
    auto & reply = *manager.post(request, postdata.toUtf8());
    return blockDownloadExtractData(reply);
}

template<typename T>
QByteArray getCSRF(const T & cookies) {
    for (const auto & cookie : cookies) {
        if (cookie.name() == "csrftoken") {
            return cookie.value();
        }
    }
    qCritical() << "no »csrftoken« cookie. available cookies: " << cookies;
    return {};
}

QPair<bool, QPair<QString, QByteArray>> Network::getFile(const QUrl & url) {
    auto & reply = *manager.get(QNetworkRequest(url));
    return blockDownloadExtractFilenameAndData(reply);
}

QHttpPart addFormDataPart(const QString & header, const QByteArray & body) {
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form_data; name=\"%1\"").arg(header));
    part.setBody(body);
    return part;
}

QPair<bool, QPair<QString, QByteArray>> Network::getPost(const QUrl & url) {
    QHttpMultiPart multipart(QHttpMultiPart::FormDataType);
    multipart.append(addFormDataPart("csrfmiddlewaretoken", getCSRF(cookieJar.cookiesForUrl(url))));
    multipart.append(addFormDataPart("data", QString("<currentTask>%1</currentTask>").arg("foo").toUtf8()));
    QNetworkRequest request(url);
    request.setRawHeader("Referer", url.toString(QUrl::PrettyDecoded | QUrl::RemovePath).toUtf8());
    auto & reply = *manager.post(request, &multipart);
    return blockDownloadExtractFilenameAndData(reply);
}

QPair<bool, QString> Network::submitHeidelbrain(const QUrl & url, const QString & filePath, const QString & comment, const bool final, const bool valid) {
    if (manager.cookieJar()->cookiesForUrl(url).empty()) {
        return {false, "no cookie"};
    }

    QFile uploadfile(filePath);
    uploadfile.open(QIODevice::ReadOnly);
    const auto filename = QFileInfo(filePath).fileName();

    QHttpPart file_part;
    file_part.setHeader(QNetworkRequest::ContentDispositionHeader,
        QString("form_data; name=\"submit_work_file\"; filename=\"") + filename + '\"');
    file_part.setBodyDevice(&uploadfile);

    QHttpMultiPart multipart(QHttpMultiPart::FormDataType);
    multipart.append(addFormDataPart("csrfmiddlewaretoken", getCSRF(cookieJar.cookiesForUrl(url))));
    multipart.append(file_part);
    multipart.append(addFormDataPart("filename", filename.toUtf8()));
    multipart.append(addFormDataPart("submit_comment", comment.toUtf8()));
    multipart.append(addFormDataPart("submit_work_is_final", final ? "True" : "False"));
    multipart.append(addFormDataPart("is_valid", valid ? "True" : "False"));

    QNetworkRequest request(url);
    request.setRawHeader("Referer", url.toString(QUrl::PrettyDecoded | QUrl::RemovePath).toUtf8());
    auto & reply = *manager.post(request, &multipart);
    return blockDownloadExtractData(reply);
}

QPair<bool, QString> Network::rejectTask(const QUrl & url) {
    QUrlQuery data;
    const auto csrfcookie = getCSRF(cookieJar.cookiesForUrl(url));
    data.addQueryItem("csrfmiddlewaretoken", csrfcookie);
    data.addQueryItem("delete", "");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Referer", url.toString(QUrl::PrettyDecoded | QUrl::RemovePath).toUtf8());
    auto & reply = *manager.post(request, data.toString().toUtf8());
    return blockDownloadExtractData(reply);
}

void Network::submitSegmentationJob(const QString & path) {
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/zip"));
    // Django requires enctype="multipart/form-data" for receiving file uploads
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                  QVariant("enctype=\"multipart/form-data\"; name=\"submit\"; filename=\"" + QFileInfo(path).fileName() + "\""));
    auto uploadfile = new QFile(path);
    uploadfile->open(QIODevice::ReadOnly);
    part.setBodyDevice(uploadfile);

    auto multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->append(part);
    uploadfile->setParent(multiPart);
    auto & job = Segmentation::singleton().job;
    QNetworkRequest request(QUrl(job.submitPath));
    auto reply = manager.post(request, multiPart);
    multiPart->setParent(reply);
    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        QString content = (reply->error() == QNetworkReply::NoError) ? reply->readAll() : reply->errorString();
        QMessageBox verificationBox{QApplication::activeWindow()};
        verificationBox.setIcon(QMessageBox::Information);
        verificationBox.setText(QObject::tr("Your verification"));
        verificationBox.setInformativeText(content);
        verificationBox.exec();
        reply->deleteLater();
    });
}
