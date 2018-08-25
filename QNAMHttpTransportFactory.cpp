#include "QNAMHttpTransportFactory.h"

#include "network.h"

#include "googleapis/strings/stringpiece.h"
#include "googleapis/client/data/data_reader.h"
#include "googleapis/client/data/data_writer.h"
#include "googleapis/client/transport/http_request.h"

#include <QNetworkReply>

#include <limits>

class QNAMHttpTransport : public googleapis::client::HttpTransport {
public:
    QNAMHttpTransport(const googleapis::client::HttpTransportOptions & options);
    virtual googleapis::client::HttpRequest * NewHttpRequest(const googleapis::client::HttpRequest::HttpMethod & method) override;
};

class QNAMHttpRequest : public googleapis::client::HttpRequest {
public:
    QNAMHttpRequest(const HttpMethod & method, QNAMHttpTransport * transport)
            : HttpRequest{method, transport} {}

protected:
    virtual void DoExecute(googleapis::client::HttpResponse * response) {
        auto request = QNetworkRequest{QString::fromStdString(url())};
        for (const auto & elem : headers()) {
            request.setRawHeader(QByteArray::fromStdString(elem.first), QByteArray::fromStdString(elem.second));
        }
        if (http_method() != "POST") {
            qWarning() << "methods other than POST not implemented";
        }

        std::string content;
        content_reader()->ReadToString(std::numeric_limits<int64_t>::max(), &content);
        auto & reply = *Network::singleton().manager.post(request, QByteArray::fromStdString(content));
        auto data = blockDownloadExtractData(reply);

        response->set_http_code(reply.error());
        response->body_writer()->Begin();
        response->body_writer()->Write(googleapis::StringPiece{data.second}).IgnoreError();
        response->body_writer()->End();
        if (!response->body_writer()->ok()) {
            LOG(ERROR) << "Error writing response body";
            mutable_state()->set_transport_status(response->body_writer()->status());
        }

        for (auto && elem : reply.rawHeaderPairs()) {
            response->AddHeader(elem.first.toStdString(), elem.second.toStdString());
        }

        mutable_state()->set_http_code(response->http_code());
        const auto errorStatus = googleapis::util::Status(googleapis::util::error::UNKNOWN, reply.errorString().toStdString());
        mutable_state()->set_transport_status(reply.error() == QNetworkReply::NoError ? googleapis::client::StatusOk() : errorStatus);

        qDebug() << "done request" << url().c_str() << data.first << data.second;
    }
};

QNAMHttpTransport::QNAMHttpTransport(const googleapis::client::HttpTransportOptions & options)
    : googleapis::client::HttpTransport{options} {}

googleapis::client::HttpRequest * QNAMHttpTransport::NewHttpRequest(const googleapis::client::HttpRequest::HttpMethod & method) {
    return new QNAMHttpRequest{method, this};
}

googleapis::client::HttpTransport * QNAMHttpTransportFactory::DoAlloc(const googleapis::client::HttpTransportOptions & options) {
    return new QNAMHttpTransport{options};
}
