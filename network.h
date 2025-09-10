#pragma once

#include <QApplication>
#include <QByteArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPair>
#include <QString>
#include <QWidget>
#include <QtCore/QObject>
#include <QtCore/QtGlobal>
#include <QtGlobal>

#include <iterator>
#include <utility>

class QNetworkReply;
class QUrl;

QPair<bool, QByteArray> blockDownloadExtractData(QNetworkReply& reply,
                                                 QNetworkAccessManager* manager = nullptr);

template <typename T> struct WidgetDisabler {
    T& w;
    QWidget* focusWidget;
    bool wasEnabled;
    explicit WidgetDisabler(T& w) :
        w{w},
        wasEnabled{w.isEnabled()} {
        focusWidget = QApplication::focusWidget();
        if (wasEnabled) { // don't explicitely disable a disabled widget
            w.setEnabled(false);
        }
    }
    ~WidgetDisabler() {
        if (wasEnabled) { // don't explicitely disable a disabled widget
            w.setEnabled(true);
        }
        if (focusWidget != nullptr) {
            focusWidget->setFocus();
        }
    }
};

class Network : public QObject {
    Q_OBJECT

  public:
    QNetworkAccessManager manager;

    explicit Network(const QObject* parent = nullptr);
    static Network& singleton() {
        static Network& network = *new Network;
        return network;
    }

    QPair<bool, QByteArray> login(const QUrl& url, const QString& username,
                                  const QString& password);
    QPair<bool, QByteArray> refresh(const QUrl& url);
    QPair<bool, QByteArray> send(const QUrl& url, const QString& method,
                                 const QString& contentTypeHeader, const QByteArray& data);
    std::pair<int, int> checkOnlineMags(const QUrl& url);

  signals:
    void startedNetworkRequest(QNetworkReply& reply);
    void progressChanged(const qint64 value, const qint64 maxValue);
    void finishedNetworkRequest();
};
