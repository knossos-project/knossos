/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "network.h"

#include "loader.h"
#include "stateInfo.h"
#include "version.h"
#include "widgets/GuiConstants.h"
#include "widgets/mainwindow.h"

#include <QDir>
#include <QEventLoop>
#include <QHttpMultiPart>
#include <QMessageBox>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QSemaphore>
#include <QSettings>
#include <QTimer>
#include <QVariantList>

Network::Network(const QObject *) {
    manager.setCookieJar(&cookieJar);
}

QVariantList Network::getCookiesForHost(const QString & host) {
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
    int lowestAvailableMag = NUM_MAG_DATASETS;
    int highestAvailableMag = 0;
    int maxMagCount = int_log(NUM_MAG_DATASETS) + 1;

    QProgressDialog progress("Network Operation…", "Abort", 0, 100, state->mainWindow);
    progress.setModal(true);
    std::vector<qint64> bytesReceivedAll(maxMagCount);
    std::vector<qint64> bytesTotalAll(maxMagCount);
    std::vector<std::unique_ptr<QNetworkReply>> replies(maxMagCount);
    QTimer::singleShot(400, &progress, &QProgressDialog::show);

    QEventLoop pause;
    for (int currMag = 1; currMag <= NUM_MAG_DATASETS; currMag *= 2) {
        QUrl magUrl = url;
        magUrl.setPath(QString("%1/mag%2/knossos.conf").arg(url.path()).arg(currMag));
        auto * replyPtr = manager.get(QNetworkRequest{magUrl});
        replies[int_log(currMag)] = decltype(replies)::value_type{replyPtr};
        QObject::connect(replyPtr, &QNetworkReply::finished, [magUrl, &pause, &maxMagCount, currMag, replyPtr, &lowestAvailableMag, &highestAvailableMag]() {
            auto & reply = *replyPtr;
            if (reply.error() == QNetworkReply::NoError) {
                lowestAvailableMag = std::min(lowestAvailableMag, currMag);
                highestAvailableMag = std::max(highestAvailableMag, currMag);
            }
            if (--maxMagCount == 0) {
                pause.exit();
            }
        });
        auto processProgress = [&progress, currMag, &bytesReceivedAll, &bytesTotalAll](qint64 bytesReceived, qint64 bytesTotal){
            bytesReceivedAll[int_log(currMag)] = bytesReceived;
            bytesTotalAll[int_log(currMag)] = bytesTotal;
            const auto received = std::accumulate(std::begin(bytesReceivedAll), std::end(bytesReceivedAll), qint64{0});
            const auto total = std::accumulate(std::begin(bytesTotalAll), std::end(bytesTotalAll), qint64{0});
            progress.setRange(0, total);
            progress.setValue(received);
        };
        QObject::connect(replyPtr, &QNetworkReply::downloadProgress, processProgress);
        QObject::connect(replyPtr, &QNetworkReply::uploadProgress, processProgress);
        QObject::connect(&progress, &QProgressDialog::canceled, replyPtr, &QNetworkReply::abort);
    }

    pause.exec();

    if (lowestAvailableMag > highestAvailableMag) {
        throw std::runtime_error{"no mags detected"};
    }

    return {lowestAvailableMag, highestAvailableMag};
}

QPair<bool, QByteArray> blockDownloadExtractData(QNetworkReply & reply) {
    QEventLoop pause;
    QObject::connect(&reply, &QNetworkReply::finished, [&pause]() {
        pause.exit();
    });

    QProgressDialog progress("Network Operation…", "Abort", 0, 100, state->mainWindow);
    progress.setModal(true);
    QObject::connect(&progress, &QProgressDialog::canceled, &reply, &QNetworkReply::abort);
    auto processProgress = [&progress](qint64 bytesReceived, qint64 bytesTotal){
        progress.setRange(0, bytesTotal);
        progress.setValue(bytesReceived);
    };
    QObject::connect(&reply, &QNetworkReply::downloadProgress, processProgress);
    QObject::connect(&reply, &QNetworkReply::uploadProgress, processProgress);
    QTimer::singleShot(400, &progress, &QProgressDialog::show);

    pause.exec();

    if (reply.error() != QNetworkReply::NoError) {
        qDebug() << reply.errorString();
    }
    reply.deleteLater();
    return {reply.error() == QNetworkReply::NoError, reply.readAll()};
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
    auto & reply = *manager.get(QNetworkRequest(url));
    return blockDownloadExtractData(reply);
}

QPair<bool, QString> Network::login(const QUrl & url, const QString & username, const QString & password) {
    const auto postdata = QString("<login><username>%1</username><password>%2</password><knossos_version>%3</knossos_version></login>").arg(username).arg(password).arg(KVERSION);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain; charset=utf-8");
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
    auto & reply = *manager.post(QNetworkRequest(url), &multipart);
    return blockDownloadExtractFilenameAndData(reply);
}

QPair<bool, QString> Network::submitHeidelbrain(const QUrl & url, const QString & filePath, const QString & comment, const bool final) {
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

    auto & reply = *manager.post(QNetworkRequest(url), &multipart);
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
       QMessageBox verificationBox(QMessageBox::Information, "Your verification", content);
       verificationBox.exec();
       reply->deleteLater();
   });
}
