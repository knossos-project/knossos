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

#pragma once

#include <QApplication>
#include <QNetworkCookieJar>
#include <QNetworkAccessManager>
#include <QObject>
#include <QWidget>

QPair<bool, QByteArray> blockDownloadExtractData(QNetworkReply & reply);

template<typename T>
struct WidgetDisabler {
    T & w;
    QWidget * focusWidget;
    bool wasEnabled;
    explicit WidgetDisabler(T & w) : w{w}, wasEnabled{w.isEnabled()} {
        focusWidget = QApplication::focusWidget();
        w.setEnabled(false);
    }
    ~WidgetDisabler() {
        w.setEnabled(wasEnabled);
        if (focusWidget != nullptr) {
            focusWidget->setFocus();
        }
    }
};

class Network : public QObject {
    Q_OBJECT

public:
    QNetworkAccessManager manager;

private:
    QNetworkCookieJar cookieJar;

public:
    explicit Network(const QObject *parent = nullptr);
    void setCookies(const QVariantList & setting);
    QVariantList getCookiesForHost(const QUrl & host);

    static Network & singleton() {
        static Network & network = *new Network;
        return network;
    }

    QPair<bool, QString> login(const QUrl & url, const QString & username, const QString & password);
    QPair<bool, QString> refresh(const QUrl & url);
    QPair<bool, QPair<QString, QByteArray> > getFile(const QUrl & url);
    QPair<bool, QPair<QString, QByteArray> > getPost(const QUrl & url);
    QPair<bool, QString> submitHeidelbrain(const QUrl & url, const QString & filePath, const QString & comment, const bool final, const bool valid);
    QPair<bool, QString> rejectTask(const QUrl & url);
    void submitSegmentationJob(const QString &filename);
    std::pair<int, int> checkOnlineMags(const QUrl & url);

signals:
    void startedNetworkRequest(QNetworkReply & reply);
    void progressChanged(const qint64 value, const qint64 maxValue);
    void finishedNetworkRequest();
};
