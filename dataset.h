#ifndef DATASET_H
#define DATASET_H

#include "coordinate.h"

#include <QString>
#include <QUrl>

struct Dataset {
    enum class API {
        Heidelbrain, WebKnossos, GoogleBrainmaps, OpenConnectome
    };

    static Dataset dummyDataset();
    static Dataset parseGoogleJson(const QString & json_raw);
    static Dataset parseOpenConnectomeJson(const QUrl & infoUrl, const QString & json_raw);
    static Dataset parseWebKnossosJson(const QString & json_raw);
    static Dataset fromLegacyConf(const QUrl & url, QString config);
    void checkMagnifications();
    void applyToState() const;

    API api;
    Coordinate boundary{0,0,0};
    floatCoordinate scale{0,0,0};
    int magnification{0};
    int lowestAvailableMag{0};
    int highestAvailableMag{0};
    int cubeEdgeLength{128};
    int compressionRatio{0};
    bool remote{false};
    bool overlay{false};
    QString experimentname{};
    QUrl url;
};

#endif//DATASET_H
