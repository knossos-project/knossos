#ifndef DATASET_H
#define DATASET_H

#include "coordinate.h"

#include <QString>
#include <QUrl>

struct Dataset {
    static Dataset fromLegacyConf(QString config);

    Coordinate boundary{0,0,0};
    floatCoordinate scale{0,0,0};
    int magnification = 0;
    int cubeEdgeLength = 0;
    int compressionRatio = 0;
    bool remote = false;
    QString experimentname{""};
    QUrl url;
};

#endif//DATASET_H
