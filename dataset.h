/*
 *  (C) Copyright 2007-2016
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  This file is a part of KNOSSOS.
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

#ifndef DATASET_H
#define DATASET_H

#include "coordinate.h"

#include <QString>
#include <QUrl>

struct Dataset {
    enum class API {
        Heidelbrain, WebKnossos, GoogleBrainmaps, OpenConnectome
    };
    enum class CubeType {
        RAW_UNCOMPRESSED, RAW_JPG, RAW_J2K, RAW_JP2_6, SEGMENTATION_UNCOMPRESSED, SEGMENTATION_SZ_ZIP
    };

    static bool isNeuroDataStore(const QUrl & url);

    static Dataset dummyDataset();
    static Dataset parseGoogleJson(const QString & json_raw);
    static Dataset parseNeuroDataStoreJson(const QUrl & infoUrl, const QString & json_raw);
    static Dataset parseWebKnossosJson(const QString & json_raw);
    static Dataset fromLegacyConf(const QUrl & url, QString config);
    void checkMagnifications();
    void applyToState() const;

    static QUrl apiSwitch(const API api, const QUrl & baseUrl, const Coordinate globalCoord, const int scale, const int cubeedgelength, const CubeType type);
    static bool isOverlay(const CubeType type);

    API api;
    Coordinate boundary{0,0,0};
    floatCoordinate scale{0,0,0};
    int magnification{1};
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
