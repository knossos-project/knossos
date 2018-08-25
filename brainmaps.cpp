#include "brainmaps.h"

#include "googleapis/client/transport/http_transport.h"
#include "googleapis/client/auth/credential_store.h"
#include "googleapis/client/auth/oauth2_authorization.h"
#include "googleapis/client/auth/oauth2_service_authorization.h"
#include "googleapis/client/util/status.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>

#include "QNAMHttpTransportFactory.h"

std::pair<bool, QString> getBrainmapsToken() {
    namespace gutil = googleapis::util;
    namespace gclient = googleapis::client;

    auto o2config = std::make_unique<gclient::HttpTransportLayerConfig>();
    o2config->ResetDefaultTransportFactory(new QNAMHttpTransportFactory{});
//    o2config->mutable_default_transport_options()->set_cacerts_path(gclient::HttpTransportOptions::kDisableSslVerification);

    gutil::Status gstatus;
    QFile saccfile(":resources/service_account.json");
    saccfile.open(QIODevice::ReadOnly | QIODevice::Text);
    std::unique_ptr<gclient::OAuth2AuthorizationFlow> flow{
            gclient::OAuth2AuthorizationFlow::MakeFlowFromClientSecretsJson(
                QTextStream{&saccfile}.readAll().toStdString()
                , o2config->NewDefaultTransportOrDie(), &gstatus)};

    qDebug() << gstatus.ToString().c_str();
    if (!gstatus.ok()) {
        throw std::runtime_error(gstatus.ToString().c_str());
    }

    flow->set_default_scopes("https://www.googleapis.com/auth/brainmaps");
    gclient::OAuth2Credential credential;
    auto status = flow->PerformRefreshToken(gclient::OAuth2RequestOptions{}, &credential);

    qDebug() << status.ToString().c_str() << credential.access_token().as_string().c_str();

    return std::make_pair(status.ok(), QString::fromStdString(credential.access_token().as_string()));
}
