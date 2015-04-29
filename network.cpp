#include "network.h"

#include "widgets/GuiConstants.h"
#include "loader.h"
#include "stateInfo.h"

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

QPair<bool, QByteArray> blockDownloadExtractData(QNetworkReply & reply) {
    QEventLoop pause;
    QObject::connect(&reply, &QNetworkReply::finished, [&pause]() {
       pause.exit();
    });

    QProgressDialog progress("Downloading files...", "Abort", 0, 100);
    progress.setModal(true);
    QObject::connect(&progress, &QProgressDialog::canceled, &reply, &QNetworkReply::abort);
    QObject::connect(&reply, &QNetworkReply::downloadProgress, &progress, &QProgressDialog::setValue);

    QTimer timer;
    timer.setInterval(400);
    QObject::connect(&timer, &QTimer::timeout, &progress, &QProgressDialog::show);

    pause.exec();

    if (reply.error() != QNetworkReply::NoError) {
        qDebug() << reply.errorString();
    }
    reply.deleteLater();
    return {reply.error() == QNetworkReply::NoError, reply.readAll()};
}

// for retrieving information from response headers. Useful for responses with file content
// the information should be terminated with a ';' for sucessful parsing
QString copyInfoFromHeader(QString header, const QString & info) {
    header.remove(0, header.indexOf(info));//remove everything before the key
    header.remove(0, header.indexOf('=')+1);//remove key and equals sign
    return header.mid(0, header.indexOf(';'));//return everything before the semicolon
}

QPair<bool, QPair<QString, QByteArray>> blockDownloadExtractFilenameAndData(QNetworkReply & reply) {
    auto res = blockDownloadExtractData(reply);
    auto filename = copyInfoFromHeader(reply.rawHeader("Content-Disposition"), "filename");//QNetworkRequest::ContentDispositionHeader is broken
    return {res.first, {filename, res.second}};
}

QPair<bool, QString> Network::refresh(const QUrl & url) {
    auto & reply = *manager.get(QNetworkRequest(url));
    return blockDownloadExtractData(reply);
}

QPair<bool, QString> Network::login(const QUrl & url, const QString & username, const QString & password) {
    const auto postdata = QString("<login><username>%1</username><password>%2</password></login>").arg(username, password);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain; charset=utf-8");
    auto & reply = *manager.post(request, postdata.toUtf8());
    return blockDownloadExtractData(reply);
}

QPair<bool, QString> Network::logout(const QUrl & url) {
    auto & reply = *manager.deleteResource(QNetworkRequest(url));
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
    multipart.append(addFormDataPart("submit_work_isfinal", final ? "True" : "False"));

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
