#ifndef DATASET_H
#define DATASET_H

#include "coordinate.h"

#include <QString>
#include <QUrl>

struct Dataset {
    static Dataset parseGoogleJson(const QString & json_raw);
    static Dataset parseWebKnossosJson(const QString & json_raw);
    static Dataset fromLegacyConf(const QUrl & url, QString config);
    void checkMagnifications();
    void applyToState() const;

    Coordinate boundary{0,0,0};
    floatCoordinate scale{0,0,0};
    int magnification = 0;
    int lowestAvailableMag;
    int highestAvailableMag;
    int cubeEdgeLength = 128;
    int compressionRatio = 0;
    bool remote = false;
    bool overlay = false;
    QString experimentname{""};
    QUrl url;
};

#endif//DATASET_H
