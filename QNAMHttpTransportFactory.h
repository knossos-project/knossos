#include "googleapis/client/transport/http_transport.h"

class QNAMHttpTransportFactory : public googleapis::client::HttpTransportFactory {
protected:
    virtual googleapis::client::HttpTransport * DoAlloc(const googleapis::client::HttpTransportOptions & options) override;
};
